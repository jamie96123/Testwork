#include "hal/main.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
UART_HandleTypeDef hlpuart1={LPUART1};
TIM_HandleTypeDef htim3;
static uint32_t tick,green_ccr,yellow_ccr,red_gpio,starts;
static uint8_t fail_receive;
static char output[200000];
void test_compare(uint32_t channel,uint32_t value){assert(value<=1999);if(channel==3)green_ccr=value;else if(channel==4)yellow_ccr=value;else abort();}
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *u,uint8_t *s,uint16_t n,uint32_t timeout){(void)u;(void)timeout;assert(strlen(output)+n<sizeof(output));strncat(output,(char*)s,n);return HAL_OK;}
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *u,uint8_t *s,uint16_t n){(void)u;(void)s;assert(n==1);return fail_receive?HAL_ERROR:HAL_OK;}
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *h,uint32_t c){(void)h;assert(c==3||c==4);starts++;return HAL_OK;}
void HAL_GPIO_WritePin(void *p,uint16_t pin,uint32_t v){assert(p==GPIOC&&pin==GPIO_PIN_6);red_gpio=v;}
void HAL_NVIC_SetPriority(int irq,uint32_t p,uint32_t sub){assert(irq==70&&p==5&&sub==0);}
void HAL_NVIC_EnableIRQ(int irq){assert(irq==70);}
void HAL_NVIC_DisableIRQ(int irq){assert(irq==70);}
uint32_t HAL_GetTick(void){return tick;}
void Error_Handler(void){abort();}
#include "../firmware/main_user_code.c"
static void feed(const char *s){while(*s){rxByte=(uint8_t)*s++;HAL_UART_RxCpltCallback(&hlpuart1);}}
static void run(const char *s){feed(s);Lab_Task();}
int main(void){
 Lab_Init();assert(starts==2&&green_ccr==0&&yellow_ccr==0&&!red_gpio);assert(strstr(output,"HELLO WORLD"));
 feed("GREEN ON");Lab_Task();assert(!green_ccr);run("\r\n");assert(green_ccr==1999);assert(strstr(output,"OK GREEN ON"));
 run("RED ON\n");assert(red_gpio&&green_ccr==1999);run("GREEN OFF\n");assert(!green_ccr&&red_gpio);run("RED OFF\n");assert(!red_gpio);
 run("YELLOW ON\n");assert(yellow_ccr==1999);run("BAD\n");assert(yellow_ccr==1999);assert(strstr(output,"ERR UNKNOWN"));
 run("YELLOW OFF\nGREEN ON\nBREATHE ON\n");assert(green_ccr==0&&yellow_ccr==0);
 tick=500;Lab_Task();assert(green_ccr==999);tick=1000;Lab_Task();assert(green_ccr==1999);tick=1500;Lab_Task();assert(green_ccr==999);tick=2000;Lab_Task();assert(!green_ccr);
 run("BREATHE OFF\n");assert(green_ccr==1999);tick+=2000;Lab_Task();assert(green_ccr==1999);
 // Long line that starts with a valid command must never execute the prefix.
 run("GREEN OFF\n");feed("GREEN ON");for(int i=0;i<50;i++)feed("X");run("\n");assert(!green_ccr);assert(strstr(output,"ERR LINE"));run("GREEN ON\n");assert(green_ccr==1999);
 // Ring wrap preserves order across many cycles.
 for(int i=0;i<100;i++){run("RED ON\nRED OFF\n");assert(!red_gpio);}
 // Full ring: report drops, discard through LF, then recover.
 for(int i=0;i<300;i++){feed("X");}
 assert(rxDropped>0);Lab_Task();assert(strstr(output,"ERR OVERFLOW"));run("\nRED ON\n");assert(red_gpio);
 // unsigned Tick rollover still advances a 10ms breath step.
 run("GREEN OFF\nBREATHE OFF\n");tick=UINT32_MAX-4U;run("GREEN ON\nBREATHE ON\n");tick=5;Lab_Task();assert(breathStep==1&&green_ccr==19);
 // Other UART instances do not enqueue bytes.
 UART_HandleTypeDef other={(void*)123};uint16_t oldHead=rxHead;HAL_UART_RxCpltCallback(&other);assert(oldHead==rxHead);
 // Error is latched off until reset and cannot replay buffered LED commands.
 feed("RED ON\n");HAL_UART_ErrorCallback(&hlpuart1);Lab_Task();assert(!red_gpio&&!green_ccr&&!yellow_ccr);Lab_Task();assert(!red_gpio);assert(strstr(output,"ERR UART RESET REQUIRED"));
 puts("PASS firmware: GPIO, TIM3 CCR bounds, independent commands, LF/CR, unknown/long lines, 2s breathing, steady pause, FIFO wrap, overflow recovery, Tick rollover and UART error lockout");
}
