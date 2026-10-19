/* Standalone main.c alternative. Do not also compile typerush.c. */
#include "main.h"
#include <string.h>

UART_HandleTypeDef typerush_uart;
static TIM_HandleTypeDef pwm;
static uint8_t rx;
static volatile uint8_t ring[512];
static volatile uint16_t head, tail;
static volatile uint8_t overflow;
static char line[80];
static uint8_t length, discard, running;
static int color = -1;
static uint32_t last_tick, last_command;
static const uint32_t channels[] = {TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3};
extern void Error_Handler(void);
static void check(HAL_StatusTypeDef result) { if (result != HAL_OK) Error_Handler(); }
static void reply(const char *s) { HAL_UART_Transmit(&typerush_uart,(uint8_t *)s,(uint16_t)strlen(s),100); }
static void lamps(uint32_t duty) {
  for (unsigned i=0;i<3;i++) __HAL_TIM_SET_COMPARE(&pwm,channels[i],color==(int)i?duty:0);
}
static int get_color(const char *s) {
  if(!strcmp(s,"red")) return 0;
  if(!strcmp(s,"yellow")) return 1;
  if(!strcmp(s,"green")) return 2;
  return -1;
}
static void command(void) {
  if(!strcmp(line,"PING")) { last_command=HAL_GetTick();reply("ACK PING\n");return; }
  if(!strcmp(line,"OFF")) {color=-1;running=0;lamps(0);last_command=HAL_GetTick();reply("ACK OFF\n");return;}
  if(!strncmp(line,"LED ",4)) {
    int c=get_color(line+4);
    if(c>=0){color=c;running=0;lamps(999);last_command=HAL_GetTick();reply("ACK LED\n");return;}
  }
  if(!strncmp(line,"STATE ",6)) {
    char *mode=strchr(line+6,' ');
    if(mode){*mode++=0;int c=get_color(line+6);
      if(c>=0 && (!strcmp(mode,"RUN")||!strcmp(mode,"PAUSE"))){color=c;running=!strcmp(mode,"RUN");lamps(999);last_command=HAL_GetTick();reply("ACK STATE\n");return;}
    }
  }
  reply("ERR COMMAND\n");
}
void TypeRush_Init(void) {
  __HAL_RCC_GPIOA_CLK_ENABLE();__HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_LPUART1_CLK_ENABLE();__HAL_RCC_TIM2_CLK_ENABLE();
  /* Explicit clock source: APB1, matching the supplied clock configuration. */
  RCC_PeriphCLKInitTypeDef clock={0};
  clock.PeriphClockSelection=RCC_PERIPHCLK_LPUART1;
  clock.Lpuart1ClockSelection=RCC_LPUART1CLKSOURCE_PCLK1;
  check(HAL_RCCEx_PeriphCLKConfig(&clock));
  GPIO_InitTypeDef gpio={0};
  gpio.Pin=GPIO_PIN_2|GPIO_PIN_3;gpio.Mode=GPIO_MODE_AF_PP;gpio.Pull=GPIO_NOPULL;gpio.Speed=GPIO_SPEED_FREQ_LOW;gpio.Alternate=GPIO_AF12_LPUART1;HAL_GPIO_Init(GPIOA,&gpio);
  typerush_uart.Instance=LPUART1;typerush_uart.Init.BaudRate=115200;
  typerush_uart.Init.WordLength=UART_WORDLENGTH_8B;typerush_uart.Init.StopBits=UART_STOPBITS_1;
  typerush_uart.Init.Parity=UART_PARITY_NONE;typerush_uart.Init.Mode=UART_MODE_TX_RX;
  typerush_uart.Init.HwFlowCtl=UART_HWCONTROL_NONE;typerush_uart.Init.OneBitSampling=UART_ONE_BIT_SAMPLE_DISABLE;
  typerush_uart.Init.ClockPrescaler=UART_PRESCALER_DIV1;
  check(HAL_UART_Init(&typerush_uart));
  gpio.Pin=GPIO_PIN_0|GPIO_PIN_1;gpio.Alternate=GPIO_AF1_TIM2;HAL_GPIO_Init(GPIOA,&gpio);
  gpio.Pin=GPIO_PIN_10;HAL_GPIO_Init(GPIOB,&gpio);
  uint32_t timer_clock=HAL_RCC_GetPCLK1Freq();
  if((RCC->CFGR & RCC_CFGR_PPRE1)!=0)timer_clock*=2;
  if(timer_clock<1000000 || timer_clock%1000000!=0)Error_Handler();
  pwm.Instance=TIM2;pwm.Init.Prescaler=timer_clock/1000000-1;pwm.Init.CounterMode=TIM_COUNTERMODE_UP;
  pwm.Init.Period=999;pwm.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1;pwm.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;
  check(HAL_TIM_PWM_Init(&pwm));
  TIM_OC_InitTypeDef oc={0};oc.OCMode=TIM_OCMODE_PWM1;oc.Pulse=0;oc.OCPolarity=TIM_OCPOLARITY_HIGH;oc.OCFastMode=TIM_OCFAST_DISABLE;
  for(unsigned i=0;i<3;i++){check(HAL_TIM_PWM_ConfigChannel(&pwm,&oc,channels[i]));check(HAL_TIM_PWM_Start(&pwm,channels[i]));}
  HAL_NVIC_SetPriority(LPUART1_IRQn,2,0);HAL_NVIC_EnableIRQ(LPUART1_IRQn);
  check(HAL_UART_Receive_IT(&typerush_uart,&rx,1));
  reply("READY TYPERUSH/1\n");
}
void TypeRush_RxComplete(UART_HandleTypeDef *uart) {
  if(uart->Instance!=LPUART1)return;
  uint16_t next=(head+1)&511;
  if(next==tail)overflow=1;else{ring[head]=rx;head=next;}
  HAL_UART_Receive_IT(&typerush_uart,&rx,1);
}
void TypeRush_UartError(UART_HandleTypeDef *uart) {
  if(uart->Instance!=LPUART1)return;
  overflow=1;
  HAL_UART_AbortReceive(uart);
  HAL_UART_Receive_IT(uart,&rx,1);
}
void TypeRush_Task(void) {
  if(overflow){__disable_irq();tail=head;overflow=0;__enable_irq();length=0;discard=1;reply("ERR OVERFLOW\n");}
  while(tail!=head){uint8_t c=ring[tail];tail=(tail+1)&511;
    if(c=='\r')continue;
    if(c=='\n'){if(!discard&&length){line[length]=0;command();}length=0;discard=0;}
    else if(!discard){if(c<32||c>126||length>=sizeof(line)-1){discard=1;length=0;reply("ERR LINE\n");}else line[length++]=(char)c;}
  }
  uint32_t now=HAL_GetTick();
  /* Turn lights off if the browser stops sending its heartbeat. */
  if(color>=0 && now-last_command>5000){color=-1;running=0;lamps(0);}
  if(now-last_tick<10)return;last_tick=now;
  if(running){uint32_t t=now%1600;uint32_t triangle=t<800?t:1600-t;lamps(100+triangle*899/800);}
}

void SystemClock_Config(void);
int main(void) {
  HAL_Init();
  SystemClock_Config();
  TypeRush_Init();
  while (1) TypeRush_Task();
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  TypeRush_RxComplete(huart);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  TypeRush_UartError(huart);
}
void Error_Handler(void) {
  __disable_irq();
  while (1) {}
}
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

