# TypeRush｜STM32 節奏英打測速挑戰機

依據專題 PDF 製作的繁體中文拼字遊戲，搭配 STM32G474RE 外接紅、黃、綠 LED。網站不需要框架、npm 套件或後端；使用 Web Serial 直接連接電腦上的序列埠。

## 開啟網頁

在**插著板子的電腦**安裝 Python 3，於此資料夾執行：

```sh
python3 -m http.server 8000 --bind 127.0.0.1
```

Windows 可改用 `py -m http.server 8000 --bind 127.0.0.1`。在桌面 Chrome／Edge 開啟 `http://localhost:8000`。不要直接開啟 HTML 檔案。網站部署時需 HTTPS，Web Serial 不適用一般手機瀏覽器或 Firefox／Safari。

1. 不連硬體也可直接「開始挑戰」，模擬三色燈光。
2. 每局 10 題，每題可選 15／10／6 秒。輸入單字後按 Enter。
3. 正確時依剩餘時間：≥60% 綠燈 +7、≥30% 黃燈 +5、<30% 紅燈 +3；逾時 0 分。
4. 拼錯可以在期限內修正；正確率以答對題數／已完成題數計算。連續答對遇逾時歸零。
5. 暫停會凍結時間、LED 恆亮；繼續時恢復呼吸燈。重新挑戰會清除當局分數。
6. 所有遊戲控制都在網頁操作，不需要實體按鈕。網頁暫停時修改限時會重設目前題目倒數。

## 硬體與接線

預設為 **NUCLEO-G474RE（MB1367）**，不是 PDF 圖示中的 STM32F103 Blue Pill。若你使用 G474RE 裸晶片或其他開發板，須依實際原理圖確認接腳，並以 3.3V USB-UART 轉接器連接 LPUART1。先斷電再接線；不要將 5V 接到 GPIO。

| 功能 | MCU 腳位 | 連接方式 |
| --- | --- | --- |
| 紅色 LED | PA0 / TIM2_CH1 | PA0 → 330Ω–1kΩ 電阻 → LED 長腳（陽極）；短腳接 GND |
| 黃色 LED | PA1 / TIM2_CH2 | PA1 → 獨立電阻 → LED 陽極；陰極接 GND |
| 綠色 LED | PB10 / TIM2_CH3 | PB10 → 獨立電阻 → LED 陽極；陰極接 GND |
| LPUART1 TX | PA2 / AF12 | ST-LINK VCP 接收端，或 USB-UART RX |
| LPUART1 RX | PA3 / AF12 | ST-LINK VCP 傳送端，或 USB-UART TX |
| 共地 | GND | 麵包板地線與板子 GND 相連 |

```text
PA0 ──[330Ω]──▶| 紅 LED ── GND
PA1 ──[330Ω]──▶| 黃 LED ── GND
PB10──[330Ω]──▶| 綠 LED ── GND
```

使用標示 MCU 腳位的 Morpho／開發板排針，勿將 PA0 誤認為排針第 0 腳。每個 LED 各自限流。預設 Nucleo 透過 ST-LINK USB 的 VCP 溝通，確認板上 VCP 焊橋仍接到 PA2/PA3；若無回應請依你的板卡版本原理圖查核。不要同時接上 ST-LINK UART 和另一組正在驅動相同腳位的 USB-UART。

## 將韌體整合到你貼上的 STM32CubeIDE 專案

附件只有 `main.c`，沒有完整 `.ioc`、HAL、啟動碼或連結腳本，因此這裡提供**可整合的 HAL 模組**，不是可直接燒錄的 `.hex`。保留你原本的 `SystemClock_Config()`（HSI + PLL，170 MHz）。

