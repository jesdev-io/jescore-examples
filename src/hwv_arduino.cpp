#include <Arduino.h>
#include <jescore.h>

#define VIRTUAL_BUTTON_1_PIN 2
#define VIRTUAL_BUTTON_2_PIN 3

static bool led_state = false;
static uint8_t brightness = 128;

void button_1_ISR(){
    led_state = !led_state;
    digitalWrite(LED_BUILTIN, led_state ? HIGH : LOW);
}

void button_2_ISR(){
    brightness = (brightness + 64) % 256;
    analogWrite(LED_BUILTIN, brightness);
}

void virtual_button_press(void* p) {
    char* arg = jes_job_get_args();
    if (!arg) {
        jes_print("Usage: button <1|2>\r\n");
        return;
    }
    if (jes_job_is_arg(arg, "1")) {
        jes_print("Virtual Button 1 pressed!\r\n");
        button_1_ISR();
    }
    else if (jes_job_is_arg(arg, "2")) {
        jes_print("Virtual Button 2 pressed!\r\n");
        button_2_ISR();
    }
    else {
        jes_print("Unknown button number <%s>! Use 1 or 2\r\n", arg);
    }
}

void get_hardware_state(void* p) {
    jes_print("LED State: %s\r\n", led_state ? "ON" : "OFF");
    jes_print("Brightness: %d\r\n", brightness);
}

void setup() {
    jes_init();
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    pinMode(VIRTUAL_BUTTON_1_PIN, INPUT_PULLUP);
    pinMode(VIRTUAL_BUTTON_2_PIN, INPUT_PULLUP);
    /*
    This section is key: by attaching an interrupt as well as registering a
    jescore job with the same function as the ISR, you now linked hardware
    and software. Both a physical button press and the "button" CLI command
    will now trigger the same routine.
    */
    attachInterrupt(digitalPinToInterrupt(VIRTUAL_BUTTON_1_PIN), button_1_ISR, RISING);
    attachInterrupt(digitalPinToInterrupt(VIRTUAL_BUTTON_2_PIN), button_2_ISR, RISING);
    jes_register_job("button", 2048, 1, virtual_button_press, 0, 1);
    jes_register_job("state", 2048, 1, get_hardware_state, 0, 1);
}

void loop() {
    // Nothing to do here!
    // The virtual hardware is controlled entirely through CLI commands
}