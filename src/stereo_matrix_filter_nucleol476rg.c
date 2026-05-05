#include <jescore.h>
#include "port.h"
#include "arm_math.h"
#include "arm_vec_math.h"

#define NUM_CHANNELS 2
#define BLOCK_SIZE 1024*2
#define UINT32_TO_FLOAT32(x) ((((int32_t)x) << 8) / 256) / 8388608.0f

#define N 7
#define M 2
#define P 2

typedef union {
    struct {
        q31_t r;
        q31_t l;
    }ch;
    q31_t channels[NUM_CHANNELS];
}stereo_t;

typedef union {
    struct {
        float r;
        float l;
    }ch;
    float channels[NUM_CHANNELS];
}stereo_val_t;

uint8_t hlf_rdy = 0;
uint8_t rdy = 0;

stereo_t rx_buf[BLOCK_SIZE] __attribute__((aligned(8)));
stereo_t tx_buf[BLOCK_SIZE] __attribute__((aligned(8)));

q31_t u[M] __attribute__((aligned(8)));
q31_t y[P] __attribute__((aligned(8)));
q31_t x[N] __attribute__((aligned(8))) = {0};
q31_t x1[N] __attribute__((aligned(8))) = {0};
q31_t _A[N][N] __attribute__((aligned(8))) = {
    #include "matrix/A.csv"
};
q31_t _B[N][M] __attribute__((aligned(8))) = {
    #include "matrix/B.csv"
};
q31_t _C[N][P] __attribute__((aligned(8))) = {
    #include "matrix/C.csv"
};
q31_t _D[M][P] __attribute__((aligned(8))) = {
    #include "matrix/D.csv"
};

arm_matrix_instance_q31 A;
arm_matrix_instance_q31 B;
arm_matrix_instance_q31 C;
arm_matrix_instance_q31 D;
arm_matrix_instance_q31 test;

void dsp(stereo_t* data, uint32_t len){
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    q31_t (*matrix)[2] = (q31_t (*)[2])data;
    for(uint16_t i = 0; i < len; i++){
        for(uint16_t j = 0; j < M; j++){
            u[j] = data->channels[j];
        }
        
        arm_mat_vec_mult_q31(&C, x, y);
        arm_mat_vec_mult_q31(&A, x, x1);
        arm_mat_vec_mult_q31(&B, u, x);
        arm_add_q31(x1, x, x, N);

        for(uint16_t j = 0; j < M; j++){
            tx_buf->channels[j] = y[j];
        }
    }
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}

void audio(void* p){
    // job_struct_t* pj = (job_struct_t*)p;
    while(1){
        // stereo_t* data = jes_wait_for_notification();
        // __job_set_timing_begin(__get_systime_ms(), pj);
        
        if(hlf_rdy){
            hlf_rdy = 0;
            dsp(&tx_buf[0], BLOCK_SIZE/2);
        }
        if(rdy){
            rdy = 0;
            dsp(&tx_buf[BLOCK_SIZE/2], BLOCK_SIZE/2);
        }
        // dsp(data, BLOCK_SIZE/2);
        
        // __job_set_timing_end(__get_systime_ms(), pj);
    }
}

void port_setup(){
    jes_err_t e = jes_init();
    if(e != e_err_no_err){
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        while(1);
    }
    // e = jes_register_and_launch_job("_audio", 1024, 1, audio, 1);
    // if(e != e_err_no_err){
    //     HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    //     while(1);
    // }
    HAL_StatusTypeDef stat;
    if ((stat = HAL_SAI_Receive_DMA(&hsai_BlockA1, (uint8_t*)rx_buf, BLOCK_SIZE*2)) != HAL_OK){
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        while(1);
    }
    if ((stat = HAL_SAI_Transmit_DMA(&hsai_BlockB1, (uint8_t*)tx_buf, BLOCK_SIZE*2)) != HAL_OK){
        HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
        while(1);
    }
    memset(x, 0, N*sizeof(q31_t));
    memset(x1, 0, N*sizeof(q31_t));
    arm_mat_init_q31(&test, BLOCK_SIZE/2, 2, (q31_t*)tx_buf);
    arm_mat_init_q31(&A, N, N, (q31_t*)_A);
    arm_mat_init_q31(&B, N, M, (q31_t*)_B);
    arm_mat_init_q31(&C, P, N, (q31_t*)_C);
    arm_mat_init_q31(&D, P, M, (q31_t*)_D);
    audio(NULL);
    // jes_dispatch();
}

void port_loop(){

}

void HAL_SAI_RxHalfCpltCallback(SAI_HandleTypeDef* hsai) {
    memcpy((void*)&tx_buf[0], (const void*)&rx_buf[0], (BLOCK_SIZE/2) * sizeof(stereo_t));
    hlf_rdy = 1;
    // HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    // jes_notify_job_ISR("_audio", &tx_buf[0]);

}

void HAL_SAI_RxCpltCallback(SAI_HandleTypeDef* hsai) {
    memcpy((void*)&tx_buf[BLOCK_SIZE/2], (const void*)&rx_buf[BLOCK_SIZE/2], (BLOCK_SIZE/2) * sizeof(stereo_t));
    rdy = 1;
    // HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    // jes_notify_job_ISR("_audio", &tx_buf[BLOCK_SIZE/2]);
}
