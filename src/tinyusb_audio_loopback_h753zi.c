#include "jescore.h"
#include "delay_unif.h"
#include "usb_audio_h753zi_port.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <math.h>

// -----------------------------------------------------------------------------
// User playground: USB identity and stream parameters
// -----------------------------------------------------------------------------
// Change these strings/PID when experimenting so the host re-enumerates cleanly.
// The low-level descriptor code in lib/ reads this struct; user-facing data stays here.
const usb_audio_device_descriptor_config_t usb_audio_device_config = {
  .vid = 0x0483,
  .pid = 0x5731,
  .bcd_device = 0x0100,
  .manufacturer = "jesdev",
  .product = "JES TinyUSB Feedthrough",
  .serial = "H753FEEDTHRU",
  .uac1_function = "JES UAC1 Feedthrough",
  .uac2_function = "JES UAC2 Feedthrough",
};

#define USB_AUDIO_SAMPLE_RATE 48000U
#define USB_SPEAKER_CHANNELS  2U  // host -> MCU: stereo S16_LE
#define USB_MIC_CHANNELS      1U  // MCU -> host: mono S16_LE
#define USB_BYTES_PER_SAMPLE  2U
#define SAMPLES_PER_MS        (USB_AUDIO_SAMPLE_RATE / 1000U)
#define OUT_BYTES_PER_MS      (SAMPLES_PER_MS * USB_SPEAKER_CHANNELS * USB_BYTES_PER_SAMPLE)
#define IN_BYTES_PER_MS       (SAMPLES_PER_MS * USB_MIC_CHANNELS * USB_BYTES_PER_SAMPLE)
#define MIC_RING_FRAMES       64U

static volatile uint8_t audio_mute[3];          // 0=master, 1=left/playback, 2=right/playback
static volatile int16_t audio_volume_db256[3];  // UAC1 volume unit: 1/256 dB
static volatile int32_t audio_gain_q15 = 32767;

// DSP context note:
// - user_dsp_sample() runs inside the jescore worker task "_tinyusb_audio".
// - It is not an ISR, so jescore/FreeRTOS APIs are available, but keep it real-time.
// - Budget: one USB full-speed audio frame per millisecond. At 48 kHz this means
//   48 calls/ms for this sample hook, plus TinyUSB servicing. Avoid blocking,
//   allocation, printing, or long I/O here. For heavier DSP, process a 48-sample
//   frame in a separate frame hook/ring-buffered worker and watch underrun counters.
static int16_t user_dsp_sample(int16_t left, int16_t right) {
  (void)right;
  // Simple feedthrough playground: speaker left channel -> mono microphone.
  // Enter per-sample DSP here, e.g. gain, soft clipping, filtering state update.
  int32_t y = left;

  // Standard OS mixer controls from the USB Feature Unit.
  if (audio_mute[0] || audio_mute[1]) return 0;
  y = (y * audio_gain_q15) / 32767;
  if (y > INT16_MAX) return INT16_MAX;
  if (y < INT16_MIN) return INT16_MIN;
  return (int16_t)y;
}

static volatile uint32_t tx_packets;
static volatile uint32_t rx_packets;
static volatile uint32_t bytes_in;
static volatile uint32_t bytes_out;
static volatile uint32_t worker_loops;
static volatile uint32_t usb_init_ok;
static volatile uint32_t fresh_writes;
static volatile uint32_t silence_writes;
static volatile uint32_t short_writes;
static volatile uint32_t multi_read_calls;
static volatile uint32_t max_reads_per_call;
static volatile uint32_t ring_overflows;
static volatile uint32_t max_ring_fill;
static volatile bool stream_enabled = true;

static uint8_t out_buf[OUT_BYTES_PER_MS];
static uint8_t in_buf[IN_BYTES_PER_MS];
static int16_t mic_ring[MIC_RING_FRAMES][SAMPLES_PER_MS];
static uint32_t mic_ring_head;
static uint32_t mic_ring_tail;
static uint32_t mic_ring_count;
static uint32_t last_audio_ms;

