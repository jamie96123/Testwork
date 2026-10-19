#ifndef LAB_MAIN_USER_CODE_H
#define LAB_MAIN_USER_CODE_H
#include "main.h"
/* CubeMX 產生的 handle，不在此重新建立周邊。 */
extern UART_HandleTypeDef hlpuart1;
extern TIM_HandleTypeDef htim3;
void Lab_Init(void);
void Lab_Task(void);
#endif
