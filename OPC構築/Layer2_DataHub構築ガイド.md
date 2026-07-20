# Layer 2 構築ガイド: Cogent DataHub による SCADA / OPC DA クライアント

**VM3（192.168.56.30）上で Cogent DataHub を OPC DA クライアント兼 SCADA として構成する**

---

## 目次

- [1. 前提条件](#1-前提条件)
- [2. インストール](#2-インストール)
- [3. DCOM 事前設定（VM3 側）](#3-dcom-事前設定vm3-側)
- [4. OPC DA クライアント接続の設定](#4-opc-da-クライアント接続の設定)
  - [4.1 方法A: KEPServerEX に接続する場合](#41-方法a-kepserverex-に接続する場合)
  - [4.2 方法B/C: MatrikonOPC Simulation Server に接続する場合](#42-方法bc-matrikonopc-simulation-server-に接続する場合)
- [5. データの確認（Data Browser）](#5-データの確認data-browser)
- [6. Web HMI の構成](#6-web-hmi-の構成)
- [7. データベースロギング（ヒストリアン連携）](#7-データベースロギングヒストリアン連携)
  - [7.1 SQL Server Express のセットアップ](#71-sql-server-express-のセットアップ)
  - [7.2 ODBC データソースの作成](#72-odbc-データソースの作成)
  - [7.3 DataHub のデータベース接続設定](#73-datahub-のデータベース接続設定)
- [8. スクリプト処理（Gamma Script）](#8-スクリプト処理gamma-script)
- [9. アラーム設定](#9-アラーム設定)
- [10. 構成全体の動作確認](#10-構成全体の動作確認)
- [11. トラブルシューティング](#11-トラブルシューティング)

---

## 1. 前提条件

以下が完了していること：

| 項目 | 状態 |
|---|---|
| VM1 (IED): substation_server が起動中 | ✅ Layer 0 完了 |
| VM2 (RTU/GW): OPC DA サーバが起動中 | ✅ Layer 1 完了 |
| VM3 (SCADA): Windows 7 SP1 以降がインストール済み | ✅ |
| ネットワーク: 全 VM が 192.168.56.0/24 で相互疎通可能 | ✅ |

### VM 構成の確認

```
VM1 (IED)           VM2 (RTU/GW)              VM3 (SCADA)
192.168.56.10       192.168.56.20              192.168.56.30
                                                ← 今回構築する
[substation_server] → [KEPServerEX           → [Cogent DataHub
 MMS port 102]        or MatrikonOPC]           OPC DA Client
                      OPC DA Server              + DB Logger
                      ProgID:                    + Web HMI]
                      Kepware.KEPServerEX.V6
                      or
                      MatrikonOPC.Simulation.1
```

### インストーラの場所

```
ワークスペース内:
  OPC UA Test Pack\CogentDataHubFull_x64-11.0.4-260116-Windows.exe

マニュアル（日本語訳あり）:
  01_OPC_UA規格以上\02_マニアル_DataHub_Skkynet社\DataHub-user-manuals.pdf
  01_OPC_UA規格以上\02_マニアル_DataHub_Skkynet社\翻訳_DeepL_DataHub-user-manuals_1 ja.pdf
  01_OPC_UA規格以上\02_マニアル_DataHub_Skkynet社\翻訳_DeepL_DataHub-user-manuals_2 ja.pdf
```

---

## 2. インストール

### 手順

1. `CogentDataHubFull_x64-11.0.4-260116-Windows.exe` を VM3 にコピー
2. 管理者権限で実行
3. インストールウィザードに従う：
   - ライセンス: 評価版（30日間フル機能）
   - インストール先: デフォルト（`C:\Program Files\Cogent\`）
   - コンポーネント: **すべて選択**（OPC DA、OPC UA、Database、Web Server、Scripting）
4. インストール完了後、**再起動を求められた場合は再起動する**（DCOM 設定の反映に必要）

### 起動確認

インストール後、DataHub は Windows サービスとして自動起動する。
タスクトレイに DataHub アイコンが表示されていることを確認。

```
DataHub Manager の起動:
  [スタートメニュー] → [Cogent] → [DataHub Manager]
  または
  タスクトレイの DataHub アイコンをダブルクリック
```

DataHub Manager のメインウィンドウが開き、左ペインにツリー構造で各機能が表示される。

---

## 3. DCOM 事前設定（VM3 側）

DataHub が VM2 の OPC DA サーバにリモート DCOM 接続するため、VM3 側にも DCOM の設定が必要。

### dcomcnfg の設定

```
[スタート] → [ファイル名を指定して実行] → dcomcnfg
```

#### 3.1 マイ コンピュータ → プロパティ → 既定のプロパティ

| 項目 | 設定値 |
|---|---|
| このコンピュータ上で DCOM を有効にする | ☑ ON |
| 既定の認証レベル | **接続 (Connect)** ※ シナリオ1の場合は「なし (None)」 |
| 既定の偽装レベル | **識別 (Identify)** |

#### 3.2 COM セキュリティ → アクセス許可 → 制限の編集

| アカウント | ローカルアクセス | リモートアクセス |
|---|---|---|
| Everyone | ☑ | ☑ |
| ANONYMOUS LOGON | ☑ | ☑（シナリオ1のみ） |

#### 3.3 COM セキュリティ → 起動とアクティブ化の許可 → 制限の編集

| アカウント | ローカル起動 | リモート起動 | ローカルアクティブ化 | リモートアクティブ化 |
|---|---|---|---|---|
| Everyone | ☑ | ☑ | ☑ | ☑ |

#### 3.4 ファイアウォール

VM3 でも DCOM 用ポートを開放する（コールバック通信のため）：

```powershell
netsh advfirewall firewall add rule name="DCOM-EPM" dir=in action=allow protocol=TCP localport=135
netsh advfirewall firewall add rule name="DCOM-Dynamic" dir=in action=allow protocol=TCP localport=5000-5020
```

#### 3.5 hosts ファイル

`C:\WINDOWS\system32\drivers\etc\hosts` に以下を追記（未設定の場合）：

```
192.168.56.10  VM1-IED
192.168.56.20  VM2-RTUGW
192.168.56.30  VM3-SCADA
```

> **重要:** DCOM はリモート接続時にホスト名を逆引きする場合がある。DNS がない環境では hosts ファイルでの名前解決が必須。

---

## 4. OPC DA クライアント接続の設定

### 4.1 方法A: KEPServerEX に接続する場合

```
DataHub Manager を開く

左ペイン:
  [Data Sources] → [OPC DA]

右ペインで:
  [Add OPC DA Connection...] をクリック

以下を入力:

┌─────────────────────────────────────────────────┐
│  OPC DA Connection Settings                      │
│                                                   │
│  Connection Name:  VM2_RTUGW_Kepware              │
│                                                   │
│  Server:                                          │
│    ○ Local                                        │
│    ● Remote                                       │
│                                                   │
│  Computer Name:  VM2-RTUGW                        │
│  (または IP):    192.168.56.20                    │
│                                                   │
│  OPC Server:     Kepware.KEPServerEX.V6           │
│                                                   │
│  ┌─ Advanced ─────────────────────────────┐      │
│  │  Update Rate (ms):  1000               │      │
│  │  Group Name:        SubstationData     │      │
│  │  Active:            ☑                  │      │
│  │  Dead Band (%):     0                  │      │
│  └────────────────────────────────────────┘      │
│                                                   │
│            [Connect]   [Cancel]                   │
└─────────────────────────────────────────────────┘
```

**Update Rate** の設定指針:
- 1000 ms（1秒）: 標準的な監視間隔。計測値の変化を観測可能
- 100 ms: 高速更新。遮断器操作の瞬時応答を確認したい場合
- 5000 ms: 低負荷。DB ロギング中心の用途

### 4.2 方法B/C: MatrikonOPC Simulation Server に接続する場合

```
  Connection Name:  VM2_RTUGW_Matrikon
  Computer Name:    192.168.56.20
  OPC Server:       MatrikonOPC.Simulation.1
  Update Rate:      1000
```

### 接続成功の確認

接続が成功すると:
- DataHub Manager の左ペインに接続名が表示される
- 接続名の横にアイコンが **緑色** になる
- 赤色の場合は接続失敗（→ 第11章トラブルシューティング参照）

---

## 5. データの確認（Data Browser）

接続成功後、OPC DA サーバのタグが DataHub 内部のポイントとしてマッピングされる。

### Data Browser を開く

```
DataHub Manager:
  左ペイン → [View] → [Data Browser]
  または
  メニュー → [View] → [Data Browser]
```

### タグツリーの構造

方法A（KEPServerEX）の場合:
```
DataHub Points
└── VM2_RTUGW_Kepware
    └── IEC61850_Ch1
        └── IED1
            └── CBIED
                ├── XCBR1
                │   ├── Pos
                │   │   ├── stVal    = 2 (Closed)  ★
                │   │   └── ctlModel = 1
                │   └── OpCnt
                │       └── stVal    = 0
                ├── XCBR2
                │   └── Pos
                │       └── stVal    = 2 (Closed)
                ├── XSWI1
                │   └── Pos
                │       └── stVal    = 2 (Closed)
                └── MMXU1
                    ├── A
                    │   ├── phsA.cVal.mag.f = ~250.0  ★
                    │   ├── phsB.cVal.mag.f = ~248.0
                    │   └── phsC.cVal.mag.f = ~251.0
                    ├── PhV
                    │   ├── phsA.cVal.mag.f = ~110.0
                    │   ├── phsB.cVal.mag.f = ~110.0
                    │   └── phsC.cVal.mag.f = ~110.0
                    └── Hz
                        └── mag.f = ~50.0
```

方法B/C（MatrikonOPC）の場合:
```
DataHub Points
└── VM2_RTUGW_Matrikon
    └── Substation
        ├── Bay1
        │   ├── XCBR1
        │   │   ├── Pos_stVal    = 2     ★
        │   │   └── Pos_Oper     = 0
        │   └── XSWI1
        │       └── Pos_stVal    = 2
        ├── Bay2
        │   └── XCBR2
        │       └── Pos_stVal    = 2
        └── Measurement
            ├── PhaseA_Current   = ~250.0  ★
            ├── PhaseB_Current   = ~248.0
            ├── PhaseC_Current   = ~251.0
            ├── BusVoltage_kV    = ~110.0
            └── Frequency_Hz     = ~50.0
```

### 表示の読み方

Data Browser の各ポイントには以下の情報が表示される:

| カラム | 意味 |
|---|---|
| **Name** | タグのフルパス |
| **Value** | 現在値 |
| **Quality** | OPC 品質コード（Good / Bad / Uncertain） |
| **Timestamp** | 最終更新時刻 |
| **Type** | データ型（VT_R4, VT_I4, VT_BOOL 等） |

**Value がリアルタイムで変動していること（特に電流・電圧・周波数）を確認する。**
値が変動していれば、VM1 → VM2 → VM3 のデータフローが正常に動作している。

---

## 6. Web HMI の構成

DataHub にはビルトインの Web サーバが搭載されており、ブラウザベースの簡易 HMI を構成できる。
これにより SCADA 画面の模擬表示を、追加ソフトなしで実現できる。

### 6.1 Web サーバの有効化

```
DataHub Manager:
  左ペイン → [Web Server]
  右ペイン:
    ☑ Enable Web Server
    Port: 80 (または 8080)
    ☑ Allow remote connections

  → [Apply]
```

### 6.2 ブラウザからのアクセス

VM3 上のブラウザまたはホスト OS のブラウザから:

```
http://192.168.56.30/
```

DataHub のデフォルト Web ページが表示される。ここから Data Browser のWeb版にアクセス可能。

### 6.3 カスタム HMI ページの作成

DataHub の Web HMI は HTML + JavaScript で独自のページを作成できる。
以下は変電所の状態表示ページの例:

ファイルの配置場所:
```
C:\Program Files\Cogent\DataHub\www\substation.html
```

```html
<!DOCTYPE html>
<html>
<head>
  <title>Substation Monitor</title>
  <meta charset="UTF-8">
  <script src="/datahub.js"></script>
  <style>
    body { font-family: Consolas, monospace; background: #1a1a2e; color: #e0e0e0; padding: 20px; }
    h1 { color: #00d4ff; }
    .panel { background: #16213e; border: 1px solid #0f3460; border-radius: 8px;
             padding: 15px; margin: 10px 0; }
    .value { font-size: 24px; font-weight: bold; }
    .closed { color: #00ff88; }
    .open   { color: #ff4444; font-weight: bold; }
    .label  { color: #8888aa; font-size: 14px; }
    table { border-collapse: collapse; width: 100%; }
    td { padding: 8px 12px; border-bottom: 1px solid #0f3460; }
  </style>
</head>
<body>
  <h1>⚡ Substation IED Monitor</h1>

  <div class="panel">
    <h2>Circuit Breakers</h2>
    <table>
      <tr>
        <td class="label">XCBR1 (Bay 1)</td>
        <td class="value" id="xcbr1">--</td>
        <td class="label">Operations</td>
        <td id="xcbr1_cnt">--</td>
      </tr>
      <tr>
        <td class="label">XCBR2 (Bay 2)</td>
        <td class="value" id="xcbr2">--</td>
        <td class="label">Operations</td>
        <td id="xcbr2_cnt">--</td>
      </tr>
      <tr>
        <td class="label">XSWI1 (Disconnector)</td>
        <td class="value" id="xswi1">--</td>
        <td></td><td></td>
      </tr>
    </table>
  </div>

  <div class="panel">
    <h2>Measurements</h2>
    <table>
      <tr>
        <td class="label">Phase A Current</td><td class="value" id="ia">-- A</td>
        <td class="label">Phase A Voltage</td><td class="value" id="va">-- kV</td>
      </tr>
      <tr>
        <td class="label">Phase B Current</td><td class="value" id="ib">-- A</td>
        <td class="label">Phase B Voltage</td><td class="value" id="vb">-- kV</td>
      </tr>
      <tr>
        <td class="label">Phase C Current</td><td class="value" id="ic">-- A</td>
        <td class="label">Phase C Voltage</td><td class="value" id="vc">-- kV</td>
      </tr>
      <tr>
        <td class="label">Frequency</td><td class="value" id="hz">-- Hz</td>
        <td></td><td></td>
      </tr>
    </table>
  </div>

  <div class="panel" id="alert-panel" style="display:none; border-color:#ff4444;">
    <h2 style="color:#ff4444;">⚠ ALERT</h2>
    <p id="alert-msg"></p>
  </div>

  <script>
    // DataHub WebView API を使用してポイントを購読
    // ※ 実際のポイント名は DataHub の Data Browser で確認した名前に合わせること

    var datahub = new DataHub();

    // KEPServerEX の場合のポイント名例（環境に合わせて変更）
    var POINTS = {
      xcbr1:     "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.XCBR1.Pos.stVal",
      xcbr2:     "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.XCBR2.Pos.stVal",
      xswi1:     "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.XSWI1.Pos.stVal",
      xcbr1_cnt: "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.XCBR1.OpCnt.stVal",
      xcbr2_cnt: "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.XCBR2.OpCnt.stVal",
      ia:        "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsA.cVal.mag.f",
      ib:        "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsB.cVal.mag.f",
      ic:        "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsC.cVal.mag.f",
      va:        "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsA.cVal.mag.f",
      vb:        "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsB.cVal.mag.f",
      vc:        "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsC.cVal.mag.f",
      hz:        "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.MMXU1.Hz.mag.f"
    };

    // MatrikonOPC の場合のポイント名例:
    // var POINTS = {
    //   xcbr1: "VM2_RTUGW_Matrikon:Substation.Bay1.XCBR1.Pos_stVal",
    //   ia:    "VM2_RTUGW_Matrikon:Substation.Measurement.PhaseA_Current",
    //   ...
    // };

    function formatBreaker(val) {
      if (val == 2) return '<span class="closed">CLOSED ●</span>';
      if (val == 1) return '<span class="open">OPEN ○</span>';
      return '<span>INTERMEDIATE ◎</span>';
    }

    datahub.onPointChange = function(name, value, quality, timestamp) {
      // 遮断器
      if (name === POINTS.xcbr1) {
        document.getElementById('xcbr1').innerHTML = formatBreaker(value);
        checkAlert(value, 'XCBR1');
      }
      if (name === POINTS.xcbr2) {
        document.getElementById('xcbr2').innerHTML = formatBreaker(value);
        checkAlert(value, 'XCBR2');
      }
      if (name === POINTS.xswi1)
        document.getElementById('xswi1').innerHTML = formatBreaker(value);

      // カウンタ
      if (name === POINTS.xcbr1_cnt)
        document.getElementById('xcbr1_cnt').textContent = value;
      if (name === POINTS.xcbr2_cnt)
        document.getElementById('xcbr2_cnt').textContent = value;

      // 計測値
      if (name === POINTS.ia)
        document.getElementById('ia').textContent = parseFloat(value).toFixed(1) + ' A';
      if (name === POINTS.ib)
        document.getElementById('ib').textContent = parseFloat(value).toFixed(1) + ' A';
      if (name === POINTS.ic)
        document.getElementById('ic').textContent = parseFloat(value).toFixed(1) + ' A';
      if (name === POINTS.va)
        document.getElementById('va').textContent = parseFloat(value).toFixed(1) + ' kV';
      if (name === POINTS.vb)
        document.getElementById('vb').textContent = parseFloat(value).toFixed(1) + ' kV';
      if (name === POINTS.vc)
        document.getElementById('vc').textContent = parseFloat(value).toFixed(1) + ' kV';
      if (name === POINTS.hz)
        document.getElementById('hz').textContent = parseFloat(value).toFixed(3) + ' Hz';
    };

    function checkAlert(breakerValue, breakerName) {
      var panel = document.getElementById('alert-panel');
      var msg = document.getElementById('alert-msg');
      if (breakerValue == 1) {
        panel.style.display = 'block';
        msg.textContent = breakerName + ' is OPEN! Possible unauthorized operation at ' +
                          new Date().toLocaleTimeString();
      }
    }

    // 全ポイントを購読
    for (var key in POINTS) {
      datahub.subscribe(POINTS[key]);
    }
    datahub.connect();
  </script>
</body>
</html>
```

アクセス: `http://192.168.56.30/substation.html`

> **注:** DataHub の WebView API（`datahub.js`）のインターフェースはバージョンにより異なる場合がある。上記は DataHub 11.x の一般的な WebView API を想定したサンプル。実際の API 仕様は DataHub Manager の [Help] → [Web Server API] または付属マニュアルを参照。

---

## 7. データベースロギング（ヒストリアン連携）

DataHub のデータベースロギング機能により、OPC DA の値を SQL Server に自動記録する。

### 7.1 SQL Server Express のセットアップ

VM3 に SQL Server Express をインストールする（クローズ環境移行ガイド参照）。

```sql
-- SSMS で実行

-- データベース作成
CREATE DATABASE Substation_Historian;
GO

USE Substation_Historian;
GO

-- プロセスデータ蓄積テーブル
CREATE TABLE ProcessData (
    ID          BIGINT IDENTITY(1,1) PRIMARY KEY,
    PointName   NVARCHAR(512)  NOT NULL,
    Value       FLOAT          NULL,
    ValueStr    NVARCHAR(256)  NULL,
    Quality     NVARCHAR(50)   NOT NULL DEFAULT 'Good',
    Timestamp   DATETIME2(3)   NOT NULL,
    InsertTime  DATETIME2(3)   NOT NULL DEFAULT SYSDATETIME()
);
GO

-- 検索用インデックス
CREATE INDEX IX_ProcessData_PointTime
    ON ProcessData (PointName, Timestamp);
GO

-- 遮断器イベントビュー（遮断器の状態変化のみ抽出）
CREATE VIEW vw_BreakerEvents AS
SELECT
    ID,
    PointName,
    CASE CAST(Value AS INT)
        WHEN 1 THEN 'OPEN'
        WHEN 2 THEN 'CLOSED'
        ELSE 'INTERMEDIATE'
    END AS BreakerState,
    Timestamp,
    InsertTime
FROM ProcessData
WHERE PointName LIKE '%XCBR%Pos%stVal%'
   OR PointName LIKE '%XCBR%Pos_stVal%';
GO

-- DataHub が使用するログインアカウント
CREATE LOGIN datahub_user WITH PASSWORD = 'DH_Log2026!';
GO

USE Substation_Historian;
CREATE USER datahub_user FOR LOGIN datahub_user;
ALTER ROLE db_datawriter ADD MEMBER datahub_user;
ALTER ROLE db_datareader ADD MEMBER datahub_user;
GO
```

### 7.2 ODBC データソースの作成

```
[コントロールパネル] → [管理ツール] → [ODBC データ ソース (64bit)]
→ [システム DSN] タブ → [追加]

  ドライバ:     SQL Server Native Client 11.0
  DSN 名:       SubstationHistorian
  サーバー:     (local)\SQLEXPRESS
  認証:         SQL Server 認証
  ログイン ID:  datahub_user
  パスワード:   DH_Log2026!
  既定のDB:     Substation_Historian

→ [接続のテスト] → 「テストは正常に完了しました」を確認
→ [OK]
```

### 7.3 DataHub のデータベース接続設定

```
DataHub Manager:
  左ペイン → [Database]

右ペイン:
  ☑ Enable Database Logging

  [Add Connection...]

┌─────────────────────────────────────────────────┐
│  Database Connection Settings                    │
│                                                   │
│  Connection Name:  SubstationHistorian            │
│                                                   │
│  Connection Type:  ● ODBC                         │
│                    ○ ADO                          │
│                                                   │
│  ODBC DSN:  SubstationHistorian                   │
│  User:      datahub_user                          │
│  Password:  DH_Log2026!                           │
│                                                   │
│            [Test Connection]                      │
│            → "Connection successful" を確認       │
└─────────────────────────────────────────────────┘
```

### ロギングルールの設定

```
[Logging Rules] → [Add Rule...]

┌─────────────────────────────────────────────────┐
│  Logging Rule                                     │
│                                                   │
│  Rule Name:     AllSubstationData                 │
│                                                   │
│  Source Points:  VM2_RTUGW_Kepware:*              │
│  (または)        VM2_RTUGW_Matrikon:*             │
│                                                   │
│  Table Name:     ProcessData                      │
│                                                   │
│  Column Mapping:                                  │
│    PointName  ←  {PointName}                      │
│    Value      ←  {Value}                          │
│    Quality    ←  {Quality}                        │
│    Timestamp  ←  {Timestamp}                      │
│                                                   │
│  Logging Mode:                                    │
│    ● On Change (値が変化した時のみ記録)            │
│    ○ Periodic  (一定間隔で記録)                   │
│      Interval: [    1000] ms                      │
│    ○ Both                                         │
│                                                   │
│  ☑ Log on startup                                │
│  ☑ Log quality changes                           │
└─────────────────────────────────────────────────┘

→ [OK] → [Apply]
```

**推奨設定:**

| タグ種類 | ロギングモード | 理由 |
|---|---|---|
| 遮断器/断路器の位置 (Pos.stVal) | **On Change** | 状態変化イベントの記録が重要。Industroyer 攻撃の痕跡が残る |
| 計測値 (電流/電圧/周波数) | **Periodic (1000ms)** | トレンドデータとして一定間隔の記録が必要 |
| 動作カウンタ (OpCnt) | **On Change** | 操作回数の変化を記録 |

遮断器と計測値でルールを分ける場合:

```
Rule 1: BreakerEvents
  Source Points: *XCBR*stVal, *XSWI*stVal, *OpCnt*
  Mode: On Change

Rule 2: MeasurementTrend
  Source Points: *MMXU*, *Current*, *Voltage*, *Frequency*, *Hz*
  Mode: Periodic 1000ms
```

### ロギングの確認

SSMS で以下のクエリを実行し、データが蓄積されていることを確認:

```sql
-- 最新10件
SELECT TOP 10 * FROM ProcessData ORDER BY ID DESC;

-- 遮断器イベントの確認
SELECT * FROM vw_BreakerEvents ORDER BY Timestamp DESC;

-- ポイント別の件数確認
SELECT PointName, COUNT(*) as Records
FROM ProcessData
GROUP BY PointName
ORDER BY Records DESC;
```

---

## 8. スクリプト処理（Gamma Script）

DataHub には Gamma Script（Lisp 系スクリプト言語）が内蔵されており、データの加工やイベント駆動処理を記述できる。

### 遮断器状態変化の検知スクリプト

```
DataHub Manager:
  左ペイン → [Scripting] → [Gamma Script]
  [New Script...] → 名前: breaker_monitor
```

```lisp
;; breaker_monitor.g
;; 遮断器の状態変化を監視し、Open イベントをログに記録する

;; コールバック: XCBR1 の Pos.stVal が変化したとき
(define (on_xcbr1_change point value quality timestamp)
  (let ((state (if (= value 1) "OPEN"
                 (if (= value 2) "CLOSED" "INTERMEDIATE"))))
    ;; コンソールにログ出力
    (princ (format nil "*** XCBR1 STATE CHANGE: ~A at ~A ***~%"
                   state (timestamp-to-string timestamp)))
    ;; Open イベントの場合にアラートを発行
    (when (= value 1)
      (princ "!!! ALERT: XCBR1 opened - possible unauthorized operation !!!~%")
      ;; 別のポイントにアラートフラグをセット（Web HMI 連携用）
      (set! datahub:alert_xcbr1 1))))

;; ポイント変化のリスナー登録
;; ※ ポイント名は Data Browser で確認した実際のパスに合わせること
(register-point-callback
  "VM2_RTUGW_Kepware:IEC61850_Ch1.IED1.CBIED.XCBR1.Pos.stVal"
  on_xcbr1_change)
```

> **注:** Gamma Script の正確な API は DataHub のバージョンにより異なる。上記は概念的なサンプル。実際のAPI については DataHub Manager の [Help] → [Scripting Reference] または付属マニュアルを参照。

---

## 9. アラーム設定

DataHub のアラーム機能により、遮断器が意図せず Open になった場合に警報を発生させる。

```
DataHub Manager:
  左ペイン → [Alarms & Events]

[Add Alarm...]

┌─────────────────────────────────────────────────┐
│  Alarm Definition                                │
│                                                   │
│  Alarm Name:   XCBR1_Unexpected_Open              │
│  Source Point:  (XCBR1 の Pos.stVal ポイント)      │
│                                                   │
│  Condition:                                       │
│    Trigger when value  [==]  [1]                  │
│    (1 = Open)                                     │
│                                                   │
│  Severity:     ● Critical                         │
│  Message:      "XCBR1 breaker opened"             │
│                                                   │
│  ☑ Log to database                               │
│  ☑ Log to event viewer                           │
└─────────────────────────────────────────────────┘
```

同様に XCBR2、XSWI1 にもアラームを設定。

---

## 10. 構成全体の動作確認

### チェックリスト

| # | 確認項目 | 手順 | 期待結果 |
|---|---|---|---|
| 1 | DataHub → OPC DA 接続 | DataHub Manager で接続状態を確認 | 緑アイコン（接続成功） |
| 2 | タグの値更新 | Data Browser で計測値の変動を確認 | 電流/電圧/周波数がリアルタイム変動 |
| 3 | 遮断器の初期状態 | Data Browser で XCBR1.Pos.stVal を確認 | 値 = 2 (Closed) |
| 4 | DB ロギング | SSMS で `SELECT TOP 5 * FROM ProcessData` | レコードが蓄積されている |
| 5 | Web HMI | ブラウザで `http://192.168.56.30/substation.html` | 値がリアルタイム表示 |

### エンドツーエンドのデータフロー確認

```
VM1 (IED)                VM2 (RTU/GW)           VM3 (SCADA/DataHub)
substation_server        OPC DA Server           DataHub
                                                  ├── Data Browser ← 値を目視
1. 電流値が変動    →  2. タグ値が更新  →   3. ポイント値が変動
   (正弦波シミュレーション)                       4. DB に記録
                                                  5. Web HMI に表示
```

### 遮断器操作テスト

VM1 側の `breaker_control_test` または VM4 からの OPC DA Write で遮断器を Open にした際に:

1. DataHub の Data Browser で `Pos.stVal` が `2 → 1` に変化することを確認
2. MMXU1 の電流値が `~250A → 0A` に変化することを確認
3. DB の `vw_BreakerEvents` に Open イベントが記録されることを確認
4. Web HMI にアラートが表示されることを確認（カスタム HTML を配置した場合）

---

## 11. トラブルシューティング

### DataHub が OPC DA サーバに接続できない

| 症状 | 原因 | 対策 |
|---|---|---|
| 接続アイコンが赤 | DCOM 設定不足 | VM2, VM3 双方の dcomcnfg を再確認 |
| "RPC server unavailable" | TCP 135 がブロック | VM2 のファイアウォールで 135 を開放 |
| "Access denied" | DCOM 権限不足 | VM2 の COM セキュリティに Everyone を追加 |
| サーバ一覧が空 | OPCEnum 停止 | VM2 で `services.msc` → OPCEnum を開始 |
| サーバ一覧にあるが接続不可 | 動的ポートブロック | VM2 で DCOM 動的ポート（5000-5020）を FW 開放 |

### 接続できるがデータが来ない

| 症状 | 原因 | 対策 |
|---|---|---|
| Quality = Bad | OPC サーバのデータソースが未接続 | VM2 のブリッジスクリプト / KEPServerEX のチャネル状態を確認 |
| 値が更新されない | Update Rate が大きすぎる | DataHub の接続設定で Update Rate を 1000ms 以下に |
| 一部のタグだけ Bad | タグパスの不一致 | Data Browser でタグツリーを確認し、正しいパスを指定 |

### DB ロギングが動かない

| 症状 | 原因 | 対策 |
|---|---|---|
| テーブルにレコードがない | ODBC DSN の設定ミス | DataHub の [Test Connection] で確認 |
| "Login failed" | SQL Server 認証無効 | SSMS → サーバプロパティ → セキュリティ → SQL Server 認証モード を有効化後、サービス再起動 |
| "Cannot open database" | DB 名の不一致 | ODBC DSN の既定のDBを確認 |
| ロギングルールが未適用 | [Apply] を押していない | DataHub Manager で [Apply] → サービス再起動 |

### Web HMI が表示されない

| 症状 | 原因 | 対策 |
|---|---|---|
| 接続拒否 | Web Server 無効 | DataHub Manager → Web Server → Enable |
| 外部からアクセス不可 | リモート接続不許可 | "Allow remote connections" を有効化 |
| ポート競合 | IIS や他のサービスが 80 番を使用 | DataHub の Web Server ポートを 8080 に変更 |
| ページが真っ白 | datahub.js のパスが間違っている | ブラウザの DevTools で 404 エラーを確認 |
