#include <Arduino.h>
#include <jescore.h>

#define LED_PIN LED_BUILTIN

void blink(void* p){
    static uint8_t act = 0;
    act = !act;
    while(act){
        digitalWrite(LED_PIN, HIGH);
        jes_delay_job_ms(1000);
        digitalWrite(LED_PIN, LOW);
        jes_delay_job_ms(1000);
    }
}

void hello(void* p){
    uart_unif_write("Hello World!\n\r");
}

void setup(){
    pinMode(LED_PIN, OUTPUT);
    jes_init();
    jes_register_and_launch_job("blink", 1024, 1, blink, 1);
    jes_register_job("hello", 1024, 1, hello, 0);
}

void loop(){

}
