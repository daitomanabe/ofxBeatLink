# ofxBeatLink Examples 開発計画

## 概要

以下の4つのexampleを順番に開発する。

| 順序 | Example | 目的 |
|------|---------|------|
| 1 | example-beatsync | ビートに同期したビジュアル表示 |
| 2 | example-multidevice | 複数デバイスの個別管理・表示 |
| 3 | example-osc | OSCプロトコルでのデータ送信 |
| 4 | example-statusmonitor | CDJの詳細ステータス監視 |

---

## 1. example-beatsync

### 目的
ビート情報を使ってビジュアルを同期させるデモ。nextBeatMs/nextBarMsを活用した精密なタイミング制御を示す。

### 機能
- マスターデバイスのBPMに同期した円形アニメーション
- ビート位置(1-4)に応じた4分割表示
- nextBeatMsを使った先読み同期
- ダウンビート(beat 1)で特別なエフェクト

### 使用するAPI
```cpp
ofxBeatLinkBeat getLatestBeat()
- bpm: 現在のBPM
- beatWithinBar: 1-4のビート位置
- nextBeatMs: 次のビートまでのミリ秒
- nextBarMs: 次の小節までのミリ秒
```

### ファイル構成
```
example-beatsync/
├── src/
│   ├── main.cpp
│   └── ofApp.cpp
│   └── ofApp.h
└── Makefile
```

### 画面設計
```
┌─────────────────────────────────────────┐
│  BPM: 128.00                            │
│                                         │
│          ╭──────────╮                   │
│         ╱    ●       ╲   ← ビート円     │
│        │      ○      │      (拡大/縮小)  │
│         ╲    ○   ○  ╱                   │
│          ╰──────────╯                   │
│                                         │
│  Beat: [■][□][□][□]  ← 4ビートインジケータ│
│  Next Beat: 234ms                       │
│  Next Bar: 1872ms                       │
└─────────────────────────────────────────┘
```

### 実装ステップ
1. example-basicをベースにプロジェクト作成
2. ビート同期用のタイミング計算クラス実装
3. 円形アニメーションの描画
4. 4ビートインジケータの実装
5. ダウンビートエフェクトの追加

---

## 2. example-multidevice

### 目的
複数のCDJ/XDJデバイスを個別に監視・表示するデモ。デバイスの追加/削除のハンドリングを示す。

### 機能
- 最大4台のデバイスを個別パネルで表示
- デバイス追加/削除のリアルタイム検知
- デバイスごとのビート情報表示
- マスター/スレーブ関係の可視化

### 使用するAPI
```cpp
std::vector<ofxBeatLinkDevice> getCurrentDevices()
ofxBeatLinkBeat getLatestBeat(int deviceNumber)
deviceFoundEvent / deviceLostEvent
```

### 画面設計
```
┌─────────────────────────────────────────┐
│ ┌─────────────┐  ┌─────────────┐        │
│ │ CDJ-3000 #1 │  │ CDJ-3000 #2 │        │
│ │ BPM: 128.00 │  │ BPM: 128.00 │        │
│ │ Beat: [■]   │  │ Beat: [□]   │        │
│ │ ★ MASTER    │  │   SYNC      │        │
│ │ 192.168.1.1 │  │ 192.168.1.2 │        │
│ └─────────────┘  └─────────────┘        │
│                                         │
│ ┌─────────────┐  ┌─────────────┐        │
│ │ CDJ-3000 #3 │  │  (空)       │        │
│ │ BPM: 126.50 │  │             │        │
│ │ Beat: [□]   │  │  接続待ち   │        │
│ │   SYNC      │  │             │        │
│ │ 192.168.1.3 │  │             │        │
│ └─────────────┘  └─────────────┘        │
│                                         │
│ Devices: 3 connected                    │
└─────────────────────────────────────────┘
```

### 実装ステップ
1. デバイス管理用のmap構造を実装
2. デバイスパネル描画クラスの作成
3. deviceFoundEvent/deviceLostEventのハンドリング
4. デバイスごとのgetLatestBeat呼び出し
5. マスター/シンク状態の表示

---

## 3. example-osc

### 目的
ビート情報をOSCプロトコルで外部アプリケーション(Max/MSP, TouchDesigner, Resolume等)に送信するデモ。

### 機能
- ビートイベントをOSCメッセージとして送信
- デバイス発見/喪失イベントの送信
- 設定可能な送信先IP/ポート
- GUI上でのOSCメッセージモニタリング