static void reset_stream_counters(void) {
  tx_packets = 0;
  rx_packets = 0;
  bytes_in = 0;
  bytes_out = 0;
  worker_loops = 0;
  fresh_writes = 0;
  silence_writes = 0;
  short_writes = 0;
  multi_read_calls = 0;
  max_reads_per_call = 0;
  ring_overflows = 0;
  max_ring_fill = 0;
  mic_ring_head = 0;
  mic_ring_tail = 0;
  mic_ring_count = 0;
}

static void enqueue_microphone_frame_from_speaker_frame(void) {
  if (mic_ring_count >= MIC_RING_FRAMES) {
    mic_ring_tail = (mic_ring_tail + 1U) % MIC_RING_FRAMES;
    mic_ring_count--;
    ring_overflows++;
  }

  const int16_t *src = (const int16_t *)out_buf;
  int16_t *dst = mic_ring[mic_ring_head];
  for (uint32_t i = 0; i < SAMPLES_PER_MS; i++) {
    int16_t left = src[USB_SPEAKER_CHANNELS * i];
    int16_t right = src[USB_SPEAKER_CHANNELS * i + 1U];
    dst[i] = user_dsp_sample(left, right);
  }

  mic_ring_head = (mic_ring_head + 1U) % MIC_RING_FRAMES;
  mic_ring_count++;
  if (mic_ring_count > max_ring_fill) max_ring_fill = mic_ring_count;
}

static void drain_speaker_out(void) {
  uint32_t reads_this_call = 0;
  uint32_t avail = tud_audio_available();
  while (avail >= OUT_BYTES_PER_MS) {
    uint16_t got = tud_audio_read(out_buf, OUT_BYTES_PER_MS);
    if (got < OUT_BYTES_PER_MS) break;

    if (stream_enabled) enqueue_microphone_frame_from_speaker_frame();

    reads_this_call++;
    tx_packets++;
    bytes_in += got;
    avail = tud_audio_available();
  }

  if (reads_this_call > 1U) multi_read_calls++;
  if (reads_this_call > max_reads_per_call) max_reads_per_call = reads_this_call;
}

static void write_next_microphone_frame(void) {
  // We are inside a jescore task, so __get_systime_ms() is the FreeRTOS tick clock.
  // TinyUSB audio isochronous frames are 1 ms at full speed; write at most once/ms.
  uint32_t now = __get_systime_ms();
  if (now == last_audio_ms) return;
  last_audio_ms = now;


  if (stream_enabled && mic_ring_count > 0U) {
    memcpy(in_buf, mic_ring[mic_ring_tail], IN_BYTES_PER_MS);
    mic_ring_tail = (mic_ring_tail + 1U) % MIC_RING_FRAMES;
    mic_ring_count--;
    fresh_writes++;
  } else {
    memset(in_buf, 0, IN_BYTES_PER_MS);
    silence_writes++;
  }

  uint16_t wrote = tud_audio_write(in_buf, IN_BYTES_PER_MS);
  bytes_out += wrote;
  if (wrote < IN_BYTES_PER_MS) short_writes++;
}

static void process_audio(void) {
  drain_speaker_out();
  write_next_microphone_frame();
}

static void tinyusb_audio_status(void *p) {
  (void)p;
  jes_print("usb product=\"%s\"\n\r", usb_audio_device_config.product);
  jes_print("usb en=%u mount=%u init=%lu loops=%lu\n\r",
            stream_enabled ? 1U : 0U, tud_mounted() ? 1U : 0U, usb_init_ok, worker_loops);
  jes_print("usb rx=%lu proc=%lu in=%lu out=%lu\n\r",
            rx_packets, tx_packets, bytes_in, bytes_out);
  jes_print("usb fresh=%lu zero=%lu short=%lu multi=%lu max=%lu\n\r",
            fresh_writes, silence_writes, short_writes, multi_read_calls, max_reads_per_call);
  jes_print("usb ring now=%lu max=%lu ovf=%lu\n\r",
            mic_ring_count, max_ring_fill, ring_overflows);
  jes_print("usb mixer mute=%u,%u,%u vol_db=%ld,%ld,%ld\n\r",
            audio_mute[0], audio_mute[1], audio_mute[2],
            (long)(audio_volume_db256[0] / 256),
            (long)(audio_volume_db256[1] / 256),
            (long)(audio_volume_db256[2] / 256));
}

