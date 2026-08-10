# Layer 2 構築ガイド（代替案B）: Ignition による SCADA 構成

**VM3（192.168.56.30）上で Inductive Automation Ignition を統合 SCADA プラットフォームとして構成する**

---

## 目次

- [1. Ignition を SCADA として使う場合の構成](#1-ignition-を-scada-として使う場合の構成)
- [2. Ignition の概要とライセンス](#2-ignition-の概要とライセンス)
- [3. インストール](#3-インストール)
- [4. Gateway の初期設定](#4-gateway-の初期設定)
- [5. OPC DA 接続の設定](#5-opc-da-接続の設定)
  - [5.1 OPC-COM モジュールの確認](#51-opc-com-モジュールの確認)
  - [5.2 OPC DA サーバ接続の追加](#52-opc-da-サーバ接続の追加)
  - [5.3 タグの確認（OPC Quick Client）](#53-タグの確認opc-quick-client)
- [6. タグプロバイダの設定](#6-タグプロバイダの設定)
- [7. Tag Historian（ヒストリアン）](#7-tag-historianヒストリアン)
  - [7.1 データベース接続の作成](#71-データベース接続の作成)
  - [7.2 Tag Historian の有効化](#72-tag-historian-の有効化)
  - [7.3 タグへの履歴記録の設定](#73-タグへの履歴記録の設定)
- [8. Perspective / Vision による HMI 画面](#8-perspective--vision-による-hmi-画面)
  - [8.1 Perspective（Web ベース）](#81-perspectiveweb-ベース)
  - [8.2 Vision（Java クライアント）](#82-visionjava-クライアント)
- [9. アラーム設定](#9-アラーム設定)
- [10. DCOM 事前設定（VM3 側）](#10-dcom-事前設定vm3-側)
- [11. 動作確認](#11-動作確認)
- [12. トラブルシューティング](#12-トラブルシューティング)
- [13. DataHub / MatrikonOPC との比較](#13-datahub--matrikonopc-との比較)

---

## 1. Ignition を SCADA として使う場合の構成

Ignition は産業用 SCADA / MES プラットフォームとして近年急速に普及している製品であり、OPC DA/UA クライアント、Tag Historian、Web ベース HMI (Perspective)、アラーム管理を単一プラットフォームで提供する。

```
VM3 (SCADA) の内部構成:

┌────────────────────────────────────────────────────┐
│  VM3: 192.168.56.30                                 │
│                                                      │
│  ┌──────────────────────────────────────────────┐  │
│  │  Ignition Gateway (Java / Jetty)              │  │
│  │                                                │  │
│  │  ├── OPC-COM Module     → VM2 OPC DA 接続     │  │
│  │  ├── Tag Provider       → タグ管理            │  │
│  │  ├── Tag Historian      → DB 自動ロギング     │  │
│  │  ├── Alarm Notification → アラーム管理        │  │
│  │  ├── Perspective Module → Web HMI             │  │
│  │  └── SQL Bridge         → DB 連携             │  │
│  │                                                │  │
│  │  Web Gateway:  http://192.168.56.30:8088       │  │
│  │  Perspective:  http://192.168.56.30:8088/      │  │
│  │                data/perspective/client/...     │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  ┌──────────────────┐                               │
│  │  SQL Server Express│  ← Historian の             │
│  │  or MySQL / PostgreSQL  バックエンド DB          │
│  └──────────────────┘                               │
└────────────────────────────────────────────────────┘
```

---

## 2. Ignition の概要とライセンス

| 項目 | 内容 |
|---|---|
| **開発元** | Inductive Automation (米国) |
| **バージョン** | Ignition 8.1.x（本書執筆時点） |
| **ライセンス** | **Trial Mode: 2時間ごとにリセット（再起動で再度2時間）、機能制限なし、無期限** |
| **動作環境** | Java ベース。Windows / Linux / macOS |
| **Web ベース管理** | Gateway Config（管理画面）、Perspective Designer（HMI 設計）すべてブラウザベース |
| **OPC DA 対応** | OPC-COM Module（別途インストール、Windows のみ） |
| **OPC UA 対応** | 標準搭載 |
| **入手先** | https://inductiveautomation.com/downloads/ |

> **Trial Mode の注意:** 2時間経過するとGateway が自動停止する。再起動すれば再度2時間使用可能。データベースに保存されたデータは再起動後も保持される。

### クローズ環境向け

オフラインインストーラが提供されている。JRE も同梱されているため、Java の個別インストールは不要。

```
ダウンロードファイル:
  Ignition-windows-8.1.xx-x64.zip  (約 1GB)
  ※ "full installer" を選択すること（Web installer ではなく）
```

---

## 3. インストール

### 手順

```
1. インストーラを VM3 にコピー
2. 管理者権限で実行
3. インストールウィザード:
   - Install Type: Typical
   - Service Account: Local System
   - HTTP Port: 8088  (デフォルト)
   - HTTPS Port: 8043 (デフォルト)
   - Gateway Network Port: 8060 (デフォルト)

4. インストール完了後、ブラウザが自動的に開く:
   http://localhost:8088
```

### OPC-COM モジュールの追加インストール

Ignition の OPC DA サポートは **OPC-COM Module** として別途インストールが必要。

```
1. https://inductiveautomation.com/downloads/third-party-modules
   から OPC-COM Module (.modl) をダウンロード
   ※ クローズ環境の場合は事前にダウンロードしておく

2. Ignition Gateway Web UI:
   http://localhost:8088

3. [Config] → [Modules] → [Install or Upgrade Module...]
   → opc-com.modl をアップロード

4. モジュール一覧に "OPC-COM" が表示され、
   Status: Running であることを確認
```

---

## 4. Gateway の初期設定

### 初回セットアップウィザード

```
ブラウザで http://localhost:8088 にアクセス

1. EULA Agreement → [Accept]

2. Create Admin Account:
   Username: admin
   Password: (任意)

3. Ports: デフォルトのまま
   HTTP:  8088
   HTTPS: 8043

4. [Start Ignition]
```

### Gateway Web UI の構成

```
http://localhost:8088

左メニュー:
├── [Status]        ← 全体の稼働状態
├── [Config]        ← 各種設定
│   ├── OPC Connections  ← OPC DA/UA 接続管理
│   ├── Databases        ← DB 接続管理
│   ├── Tags             ← タグプロバイダ
│   ├── Alarming         ← アラーム設定
│   └── Modules          ← モジュール管理
└── [Designer]      ← Perspective / Vision の HMI 設計ツール起動
```

---

## 5. OPC DA 接続の設定

### 5.1 OPC-COM モジュールの確認

```
[Config] → [Modules]

Module Name         Version     Status
─────────────────   ────────    ───────
OPC-COM             x.x.x       Running  ← これが必要
Perspective         x.x.x       Running
Tag Historian       x.x.x       Running
...
```

### 5.2 OPC DA サーバ接続の追加

```
[Config] → [OPC Connections] → [OPC-COM]

[Add new OPC-COM Connection...]

┌───────────────────────────────────────────────────┐
│  OPC-COM Connection                                │
│                                                     │
│  Name:           VM2_RTUGW                          │
│  Description:    RTU/Gateway OPC DA Server           │
│                                                     │
│  Server:                                            │
│    Host:         192.168.56.20                      │
│    Prog ID:      Kepware.KEPServerEX.V6             │
│    (または)       MatrikonOPC.Simulation.1           │
│                                                     │
│  ┌─ Advanced ──────────────────────────────┐       │
│  │  Connection Mode:    ☑ Auto             │       │
│  │  Read Timeout (ms):  10000              │       │
│  │  Group Update                            │       │
│  │    Rate (ms):        1000               │       │
│  │  CLSID:              (空欄 = 自動検出)  │       │
│  └─────────────────────────────────────────┘       │
│                                                     │
│  [Save Changes]                                     │
└───────────────────────────────────────────────────┘
```

### 接続状態の確認

```
[Status] → [Connections] → [OPC-COM]

Name          Host             Server                      Status
─────────     ──────────────   ─────────────────────────   ──────
VM2_RTUGW     192.168.56.20    Kepware.KEPServerEX.V6      Connected ✅
```

**Connected** と表示されれば接続成功。**Faulted** の場合は §12 トラブルシューティング参照。

### 5.3 タグの確認（OPC Quick Client）

Ignition には OPC Quick Client が内蔵されており、接続した OPC サーバのタグをブラウズできる。

```
[Config] → [OPC Connections] → [Quick Client]

Server: [VM2_RTUGW] を選択
→ [Browse] をクリック

タグツリーが表示される:
  IEC61850_Ch1/
  └── IED1/
      └── CBIED/
          ├── XCBR1/Pos/stVal        Value: 2  Quality: Good
          ├── XCBR1/OpCnt/stVal      Value: 0  Quality: Good
          ├── XCBR2/Pos/stVal        Value: 2  Quality: Good
          ├── XSWI1/Pos/stVal        Value: 2  Quality: Good
          ├── MMXU1/A/phsA/cVal/mag/f  Value: 250.3  Quality: Good
          ├── MMXU1/PhV/phsA/cVal/mag/f Value: 110.1  Quality: Good
          └── MMXU1/Hz/mag/f         Value: 50.001  Quality: Good
```

Quick Client からは Read / Write の操作も可能:
- タグを選択 → [Read] で現在値を取得
- タグを選択 → [Write] → 値を入力 → [Write Value]

---

## 6. タグプロバイダの設定

OPC DA のタグを Ignition のタグシステムにマッピングする。

```
[Config] → [Tags] → [Realtime Tag Providers]

デフォルトの "default" プロバイダが存在する。

[Designer] を起動:
  http://localhost:8088 → [Get Designer] → Designer をダウンロード・起動
  ※ Ignition Designer は Java Web Start または Launcher でインストール

Designer 内:
  左ペイン → [Tag Browser]
  → [Tags] → [default]
  → 右クリック → [New Tag] → [OPC Tag]

  ┌─ OPC Tag 設定 ────────────────────────────┐
  │                                              │
  │  Tag Name:   XCBR1_Position                  │
  │  OPC Server: VM2_RTUGW                       │
  │  OPC Item Path:                              │
  │    [VM2_RTUGW]IEC61850_Ch1.IED1.CBIED.XCBR1.Pos.stVal │
  │                                              │
  │  Data Type:  Int4                            │
  │  Scan Class: Default (1000ms)                │
  │  Enabled:    ☑                               │
  └──────────────────────────────────────────────┘
```

### 一括タグ追加

個別に追加する代わりに、フォルダ単位でドラッグ＆ドロップでインポートすることも可能:

```
Designer:
  Tag Browser → [OPC Browser] パネル
  → VM2_RTUGW サーバを展開
  → CBIED フォルダを丸ごと [default] タグプロバイダにドラッグ＆ドロップ
  → 全タグが自動的に OPC タグとして登録される
```

### 推奨タグ構成

```
[default]
└── Substation/
    ├── Bay1/
    │   ├── XCBR1_Position      (OPC → XCBR1.Pos.stVal)
    │   ├── XCBR1_OpCount       (OPC → XCBR1.OpCnt.stVal)
    │   └── XCBR1_Control       (OPC → XCBR1.Pos.ctlVal)  ※ Write 用
    ├── Bay2/
    │   └── XCBR2_Position      (OPC → XCBR2.Pos.stVal)
    ├── Disconnector/
    │   └── XSWI1_Position      (OPC → XSWI1.Pos.stVal)
    └── Measurement/
        ├── Current_PhaseA      (OPC → MMXU1.A.phsA.cVal.mag.f)
        ├── Current_PhaseB      (OPC → MMXU1.A.phsB.cVal.mag.f)
        ├── Current_PhaseC      (OPC → MMXU1.A.phsC.cVal.mag.f)
        ├── Voltage_PhaseA      (OPC → MMXU1.PhV.phsA.cVal.mag.f)
        └── Frequency           (OPC → MMXU1.Hz.mag.f)
```

---

## 7. Tag Historian（ヒストリアン）

Ignition の Tag Historian は、タグの値を自動的にデータベースに記録する機能。外部の DB サーバをバックエンドに使用する。

### 7.1 データベース接続の作成

```
[Config] → [Databases] → [Connections]
→ [Create new Database Connection...]

┌───────────────────────────────────────────────────┐
│  Database Connection                               │
│                                                     │
│  Name:          SubstationDB                        │
│                                                     │
│  Database Type: Microsoft SQL Server                │
│  (他の選択肢: MySQL, PostgreSQL, MariaDB)          │
│                                                     │
│  Connect URL:                                       │
│    jdbc:sqlserver://localhost:1433;                  │
│    databaseName=Substation_Historian;               │
│    integratedSecurity=false                         │
│                                                     │
│  Username:   datahub_user                           │
│  Password:   DH_Log2026!                            │
│                                                     │
│  [Validate]  → "Valid" と表示されること             │
│  [Save Changes]                                     │
└───────────────────────────────────────────────────┘
```

> SQL Server Express の DB・ユーザは Layer2_DataHub構築ガイド.md §7.1 のスキーマで作成済みのものを使用。ただし Ignition Tag Historian は独自のテーブルスキーマを自動生成するため、ProcessData テーブルは不要（Ignition が `sqlth_*` テーブルを自動作成する）。

### 7.2 Tag Historian の有効化

```
[Config] → [Tags] → [History]
→ [Tag Historian]

  Storage Provider:    SubstationDB
  Partition Mode:      Month
  Data Prune Age:      1 Year  (または 無期限)

  [Save Changes]
```

### 7.3 タグへの履歴記録の設定

各タグに対して個別に履歴記録を有効化する:

```
Designer:
  Tag Browser → [Substation/Bay1/XCBR1_Position]
  → 右クリック → [Edit Tag]

  ┌─ Tag Editor ─────────────────────────────┐
  │  History Tab:                              │
  │    ☑ History Enabled                      │
  │    Storage Provider: SubstationDB         │
  │    Historical Deadband:                    │
  │      Mode: Off (全変化を記録)             │
  │    Sample Mode:                            │
  │      ● On Change (値変化時)               │
  │      ○ Periodic                           │
  │        Rate: 1000 ms                      │
  │    Max Time between Samples: 60 s         │
  └────────────────────────────────────────────┘
```

**推奨:**

| タグ | サンプルモード | 理由 |
|---|---|---|
| 遮断器/断路器位置 | **On Change** | 状態変化イベントの正確な記録 |
| 動作カウンタ (OpCnt) | **On Change** | 操作回数の変化記録 |
| 計測値（電流/電圧/周波数） | **Periodic 1000ms** | トレンドデータ |

### 履歴データの確認

Designer の **Tag History** パネル、または Gateway Web UI:

```
[Status] → [Tags] → [Tag Historian]

→ 記録済みポイント数、データベースサイズ、
   パーティション状態が表示される
```

SQL クエリで直接確認（Ignition は `sqlth_*` テーブルを自動生成）:

```sql
-- Ignition が自動生成するテーブル
SELECT TOP 10 * FROM sqlth_1_data ORDER BY t_stamp DESC;

-- タグ名の対応
SELECT * FROM sqlth_te;

-- 特定タグの履歴
SELECT d.t_stamp, d.intvalue, d.floatvalue
FROM sqlth_1_data d
JOIN sqlth_te t ON d.tagid = t.id
WHERE t.tagpath LIKE '%XCBR1%'
ORDER BY d.t_stamp DESC;
```

---

## 8. Perspective / Vision による HMI 画面

Ignition には2つの HMI 技術がある:

| | Perspective | Vision |
|---|---|---|
| **クライアント** | Web ブラウザ（HTML5） | Java クライアント（JRE 必要） |
| **アクセス** | URL でどこからでもアクセス | クライアントインストールが必要 |
| **推奨** | **新規プロジェクト向け** | レガシープロジェクト向け |
| **モバイル対応** | ✅ レスポンシブ | ❌ |

本環境では **Perspective**（Web ベース）を推奨する。

### 8.1 Perspective（Web ベース）

#### プロジェクトの作成

```
Designer:
  [File] → [New Project]
  → Name: SubstationHMI
  → [Create]
```

#### 変電所モニター画面の構成

```
Designer:
  左ペイン → [Perspective] → [Views]
  → 右クリック → [New View]
  → Name: SubstationOverview
  → Type: Flex Container
```

画面レイアウトの例:

```
┌─────────────────────────────────────────┐
│  Substation Overview                     │
│                                          │
│  ┌─── Breakers ───┐  ┌─── Meters ───┐  │
│  │                 │  │               │  │
│  │  CB1: [●] CLOSED│  │  Ia: 250.3 A │  │
│  │  CB2: [●] CLOSED│  │  Ib: 248.1 A │  │
│  │  DS1: [●] CLOSED│  │  Ic: 251.7 A │  │
│  │                 │  │  Va: 110.1kV  │  │
│  │  OpCnt: 0       │  │  f: 50.001Hz  │  │
│  │                 │  │               │  │
│  └─────────────────┘  └───────────────┘  │
│                                          │
│  ┌─── Trend Chart ────────────────────┐  │
│  │  [Time Series Chart Component]     │  │
│  │  Pens: Current_PhaseA, Frequency   │  │
│  └────────────────────────────────────┘  │
│                                          │
│  ┌─── Alarm Banner ──────────────────┐  │
│  │  [Alarm Status Table Component]   │  │
│  └────────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

#### コンポーネントとタグのバインディング

```
Designer:
  1. Perspective Components パレットから "Label" をドラッグ
  2. プロパティ → [text] → バインディングアイコンをクリック
  3. [Tag] → [default]/Substation/Bay1/XCBR1_Position を選択
  4. Transform を追加:
     if value == 2: return "CLOSED ●"
     if value == 1: return "OPEN ○"
     return "-- ◎"

  トレンドチャート:
  1. "Time Series Chart" コンポーネントをドラッグ
  2. [Pens] プロパティ:
     → Add Pen → Tag: [default]/Substation/Measurement/Current_PhaseA
     → Add Pen → Tag: [default]/Substation/Measurement/Frequency
  3. Historical Range: Last 1 Hour
```

#### ブラウザからのアクセス

```
http://192.168.56.30:8088/data/perspective/client/SubstationHMI
```

ホスト OS やネットワーク内の任意のブラウザからアクセス可能。

### 8.2 Vision（Java クライアント）

旧来の Java Swing ベースの HMI。OPC DA 環境との親和性が高い。

```
1. Gateway Web UI → [Home] → [Launch Vision Client]
   → Vision Client Launcher がダウンロードされる

2. Launcher を起動 → プロジェクト「SubstationHMI」を選択
   → [Launch]

3. Java クライアントが起動し、HMI 画面が表示される
```

> **Vision は Perspective とは別のモジュール。** Designer でも Vision 用のウィンドウデザイナを使う。新規構築であれば Perspective を推奨。

---

## 9. アラーム設定

### タグレベルのアラーム定義

```
Designer:
  Tag Browser → XCBR1_Position
  → 右クリック → [Edit Tag]

  ┌─ Alarms Tab ─────────────────────────────┐
  │  [Add New Alarm...]                        │
  │                                            │
  │  Name:       BreakerOpen                   │
  │  Priority:   Critical                      │
  │                                            │
  │  Mode:       ● Equal                       │
  │  Setpoint:   1   (1 = Open)               │
  │                                            │
  │  ☑ Enabled                                │
  │                                            │
  │  Display Path: Substation/Bay1/XCBR1       │
  │  Notes:       "Breaker opened -            │
  │               possible unauthorized access"│
  └────────────────────────────────────────────┘
```

同様に XCBR2_Position, XSWI1_Position にもアラームを設定。

### アラーム通知パイプライン

```
[Config] → [Alarming] → [Notification]
→ [Alarm Notification Pipeline]
→ [Create New Pipeline...]

  Pipeline Name: SubstationAlerts

  ブロック構成:
  [Alarm Active]
    → [Notification Block: Email / Console / Custom]
```

### Perspective でのアラーム表示

```
Designer:
  Perspective View にコンポーネントを追加:
  - "Alarm Status Table" → アクティブアラームの一覧
  - "Alarm Journal Table" → アラーム履歴

  タグバインディングで自動的にアラーム状態が表示される
```

---

## 10. DCOM 事前設定（VM3 側）

Ignition の OPC-COM モジュールは内部で DCOM 接続を使用する。
VM3 側の DCOM 設定は Layer2_DataHub構築ガイド.md §3 と同一。

追加の注意点:

- Ignition は **Windows サービス（Local System アカウント）** として動作するため、DCOM の認証にはサービスアカウントの権限が使用される
- `Local System` が VM2 の OPC DA サーバにアクセスできるよう、VM2 側の DCOM 設定で `SYSTEM` アカウント（または `Everyone`）にリモートアクセスを許可すること
- Ignition サービスを特定のユーザアカウント（例: `OpcOperator`）で実行するよう変更することも可能（`services.msc` → Ignition Gateway → ログオン → このアカウント）

---

## 11. 動作確認

| # | 確認項目 | 手順 | 期待結果 |
|---|---|---|---|
| 1 | OPC-COM 接続 | Gateway → Status → OPC-COM | Connected |
| 2 | Quick Client | Gateway → Config → OPC → Quick Client | タグツリーが表示、値が更新 |
| 3 | タグプロバイダ | Designer → Tag Browser | OPC タグの値がリアルタイム更新 |
| 4 | Tag Historian | Gateway → Status → Tag Historian | 記録ポイント数 > 0 |
| 5 | DB 確認 | SSMS → `SELECT * FROM sqlth_1_data` | レコードが蓄積 |
| 6 | Perspective HMI | ブラウザで Perspective URL にアクセス | 変電所状態が表示 |
| 7 | アラーム | 遮断器を Open → アラームテーブルに表示 | Critical アラーム発報 |

---

## 12. トラブルシューティング

### OPC-COM 接続エラー

| 症状 | 原因 | 対策 |
|---|---|---|
| Status: Faulted | DCOM 設定不足 | VM2, VM3 双方の dcomcnfg を確認 |
| "Unable to connect to OPC server" | ProgID / ホスト名が間違い | Quick Client で Browse して正しい ProgID を確認 |
| "Access denied (0x80070005)" | DCOM 権限不足 | VM2 の COM セキュリティに `Everyone` と `SYSTEM` を追加 |
| "RPC server unavailable" | TCP 135 ブロック | FW 確認 |
| OPC-COM モジュールが表示されない | モジュール未インストール | §3 の手順で .modl をインストール |

### Tag Historian の問題

| 症状 | 原因 | 対策 |
|---|---|---|
| データが記録されない | History Enabled がOFF | Designer で各タグの History タブを確認 |
| DB 接続エラー | JDBC URL / 認証情報の間違い | Gateway → Config → Databases → Validate |
| sqlth テーブルが存在しない | Ignition が自動生成に失敗 | DB ユーザに DDL 権限（`db_ddladmin`）を付与 |

### Perspective の問題

| 症状 | 原因 | 対策 |
|---|---|---|
| ブラウザでページが開かない | Perspective モジュール未起動 | Gateway → Config → Modules で確認 |
| ページは開くが値が表示されない | タグバインディングの設定ミス | Designer でバインディングの Target を確認 |
| 外部からアクセス不可 | FW がポート 8088 をブロック | `netsh advfirewall firewall add rule name="Ignition" dir=in action=allow protocol=TCP localport=8088` |

---

## 13. DataHub / MatrikonOPC との比較

| 観点 | DataHub | MatrikonOPC + Python | **Ignition** |
|---|---|---|---|
| **ライセンス** | 評価版30日 | 完全無料 | **評価版2時間（無期限リセット）** |
| **OPC DA** | ✅ 内蔵 | ✅ Explorer + OpenOPC | **✅ OPC-COM Module** |
| **OPC UA** | ✅ 内蔵 | ❌ | **✅ 標準搭載** |
| **HMI** | WebView（簡易） | Flask 自作 | **✅ Perspective（高機能Web HMI）** |
| **Historian** | DB Logger | Python スクリプト | **✅ Tag Historian（標準搭載）** |
| **アラーム** | 簡易 | Python 自作 | **✅ Alarm Pipeline（高機能）** |
| **トレンドチャート** | ❌ | ❌ | **✅ Time Series Chart** |
| **スクリプト** | Gamma Script | Python | **Jython (Python on JVM)** |
| **産業界での採用** | 中（DMZ 用途） | - | **高（北米 SCADA の主流）** |
| **構築工数** | 低 | 高 | **低（GUI で完結）** |
| **学習コスト** | 低 | 中 | **中〜高（機能が多い）** |
| **Industroyer 研究への適性** | ○ | ◎（透明性が高い） | **○（実環境に近い）** |

### 選定の指針

| 目的 | 推奨 |
|---|---|
| 攻撃スクリプトの開発・テストが主目的 | **MatrikonOPC + Python**（透明性が高く、攻撃コードと同じ言語で統一） |
| 実際の SCADA 運用環境に近い構成が必要 | **Ignition**（産業界での実採用が多く、HMI/Historian/Alarm が統合） |
| 短期間で動作する環境を構築したい | **DataHub**（設定項目が少なく最も早く立ち上がる） |
| ライセンスの制約を完全に排除したい | **MatrikonOPC + Python**（すべて無料） |
