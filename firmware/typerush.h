#ifndef TYPERUSH_H
#define TYPERUSH_H
#include "stm32g4xx_hal.h"
extern UART_HandleTypeDef typerush_uart;
void TypeRush_Init(void);
void TypeRush_Task(void);
void TypeRush_RxComplete(UART_HandleTypeDef *uart);
void TypeRush_UartError(UART_HandleTypeDef *uart);
#endif