static void tinyusb_audio_on(void *p) {
  (void)p;
  stream_enabled = true;
  jes_print("tinyusb_audio stream on\n\r");
}

static void tinyusb_audio_off(void *p) {
  (void)p;
  stream_enabled = false;
  mic_ring_count = 0;
  jes_print("tinyusb_audio stream off\n\r");
}

static void tinyusb_audio_zero(void *p) {
  (void)p;
  reset_stream_counters();
  jes_print("tinyusb_audio counters zeroed\n\r");
}

static void tinyusb_audio_worker(void *p) {
  (void)p;
  port_usb_peripheral_init();
  tusb_rhport_init_t dev_init = { .role = TUSB_ROLE_DEVICE, .speed = TUSB_SPEED_FULL };
  usb_init_ok = tusb_init(0, &dev_init) ? 1U : 0U;
  tud_disconnect();
  jes_delay_job_ms(50);
  tud_connect();

  while (1) {
    worker_loops++;
    // This is not a spin poll: with FreeRTOS OSAL, tud_task_ext waits for USB
    // events or a 1 ms timeout. The timeout is the audio pacing heartbeat used
    // to write one microphone frame when no USB control event arrives.
    tud_task_ext(1, false);
    process_audio();
    port_usb_set_mounted_led(tud_mounted());
  }
}

void port_setup(void) {
  jes_init();
  jes_register_job("tinyusb_audio", 1536, 1, tinyusb_audio_status, 0, 0);
  jes_register_job("tinyusb_audio_on", 1024, 1, tinyusb_audio_on, 0, 0);
  jes_register_job("tinyusb_audio_off", 1024, 1, tinyusb_audio_off, 0, 0);
  jes_register_job("tinyusb_audio_zero", 1024, 1, tinyusb_audio_zero, 0, 0);
  jes_register_and_launch_job("_tinyusb_audio", 2048, 3, tinyusb_audio_worker, 1, 0);
  jes_dispatch();
}

void port_loop(void) {
  /* jes_dispatch() starts the scheduler; this loop is never reached. */
}

bool tud_audio_rx_done_isr(uint8_t rhport, uint16_t n_bytes_received, uint8_t func_id, uint8_t ep_out, uint8_t cur_alt_setting) {
  (void)rhport; (void)n_bytes_received; (void)func_id; (void)ep_out; (void)cur_alt_setting;
  rx_packets++;
  return true;
}

bool tud_audio_tx_done_isr(uint8_t rhport, uint16_t n_bytes_sent, uint8_t func_id, uint8_t ep_in, uint8_t cur_alt_setting) {
  (void)rhport; (void)n_bytes_sent; (void)func_id; (void)ep_in; (void)cur_alt_setting;
  return true;
}

bool tud_audio_set_req_ep_cb(uint8_t rhport, tusb_control_request_t const *req, uint8_t *buf) {
  (void)rhport;
  uint8_t ctrl = TU_U16_HIGH(req->wValue);
  if (ctrl == AUDIO10_EP_CTRL_SAMPLING_FREQ && req->bRequest == AUDIO10_CS_REQ_SET_CUR && req->wLength == 3) {
    (void)buf; // fixed 48 kHz stream; accept host's selected supported rate
    return true;
  }
  return false;
}

