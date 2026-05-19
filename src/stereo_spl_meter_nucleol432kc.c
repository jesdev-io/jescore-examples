/*
This project was generated with the cube2pio tool: 

*** https://github.com/jesdev-io/cube2pio ***

Hardware access is performed with the STM32Cube HAL and 
can be found in `lib/port_stereo_spl_meter_stm32`. There, 
see `port.c` for the STM32Cube HAL specific driver calls.
The HAL redirects to `HAL_SAI_RxHalfCpltCallback()` and
`HAL_SAI_RxCpltCallback()`, which are defined in this file
and used for audio retrieval. 

CLI (Python):
- Call `jescore audio spl` to get the current stereo SPL (Z)
- Call `watch -n 1 jescore audio spl` to see the SPL meter
  Update with 1 Hz.

CLI (Serial connection):
- Call `audio spl` to get the current stereo SPL (Z)

MICROPHONES:
- 2x INMP441 Breakout Boards

PINDEFS (set in `port.c`):
- `BCLK`: PA8
- `LRCLK`: PA9
- `SD`: PA10
*/

#include <jescore.h>
#include "port.h"
#include "utils.h"
#include "fastmath.h"
#include <math.h>

#define NUM_CHANNELS 2
#define BLOCK_SIZE 512
#define UINT32_TO_FLOAT32(x) ((((int32_t)x) << 8) / 256) / 8388608.0f
#define MIC_SENS_DB -26         // data sheet (adjust if needed)
#define MIC_CALIB_POINT_DB 94   // typical
#define ROLLING_WINDOW_SIZE 100 // roughly 1 second

typedef struct stereo_t{
    uint32_t r;
    uint32_t l;
}stereo_t;

typedef union {
    struct {
        float r;
        float l;
    }ch;
    float channels[NUM_CHANNELS];
}stereo_val_t;

stereo_t mem[BLOCK_SIZE];

#define LED_PORT GPIOB
#define LED_GPIO GPIO_PIN_3
#define LED_ENABLE_PORT() __HAL_RCC_GPIOB_CLK_ENABLE()

/// @brief Main audio loop. This loop receives valid buffer
///        pointers to newly sampled audio triggered by
///        `HAL_SAI_RxCpltCallback()`. It then computes the
///        moving average SPL over 100 buffers (roughly one 
///        sec.).
/// @param p `jescore` job pointer.
void audio_loop(void* p){
    static uint8_t act = 0;
    act = !act;
    float sample_r = 0.;
    float sample_l = 0.;
    uint8_t buf_pos = 0;
    static stereo_val_t spl[NUM_CHANNELS];
    const stereo_val_t fail_val = {.ch.l = -999., .ch.r = -999.};
    stereo_val_t roll_window[ROLLING_WINDOW_SIZE];
    memset(roll_window, 0, ROLLING_WINDOW_SIZE*sizeof(stereo_val_t));
    uint8_t roll_idx = 0;
   
    while(act){
        stereo_t* buf = jes_wait_for_notification();
        float sqr_sum_r = 0.;
        float sqr_sum_l = 0.;
		for(uint16_t i = 0; i < (BLOCK_SIZE/2); i++){
            sample_r = UINT32_TO_FLOAT32(buf[i].r);
            sample_l = UINT32_TO_FLOAT32(buf[i].l);
			sqr_sum_r += (sample_r * sample_r);
            sqr_sum_l += (sample_l * sample_l);
		}
        if(sqr_sum_r < 0.00000001 || sqr_sum_l < 0.00000001){
            jes_notify_job("audio", (uint32_t*)&fail_val);
        }
        else{
            roll_window[roll_idx].ch.r = sqr_sum_r / BLOCK_SIZE/2;
            roll_window[roll_idx].ch.l = sqr_sum_l / BLOCK_SIZE/2;
            if(++roll_idx == ROLLING_WINDOW_SIZE) roll_idx = 0;
            stereo_val_t mean_sqr = {.ch.r = 0, .ch.l = 0};
            for(uint16_t i = 0; i < ROLLING_WINDOW_SIZE; i++){
                mean_sqr.ch.r += roll_window[i].ch.r;
                mean_sqr.ch.l += roll_window[i].ch.l;
            }
            mean_sqr.ch.r /= ROLLING_WINDOW_SIZE;
            mean_sqr.ch.l /= ROLLING_WINDOW_SIZE;
            spl[buf_pos].ch.r = 10*log10f(mean_sqr.ch.r) - MIC_SENS_DB + MIC_CALIB_POINT_DB;
            spl[buf_pos].ch.l = 10*log10f(mean_sqr.ch.l) - MIC_SENS_DB + MIC_CALIB_POINT_DB;
            jes_notify_job("audio", &spl[buf_pos]);
            buf_pos = !buf_pos;
        }
    }
}

/// @brief Main audio controller. This worker receives valid 
///        SPL data from the audio loop for both channels and  
///        formats it as printable data. Takes one arg `spl`.
///        Evoke it on the CLI with `jescore audio spl`.
/// @param p `jescore` job pointer.
void audio_controller(void* p) {
    char* arg = jes_job_arg_next();
    char buf[128];
    char fmt[8];
    const float min_SPL = 30.0;
    const float max_SPL = 100.0;
    const uint8_t max_bar_length = 40;
    int16_t bar_lens[NUM_CHANNELS];
    char ch_desc[NUM_CHANNELS] = {'r', 'l'};

    while(arg != NULL) {
        if(jes_job_is_arg(arg, "spl")) {
            volatile stereo_val_t val = *(stereo_val_t*)jes_wait_for_notification();
            jes_print("Monitoring %d channels (%d buffer average)\n\r", NUM_CHANNELS, ROLLING_WINDOW_SIZE);
            for(uint8_t i = 0; i < NUM_CHANNELS; i++){
                formatFloat(val.channels[i], 2, fmt);
                bar_lens[i] = (int16_t)((val.channels[i] - min_SPL) / (max_SPL - min_SPL) * max_bar_length);
                if(bar_lens[i] < 0) bar_lens[i] = 0;
                if (bar_lens[i] > max_bar_length) bar_lens[i] = max_bar_length;
                sprintf(buf, "%c: %s dB(Z) [", ch_desc[i], fmt);
                for(uint8_t j = 0; j < max_bar_length; j++) {
                    if(j < bar_lens[i]) {
                        strcat(buf, "=");
                    } else {
                        strcat(buf, " ");
                    }
                }
                strcat(buf, "]\n\r");
                jes_print(buf);
            }
        }
        else {
            jes_print("Unknown arg <%s>\n\r", arg);
        }
        arg = jes_job_arg_next();
    }
}

/// @brief Setup function (ported from STM32CubeIDE).
///        Activate `jescore` and register the audio
///        loop and controller. Activate the DMA audio
///        transfer.
/// @param p `jescore` job pointer.
void port_setup(){
    jes_init();
    jes_register_and_launch_job("_audio", 2048, 1, audio_loop, 1, 1);
    jes_register_job("audio", 1024, 1, audio_controller, 0, 1);
    LED_ENABLE_PORT();
    HAL_StatusTypeDef stat;
    if ((stat = HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t*)mem, BLOCK_SIZE*2)) != HAL_OK){
        HAL_GPIO_TogglePin(LED_PORT, LED_GPIO);
        while(1);
    }
    jes_dispatch();
}

void port_loop(){
    // this function is never reached.
}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef* hsai){
    jes_notify_job_ISR("_audio", &mem[0]);
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef* hsai){
    jes_notify_job_ISR("_audio", &mem[BLOCK_SIZE/2]);
}
