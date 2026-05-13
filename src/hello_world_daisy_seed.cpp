#include "daisy_seed.h"
extern "C" {
#include "jescore.h"
}

using namespace daisy;
DaisySeed hw;

void blink(void* p){
    static uint8_t act = 0;
    act = !act;
    while(act){
        hw.SetLed(true);
        jes_delay_job_ms(1000);
        hw.SetLed(false);
        jes_delay_job_ms(1000);
    }
}

void hello(void* p){
    uart_unif_writef("Hello World!\n\r");
}

void __disp(void* p){
    hw.Init();
}

int main(void){
    // hw.Init();

    jes_init();
    jes_register_and_launch_job("__disp", 512, 1, __disp, 0, 1);
    jes_register_and_launch_job("blink", 256, 1, blink, 1, 0);
    jes_register_job("hello", 256, 1, hello, 0, 0);
    jes_dispatch();
}
