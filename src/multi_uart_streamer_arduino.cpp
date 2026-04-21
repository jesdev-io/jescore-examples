/*
This code demonstrates how jescore can split UART messages
originating from different jobs. This is very useful for
debugging and low rate cross-platform data transfer. 

jescore CLI:
- Call `jescore -l` to see all jobs printing
- Call `jescore -l --filter [printer1]` to just see the output 
    of the job "printer1"
- Call `jescore -l --filter [printer1, printer2]` to see the 
    output of both "printer1" and "printer2"
- Call `jescore hello` to see the response of the "hello" job,
    where the name of the job is directly applied as filter
*/

#include <Arduino.h>
#include <jescore.h>

void printer1(void* p){
    while(1){
        jes_print("Hello from printer 1!\n\r");
        jes_delay_job_ms(1000);
    }
}

void printer2(void* p){
    while(1){
        jes_print("Hello from printer 2!\n\r");
        jes_delay_job_ms(2000);
    }
}

void printer3(void* p){
    while(1){
        jes_print("Hello from printer 3!\n\r");
        jes_delay_job_ms(3000);
    }
}

void printer_on_demand(void* p){
    jes_print("Hello on demand!\n\r");
}


void setup() {
    jes_init();
    jes_register_and_launch_job("printer1", 2048, 1, printer1, 1, 1);
    jes_register_and_launch_job("printer2", 2048, 1, printer2, 1, 1);
    jes_register_and_launch_job("printer3", 2048, 1, printer3, 1, 1);
    jes_register_job("hello", 2048, 1, printer_on_demand, 0, 1);
}

void loop() {

}
