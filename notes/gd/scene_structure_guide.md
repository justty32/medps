# 完整的2D策略遊戲場景結構指南

## 📁 項目文件結構

```
res://
├── scenes/
│   ├── MainMenu.tscn          # 主選單場景
│   ├── Settings.tscn          # 設置場景
│   ├── LoadGame.tscn          # 載入遊戲場景
│   └── Main.tscn              # 主遊戲場景
├── scripts/
│   ├── MainMenu.gd            # 主選單腳本
│   ├── Settings.gd            # 設置腳本
│   ├── LoadGame.gd            # 載入遊戲腳本
│   ├── Main.gd                # 主遊戲腳本
│   ├── WorldMap.gd            # 世界地圖腳本
│   ├── UIManager.gd           # UI管理器腳本
│   └── GameManager.gd         # 遊戲管理器腳本
└── autoload/
    └── GameGlobals.gd         # 全域數據管理（AutoLoad）
```

## 🎬 場景設置詳細步驟

### 1. 創建主選單場景 (MainMenu.tscn)

**節點結構:**
```
MainMenu (Control) - 附加 MainMenu.gd
└── (所有UI元素都在腳本中動態創建)
```

**設置步驟:**
1. 新建場景，根節點選擇 `Control`，命名為 `MainMenu`
2. 將 `MainMenu.gd` 腳本附加到根節點
3. 保存為 `MainMenu.tscn`

### 2. 創建設置場景 (Settings.tscn)

**節點結構:**
```
Settings (Control) - 附加 Settings.gd
└── (所有UI元素都在腳本中動態創建)
```

**設置步驟:**
1. 新建場景，根節點選擇 `Control`，命名為 `Settings`
2. 將 `Settings.gd` 腳本附加到根節點
3. 保存為 `Settings.tscn`

### 3. 創建載入遊戲場景 (LoadGame.tscn)

**節點結構:**
```
LoadGame (Control) - 附加 LoadGame.gd
└── (所有UI元素都在腳本中動態創建)
```

**設置步驟:**
1. 新建場景，根節點選擇 `Control`，命名為 `LoadGame`
2. 將 `LoadGame.gd` 腳本附加到根節點
3. 保存為 `LoadGame.tscn`

### 4. 主遊戲場景 (Main.tscn)

**節點結構:**
```
Main (Node2D) - 附加 Main.gd
├── WorldMap (Node2D) - 附加 WorldMap.gd
├── UIManager (CanvasLayer) - 附加 UIManager.gd
└── GameManager (Node) - 附加 GameManager.gd
```

**設置步驟:**
1. 新建場景，根節點選擇 `Node2D`，命名為 `Main`
2. 將 `Main.gd` 腳本附加到根節點
3. 添加子節點並附加相應腳本
4. 保存為 `Main.tscn`

### 5. 設置AutoLoad (全域單例)

在項目設置中添加AutoLoad：
1. 項目 > 項目設置 > AutoLoad
2. 路徑: `res://autoload/GameGlobals.gd`
3. 節點名稱: `GameGlobals`
4. 勾選"啟用"

## 🎮 輸入映射設置

在項目設置 > 輸入映射中添加：

| 動作名稱 | 按鍵綁定 | 用途 |
|---------|---------|------|
| `ui_accept` | Enter/Space | 確認/開始遊戲 |
| `ui_cancel` | ESC | 取消/返回 |
| `end_turn` | Space | 結束回合 |

## 🎨 主選單功能特點

### 🌟 視覺效果
- **動態背景**: 程序生成的裝飾圖案
- **進入動畫**: 標題和按鈕的淡入效果
- **懸停效果**: 按鈕縮放動畫
- **現代UI設計**: 圓角按鈕與漸變效果

### 🎯 按鈕功能
1. **開始遊戲**: 切換到主遊戲場景
2. **載入遊戲**: 打開存檔管理界面
3. **設置**: 打開遊戲設置界面
4. **離開**: 顯示確認對話框後退出

## ⚙️ 設置系統功能

### 🔊 音訊設置
- 主音量控制（即時預覽）
- 音樂音量控制
- 音效音量控制
- 滑桿數值即時顯示

### 🖥️ 顯示設置
- 螢幕模式（視窗/全螢幕）
- 解析度選擇（1280x720, 1920x1080, 2560x1440）
- 垂直同步開關

### 🎮 遊戲設置
- 自動保存開關
- 遊戲速度（慢/正常/快）
- 顯示網格開關
- 顯示提示開關

### 💾 設置保存
- 自動保存到 `user://settings.cfg`
- 啟動時自動載入設置
- 重置到預設值功能

## 📂 存檔系統功能

### 📋 存檔列表
- 自動掃描 `user://saves/` 目錄
- 按時間排序顯示
- 顯示存檔信息（名稱、回合、玩家、時間）
- 可滾動列表支持

### 🎮 存檔操作
- **載入存檔**: 選中後載入到遊戲
- **刪除存檔**: 確認後永久刪除
- **存檔預覽**: 顯示基本遊戲信息

### 🔒 安全機制
- 刪除前顯示確認對話框
- 文件格式驗證
- 錯誤處理與提示

## 🎯 場景切換流程

```
MainMenu (啟動場景)
├── 開始遊戲 → Main (遊戲場景)
├── 載入遊戲 → LoadGame → Main (載入存檔)
├── 設置 → Settings → MainMenu (返回)
└── 離開 → 退出程序
```

## 🔧 重要提醒

### 文件路徑檢查
確保所有場景文件路徑正確：
- `res://scenes/MainMenu.tscn`
- `res://scenes/Settings.tscn` 
- `res://scenes/LoadGame.tscn`
- `res://scenes/Main.tscn`

### AutoLoad設置
GameGlobals必須設置為AutoLoad，否則存檔系統無法正常工作。

### 輸入映射
確保輸入映射已正確設置，支持鍵盤快捷鍵操作。

## 🚀 運行順序

1. **首次運行**: 將 `MainMenu.tscn` 設為主場景
2. **測試流程**: 
   - 進入主選單
   - 測試各個按鈕
   - 檢查設置保存
   - 測試存檔系統
   - 最後測試遊戲主體

## 📈 後續擴展建議

### UI增強
- 添加背景音樂
- 實現主題切換
- 添加動態背景效果
- 支持多語言

### 功能擴展
- 玩家統計
- 成就系統
- 教學模式
- 線上排行榜

這個完整的選單系統為你的策略遊戲提供了專業的用戶體驗，包含了現代遊戲必備的所有基礎功能。