1. 在 CubeMX/CubeIDE 為 STM32G474RE 啟用 **HAL UART 與 TIM 模組**；若 HAL 設定檔未啟用，需啟用 `HAL_UART_MODULE_ENABLED` 與 `HAL_TIM_MODULE_ENABLED`。保留 SysTick 與 SWD。
2. 將 `firmware/typerush.h` 複製到 `Core/Inc/`、`typerush.c` 複製到 `Core/Src/`。模組自行初始化 LPUART1、TIM2 與上述 GPIO，**不要再對同一周邊呼叫 Cube 產生的 MX_LPUART1_UART_Init / MX_TIM2_Init**。PA0、PA1、PB10 需空給 TIM2。
3. 在 `main.c` 的 `USER CODE BEGIN Includes` 加入：

```c
#include "typerush.h"
```

4. 在 `MX_GPIO_Init();` 後面的 `USER CODE BEGIN 2` 加入：

```c
TypeRush_Init();
```

5. 在 `while (1)` 的 `USER CODE BEGIN 3` 加入（不要另加長時間 HAL_Delay）：

```c
TypeRush_Task();
```

6. 在 `main.c` 的 `USER CODE BEGIN 4` 加入。若已存在同名 callback，將呼叫合併到原有函式，勿重複定義：

```c
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    TypeRush_RxComplete(huart);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    TypeRush_UartError(huart);
}
```

7. 在 `stm32g4xx_it.c` 的 user include 加入 `#include "typerush.h"`，並確保以下中斷處理函式**僅有一份**。若 CubeMX 已產生同名函式，替換其中的 handle 呼叫，勿額外再定義：

```c
void LPUART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&typerush_uart);
}
```

8. 編譯、燒錄，將 ST-LINK USB 插到執行瀏覽器的電腦，關閉占用 COM 埠的終端機。網頁按「連接 STM32」，選 STMicroelectronics Virtual COM Port。狀態須顯示「STM32 已連線」，紀錄有 `ACK PING` 才算握手成功。
9. 遊戲尚未開始時逐一按紅／黃／綠／全關，確認只有對應 LED 發亮，再開始遊戲確認呼吸與暫停。PA0、PA1、PB10 必須與麵包板接線一致。

韌體 TIM2 硬體 PWM 為 1 kHz，10 ms 更新呼吸亮度。UART 使用接收中斷與 512-byte ring buffer，長指令與溢位會回報錯誤並丟棄到換行。網頁負責出題、時間、計分、開始、暫停／繼續、重置與速度調整。板子僅接收燈光指令並輸出 PWM，不需接實體按鈕，也不傳送遊戲控制事件。UART 回傳的 ACK 僅用於確認連線與指令。

## 通訊協定

115200 baud / 8 data bits / no parity / 1 stop bit，ASCII，每行 `\n` 結束（接受 `\r\n`）。

| 電腦 → 板子 | 板子 → 電腦 | 功能 |
| --- | --- | --- |
| `PING` | `ACK PING` | 握手／1.5 秒心跳 |
| `STATE green RUN` | `ACK STATE` | 綠色呼吸燈；色彩可為 red/yellow/green |
| `STATE yellow PAUSE` | `ACK STATE` | 暫停、黃色恆亮 |
| `LED red` | `ACK LED` | 單色恆亮測試 |
| `OFF` | `ACK OFF` | 全關 |
| — | `READY TYPERUSH/1` | 開機通知 |
| — | `ERR COMMAND/LINE/OVERFLOW` | 指令、長度／內容、緩衝區錯誤 |

韌體 5 秒未收到有效指令會自動關燈；斷線時瀏覽器繼續提供模擬遊戲。序列埠開啟但未收到握手時不會宣稱硬體已就緒。網站無法自動安裝驅動或燒錄板子。

## 驗證

```sh
npm test
node --check app.js
```

純 Node 測試涵蓋計分邊界、拼字與通訊解析。實體 LED、ST-LINK 焊橋與韌體編譯需用完整 STM32CubeIDE 專案及實際板子確認；本儲存庫不含 ST HAL SDK。

## 完整 main.c 版本

可用 `firmware/main_complete.c` 的完整內容取代 `Core/Src/main.c`，此時不要同時編譯 `typerush.c`。仍須依上文在 `stm32g4xx_it.c` 接上 LPUART1 IRQ（可用 `extern UART_HandleTypeDef typerush_uart;` 宣告 handle）。
