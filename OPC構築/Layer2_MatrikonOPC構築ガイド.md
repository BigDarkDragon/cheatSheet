# Layer 2 構築ガイド（代替案A）: MatrikonOPC による SCADA 構成

**VM3（192.168.56.30）上で MatrikonOPC ツールセットを OPC DA クライアント兼 SCADA として構成する**

---

## 目次

- [1. MatrikonOPC をSCADA として使う場合の構成](#1-matrikonopc-をscada-として使う場合の構成)
- [2. 使用するソフトウェア](#2-使用するソフトウェア)
- [3. DCOM 事前設定（VM3 側）](#3-dcom-事前設定vm3-側)
- [4. MatrikonOPC Explorer によるリモート接続](#4-matrikonopc-explorer-によるリモート接続)
- [5. タグの監視とWrite 操作](#5-タグの監視とwrite-操作)
- [6. Python + OpenOPC による自動化](#6-python--openopc-による自動化)
  - [6.1 リアルタイム監視スクリプト](#61-リアルタイム監視スクリプト)
  - [6.2 DB ロギングスクリプト](#62-db-ロギングスクリプト)
  - [6.3 簡易 Web ダッシュボード](#63-簡易-web-ダッシュボード)
- [7. SQL Server Express によるヒストリアン構築](#7-sql-server-express-によるヒストリアン構築)
- [8. 動作確認](#8-動作確認)
- [9. トラブルシューティング](#9-トラブルシューティング)
- [10. DataHub / Ignition との比較](#10-datahub--ignition-との比較)

---

## 1. MatrikonOPC をSCADA として使う場合の構成

MatrikonOPC はOPC DA クライアントとして軽量かつ無料だが、DataHub や Ignition のような統合 SCADA 機能（DB ロギング、Web HMI、スクリプト）は持たない。そのため、不足部分を Python スクリプトで補完する構成となる。

```
VM3 (SCADA) の内部構成:

┌────────────────────────────────────────────────────┐
│  VM3: 192.168.56.30                                 │
│                                                      │
│  ┌─────────────────────┐  ┌─────────────────────┐  │
│  │ MatrikonOPC Explorer│  │ Python + OpenOPC     │  │
│  │ (GUI: 監視・操作)    │  │ (自動化・ロギング)    │  │
│  └─────────┬───────────┘  └──────────┬──────────┘  │
│            │ OPC DA (DCOM)           │ OPC DA       │
│            └──────────┬──────────────┘              │
│                       │                              │
│                       ▼                              │
│           ┌──── DCOM 経由 ────┐                     │
│           │  VM2: 192.168.56.20│                     │
│           │  OPC DA Server     │                     │
│           └────────────────────┘                     │
│                                                      │
│  ┌─────────────────────┐  ┌─────────────────────┐  │
│  │ SQL Server Express  │  │ Flask Web Server    │  │
│  │ (ヒストリアン)       │  │ (簡易 HMI)          │  │
│  └─────────────────────┘  └─────────────────────┘  │
└────────────────────────────────────────────────────┘
```

**この構成の利点:**
- すべて無料（ライセンス期限なし）
- 構成要素ごとの動作が透明（ブラックボックスがない）
- Python で攻撃シナリオの自動化スクリプトも書ける

**欠点:**
- 統合 SCADA としての完成度は DataHub / Ignition に劣る
- 自前実装が多い

---

## 2. 使用するソフトウェア

| ソフトウェア | 用途 | ライセンス | 入手先 |
|---|---|---|---|
| **OPC Core Components** | OPC DA ランタイム（OPCEnum 等） | 無料 | https://opcfoundation.org/ |
| **MatrikonOPC Explorer** | OPC DA クライアント GUI | 無料 | https://www.matrikonopc.com/ |
| **Python 3.x** | 自動化・ロギング | 無料 | https://www.python.org/ |
| **OpenOPC** | Python OPC DA ライブラリ | GPLv2 | https://openopc2.readthedocs.io/ |
| **pyodbc** | Python ODBC ライブラリ | MIT | pip install pyodbc |
| **Flask** | 簡易 Web サーバ（HMI 用） | BSD | pip install flask |
| **SQL Server Express** | ヒストリアン DB | 無料 | https://www.microsoft.com/ |

### クローズ環境向け Python パッケージの事前ダウンロード

```bash
# 開発VM（インターネットあり）で実行
pip download openopc-da pyodbc flask -d ./python_packages/
```

---

## 3. DCOM 事前設定（VM3 側）

Layer2_DataHub構築ガイド.md の「3. DCOM 事前設定」と同一の設定を行う。

要約:

```
dcomcnfg:
  ├── 既定のプロパティ
  │   ├── DCOM 有効化: ON
  │   ├── 認証レベル: 接続 (Connect)  ※ シナリオ1なら「なし」
  │   └── 偽装レベル: 識別 (Identify)
  ├── COM セキュリティ
  │   ├── アクセス許可 → Everyone: リモート ☑
  │   └── 起動許可 → Everyone: すべて ☑
  └── ファイアウォール
      ├── TCP 135 開放
      └── TCP 5000-5020 開放

hosts ファイル:
  192.168.56.20  VM2-RTUGW
```

---

## 4. MatrikonOPC Explorer によるリモート接続

### 起動と接続

```
1. MatrikonOPC Explorer を起動

2. 左ペインで [OPC Servers] を右クリック → [Connect...]

3. [Remote] タブを選択:
   ┌────────────────────────────────────┐
   │  Computer: VM2-RTUGW               │
   │  (または)  192.168.56.20           │
   │                                     │
   │  → [Refresh] をクリック            │
   │  → サーバ一覧が表示される          │
   │                                     │
   │  Available Servers:                 │
   │    ☐ Kepware.KEPServerEX.V6        │
   │    ☐ MatrikonOPC.Simulation.1      │
   │                                     │
   │  → 対象サーバを選択                │
   │  → [Connect]                       │
   └────────────────────────────────────┘
```

### 接続成功後の画面構成

```
MatrikonOPC Explorer
├── OPC Servers
│   └── \\VM2-RTUGW\Kepware.KEPServerEX.V6  [Connected]
│       └── IEC61850_Ch1
│           └── IED1
│               └── CBIED
│                   ├── XCBR1
│                   │   └── Pos
│                   │       └── stVal
│                   ├── XCBR2
│                   ├── XSWI1
│                   └── MMXU1
│                       ├── A
│                       ├── PhV
│                       └── Hz
│
└── [Groups] ← タグをドラッグ＆ドロップして監視グループを作成
```

---

## 5. タグの監視と Write 操作

### 監視グループの作成

```
1. 右ペインの空白領域を右クリック → [Add Group...]
   Group Name: SubstationMonitor
   Update Rate: 1000 ms
   Active: ☑

2. 左ペインのタグツリーから以下をグループにドラッグ＆ドロップ:
   - XCBR1.Pos.stVal
   - XCBR2.Pos.stVal
   - XSWI1.Pos.stVal
   - XCBR1.OpCnt.stVal
   - MMXU1.A.phsA.cVal.mag.f
   - MMXU1.A.phsB.cVal.mag.f
   - MMXU1.A.phsC.cVal.mag.f
   - MMXU1.PhV.phsA.cVal.mag.f
   - MMXU1.Hz.mag.f

3. 右ペインに以下のカラムが表示される:
   | Item ID | Value | Quality | Timestamp |
```

### Write 操作（遮断器の制御）

これは Industroyer OPC DA モジュールの Phase 4 を手動で再現する操作に相当する。

```
1. 右ペインで XCBR1.Pos.stVal を右クリック
2. [Write Value...] を選択
3. Value: 1 (= Open)
4. [Write]

→ 遮断器が Open に変化する
→ 電流値が 0A に変化することを確認
```

> **注意:** MatrikonOPC Explorer は OPC DA の `IOPCSyncIO::Write` をそのまま呼び出す。DCOM 認証設定がシナリオ1（匿名）の場合、任意のクライアントからこの Write が無認証で実行できることが、Industroyer 攻撃が成立した技術的背景である。

---

## 6. Python + OpenOPC による自動化

MatrikonOPC Explorer は GUI ツールのため、自動化やDB連携には Python スクリプトを使用する。

### 6.1 リアルタイム監視スクリプト

```python
"""
opc_monitor.py
OPC DA サーバに接続し、変電所のタグをリアルタイム監視する
"""
import OpenOPC
import time
import sys

OPC_HOST   = '192.168.56.20'
OPC_SERVER = 'Kepware.KEPServerEX.V6'
# OPC_SERVER = 'MatrikonOPC.Simulation.1'

# 監視対象タグ（環境に合わせて修正）
TAGS_BREAKER = [
    'IEC61850_Ch1.IED1.CBIED.XCBR1.Pos.stVal',
    'IEC61850_Ch1.IED1.CBIED.XCBR2.Pos.stVal',
    'IEC61850_Ch1.IED1.CBIED.XSWI1.Pos.stVal',
    'IEC61850_Ch1.IED1.CBIED.XCBR1.OpCnt.stVal',
]
TAGS_MEASUREMENT = [
    'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsA.cVal.mag.f',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsB.cVal.mag.f',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsC.cVal.mag.f',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsA.cVal.mag.f',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.Hz.mag.f',
]

# MatrikonOPC Simulation の場合のタグ名:
# TAGS_BREAKER = [
#     'Substation.Bay1.XCBR1.Pos_stVal',
#     'Substation.Bay1.XCBR1.Pos_stVal',
#     ...
# ]

DBPOS = {0: 'INTERMEDIATE', 1: 'OPEN', 2: 'CLOSED', 3: 'BAD'}

def main():
    opc = OpenOPC.client()
    opc.connect(OPC_SERVER, OPC_HOST)
    print(f'[+] Connected to {OPC_SERVER} @ {OPC_HOST}')

    all_tags = TAGS_BREAKER + TAGS_MEASUREMENT
    prev_breaker = {}

    try:
        while True:
            values = opc.read(all_tags)
            ts = time.strftime('%H:%M:%S')

            print(f'\n[{ts}] === Substation Status ===')

            # 遮断器
            for tag, val, quality, timestamp in values[:len(TAGS_BREAKER)]:
                short = tag.split('.')[-1]
                if 'Pos' in tag and 'stVal' in tag:
                    state = DBPOS.get(int(val), '?')
                    marker = ''
                    if tag in prev_breaker and prev_breaker[tag] != val:
                        marker = '  *** CHANGED ***'
                    prev_breaker[tag] = val
                    print(f'  {short:20s} = {val} ({state}) [{quality}]{marker}')
                else:
                    print(f'  {short:20s} = {val} [{quality}]')

            # 計測値
            for tag, val, quality, timestamp in values[len(TAGS_BREAKER):]:
                short = tag.split('.')[-2] + '.' + tag.split('.')[-1]
                if val is not None:
                    print(f'  {short:20s} = {float(val):>10.2f} [{quality}]')

            time.sleep(1)

    except KeyboardInterrupt:
        print('\n[*] Stopped.')
    finally:
        opc.close()

if __name__ == '__main__':
    main()
```

### 6.2 DB ロギングスクリプト

```python
"""
opc_logger.py
OPC DA の値を SQL Server に周期的に記録するヒストリアンロガー
"""
import OpenOPC
import pyodbc
import time
import threading
from datetime import datetime

# === 設定 ===
OPC_HOST      = '192.168.56.20'
OPC_SERVER    = 'Kepware.KEPServerEX.V6'
DB_DSN        = 'SubstationHistorian'
DB_USER       = 'datahub_user'
DB_PASSWORD   = 'DH_Log2026!'
LOG_INTERVAL  = 1.0  # 秒

TAGS = [
    'IEC61850_Ch1.IED1.CBIED.XCBR1.Pos.stVal',
    'IEC61850_Ch1.IED1.CBIED.XCBR2.Pos.stVal',
    'IEC61850_Ch1.IED1.CBIED.XSWI1.Pos.stVal',
    'IEC61850_Ch1.IED1.CBIED.XCBR1.OpCnt.stVal',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsA.cVal.mag.f',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsB.cVal.mag.f',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsC.cVal.mag.f',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsA.cVal.mag.f',
    'IEC61850_Ch1.IED1.CBIED.MMXU1.Hz.mag.f',
]


def create_db_connection():
    conn_str = f'DSN={DB_DSN};UID={DB_USER};PWD={DB_PASSWORD}'
    return pyodbc.connect(conn_str)


def log_to_db(db_conn, tag, value, quality, timestamp):
    cursor = db_conn.cursor()
    try:
        float_val = float(value) if value is not None else None
        cursor.execute(
            "INSERT INTO ProcessData (PointName, Value, Quality, Timestamp) "
            "VALUES (?, ?, ?, ?)",
            tag, float_val, str(quality), timestamp or datetime.now()
        )
    except (ValueError, TypeError):
        cursor.execute(
            "INSERT INTO ProcessData (PointName, ValueStr, Quality, Timestamp) "
            "VALUES (?, ?, ?, ?)",
            tag, str(value), str(quality), timestamp or datetime.now()
        )


def main():
    # OPC DA 接続
    opc = OpenOPC.client()
    opc.connect(OPC_SERVER, OPC_HOST)
    print(f'[+] OPC: {OPC_SERVER} @ {OPC_HOST}')

    # DB 接続
    db = create_db_connection()
    print(f'[+] DB: {DB_DSN}')

    record_count = 0

    try:
        while True:
            values = opc.read(TAGS)
            for tag, val, quality, ts in values:
                log_to_db(db, tag, val, quality, ts)
                record_count += 1

            db.commit()
            t = time.strftime('%H:%M:%S')
            print(f'[{t}] Logged {len(values)} points (total: {record_count})')
            time.sleep(LOG_INTERVAL)

    except KeyboardInterrupt:
        print(f'\n[*] Stopped. Total records: {record_count}')
    finally:
        db.commit()
        db.close()
        opc.close()


if __name__ == '__main__':
    main()
```

### 6.3 簡易 Web ダッシュボード

MatrikonOPC Explorer には Web 機能がないため、Flask で簡易ダッシュボードを自作する。

```python
"""
opc_dashboard.py
Flask ベースの簡易変電所ダッシュボード
http://192.168.56.30:8080 でアクセス
"""
import OpenOPC
import time
import json
import threading
from flask import Flask, render_template_string, jsonify

# === 設定 ===
OPC_HOST   = '192.168.56.20'
OPC_SERVER = 'Kepware.KEPServerEX.V6'

TAGS = {
    'xcbr1_pos':  'IEC61850_Ch1.IED1.CBIED.XCBR1.Pos.stVal',
    'xcbr2_pos':  'IEC61850_Ch1.IED1.CBIED.XCBR2.Pos.stVal',
    'xswi1_pos':  'IEC61850_Ch1.IED1.CBIED.XSWI1.Pos.stVal',
    'xcbr1_cnt':  'IEC61850_Ch1.IED1.CBIED.XCBR1.OpCnt.stVal',
    'current_a':  'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsA.cVal.mag.f',
    'current_b':  'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsB.cVal.mag.f',
    'current_c':  'IEC61850_Ch1.IED1.CBIED.MMXU1.A.phsC.cVal.mag.f',
    'voltage_a':  'IEC61850_Ch1.IED1.CBIED.MMXU1.PhV.phsA.cVal.mag.f',
    'frequency':  'IEC61850_Ch1.IED1.CBIED.MMXU1.Hz.mag.f',
}

# 最新データの格納
latest_data = {}
data_lock = threading.Lock()

app = Flask(__name__)

# === OPC DA ポーリングスレッド ===
def opc_poller():
    global latest_data
    opc = OpenOPC.client()
    opc.connect(OPC_SERVER, OPC_HOST)
    tag_list = list(TAGS.values())

    while True:
        try:
            values = opc.read(tag_list)
            with data_lock:
                for tag, val, quality, ts in values:
                    key = [k for k, v in TAGS.items() if v == tag][0]
                    latest_data[key] = {
                        'value': val,
                        'quality': str(quality),
                        'timestamp': str(ts),
                    }
        except Exception as e:
            print(f'OPC read error: {e}')
        time.sleep(1)

# === API エンドポイント ===
@app.route('/api/data')
def api_data():
    with data_lock:
        return jsonify(latest_data)

# === ダッシュボード HTML ===
DASHBOARD_HTML = """
<!DOCTYPE html>
<html>
<head>
  <title>Substation Dashboard</title>
  <meta charset="UTF-8">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body { font-family: 'Segoe UI', Consolas, monospace; background: #0a0a1a; color: #c0c0d0; padding: 20px; }
    h1 { color: #00ccff; margin-bottom: 20px; font-size: 22px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; max-width: 900px; }
    .panel { background: #12122a; border: 1px solid #1a1a40; border-radius: 6px; padding: 15px; }
    .panel h2 { color: #6688aa; font-size: 14px; margin-bottom: 10px; text-transform: uppercase; letter-spacing: 1px; }
    .row { display: flex; justify-content: space-between; padding: 6px 0; border-bottom: 1px solid #1a1a30; }
    .label { color: #667788; }
    .val { font-weight: bold; font-size: 16px; }
    .closed { color: #00ff88; }
    .open { color: #ff3344; animation: blink 1s infinite; }
    @keyframes blink { 50% { opacity: 0.5; } }
    .alert { background: #2a0a0a; border-color: #ff3344; display: none; grid-column: 1 / -1; }
    .alert.active { display: block; }
    .alert h2 { color: #ff3344; }
    .updated { color: #445566; font-size: 11px; text-align: right; margin-top: 15px; }
  </style>
</head>
<body>
  <h1>⚡ Substation Monitor</h1>
  <div class="grid">
    <div class="panel">
      <h2>Circuit Breakers</h2>
      <div class="row"><span class="label">XCBR1 (Bay 1)</span><span class="val" id="xcbr1">--</span></div>
      <div class="row"><span class="label">XCBR2 (Bay 2)</span><span class="val" id="xcbr2">--</span></div>
      <div class="row"><span class="label">XSWI1 (Disc.)</span><span class="val" id="xswi1">--</span></div>
      <div class="row"><span class="label">XCBR1 Op Count</span><span class="val" id="opcnt">--</span></div>
    </div>
    <div class="panel">
      <h2>Measurements</h2>
      <div class="row"><span class="label">Phase A Current</span><span class="val" id="ia">-- A</span></div>
      <div class="row"><span class="label">Phase B Current</span><span class="val" id="ib">-- A</span></div>
      <div class="row"><span class="label">Phase C Current</span><span class="val" id="ic">-- A</span></div>
      <div class="row"><span class="label">Phase A Voltage</span><span class="val" id="va">-- kV</span></div>
      <div class="row"><span class="label">Frequency</span><span class="val" id="hz">-- Hz</span></div>
    </div>
    <div class="panel alert" id="alertPanel">
      <h2>⚠ BREAKER ALERT</h2>
      <p id="alertMsg"></p>
    </div>
  </div>
  <p class="updated" id="ts">--</p>

  <script>
    function fmt(v) { return v != null ? parseFloat(v).toFixed(1) : '--'; }
    function fmtHz(v) { return v != null ? parseFloat(v).toFixed(3) : '--'; }
    function brk(v) {
      if (v == 2) return '<span class="closed">CLOSED ●</span>';
      if (v == 1) return '<span class="open">OPEN ○</span>';
      return '-- ◎';
    }
    function poll() {
      fetch('/api/data').then(r => r.json()).then(d => {
        document.getElementById('xcbr1').innerHTML = brk(d.xcbr1_pos?.value);
        document.getElementById('xcbr2').innerHTML = brk(d.xcbr2_pos?.value);
        document.getElementById('xswi1').innerHTML = brk(d.xswi1_pos?.value);
        document.getElementById('opcnt').textContent = d.xcbr1_cnt?.value ?? '--';
        document.getElementById('ia').textContent = fmt(d.current_a?.value) + ' A';
        document.getElementById('ib').textContent = fmt(d.current_b?.value) + ' A';
        document.getElementById('ic').textContent = fmt(d.current_c?.value) + ' A';
        document.getElementById('va').textContent = fmt(d.voltage_a?.value) + ' kV';
        document.getElementById('hz').textContent = fmtHz(d.frequency?.value) + ' Hz';
        document.getElementById('ts').textContent = 'Updated: ' + new Date().toLocaleTimeString();

        var ap = document.getElementById('alertPanel');
        if (d.xcbr1_pos?.value == 1 || d.xcbr2_pos?.value == 1) {
          ap.classList.add('active');
          document.getElementById('alertMsg').textContent =
            'Breaker OPEN detected at ' + new Date().toLocaleTimeString();
        } else {
          ap.classList.remove('active');
        }
      }).catch(e => console.error(e));
    }
    setInterval(poll, 1000);
    poll();
  </script>
</body>
</html>
"""

@app.route('/')
def dashboard():
    return render_template_string(DASHBOARD_HTML)

if __name__ == '__main__':
    t = threading.Thread(target=opc_poller, daemon=True)
    t.start()
    print('[+] Dashboard: http://192.168.56.30:8080')
    app.run(host='0.0.0.0', port=8080, debug=False)
```

実行方法:
```bash
python opc_dashboard.py
# ブラウザで http://192.168.56.30:8080 にアクセス
```

---

## 7. SQL Server Express によるヒストリアン構築

DB スキーマは Layer2_DataHub構築ガイド.md の §7.1 と同一。

```sql
CREATE DATABASE Substation_Historian;
GO
USE Substation_Historian;
GO

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

CREATE INDEX IX_ProcessData_PointTime
    ON ProcessData (PointName, Timestamp);
GO

CREATE VIEW vw_BreakerEvents AS
SELECT ID, PointName,
    CASE CAST(Value AS INT) WHEN 1 THEN 'OPEN' WHEN 2 THEN 'CLOSED' ELSE 'OTHER' END AS BreakerState,
    Timestamp, InsertTime
FROM ProcessData
WHERE PointName LIKE '%XCBR%stVal%' OR PointName LIKE '%XCBR%Pos_stVal%';
GO

CREATE LOGIN datahub_user WITH PASSWORD = 'DH_Log2026!';
GO
USE Substation_Historian;
CREATE USER datahub_user FOR LOGIN datahub_user;
ALTER ROLE db_datawriter ADD MEMBER datahub_user;
ALTER ROLE db_datareader ADD MEMBER datahub_user;
GO
```

ODBC DSN の設定も Layer2_DataHub構築ガイド.md §7.2 と同一。
設定完了後、`opc_logger.py` を起動すれば DB ロギングが開始される。

---

## 8. 動作確認

| # | 確認項目 | 手順 | 期待結果 |
|---|---|---|---|
| 1 | MatrikonOPC Explorer 接続 | VM2 にリモート接続 | タグツリーが表示される |
| 2 | 値の更新 | MMXU1 の計測値を監視 | リアルタイムで変動 |
| 3 | Write 操作 | XCBR1.Pos.stVal に 1 を Write | 遮断器が Open に変化 |
| 4 | Python 監視 | `opc_monitor.py` 実行 | コンソールに値が表示 |
| 5 | DB ロギング | `opc_logger.py` 実行後、SSMS で確認 | ProcessData にレコード蓄積 |
| 6 | Web ダッシュボード | `opc_dashboard.py` 実行後、ブラウザアクセス | 変電所状態が表示 |

---

## 9. トラブルシューティング

### OpenOPC の接続エラー

| エラー | 原因 | 対策 |
|---|---|---|
| `pywintypes.com_error` | pywin32 がインストールされていない | `pip install pywin32` |
| `Connect: 'OPC server not found'` | ProgID の指定ミス | `opc.servers()` で正しい ProgID を確認 |
| `Connect: Access is denied` | DCOM 権限不足 | VM2 の dcomcnfg を確認 |
| `Connect: RPC server unavailable` | ネットワーク / ポート問題 | ping 確認、FW の TCP 135 開放 |
| `Read: 'The item is not valid'` | タグパスが間違っている | `opc.list('*', recursive=True)` で正しいパスを確認 |

### Flask ダッシュボードの問題

| 症状 | 原因 | 対策 |
|---|---|---|
| 外部からアクセス不可 | FW がポート 8080 をブロック | `netsh advfirewall firewall add rule name="Dashboard" dir=in action=allow protocol=TCP localport=8080` |
| データが表示されない | OPC ポーラースレッドがエラー | コンソールのエラー出力を確認 |

---

## 10. DataHub / Ignition との比較

| 観点 | MatrikonOPC + Python | DataHub | Ignition |
|---|---|---|---|
| **ライセンス費用** | 完全無料 | 評価版30日 | 評価版2時間（無期限リセット） |
| **OPC DA クライアント** | ✅ Explorer + OpenOPC | ✅ 内蔵 | ✅ 内蔵 |
| **DB ロギング** | Python スクリプト | ✅ 内蔵 GUI | ✅ 内蔵（Tag Historian） |
| **Web HMI** | Flask 自作 | ✅ 内蔵 WebView | ✅ Perspective（高機能） |
| **アラーム** | Python 自作 | ✅ 内蔵 | ✅ 内蔵（Alarm Pipeline） |
| **構築工数** | 高（自作が多い） | 低 | 低 |
| **透明性** | 高（全コード可視） | 中 | 低 |
| **攻撃スクリプトとの親和性** | 高（Python で統一） | 低 | 低 |
