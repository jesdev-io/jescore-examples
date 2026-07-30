#include <Arduino.h>
#include <jescore.h>

#define HIDDEN_HASH "DEADBEEF"
#define HIDDEN_SECRET "cake🎂"

static bool dev_mode = 0;

void devmode(void* p){
    char* arg = jes_job_get_args();
    if (!arg) {
        /*
        Fail silently. We dont want to give away hints
        on how to use devmode.
        */ 
        return;
    }
    
    if (jes_job_is_arg(arg, HIDDEN_HASH)) {
        jes_print("Devmode unlocked!\r\n");
        jes_print("You can now call 'secret' \r\n");
        jes_print("Use 'devmode lock' to close devmode.\r\n");
        dev_mode = 1;
    }
    else if (jes_job_is_arg(arg, "lock")) {
        jes_print("Devmode locked!\r\n");
        dev_mode = 0;
    }
    else {
        jes_print("Unknown devmode command <%s>\r\n", arg);
    }
}

void show_secret(void* p){
    if(dev_mode){
        jes_print("Your secret is: " HIDDEN_SECRET "\n\r");
    }
    else{
        jes_print("Forbidden.\n\r");
    }
}


void setup() {
    jes_init();
    /*
    Not only is there no way to evoke developer access from
    the UI (no UI callbacks present), but it also has to be
    unlocked with a predefined hash value in string format
    that only the developer knows.
    */
    jes_register_job("devmode", 2048, 1, devmode, 0, 1);
    jes_register_job("secret", 2048, 1, show_secret, 0, 1);
}

void loop() {
    // Nothing to do here!
    // The virtual hardware is controlled entirely through CLI commands
}