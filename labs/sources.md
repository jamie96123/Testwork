# 教材對照與實作邊界

本版以你提供的 7 份 PPT 為來源；`Lab4_BASIC_v11_Buffer_200ms_FIXED (1).pptx` 與無 `(1)` 的版本 SHA-256 相同，屬於同一份教材，共 6 份不同內容、156 張不同投影片。

## 採用內容

| 來源 | 投影片 | 本版使用方式 |
| --- | --- | --- |
| Lab1 | 4、10、21、23、27 | NUCLEO-G474RE、HAL、CubeMX 設定腳位、保留生成程式、LED 限流 |
| Lab2 | 5、7–11、24–25 | 區分輪詢與中斷、中斷不可長時間等待；不需要實體按鈕，因此不設定 EXTI |
| Lab3 | 4、6、8–9、14 | 狀態變數、HAL_GetTick 非阻塞計時、切換模式重設時間；板子不使用 enum／typedef 管理狀態 |
| Lab4_UART | 4–5、9–13、15–20 | ST-LINK VCP、LPUART1 PA2/PA3、115200/8N1、LF/CR、未知／超長行、FIFO 及溢位復原 |
| Lab4 BASIC v11 | 2–5、8–10、15–19 | PC8／PC6、HELLO WORLD、strcmp、IRQ、HAL_UART_Receive_IT、256-byte ring buffer、callback 只保存 byte |
| Lab5 Timer PWM v3 | 13–15、18–25、35–39 | TIM3 CH3 PC8／CH4 PC9、AF2、500 Hz 計算、PWM Start、每 10 ms 改 CCR、約 2 秒 PC8 呼吸燈 |

## 教材間不同版本如何處理

1. **LED 腳位不一致**：Lab1、Lab3 中不同頁面對顏色的對應不同。本版沿用較後面的 Lab4：綠 PC8、紅 PC6；依 Lab5 將 PC8 改為 TIM3_CH3，第二路 PC9/TIM3_CH4 接黃色 LED。黃色只是第二顆 LED 的應用命名，不增加教材外的周邊。舊網站的 PA0／PA1／PB10 已不適用本版。
2. **UART 輪詢 vs 中斷**：Lab4_UART 使用輪詢＋128-byte ring buffer；BASIC v11 使用中斷＋256-byte ring buffer。本版明確採後者，不把輪詢與中斷同時開啟。
3. **200 ms 延遲**：BASIC 以 HAL_Delay(200) 模擬忙碌，為比較接收效率的對照實驗。Lab5 呼吸燈明確不使用 HAL_Delay，因此整合版不放入人為延遲，也不使用每 200 ms 才處理一條指令的實驗節奏。
4. **PWM 頻率**：採 Lab5 p15 的 500 Hz 參數 PSC=169、ARR=1999，前提 TIM3 clock=170 MHz。CCR 上限 1999，遵守 p38 不超過 ARR；恆亮模式約 99.95% duty。Lab5 的 5 Hz Basic 是觀察閃爍用，不能拿來做無明顯閃爍的呼吸燈。
5. **不用整檔重寫初始化**：主要交付是可貼入既有 main.c 的 USER CODE 程式與精確設定表。沒有提供自行手寫的 MX_TIM3_Init 或 SystemClock_Config；它們由 CubeMX 產生。

## 應用層不冒充教材原文

教材沒有網頁、Web Serial 或拼字遊戲。網頁為達成你的需求所需的應用層；拼字玩法保留先前專題的三階計分，不宣稱來自 Lab1–5。

教材原有四條 `GREEN ON/OFF`、`RED ON/OFF` 原樣保留。`YELLOW ON/OFF`、`BREATHE ON/OFF`、`HELLO` 與 `OK ...`／`ERR ...` 是專題所需的文字協定擴充，實作仍只使用教材的字串判斷、HAL UART、GPIO、TIM3 PWM、Tick 與緩衝區技術。沒有加入 DMA、RTOS、Wi-Fi、USB CDC、新 Timer 或獨立遊戲按鈕。

呼吸燈限綠色 PC8，對應 Lab5 單顆呼吸燈 Challenge。黃 PC9 與紅 PC6 恆亮；不把 PC6 GPIO 假裝成 PWM。
