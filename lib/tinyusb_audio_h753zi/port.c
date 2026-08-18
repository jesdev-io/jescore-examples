#include "usb_audio_h753zi_port.h"
#include "tusb.h"
#include "FreeRTOS.h"
#include "task.h"

#define LED_PORT GPIOB
#define LED_GPIO GPIO_PIN_0
#define LED_ENABLE_PORT() __HAL_RCC_GPIOB_CLK_ENABLE()

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void Error_Handler(void);

uint32_t tusb_time_millis_api(void) {
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
  }
  return HAL_GetTick();
}

void tusb_time_delay_ms_api(uint32_t ms) {
  uint32_t cycles_per_ms = SystemCoreClock / 8000U;
  if (cycles_per_ms == 0U) cycles_per_ms = 1U;
  for (uint32_t m = 0; m < ms; m++) {
    for (volatile uint32_t i = 0; i < cycles_per_ms; i++) __NOP();
  }
}

void OTG_FS_IRQHandler(void) { tud_int_handler(0); }
void OTG_FS_EP1_OUT_IRQHandler(void) { tud_int_handler(0); }
void OTG_FS_EP1_IN_IRQHandler(void) { tud_int_handler(0); }
void OTG_FS_WKUP_IRQHandler(void) { tud_int_handler(0); }
void OTG_HS_IRQHandler(void) { tud_int_handler(0); }
void OTG_HS_EP1_OUT_IRQHandler(void) { tud_int_handler(0); }
void OTG_HS_EP1_IN_IRQHandler(void) { tud_int_handler(0); }
void OTG_HS_WKUP_IRQHandler(void) { tud_int_handler(0); }

int main(void) {
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();

  port_setup();
  while (1) {
    port_loop();
  }
}

void port_usb_peripheral_init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_USB_OTG_FS_CLK_ENABLE();
  HAL_PWREx_EnableUSBVoltageDetector();

  GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  HAL_NVIC_SetPriority(OTG_FS_IRQn, 7, 0);
  HAL_NVIC_EnableIRQ(OTG_FS_IRQn);
}

void port_usb_set_mounted_led(bool mounted) {
  HAL_GPIO_WritePin(LED_PORT, LED_GPIO, mounted ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI | RCC_OSCILLATORTYPE_HSI48;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV1;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) Error_Handler();

  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USB;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  LED_ENABLE_PORT();
  GPIO_InitStruct.Pin = LED_GPIO;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_PORT, &GPIO_InitStruct);
  HAL_GPIO_WritePin(LED_PORT, LED_GPIO, GPIO_PIN_RESET);
}

static void Error_Handler(void) {
  __disable_irq();
  while (1) {}
}
