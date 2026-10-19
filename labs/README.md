# TypeRush LAB — Lab1–5 教材整合版

新版網址：https://jamie96123.github.io/Testwork/labs/

網頁負責拼字遊戲、開始、暫停／繼續、重置、速度與燈光測試。板子只接收 UART 指令，綠 PC8/TIM3_CH3 呼吸燈、黃 PC9/TIM3_CH4 恆亮、紅 PC6 GPIO 恆亮。

- [CubeMX 設定、接線及韌體整合](firmware/README.md)
- [完整 USER CODE 貼上版](firmware/main_USER_CODE.txt)
- [教材逐項對照與版本差異](sources.md)
- [下載完整新版](TypeRush-Lab.zip)

## 開發與測試

不需 npm install。Python 3 執行靜態網頁；Node.js 與 gcc 用於開發測試。

```sh
cd labs
npm start
npm test
```

本機 Chrome/Edge 開啟 localhost，Web Serial 才能選到插在同一電腦的板子。正式網站使用 HTTPS。

瀏覽器整合測試使用環境已提供的 Playwright 和 `/usr/bin/chromium`，預設本機服務是儲存庫根目錄 8010 埠的 `/labs/`，也可設定 `TYPERUSH_TEST_URL` 指向新版網站後執行 `node tests/browser.cjs`。測試使用模擬 Web Serial，不會代表真實板子已驗證。

沒有完整 CubeMX 專案，因此不提供可直接燒錄 HEX；依 firmware/README.md 設定並將應用程式放入 USER CODE，保留 CubeMX 自動產生的初始化。
