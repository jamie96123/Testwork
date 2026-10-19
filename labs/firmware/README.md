# 新版板子程式：Lab1–5 整合

**適用 NUCLEO-G474RE。不要沿用舊版 TIM2/PA0/PA1/PB10 接線或整份 main.c。**

所有遊戲邏輯在網頁，板子只接收字串控制 LED。綠燈 PC8 有約 2 秒呼吸效果；黃燈 PC9、紅燈 PC6 恆亮。網頁暫停：倒數凍結、綠燈改恆亮；繼續：恢復倒數與選用的呼吸效果；重置／一局結束：三燈全關。

## 1. 斷電接線

| 功能 | MCU 腳位 | CubeMX 功能 | 接法 |
| --- | --- | --- | --- |
| 綠 LED | PC8 | TIM3_CH3，AF2 | 腳位 → 獨立 330Ω–1kΩ 電阻 → LED 陽極；陰極接 GND |
| 黃 LED | PC9 | TIM3_CH4，AF2 | 同上，第二個獨立電阻 |
| 紅 LED | PC6 | GPIO_Output，Push Pull，預設 Low | 同上，第三個獨立電阻 |
| UART TX | PA2 | LPUART1_TX，AF12 | Nucleo 預設 ST-LINK VCP 路由；確認板卡焊橋 |
| UART RX | PA3 | LPUART1_RX，AF12 | 同上 |
| 除錯 | PA13/PA14 | Serial Wire | 保留 SWD |

三顆 LED 為高電位點亮、負極共地。不用實體按鈕，也不用外接 Wi-Fi／USB 通訊模組。PC8、PC9 是 MCU 名稱，不是排針第 8/9 腳；依你的板卡原理圖確認對應排針。不要把 5V 接到 GPIO。

## 2. 用 CubeMX 設定，不手寫生成函式

1. 在既有 STM32G474RE 專案設定上述腳位。
2. LPUART1：Asynchronous、115200 baud、8 bits、No parity、1 stop bit、TX/RX、No flow control。NVIC 啟用 LPUART1 global interrupt，priority=5。
3. TIM3：Internal Clock，CH3/CH4 均為 PWM Generation CHx、PWM mode 1、Polarity High。
4. Clock Configuration 確認 **APB1 timer clocks = 170 MHz**。若不相同，不可照抄參數，需依教材公式重算。
5. TIM3 Prescaler=169、Counter Period=1999、CH3 Pulse=0、CH4 Pulse=0。頻率為 170000000 / (170×2000) = 500 Hz。預設兩路 duty=0，開機全滅。
6. PC6 Output Push Pull、No pull、初始 Low。PC8/PC9 不可同時設為 GPIO Output。
7. Generate Code，保留 HAL UART/TIM、SysTick、SWD。確認產生的 handle 為 `hlpuart1`、`htim3`。

## 3. 貼入 USER CODE（推薦）

下載 [main_USER_CODE.txt](main_USER_CODE.txt)，裡面包含四段完整內容，依標記分別貼進 main.c：

- Includes → 加 `#include <string.h>`。
- USER CODE BEGIN 0 → 所有變數與函式。
- USER CODE BEGIN 2 → 在 MX_GPIO_Init、MX_LPUART1_UART_Init、MX_TIM3_Init 之後呼叫 `Lab_Init();`。
- USER CODE BEGIN 3 → 迴圈內呼叫 `Lab_Task();`，不加 HAL_Delay。

原有 USER CODE 若有舊版 UART 接收、重複的 HAL_UART_RxCpltCallback/ErrorCallback、GPIO LED 控制，請先備份並移除或合併，不能同時接收或重複定義 callback。

CubeMX 若選擇把初始化分到 `usart.c`／`tim.c`，USER CODE 0 內的 extern 宣告仍適用，記得原專案正常呼叫相應 MX 初始化。

### UART IRQ

在 `stm32g4xx_it.c` user include 區加入（若已存在勿重複）：

