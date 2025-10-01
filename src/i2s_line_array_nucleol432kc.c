#include <jescore.h>
#include "port.h"
#include "stm32l4xx_hal.h"
#include "fastmath.h"

#define BLOCK_SIZE 512
#define UINT32_TO_FLOAT32(x) ((((int32_t)x) << 8) / 256) / 8388608.0f
#define UNIFORM_DISTANCE 0.033 // in m
#define UNIFORM_DELAY 5 // in samples
#define MIC_SENS_DB -26 // data sheet
#define MIC_CALIB_POINT_DB 94 // typical

typedef struct stereo_t{
    uint32_t r;
    uint32_t l;
}stereo_t;

typedef struct stereo_val_t{
    float r;
    float l;
}stereo_val_t;

stereo_t mem[BLOCK_SIZE];

void formatFloat(float value, int decimal_places, char *out_str);

#define LED_PORT GPIOB
#define LED_GPIO GPIO_PIN_3
#define LED_ENABLE_PORT() __HAL_RCC_GPIOB_CLK_ENABLE()


void audio_loop(void* p){
    static uint8_t act = 0;
    act = !act;
    float sample_r = 0.;
    float sample_l = 0.;
    uint8_t buf_pos = 0;
    static stereo_val_t dbfs[2];
   
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
        if(sqr_sum_r < 0.00000001){
            float fail_val = -999.;
            jes_notify_job("audio", &fail_val);    
        }
        else{
            dbfs[buf_pos].r = 10*log10f(sqr_sum_r / BLOCK_SIZE/2) - MIC_SENS_DB + MIC_CALIB_POINT_DB;
            dbfs[buf_pos].l = 10*log10f(sqr_sum_l / BLOCK_SIZE/2) - MIC_SENS_DB + MIC_CALIB_POINT_DB;
            jes_notify_job("audio", &dbfs[buf_pos]);
            buf_pos = !buf_pos;
        }
    }
}

void audio_controller(void* p) {
    char* arg = jes_job_arg_next();
    char buf[64];
    char r_fmt[8];
    while(arg != NULL) {
        if(jes_job_is_arg(arg, "dbfs")) {
            volatile stereo_val_t val = *(stereo_val_t*)jes_wait_for_notification();
            formatFloat(val.r, 2, r_fmt);
            
            // For positive SPL values - adjust these ranges based on your actual SPL range
            float min_SPL = 30.0;   // Typical quiet room SPL
            float max_SPL = 100.0;  // Typical loud sound SPL
            int max_bar_length = 40;
            
            // Scale the SPL value to bar length
            int bar_length = (int)((val.r - min_SPL) / (max_SPL - min_SPL) * max_bar_length);
            
            // Ensure the bar length stays within bounds
            if (bar_length < 0) bar_length = 0;
            if (bar_length > max_bar_length) bar_length = max_bar_length;

            sprintf(buf, "r: %s dB(Z) [", r_fmt);
            for(int i = 0; i < max_bar_length; i++) {
                if(i < bar_length) {
                    strcat(buf, "=");
                } else {
                    strcat(buf, " ");
                }
            }
            strcat(buf, "]\n\r");
            uart_unif_write(buf);
        }
        else {
            uart_unif_writef("Unknown arg <%s>\n\r", arg);
        }
        arg = jes_job_arg_next();
    }
}

void port_setup(){
    jes_init();
    jes_register_and_launch_job("_audio", 1024, 1, audio_loop, 1);
    jes_register_job("audio", 1024, 1, audio_controller, 0);
    LED_ENABLE_PORT();
    HAL_StatusTypeDef stat;
    if ((stat = HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t*)mem, BLOCK_SIZE*2)) != HAL_OK){
        HAL_GPIO_TogglePin(LED_PORT, LED_GPIO);
        while(1);
    }
    if ((stat = HAL_SAI_Receive_DMA(&hsai_BlockB1, (uint8_t*)mem, BLOCK_SIZE*2)) != HAL_OK){
        HAL_GPIO_TogglePin(LED_PORT, LED_GPIO);
        while(1);
    }
    jes_dispatch();
}

void port_loop(){

}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef* hsai){
    jes_notify_job_ISR("_audio", &mem[0]);
}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef* hsai){
    jes_notify_job_ISR("_audio", &mem[BLOCK_SIZE/2]);
}

void formatFloat(float value, int decimal_places, char *out_str)
{
    int int_part = (int)value;          // Extract integer part
    float frac_part = value - int_part; // Extract fractional part

    // Handle negative sign if needed
    if (int_part < 0)
    {
        int_part = -int_part;
        *out_str++ = '-';
    }

    // Convert integer part to string
    int int_digits = sprintf(out_str, "%d", int_part);
    out_str += int_digits;

    // Add decimal point if necessary
    if (decimal_places > 0)
    {
        *out_str++ = '.';

        // Ensure the fractional part is positive before converting
        frac_part = fabs(frac_part);

        // Multiply fractional part to get desired precision
        frac_part *= pow(10, decimal_places);

        // Convert fractional part (now an integer) to string
        sprintf(out_str, "%d", (int)frac_part);
    }
}
