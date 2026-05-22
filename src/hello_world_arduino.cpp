#include <Arduino.h>
#include <jescore.h>

#define LED_PIN LED_BUILTIN

void blink(void* p){
    static uint8_t act = 0;
    act = !act;
    while(act){
        jes_print("Blinking!\n\r");
        digitalWrite(LED_PIN, HIGH);
        jes_delay_job_ms(1000);
        digitalWrite(LED_PIN, LOW);
        jes_delay_job_ms(1000);
    }
}

void hello(void* p){
    jes_print("Hello World!\n\r");
}

void setup(){
    pinMode(LED_PIN, OUTPUT);
    jes_init();
    jes_register_and_launch_job("blink", 1024, 1, blink, 1, 0);
    jes_register_job("hello", 1024, 1, hello, 0, 0);
}

void loop(){

}
