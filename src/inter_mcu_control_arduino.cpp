/*
This example shows a "local" to "remote" inter-MCU UART communication. 
It only makes sense if another MCU is connected to the one receiving 
this firmware AND is running jescore as well. Connect the pins 18 (RX) 
and 19 (TX) of the local MCU to the native UART pins (usually labeled TX0
and RX0). You can flash any example you like to the remote MCU. You can now
control the remote MCU via the local one with the "feed" job.

If you flashed the "hwv_arduino.cpp" example to the remote host, you can
now control its firmware from the local host:

From the jescore-CLI:
$ jescore feed button 1

This will send the command "button 1" to the remote MCU, activating the LED.
*/

#include <Arduino.h>
#include <jescore.h>
#include <cli.h>

#define MSG_BUFFER_LEN 256

void feed(void* p){
    /*
    This function pipes through received feed commands to the
    connected remote MCU. It has a timeout of 2 seconds.
    Return prints of the remote MCU are marked in grey.
    */
    char msg[MSG_BUFFER_LEN] = {0};
    char* args = jes_job_get_args();
    jes_print("Feeding <%s> to connected MCU...\n\r", args);
    Serial1.printf(args);
    Serial1.readBytes(msg, MSG_BUFFER_LEN);
    jes_print("(remote) %s%s%s\n\r", CLR_Gr, msg, CLR_X);
}

void setup(){
    jes_init();
    Serial1.begin(115200, 134217756U, 18, 19);
    Serial1.setTimeout(2000);
    jes_register_job("feed", 2048, 1, feed, 0, 1);
}

void loop(){

}
