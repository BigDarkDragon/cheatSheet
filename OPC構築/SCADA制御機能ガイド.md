# 変電所 SCADA 制御機能 構築ガイド

**SCADA（DataHub）から IED の遮断器・断路器を操作する正常運用の模擬環境を構築する**

---

## 目次

- [1. 全体構成と制御フロー](#1-全体構成と制御フロー)
- [2. KEPServerEX: IEC 61850 MMS ドライバの制御設定](#2-kepserverex-iec-61850-mms-ドライバの制御設定)
  - [2.1 チャネル・デバイスの確認](#21-チャネルデバイスの確認)
  - [2.2 制御タグの追加](#22-制御タグの追加)
  - [2.3 タグの Read/Write 属性設定](#23-タグの-readwrite-属性設定)
  - [2.4 動作確認（KEPServerEX Quick Client）](#24-動作確認kepserverex-quick-client)
- [3. Cogent DataHub: 書き込み許可と制御設定](#3-cogent-datahub-書き込み許可と制御設定)
  - [3.1 OPC DA 接続の Write 許可](#31-opc-da-接続の-write-許可)
  - [3.2 Data Browser からの手動操作](#32-data-browser-からの手動操作)
  - [3.3 Web HMI への制御ボタン追加](#33-web-hmi-への制御ボタン追加)
- [4. 運用シナリオ別の操作手順](#4-運用シナリオ別の操作手順)
  - [4.1 遮断器の開放操作（CB Trip）](#41-遮断器の開放操作cb-trip)
  - [4.2 遮断器の投入操作（CB Close）](#42-遮断器の投入操作cb-close)
  - [4.3 断路器の操作](#43-断路器の操作)
  - [4.4 計測値の監視](#44-計測値の監視)
- [5. 制御のプロトコルシーケンス](#5-制御のプロトコルシーケンス)
- [6. 運用確認チェックリスト](#6-運用確認チェックリスト)
- [7. トラブルシューティング](#7-トラブルシューティング)

---

## 1. 全体構成と制御フロー

### Read 方向（監視）— 構築済み

```
IED (VM1)              RTU/GW (VM2)              SCADA (VM3)
substation_server      KEPServerEX               DataHub
  │                      │                         │
  │ ←── MMS Read ──────  │ ←── OPC DA Read ──────  │
  │    (周期ポーリング)    │    (DCOM Subscription)  │
  │                      │                         │
  │  Pos.stVal = 2       │  XCBR1.Pos.stVal = 2   │  値表示: CLOSED
  │  A.phsA = 250.3      │  MMXU1.A.phsA = 250.3  │  電流: 250.3 A
```

### Write 方向（制御）— 今回構築する

```
IED (VM1)              RTU/GW (VM2)              SCADA (VM3)
substation_server      KEPServerEX               DataHub
  │                      │                         │
  │  MMS Operate ──────→ │  OPC DA Write ────────→ │
  │  (制御コマンド)       │  (DCOM IOPCSyncIO)     │
  │                      │                         │
  │  ctlVal=false(Open)  │  Write(tag, 1)          │  操作員: [Open] ボタン押下
  │                      │                         │
  │  ControlHandler発火  │                         │
  │  Pos.stVal: 2→1      │  値変化を検出           │  表示更新: CLOSED → OPEN
  │  電流: 250A→0A       │                         │  電流: 0 A
```

### IEC 61850 制御モデル

substation_server は `direct-with-normal-security`（ctlModel=1）を使用している。
これは IEC 61850 で定義される最もシンプルな制御モデルで、Select（選択）なしに直接 Operate（実行）できる。

```
IEC 61850 制御モデル一覧:
  0: status-only        — 制御不可（監視のみ）
  1: direct-with-normal-security  ← 本環境で使用
  2: sbo-with-normal-security     — Select Before Operate
  3: direct-with-enhanced-security
  4: sbo-with-enhanced-security

※ 実際の変電所では 2 または 4（SBO）を使用することが多い。
  SBO は誤操作防止のため「選択→確認→実行」の2段階を要求する。
  本環境では Industroyer が使った direct 方式を再現するため ctlModel=1 を使用。
```

---

## 2. KEPServerEX: IEC 61850 MMS ドライバの制御設定

### 2.1 チャネル・デバイスの確認

KEPServerEX Configuration で既存の IEC 61850 MMS 接続を確認する。

```
KEPServerEX Configuration:
  └── Connectivity
      └── IEC61850_Ch1  (Channel)
          └── IED1      (Device)
              └── CBIED  (Auto-generated tags)
                  ├── XCBR1.Pos.stVal     ← Read で動作確認済み
                  ├── XCBR2.Pos.stVal
                  ├── XSWI1.Pos.stVal
                  ├── MMXU1.A.phsA.cVal.mag.f
                  └── ...
```

### 2.2 制御タグの追加

監視用の `stVal`（Status Value）タグに加え、制御用の `Oper` タグを追加する。

```
KEPServerEX Configuration:
  IEC61850_Ch1 → IED1 → 右クリック → [New Tag]

┌──────────────────────────────────────────────────┐
│  Tag Properties                                   │
│                                                    │
│  === 遮断器 XCBR1 制御タグ ===                     │
│                                                    │
│  Tag Name:     XCBR1_Control                       │
│  Address:      CBIED/XCBR1.Pos.Oper.ctlVal         │
│  Description:  XCBR1 遮断器制御                     │
│  Data Type:    Boolean                              │
│  Client Access: Read/Write                          │
│                                                    │
│  === 遮断器 XCBR2 制御タグ ===                     │
│                                                    │
│  Tag Name:     XCBR2_Control                       │
│  Address:      CBIED/XCBR2.Pos.Oper.ctlVal         │
│  Data Type:    Boolean                              │
│  Client Access: Read/Write                          │
│                                                    │
│  === 断路器 XSWI1 制御タグ ===                     │
│                                                    │
│  Tag Name:     XSWI1_Control                       │
│  Address:      CBIED/XSWI1.Pos.Oper.ctlVal         │
│  Data Type:    Boolean                              │
│  Client Access: Read/Write                          │
│                                                    │
└──────────────────────────────────────────────────┘
```

> **アドレスの意味:**
> - `CBIED` — 論理デバイス名
> - `XCBR1.Pos` — DPC（Double Point Control）データオブジェクト
> - `Oper` — Operate（操作実行）の構造体
> - `ctlVal` — 制御値（Boolean: true=Close, false=Open）

### KEPServerEX のアドレス自動検出を使う場合

手動でアドレスを入力する代わりに、Tag Browser でブラウズして追加することもできる。

```
KEPServerEX Configuration:
  IED1 → 右クリック → [Tag Browser...]

  ツリーを展開:
    CBIED
    └── XCBR1
        └── Pos
            ├── stVal     ← 既存（状態値、Read）
            ├── Oper      ← ★ これを追加
            │   ├── ctlVal    ← 制御値（Boolean）
            │   ├── origin    ← 制御元情報
            │   └── ctlNum    ← 制御番号
            └── ctlModel  ← 制御モデル（Read Only）

  XCBR1 → Pos → Oper → ctlVal を選択
  → [Add Items] で制御タグを追加
```

### 2.3 タグの Read/Write 属性設定

既存の監視タグ（stVal）も書き込み可能にする場合：

```
KEPServerEX Configuration:
  IED1 → CBIED → XCBR1.Pos.stVal を選択
  → Properties:
    Client Access:  Read/Write  ← Read Only から変更
```

ただし、**推奨は監視タグ（stVal）は Read Only のままで、制御専用タグ（Oper.ctlVal）を別途追加する方法**。理由：

| 方法 | 動作 | 推奨度 |
|---|---|---|
| `stVal` を Read/Write | stVal に直接値を書き込む。KEPServerEX が MMS Write を発行 | △ IEC 61850 の正規の制御手順ではない |
| `Oper.ctlVal` に Write | KEPServerEX が MMS Operate を発行（IEC 61850 準拠） | ◎ 正規の制御手順 |

> **注意:** KEPServerEX のバージョン・ドライバ設定によっては、`Oper.ctlVal` への Write が自動的に `Operate` サービスに変換されない場合がある。その場合は `stVal` への直接 Write でも制御は機能する（substation_server 側は両方を受け付ける）。

### 2.4 動作確認（KEPServerEX Quick Client）

制御タグを追加したら、KEPServerEX の Quick Client で動作確認する。

```
KEPServerEX:
  [Tools] → [Quick Client] 起動

Quick Client:
  左ペインで IEC61850_Ch1.IED1 を展開
  → 制御タグ XCBR1_Control を選択

  右ペインで:
    XCBR1_Control を右クリック → [Sync Write]
    → Value: 0 (= false = Open)
    → [OK]

  → IED のコンソールに以下が表示されること:
    [CONTROL] XCBR1 command from 192.168.56.20
    [CONTROL]   ctlVal = false (OPEN)

  → XCBR1.Pos.stVal が 2 → 1 に変化すること
  → MMXU1.A.phsA が ~250A → 0A に変化すること

  復旧:
    XCBR1_Control を右クリック → [Sync Write]
    → Value: 1 (= true = Close)
    → [OK]

  → stVal が 1 → 2 に復旧
  → 電流値が復旧
```

**この段階で KEPServerEX → IED の制御パスが動作することを確認する。**
DataHub の設定に進む前にこのステップを必ず完了させること。

---

## 3. Cogent DataHub: 書き込み許可と制御設定

### 3.1 OPC DA 接続の Write 許可

DataHub はデフォルトで OPC DA の Write を許可していない場合がある。明示的に有効化する。

```
DataHub Manager:
  左ペイン → [Data Sources] → [OPC DA]
  → VM2_RTUGW_Kepware 接続を選択

  Properties:
    ☑ Allow Write     ← ★ これを有効にする
    ☑ Active
    Update Rate: 1000 ms

  → [Apply]
```

### 3.2 Data Browser からの手動操作

Write 許可を有効にすると、Data Browser からタグに値を書き込める。

```
DataHub Manager:
  [View] → [Data Browser]

  タグツリーを展開:
    VM2_RTUGW_Kepware
    └── IEC61850_Ch1
        └── IED1
            └── CBIED
                ├── XCBR1
                │   ├── Pos
                │   │   └── stVal    = 2 (Closed)
                │   └── Control      = (制御タグ)
                │       └── ctlVal
                ...

  === 遮断器を開放する操作 ===

  方法A: 制御タグ (ctlVal) を使用（推奨）
    XCBR1_Control を右クリック → [Write Value...]
    → Value: 0  (false = Open)
    → [Write]

  方法B: 状態タグ (stVal) に直接書き込み
    XCBR1.Pos.stVal を右クリック → [Write Value...]
    → Value: 1  (1 = Open)
    → [Write]

  === 遮断器を投入する操作 ===

  XCBR1_Control を右クリック → [Write Value...]
  → Value: 1  (true = Close)
  → [Write]
```

### 3.3 Web HMI への制御ボタン追加

Layer2_DataHub構築ガイド.md §6.3 で作成した Web HMI に、遮断器の制御ボタンを追加する。

以下のファイルを DataHub の Web サーバディレクトリに配置する:

```
C:\Program Files\Cogent\DataHub\www\substation_control.html
```

```html
<!DOCTYPE html>
<html>
<head>
  <title>Substation Control Panel</title>
  <meta charset="UTF-8">
  <script src="/datahub.js"></script>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Consolas, monospace;
      background: #0a0a1a; color: #c0c0d0; padding: 20px;
    }
    h1 { color: #00ccff; margin-bottom: 5px; font-size: 22px; }
    h1 + p { color: #556677; font-size: 12px; margin-bottom: 20px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; max-width: 960px; }
    .panel {
      background: #12122a; border: 1px solid #1a1a40;
      border-radius: 8px; padding: 18px;
    }
    .panel h2 {
      color: #6688aa; font-size: 13px; text-transform: uppercase;
      letter-spacing: 1px; margin-bottom: 12px;
    }
    .breaker-row {
      display: flex; align-items: center; justify-content: space-between;
      padding: 10px 0; border-bottom: 1px solid #1a1a30;
    }
    .breaker-name { color: #8899aa; min-width: 130px; }
    .breaker-state { font-size: 18px; font-weight: bold; min-width: 120px; text-align: center; }
    .closed { color: #00ff88; }
    .open { color: #ff3344; animation: blink 1s infinite; }
    @keyframes blink { 50% { opacity: 0.6; } }
    .btn-group { display: flex; gap: 6px; }
    .btn {
      padding: 6px 16px; border: none; border-radius: 4px;
      font-size: 13px; font-weight: bold; cursor: pointer;
      transition: all 0.2s;
    }
    .btn:hover { transform: scale(1.05); }
    .btn:active { transform: scale(0.95); }
    .btn-open {
      background: #cc2200; color: #fff;
    }
    .btn-close {
      background: #007744; color: #fff;
    }
    .btn-disabled {
      background: #333; color: #666; cursor: not-allowed;
    }
    .meas-row {
      display: flex; justify-content: space-between;
      padding: 6px 0; border-bottom: 1px solid #1a1a30;
    }
    .meas-label { color: #667788; }
    .meas-val { font-weight: bold; font-size: 15px; }
    .counter { color: #8899aa; font-size: 13px; min-width: 80px; text-align: center; }
    .confirm-overlay {
      display: none; position: fixed; top: 0; left: 0; right: 0; bottom: 0;
      background: rgba(0,0,0,0.7); z-index: 100;
      justify-content: center; align-items: center;
    }
    .confirm-overlay.active { display: flex; }
    .confirm-box {
      background: #1a1a30; border: 2px solid #ff6600; border-radius: 10px;
      padding: 25px; text-align: center; max-width: 400px;
    }
    .confirm-box h3 { color: #ff6600; margin-bottom: 15px; }
    .confirm-box p { margin-bottom: 20px; color: #aab; }
    .confirm-box .btn { margin: 0 8px; padding: 10px 30px; font-size: 15px; }
    .log-panel { grid-column: 1 / -1; max-height: 200px; overflow-y: auto; }
    .log-entry { font-size: 12px; color: #556677; padding: 2px 0; font-family: monospace; }
    .log-entry.control { color: #ffaa00; }
    .footer { color: #334; font-size: 11px; text-align: right; margin-top: 15px; }
  </style>
</head>
<body>
  <h1>⚡ Substation Control Panel</h1>
  <p>IED: CBIED — Breaker & Disconnector Control</p>

  <div class="grid">
    <!-- 遮断器/断路器制御パネル -->
    <div class="panel">
      <h2>Switching Devices</h2>

      <!-- XCBR1 -->
      <div class="breaker-row">
        <span class="breaker-name">XCBR1 (Bay 1)</span>
        <span class="breaker-state" id="xcbr1_state">--</span>
        <span class="counter" id="xcbr1_cnt">OpCnt: --</span>
        <div class="btn-group">
          <button class="btn btn-open" onclick="confirmAction('XCBR1','OPEN')">Open</button>
          <button class="btn btn-close" onclick="confirmAction('XCBR1','CLOSE')">Close</button>
        </div>
      </div>

      <!-- XCBR2 -->
      <div class="breaker-row">
        <span class="breaker-name">XCBR2 (Bay 2)</span>
        <span class="breaker-state" id="xcbr2_state">--</span>
        <span class="counter" id="xcbr2_cnt">OpCnt: --</span>
        <div class="btn-group">
          <button class="btn btn-open" onclick="confirmAction('XCBR2','OPEN')">Open</button>
          <button class="btn btn-close" onclick="confirmAction('XCBR2','CLOSE')">Close</button>
        </div>
      </div>

      <!-- XSWI1 -->
      <div class="breaker-row">
        <span class="breaker-name">XSWI1 (Disc.)</span>
        <span class="breaker-state" id="xswi1_state">--</span>
        <span class="counter"></span>
        <div class="btn-group">
          <button class="btn btn-open" onclick="confirmAction('XSWI1','OPEN')">Open</button>
          <button class="btn btn-close" onclick="confirmAction('XSWI1','CLOSE')">Close</button>
        </div>
      </div>
    </div>

    <!-- 計測値パネル -->
    <div class="panel">
      <h2>Measurements</h2>
      <div class="meas-row">
        <span class="meas-label">Phase A Current</span>
        <span class="meas-val" id="ia">-- A</span>
      </div>
      <div class="meas-row">
        <span class="meas-label">Phase B Current</span>
        <span class="meas-val" id="ib">-- A</span>
      </div>
      <div class="meas-row">
        <span class="meas-label">Phase C Current</span>
        <span class="meas-val" id="ic">-- A</span>
      </div>
      <div class="meas-row">
        <span class="meas-label">Phase A Voltage</span>
        <span class="meas-val" id="va">-- kV</span>
      </div>
      <div class="meas-row">
        <span class="meas-label">Phase B Voltage</span>
        <span class="meas-val" id="vb">-- kV</span>
      </div>
      <div class="meas-row">
        <span class="meas-label">Phase C Voltage</span>
        <span class="meas-val" id="vc">-- kV</span>
      </div>
      <div class="meas-row">
        <span class="meas-label">Frequency</span>
        <span class="meas-val" id="hz">-- Hz</span>
      </div>
    </div>

    <!-- 操作ログ -->
    <div class="panel log-panel">
      <h2>Operation Log</h2>
      <div id="log"></div>
    </div>
  </div>

  <p class="footer" id="ts">Last update: --</p>

  <!-- 操作確認ダイアログ -->
  <div class="confirm-overlay" id="confirmOverlay">
    <div class="confirm-box">
      <h3>⚠ 操作確認</h3>
      <p id="confirmMsg">--</p>
      <button class="btn btn-open" id="confirmYes" onclick="executeAction()">実行</button>
      <button class="btn" style="background:#444;color:#ccc;" onclick="cancelAction()">キャンセル</button>
    </div>
  </div>

  <script>
    // ============================================================
    //  ポイント名の定義
    //  ★ 環境に合わせて変更すること
    //  Data Browser で確認した実際のポイント名を使用する
    // ============================================================

    // KEPServerEX 経由の場合:
    var PREFIX = "VM2_RTUGW_Kepware:";
    var POINTS = {
      // 監視（Read）
      xcbr1_pos:   PREFIX + "IEC61850_Ch1.IED1.CBIED.XCBR1.Pos.stVal",
      xcbr2_pos:   PREFIX + "IEC61850_Ch1.IED1.CBIED.XCBR2.Pos.stVal",
      xswi1_pos:   PREFIX + "IEC61850_Ch1.IED1.CBIED.XSWI1.Pos.stVal",
      xcbr1_cnt:   PREFIX + "IEC61850_Ch1.IED1.CBIED.XCBR1.OpCnt.stVal",
      xcbr2_cnt:   PREFIX + "IEC61850_Ch1.IED1.CBIED.XCBR2.OpCnt.stVal",
      ia:          PREFIX + "IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsA.cVal.mag.f",
      ib:          PREFIX + "IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsB.cVal.mag.f",
      ic:          PREFIX + "IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsC.cVal.mag.f",
      va:          PREFIX + "IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsA.cVal.mag.f",
      vb:          PREFIX + "IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsB.cVal.mag.f",
      vc:          PREFIX + "IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsC.cVal.mag.f",
      hz:          PREFIX + "IEC61850_Ch1.IED1.CBIED.MMXU1.Hz.mag.f",

      // 制御（Write） — KEPServerEX に追加した制御タグ名
      xcbr1_ctrl:  PREFIX + "IEC61850_Ch1.IED1.XCBR1_Control",
      xcbr2_ctrl:  PREFIX + "IEC61850_Ch1.IED1.XCBR2_Control",
      xswi1_ctrl:  PREFIX + "IEC61850_Ch1.IED1.XSWI1_Control"
    };

    // ============================================================
    //  状態管理
    // ============================================================

    var pendingAction = null;  // { device: 'XCBR1', action: 'OPEN' }

    // ============================================================
    //  表示更新
    // ============================================================

    function formatBreaker(val) {
      if (val == 2) return '<span class="closed">CLOSED ●</span>';
      if (val == 1) return '<span class="open">OPEN ○</span>';
      return '-- ◎';
    }

    function fmt(v) { return v != null ? parseFloat(v).toFixed(1) : '--'; }
    function fmtHz(v) { return v != null ? parseFloat(v).toFixed(3) : '--'; }

    // DataHub WebView API で値変化を受信
    var datahub = new DataHub();

    datahub.onPointChange = function(name, value, quality, timestamp) {
      if (name === POINTS.xcbr1_pos)
        document.getElementById('xcbr1_state').innerHTML = formatBreaker(value);
      if (name === POINTS.xcbr2_pos)
        document.getElementById('xcbr2_state').innerHTML = formatBreaker(value);
      if (name === POINTS.xswi1_pos)
        document.getElementById('xswi1_state').innerHTML = formatBreaker(value);
      if (name === POINTS.xcbr1_cnt)
        document.getElementById('xcbr1_cnt').textContent = 'OpCnt: ' + value;
      if (name === POINTS.xcbr2_cnt)
        document.getElementById('xcbr2_cnt').textContent = 'OpCnt: ' + value;
      if (name === POINTS.ia) document.getElementById('ia').textContent = fmt(value) + ' A';
      if (name === POINTS.ib) document.getElementById('ib').textContent = fmt(value) + ' A';
      if (name === POINTS.ic) document.getElementById('ic').textContent = fmt(value) + ' A';
      if (name === POINTS.va) document.getElementById('va').textContent = fmt(value) + ' kV';
      if (name === POINTS.vb) document.getElementById('vb').textContent = fmt(value) + ' kV';
      if (name === POINTS.vc) document.getElementById('vc').textContent = fmt(value) + ' kV';
      if (name === POINTS.hz) document.getElementById('hz').textContent = fmtHz(value) + ' Hz';

      document.getElementById('ts').textContent =
        'Last update: ' + new Date().toLocaleTimeString();
    };

    // 全ポイントを購読
    for (var key in POINTS) {
      if (!key.endsWith('_ctrl')) {  // 制御タグは購読不要
        datahub.subscribe(POINTS[key]);
      }
    }
    datahub.connect();

    // ============================================================
    //  操作確認ダイアログ
    // ============================================================

    function confirmAction(device, action) {
      pendingAction = { device: device, action: action };
      var msg = device + ' を ' + (action === 'OPEN' ? '【開放 (Open)】' : '【投入 (Close)】') +
                ' します。よろしいですか？';
      document.getElementById('confirmMsg').textContent = msg;

      var btn = document.getElementById('confirmYes');
      if (action === 'OPEN') {
        btn.className = 'btn btn-open';
        btn.textContent = '開放実行';
      } else {
        btn.className = 'btn btn-close';
        btn.textContent = '投入実行';
      }

      document.getElementById('confirmOverlay').classList.add('active');
    }

    function cancelAction() {
      pendingAction = null;
      document.getElementById('confirmOverlay').classList.remove('active');
    }

    function executeAction() {
      if (!pendingAction) return;

      var device = pendingAction.device;
      var action = pendingAction.action;
      var ctlKey = device.toLowerCase() + '_ctrl';  // e.g., 'xcbr1_ctrl'
      var ctlPoint = POINTS[ctlKey];

      // 制御値: OPEN=false(0), CLOSE=true(1)
      var ctlVal = (action === 'CLOSE') ? 1 : 0;

      if (ctlPoint) {
        // DataHub WebView API で値を書き込み
        datahub.write(ctlPoint, ctlVal);
        addLog(device + ': ' + action + ' command sent (ctlVal=' + ctlVal + ')');
      } else {
        addLog('ERROR: Control point not found for ' + device);
      }

      document.getElementById('confirmOverlay').classList.remove('active');
      pendingAction = null;
    }

    // ============================================================
    //  操作ログ
    // ============================================================

    function addLog(msg) {
      var log = document.getElementById('log');
      var ts = new Date().toLocaleTimeString();
      var entry = document.createElement('div');
      entry.className = 'log-entry control';
      entry.textContent = '[' + ts + '] ' + msg;
      log.insertBefore(entry, log.firstChild);
      // 最大100件
      while (log.children.length > 100) {
        log.removeChild(log.lastChild);
      }
    }
  </script>
</body>
</html>
```

> **注意:**
> - `datahub.write()` は DataHub の WebView API に依存する。バージョンにより API 名が異なる場合がある（`datahub.setValue()` 等）。実際の API は DataHub Manager の [Help] → [Web Server API] を参照。
> - ポイント名（`POINTS` オブジェクト内）は、Data Browser で確認した実際の名前に合わせて変更すること。

アクセス: `http://192.168.56.30/substation_control.html`

---

## 4. 運用シナリオ別の操作手順

### 4.1 遮断器の開放操作（CB Trip）

実際の変電所における遮断器開放の操作手順と、本環境での対応：

```
┌────────────────────────────────────────────────┐
│  操作: XCBR1 遮断器を開放する                    │
│                                                  │
│  [実際の変電所]                                  │
│  1. 操作員が SCADA 画面で XCBR1 を選択           │
│  2. 「Trip」ボタンを押下                         │
│  3. 確認ダイアログで「実行」を選択               │
│  4. SCADA → RTU → IED と制御コマンドが伝搬      │
│  5. 遮断器が物理的に開放される                   │
│  6. 電流が遮断される                             │
│  7. SCADA 画面で状態が CLOSED → OPEN に変化      │
│                                                  │
│  [本環境]                                        │
│  1. Web HMI で XCBR1 の [Open] ボタンを押下     │
│  2. 確認ダイアログで [開放実行] を選択           │
│  3. DataHub → KEPServerEX → IED と伝搬          │
│  4. IED コンソール:                              │
│     [CONTROL] XCBR1 command: OPEN                │
│     [UPDATE] XCBR1: state=1 (Open), OpCnt=1     │
│  5. 計測値: 電流 ~250A → 0A                     │
│  6. Web HMI: CLOSED ● → OPEN ○ (赤点滅)        │
└────────────────────────────────────────────────┘
```

### 4.2 遮断器の投入操作（CB Close）

```
┌────────────────────────────────────────────────┐
│  操作: XCBR1 遮断器を投入（復旧）する           │
│                                                  │
│  1. Web HMI で XCBR1 の [Close] ボタンを押下    │
│  2. 確認ダイアログで [投入実行] を選択           │
│  3. IED コンソール:                              │
│     [CONTROL] XCBR1 command: CLOSE               │
│     [UPDATE] XCBR1: state=2 (Closed), OpCnt=2   │
│  4. 計測値: 電流 0A → ~250A                     │
│  5. Web HMI: OPEN ○ → CLOSED ● (緑)            │
└────────────────────────────────────────────────┘
```

### 4.3 断路器の操作

```
┌────────────────────────────────────────────────┐
│  操作: XSWI1 断路器を開放する                    │
│                                                  │
│  ★ 実際の変電所では、断路器は無負荷時にのみ     │
│    操作可能（インターロック条件）。              │
│    本環境ではインターロックは実装していないため  │
│    任意のタイミングで操作可能。                  │
│                                                  │
│  正規の操作手順（参考）:                         │
│  1. まず遮断器 XCBR1 を開放（電流を遮断）       │
│  2. 電流が 0A であることを確認                   │
│  3. 断路器 XSWI1 を開放                         │
│  4. 回路が物理的に切り離される                   │
└────────────────────────────────────────────────┘
```

### 4.4 計測値の監視

```
正常時の計測値（遮断器 Closed 時）:
  Phase A Current:  ~250 A  ± 5A（正弦波変動）
  Phase B Current:  ~250 A  ± 5A（+120° 位相差）
  Phase C Current:  ~250 A  ± 5A（+240° 位相差）
  Phase A Voltage:  ~110 kV ± 0.5kV
  Phase B Voltage:  ~110 kV ± 0.5kV
  Phase C Voltage:  ~110 kV ± 0.5kV
  Frequency:        ~50.00 Hz ± 0.02Hz

異常時（遮断器 Open 時）:
  Phase A/B/C Current: 0 A  ← 電流遮断
  Voltage / Frequency: 変化なし（電源側は生きている）
```

---

## 5. 制御のプロトコルシーケンス

SCADA の [Open] ボタン押下から IED の遮断器開放までのプロトコルレベルのシーケンス：

```
SCADA (DataHub)          RTU (KEPServerEX)          IED (substation_server)
  │                          │                          │
  │ ① DataHub WebView API   │                          │
  │    write(ctlVal, 0)      │                          │
  │                          │                          │
  │ ② OPC DA Write ────────→│                          │
  │    IOPCSyncIO::Write()   │                          │
  │    ItemID: XCBR1_Control │                          │
  │    Value: VT_BOOL=false  │                          │
  │    (DCOM RPC)            │                          │
  │                          │                          │
  │                          │ ③ MMS Operate ─────────→│
  │                          │    IEC 61850 Operate      │
  │                          │    Object: XCBR1.Pos      │
  │                          │    ctlVal: false           │
  │                          │                          │
  │                          │                          │ ④ ControlHandler
  │                          │                          │    xcbr1_pending=1
  │                          │                          │    xcbr1_new_pos=1
  │                          │                          │
  │                          │                          │ ⑤ Main loop
  │                          │                          │    lock()
  │                          │                          │    updateDbpos(1)
  │                          │                          │    updateOpCnt(+1)
  │                          │                          │    unlock()
  │                          │                          │
  │                          │ ⑥ MMS Read/Report ←────│
  │                          │    Pos.stVal = 1 (Open)  │
  │                          │    A.phsA = 0.0          │
  │                          │                          │
  │ ⑦ OPC DA Callback ←────│                          │
  │    OnDataChange()        │                          │
  │    XCBR1.Pos.stVal = 1   │                          │
  │    MMXU1.A.phsA = 0.0   │                          │
  │                          │                          │
  │ ⑧ Web HMI 更新          │                          │
  │    CLOSED → OPEN         │                          │
  │    250A → 0A             │                          │
```

---

## 6. 運用確認チェックリスト

### Read 方向（監視）の確認 — 既存

| # | 確認項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| R1 | IED → KEPServerEX | KEP Quick Client でタグ値を確認 | 計測値がリアルタイム変動 | □ |
| R2 | KEPServerEX → DataHub | DataHub Data Browser で値確認 | 値が連動して変動 | □ |
| R3 | DataHub → Web HMI | ブラウザで HMI ページを表示 | 値がリアルタイム表示 | □ |

### Write 方向（制御）の確認 — 今回追加

| # | 確認項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| W1 | 制御タグの追加 | KEP に XCBR1_Control タグ作成 | タグが表示される | □ |
| W2 | KEP → IED 制御 | KEP Quick Client で Sync Write | IED コンソールに CONTROL 表示 | □ |
| W3 | DataHub Write 許可 | OPC DA 接続で Allow Write 有効 | 設定保存成功 | □ |
| W4 | DataHub → IED 制御 | Data Browser で ctlVal を Write | IED コンソールに CONTROL 表示 | □ |
| W5 | XCBR1 Open | ctlVal=0 を Write | stVal: 2→1、電流: 250→0 | □ |
| W6 | XCBR1 Close | ctlVal=1 を Write | stVal: 1→2、電流: 0→250 | □ |
| W7 | XCBR2 Open/Close | 同上 | 同上 | □ |
| W8 | XSWI1 Open/Close | 同上 | 同上 | □ |
| W9 | OpCnt 増加 | 複数回操作 | OpCnt がインクリメント | □ |
| W10 | Web HMI 制御 | ブラウザのボタンで操作 | 状態変化が画面に反映 | □ |
| W11 | IED 安定性 | 連続操作（10回以上 Open/Close） | クラッシュしない | □ |

---

## 7. トラブルシューティング

### KEPServerEX 側

| 症状 | 原因 | 対策 |
|---|---|---|
| 制御タグのアドレスが見つからない | IED のデータモデルに Oper が含まれていない | genmodel.jar を再実行し、ICD に DPC の CO 定義があるか確認 |
| Sync Write が "Write failed" | IED 側の ctlModel が status-only (0) | substation_server の初期値設定で `ctlModel=1` になっているか確認 |
| Write しても stVal が変化しない | アドレスが stVal（Status）で、Oper.ctlVal（Control）ではない | 制御タグのアドレスを `Oper.ctlVal` に変更 |
| Write は成功するが IED コンソールに何も出ない | KEPServerEX が MMS Write を発行したが、Operate ではない | ドライバの制御設定を確認、またはアドレスを Oper.ctlVal に変更 |

### DataHub 側

| 症状 | 原因 | 対策 |
|---|---|---|
| Data Browser で Write メニューが出ない | Allow Write が無効 | OPC DA 接続の Properties で有効化 |
| Write しても値が戻る | KEPServerEX が Read で上書き | 制御タグ（Oper.ctlVal）に書き込んでいるか確認。stVal に直接書いても IED が応答しないとサーバ側で元の値に戻る |
| Web HMI の制御ボタンが効かない | datahub.write() の API が異なる | ブラウザの DevTools Console でエラーを確認。DataHub バージョンに合わせて API 名を修正 |

### IED 側

| 症状 | 原因 | 対策 |
|---|---|---|
| CONTROL ログに "unknown" と表示 | クライアント情報の取得に失敗 | 動作に影響なし（表示上の問題のみ） |
| Open 後に電流が 0 にならない | update_measurements の xcbr1_pos 参照タイミング | 250ms 後の次回更新で反映される（正常動作） |
| 連続操作でクラッシュ | v1 を使用している | v2（遅延更新版）にアップデートし、`make clean && make` で完全リビルド |
