#include <Arduino.h>
#include <jescore.h>
#include <DHT.h>

DHT dht(4, DHT11);

void temperature_sampler(void* p){
    float h = dht.readHumidity();
    float t = dht.readTemperature();

    uart_unif_writef("T: %f °C, H: %f\n\r", t, h);
}

void setup(){
    jes_init();
    jes_register_job("temp", 2048, 1, temperature_sampler, 0);
    dht.begin();
}

void loop(){

}
