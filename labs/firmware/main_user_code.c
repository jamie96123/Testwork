#include "main_user_code.h"
#include <string.h>

/* Lab4 BASIC p16：256 格環形 Buffer，留一格可用 255 bytes。 */
#define RX_RING_SIZE 256U
#define LINE_SIZE 32U
#define PWM_TOP 1999U
static uint8_t rxByte;
static volatile uint8_t rxRing[RX_RING_SIZE];
static volatile uint16_t rxHead, rxTail;
static volatile uint32_t rxDropped;
static volatile uint8_t rxOverflow, uartFault;
static uint8_t faultReported;
static char lineBuffer[LINE_SIZE];
static uint8_t lineLength, discardLine;

/* Lab3：用一般變數記住狀態，不使用 enum 或 typedef。 */
static uint8_t greenOn, yellowOn, redOn, breathe;
static uint16_t breathStep;
static uint32_t previousTick;

/* Lab4：文字傳送僅在主迴圈執行，不在接收回呼內等待。 */
static void Reply(const char *text)
{
    if (HAL_UART_Transmit(&hlpuart1, (uint8_t *)text,
                         (uint16_t)strlen(text), 100U) != HAL_OK)
        Error_Handler();
}

/* Lab5：兩路 TIM3 PWM，CCR 不超過 ARR。紅燈沿用 Lab4 PC6。 */
static void ApplyOutputs(void)
{
    uint32_t greenCCR = 0U;
    if (greenOn) {
        if (breathe) {
            /* 200 個 10 ms 步驟 = 約 2 秒；暗 → 亮 → 暗。 */
            uint32_t level = breathStep <= 100U ? breathStep : 200U - breathStep;
            greenCCR = level * PWM_TOP / 100U;
        } else {
            greenCCR = PWM_TOP;
        }
    }
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, greenCCR);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, yellowOn ? PWM_TOP : 0U);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, redOn ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/* Lab4 BASIC p17：接收中斷只負責保存 byte。 */
static void RingPush(uint8_t data)
{
    uint16_t next = (rxHead + 1U) % RX_RING_SIZE;
    if (next == rxTail) {
        rxDropped++;
        rxOverflow = 1U;
        return;
    }
    rxRing[rxHead] = data;
    rxHead = next;
}

/* Lab4 BASIC p18：FIFO 取出與回繞。 */
static uint8_t RingPop(uint8_t *data)
{
    if (rxHead == rxTail) return 0U;
    *data = rxRing[rxTail];
    rxTail = (rxTail + 1U) % RX_RING_SIZE;
    return 1U;
}

/* 指令字串是本專題應用層；GPIO/UART/PWM 方法均來自教材。 */
static void ExecuteCommand(const char *command)
{
    if (strcmp(command, "HELLO") == 0) {
        Reply("HELLO WORLD\r\n");
    } else if (strcmp(command, "GREEN ON") == 0) {
        if (!greenOn) { breathStep = 0U; previousTick = HAL_GetTick(); }
        greenOn = 1U;
    } else if (strcmp(command, "GREEN OFF") == 0) {
        greenOn = 0U;
    } else if (strcmp(command, "RED ON") == 0) {
        redOn = 1U;
    } else if (strcmp(command, "RED OFF") == 0) {
        redOn = 0U;
    } else if (strcmp(command, "YELLOW ON") == 0) {
        yellowOn = 1U;
    } else if (strcmp(command, "YELLOW OFF") == 0) {
        yellowOn = 0U;
    } else if (strcmp(command, "BREATHE ON") == 0) {
        if (!breathe) { breathStep = 0U; previousTick = HAL_GetTick(); }
        breathe = 1U;
    } else if (strcmp(command, "BREATHE OFF") == 0) {
        breathe = 0U;
    } else {
        Reply("ERR UNKNOWN\r\n");
        return; /* 未知指令保持原有 LED 狀態。 */
    }
    ApplyOutputs();
    /* 回覆使用同一個短緩衝區，瀏覽器收到確認才送下一條。 */
    char response[LINE_SIZE + 8U] = "OK ";
    strcat(response, command);
    strcat(response, "\r\n");
    Reply(response);
}

/* LF 才執行，CR 忽略；過長或非法字元整行丟棄。 */
static void ProcessReceivedByte(uint8_t data)
{
    if (data == '\r') return;
    if (data == '\n') {
        if (!discardLine && lineLength != 0U) {
            lineBuffer[lineLength] = '\0';
            ExecuteCommand(lineBuffer);
        }
        lineLength = 0U;
        discardLine = 0U;
    } else if (!discardLine) {
        if (data < 32U || data > 126U || lineLength >= LINE_SIZE - 1U) {
            lineLength = 0U;
            discardLine = 1U;
            Reply("ERR LINE\r\n");
        } else {
            lineBuffer[lineLength++] = (char)data;
        }
    }
}

void Lab_Init(void)
{
    /* 需先由 CubeMX 初始化 LPUART1、TIM3、GPIO。 */
    greenOn = yellowOn = redOn = breathe = 0U;
    ApplyOutputs(); /* 初始三燈全滅。 */
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3) != HAL_OK) Error_Handler();
    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4) != HAL_OK) Error_Handler();
    HAL_NVIC_SetPriority(LPUART1_IRQn, 5U, 0U);
    HAL_NVIC_EnableIRQ(LPUART1_IRQn);
    if (HAL_UART_Receive_IT(&hlpuart1, &rxByte, 1U) != HAL_OK) Error_Handler();
    previousTick = HAL_GetTick();
    Reply("HELLO WORLD\r\n");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != LPUART1) return;
    RingPush(rxByte);
    if (HAL_UART_Receive_IT(huart, &rxByte, 1U) != HAL_OK) uartFault = 1U;
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == LPUART1) uartFault = 1U;
}

void Lab_Task(void)
{
    /* 錯誤只在此通知；UART 硬體錯誤請用板子 RESET 重新啟動。 */
    if (uartFault) {
        if (faultReported) return;
        faultReported = 1U;
        greenOn = yellowOn = redOn = breathe = 0U;
        ApplyOutputs();
        Reply("ERR UART RESET REQUIRED\r\n");
        return;
    }
    if (rxOverflow) {
        /* 只暫停此 UART IRQ；避免與 ISR 同時更新索引。 */
        HAL_NVIC_DisableIRQ(LPUART1_IRQn);
        rxTail = rxHead;
        rxOverflow = 0U;
        HAL_NVIC_EnableIRQ(LPUART1_IRQn);
        lineLength = 0U;
        discardLine = 1U; /* 捨棄至下一個 LF，再接受新指令。 */
        Reply("ERR OVERFLOW\r\n");
    }
    uint8_t data;
    while (RingPop(&data)) {
        if (rxOverflow) break;
        ProcessReceivedByte(data);
    }
    /* Lab3 + Lab5：非阻塞，每 10 ms 更新 CCR；無 HAL_Delay。 */
    uint32_t now = HAL_GetTick();
    uint32_t elapsed = now - previousTick;
    if (elapsed >= 10U) {
        uint32_t steps = elapsed / 10U;
        previousTick += steps * 10U;
        if (greenOn && breathe) {
            breathStep = (uint16_t)((breathStep + steps) % 200U);
            ApplyOutputs();
        }
    }
}