### 依存関係
- ofxOsc (openFrameworks標準addon)

### OSCメッセージ仕様
```
/beatlink/beat
  - int: deviceNumber
  - float: bpm
  - int: beatWithinBar (1-4)
  - int: nextBeatMs
  - int: nextBarMs

/beatlink/device/found
  - int: deviceNumber
  - string: deviceName
  - string: ipAddress

/beatlink/device/lost
  - int: deviceNumber
  - string: deviceName

/beatlink/master
  - int: deviceNumber
  - float: bpm
```

### 画面設計
```
┌─────────────────────────────────────────┐
│ OSC Output: 127.0.0.1:9000              │
│ Status: ● Sending                       │
│                                         │
│ ┌─OSC Monitor───────────────────────┐   │
│ │ /beatlink/beat 1 128.0 2 234 1872 │   │
│ │ /beatlink/beat 2 128.0 3 234 1406 │   │
│ │ /beatlink/beat 1 128.0 3 234 1406 │   │
│ │ /beatlink/device/found 3 CDJ-3000 │   │
│ │ ...                               │   │
│ └───────────────────────────────────┘   │
│                                         │
│ Messages sent: 1,234                    │
│ Rate: 4.2 msg/sec                       │
└─────────────────────────────────────────┘
```

### 実装ステップ
1. ofxOscを追加してプロジェクト作成
2. ofxOscSenderの設定
3. onBeatでOSCメッセージ送信
4. デバイスイベントのOSC送信
5. メッセージモニター画面の実装

---

## 4. example-statusmonitor

### 目的
CDJの詳細なステータス(再生状態、トラック情報、On-Air状態など)を監視・表示するデモ。

### 機能
- 再生状態(PLAYING, PAUSED, CUED等)の表示
- On-Air状態の表示
- マスター/シンク状態の表示
- イベントログのタイムライン表示

### 使用するAPI
```cpp
// beat-link-cppから取得可能な情報
CdjStatus:
  - isPlaying()
  - isTempoMaster()
  - isSynced()
  - isOnAir()
  - getPlayState() - PLAYING, PAUSED, CUED, CUEING, etc.
  - getRekordboxId()
  - getTrackNumber()
```

### 注意
現在のofxBeatLinkはBeat情報のみをラップしている。CdjStatusの詳細情報を取得するには、ofxBeatLinkの拡張が必要になる可能性がある。

### 画面設計
```
┌─────────────────────────────────────────┐
│ ┌─CDJ #1──────────────────────────────┐ │
│ │ CDJ-3000           192.168.1.1      │ │
│ │                                     │ │
│ │ ▶ PLAYING         BPM: 128.00      │ │
│ │ ★ MASTER          Pitch: +0.12%    │ │
│ │ 🔊 ON-AIR                           │ │
│ │                                     │ │
│ │ Beat: [■][□][□][□]                  │ │
│ └─────────────────────────────────────┘ │
│                                         │
│ ┌─Event Log─────────────────────────┐   │
│ │ 12:34:56 CDJ#1 PLAYING → PAUSED   │   │
│ │ 12:34:52 CDJ#2 Became MASTER      │   │
│ │ 12:34:48 CDJ#3 Connected          │   │
│ │ ...                               │   │
│ └───────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

### 実装ステップ
1. 現在のofxBeatLinkでCdjStatus情報が取得可能か確認
2. 必要に応じてofxBeatLinkを拡張(CdjStatusListener追加)
3. ステータスパネルUIの実装
4. 状態変化検知とログ記録
5. イベントログUIの実装

---

## 共通事項

### プロジェクト構成
各exampleは以下の構成に従う:
```
example-xxx/
├── src/
│   ├── main.cpp
│   ├── ofApp.h
│   └── ofApp.cpp
├── Makefile
└── addons.make (ofxOscが必要な場合)
```

### ビルド確認
各example完成後に実行:
```bash
cd example-xxx
make -j4
```

### 参考
- example-basic: シンプルな監視例
- example-cdjstatus: 詳細なステータス表示例

---

## スケジュール

| Phase | Example | 依存関係 |
|-------|---------|---------|
| 1 | example-beatsync | なし |
| 2 | example-multidevice | なし |
| 3 | example-osc | ofxOsc |
| 4 | example-statusmonitor | ofxBeatLink拡張が必要な可能性 |