bool tud_audio_get_req_ep_cb(uint8_t rhport, tusb_control_request_t const *req) {
  uint8_t ctrl = TU_U16_HIGH(req->wValue);
  if (ctrl == AUDIO10_EP_CTRL_SAMPLING_FREQ && req->bRequest == AUDIO10_CS_REQ_GET_CUR) {
    uint8_t freq[3] = {
      (uint8_t)(USB_AUDIO_SAMPLE_RATE & 0xff),
      (uint8_t)((USB_AUDIO_SAMPLE_RATE >> 8) & 0xff),
      (uint8_t)((USB_AUDIO_SAMPLE_RATE >> 16) & 0xff)
    };
    return tud_audio_buffer_and_schedule_control_xfer(rhport, req, freq, sizeof(freq));
  }
  return false;
}

static uint8_t audio_channel_from_request(tusb_control_request_t const *req) {
  uint8_t ch = TU_U16_LOW(req->wValue);
  return (ch < 3U) ? ch : 0U;
}

static void update_audio_gain_from_volume_controls(void) {
  // Feedthrough uses speaker-left, so apply master + left-channel dB controls.
  float db = (float)(audio_volume_db256[0] + audio_volume_db256[1]) / 256.0f;
  float gain = powf(10.0f, db / 20.0f);
  if (gain > 1.995f) gain = 1.995f; // +6 dB cap
  if (gain < 0.0f) gain = 0.0f;
  audio_gain_q15 = (int32_t)(gain * 32767.0f);
}

bool tud_audio_set_req_entity_cb(uint8_t rhport, tusb_control_request_t const *req, uint8_t *buf) {
  (void)rhport;
  uint8_t ctrl = TU_U16_HIGH(req->wValue);
  uint8_t ch = audio_channel_from_request(req);

  if (req->bRequest == AUDIO10_CS_REQ_SET_CUR && ctrl == AUDIO10_FU_CTRL_MUTE && req->wLength == 1) {
    audio_mute[ch] = buf[0] ? 1U : 0U;
    return true;
  }

  if (req->bRequest == AUDIO10_CS_REQ_SET_CUR && ctrl == AUDIO10_FU_CTRL_VOLUME && req->wLength == 2) {
    int16_t vol;
    memcpy(&vol, buf, sizeof(vol));
    audio_volume_db256[ch] = vol;
    update_audio_gain_from_volume_controls();
    return true;
  }

  return false;
}

bool tud_audio_get_req_entity_cb(uint8_t rhport, tusb_control_request_t const *req) {
  uint8_t ctrl = TU_U16_HIGH(req->wValue);
  uint8_t ch = audio_channel_from_request(req);

  if (ctrl == AUDIO10_FU_CTRL_MUTE && req->bRequest == AUDIO10_CS_REQ_GET_CUR) {
    return tud_audio_buffer_and_schedule_control_xfer(rhport, req, (void *)&audio_mute[ch], 1);
  }

  if (ctrl == AUDIO10_FU_CTRL_VOLUME) {
    int16_t vol = audio_volume_db256[ch];
    int16_t min = (int16_t)(-60 * 256); // -60 dB
    int16_t max = (int16_t)(  6 * 256); // +6 dB
    int16_t res = (int16_t)(  1 * 256); // 1 dB steps

    switch (req->bRequest) {
      case AUDIO10_CS_REQ_GET_CUR: return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &vol, sizeof(vol));
      case AUDIO10_CS_REQ_GET_MIN: return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &min, sizeof(min));
      case AUDIO10_CS_REQ_GET_MAX: return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &max, sizeof(max));
      case AUDIO10_CS_REQ_GET_RES: return tud_audio_buffer_and_schedule_control_xfer(rhport, req, &res, sizeof(res));
      default: break;
    }
  }

  return false;
}

bool tud_audio_set_req_itf_cb(uint8_t rhport, tusb_control_request_t const *req, uint8_t *buf) {
  (void)rhport; (void)req; (void)buf; return false;
}

bool tud_audio_get_req_itf_cb(uint8_t rhport, tusb_control_request_t const *req) {
  (void)rhport; (void)req; return false;
}