```c
extern UART_HandleTypeDef hlpuart1;
```

確認同檔案僅有一份：

```c
void LPUART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&hlpuart1);
}
```

CubeMX 已產生時只檢查，不要重複貼。SysTick_Handler 需維持 CubeMX 原有 HAL_IncTick，不要刪除。

### 可選：獨立 .c 模組

若偏好分檔，可將 `main_user_code.h` 放 Core/Inc、`.c` 放 Core/Src，在 main.c USER CODE Includes 引入該 header，USER CODE 2/3 呼叫同樣兩個函式。**分檔與整段貼入擇一**，不可同時使用。模組不更動任何 MX 初始化。

## 4. 編譯、燒錄、驗證

1. CubeIDE Build Project，確認生成 ELF。此網站不包含完整 `.ioc`、HAL SDK、啟動碼與連結腳本，所以沒有可直接燒錄的 HEX。
2. ST-LINK 接板子後 Run/Debug 燒錄。按 RESET，初始三燈全滅。
3. 桌面 Chrome/Edge 開啟新版 HTTPS 網址，按「連接 STM32」選 VCP。收到 `HELLO WORLD` 和 `OK HELLO`，才顯示連線成功。SerialAssistant 不能同時占用 COM 埠。
4. 網頁手動測試綠燈：勾選呼吸，綠燈約 2 秒暗→亮→暗；取消呼吸應恆亮。測試黃／紅，只亮對應燈；全關三燈皆滅。
5. 開始拼字：≥60% 綠、30–59% 黃、<30% 紅；暫停時間不前進，綠燈停止呼吸改恆亮，繼續恢復；重置／第 10 題完成後全關。
6. 切換速度於下一題生效，不重設暫停中的題目時間。背景分頁自動暫停。
7. UART 出現 `ERR UART RESET REQUIRED` 時，斷開網頁連線、按板子 RESET 再連線，不宣稱能自動恢復硬體錯誤。

瀏覽器「中斷連線」會先送全關並等確認。直接拔線、關閉視窗或電腦休眠不能保證送出關燈指令；此教材版沒有額外 watchdog，必要時按板子 RESET。網站上的燈光是模擬圖，不是 LED 感測器；`BOARD ACK` 表示韌體確認收到，不能代替查看實體接線。

## 通訊協定

ASCII，LF 結尾，CR 忽略；31 字元以內。未知與過長指令不改輸出。

| 指令 | 行為 |
| --- | --- |
| GREEN ON / GREEN OFF | 只開／關綠燈，不改另兩燈 |
| RED ON / RED OFF | 教材原有紅燈控制 |
| YELLOW ON / YELLOW OFF | 以相同方式控制 TIM3 CH4 黃燈 |
| BREATHE ON / BREATHE OFF | 啟用／停用 PC8 呼吸；不自動打開綠燈 |
| HELLO | 回傳 HELLO WORLD，用於開機後再連線 |

合法指令回 `OK 原指令`，其他回 `ERR UNKNOWN`、`ERR LINE`、`ERR OVERFLOW`。網頁每次等待 OK 才傳下一條，避免產生無限制佇列。輸出切換由短串逐條指令完成，先全關再打開目標色，不保證多條指令原子切換。

Ring buffer 256 bytes，保留一格；溢位記錄 rxDropped，清空緩衝／半行並丟棄到 LF，之後可恢復新指令。這是串流復原規則，不保證滿載前已收到但未執行的指令仍會保留。

## 誠實的驗證範圍

提供的是可放入 CubeMX USER CODE 的完整應用層；雲端測試使用 HAL 替身驗證 UART 組行、指令、FIFO／溢位、PWM 計算、2 秒呼吸、Tick 回繞與錯誤鎖定，不能代替真正 ARM 建置與板上量測。實體 PWM 頻率、亮度與 UART 連線需依你的 CubeIDE 專案及接線實測。
