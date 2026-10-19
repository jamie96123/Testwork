#ifndef TEST_HAL_MAIN_H
#define TEST_HAL_MAIN_H
#include <stdint.h>
#include <stddef.h>
typedef struct { void *Instance; } UART_HandleTypeDef;
typedef struct { uint32_t unused; } TIM_HandleTypeDef;
typedef int HAL_StatusTypeDef;
#define HAL_OK 0
#define HAL_ERROR 1
#define LPUART1 ((void *)0x40008000)
#define GPIOC ((void *)0x48000800)
#define GPIO_PIN_6 64U
#define GPIO_PIN_SET 1U
#define GPIO_PIN_RESET 0U
#define TIM_CHANNEL_3 3U
#define TIM_CHANNEL_4 4U
#define LPUART1_IRQn 70
void test_compare(uint32_t channel, uint32_t value);
#define __HAL_TIM_SET_COMPARE(handle, channel, value) test_compare(channel, value)
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *,uint8_t *,uint16_t,uint32_t);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *,uint8_t *,uint16_t);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *,uint32_t);
void HAL_GPIO_WritePin(void *,uint16_t,uint32_t);
void HAL_NVIC_SetPriority(int,uint32_t,uint32_t);
void HAL_NVIC_EnableIRQ(int);
void HAL_NVIC_DisableIRQ(int);
uint32_t HAL_GetTick(void);
void Error_Handler(void);
#endif
