#include <stdio.h>
#include "fastmath.h"
#include "utils.h"

void formatFloat(float value, uint8_t decimal_places, char *out_str){
    int32_t int_part = (int32_t)value;
    float frac_part = value - int_part;
    if (value < 0) {
        *out_str++ = '-';
        int_part = -int_part;
        frac_part = -frac_part;
    }
    uint8_t int_digits = sprintf(out_str, "%ld", int_part);
    out_str += int_digits;

    if (decimal_places > 0) {
        *out_str++ = '.';
        frac_part *= powf(10, decimal_places);
        int32_t frac_int = (int32_t)(frac_part + 0.5f);
        sprintf(out_str, "%0*ld", decimal_places, frac_int);
    }
}