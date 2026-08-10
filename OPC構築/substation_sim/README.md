# 変電所 IED シミュレータ (v6 — 4Bay 同居版)

## 概要

Industroyer 攻撃対象の変電所を模擬する IEC 61850 IED シミュレータ。
**1プロセス**で4フィーダー分の遮断器・断路器・計測を同時に模擬する。

### データモデル構成

```
IED1 / CBIED (論理デバイス)
├── LLN0
├── XCBR1  — 1号線 遮断器 (52R1)
├── XSWI1  — 1号線 断路器 (89R1)
├── MMXU1  — 1号線 計測
├── XCBR2  — 2号線 遮断器 (52R2)
├── XSWI2  — 2号線 断路器 (89R2)
├── MMXU2  — 2号線 計測
├── XCBR3  — 3号線 遮断器 (52R3)
├── XSWI3  — 3号線 断路器 (89R3)
├── MMXU3  — 3号線 計測
├── XCBR4  — 4号線 遮断器 (52R4)
├── XSWI4  — 4号線 断路器 (89R4)
└── MMXU4  — 4号線 計測
```

### 制御タグ（CF PosCmd）

| 号線 | 遮断器制御 | 断路器制御 |
|-----|----------|----------|
| 1号 | `XCBR1.CF.Pos.PosCmd` | `XSWI1.CF.Pos.PosCmd` |
| 2号 | `XCBR2.CF.Pos.PosCmd` | `XSWI2.CF.Pos.PosCmd` |
| 3号 | `XCBR3.CF.Pos.PosCmd` | `XSWI3.CF.Pos.PosCmd` |
| 4号 | `XCBR4.CF.Pos.PosCmd` | `XSWI4.CF.Pos.PosCmd` |

> PosCmd: `1` = Open（開放）, `2` = Close（投入）

> **電流シミュレーション**: 各フィーダーごとに遮断器 AND 断路器が共に Closed の場合のみ電流が流れる

## 前提条件

- libIEC61850 がビルド済み
- Java Runtime（genmodel.jar 用）
- CMake 3.0+, C コンパイラ

## 手順

### Step 1: genmodel

```bash
cd substation_sim/
java -jar ../libiec61850/tools/model_generator/genmodel.jar substation.icd
```

### Step 2: ビルド

```bash
# Linux
mkdir -p build && cd build
rm -rf *
cmake .. -DLIBIEC61850_HOME=../../libiec61850
make

# Windows (MinGW)
mkdir build && cd build
cmake .. -DLIBIEC61850_HOME=..\..\libiec61850 -G "MinGW Makefiles"
mingw32-make
```

### Step 3: 起動

```bash
# Linux（port 102 は root 権限が必要）
sudo ./substation_server

# 別ポートで起動
./substation_server 10102
```

起動ログ:
```
============================================================
  Substation IED Simulator v6 (4-Bay Single Process)
============================================================
  Breakers       : XCBR1〜XCBR4 (52R×4)
  Disconnectors  : XSWI1〜XSWI4 (89R×4)
  Measurement    : MMXU1〜MMXU4 (3-phase I/V/Hz×4)
  MMS Port       : 102
============================================================
```

### Step 4: KEPserver 設定

**1チャンネル・1デバイスで全タグにアクセス可能。**

```
KEPserver
└── IED1_Ch1  (チャンネル: IP=IEDのIP, Port=102)
    └── IED1  (デバイス)
        ├── XCBR1.ST.Pos.stVal    ← 1号線 遮断器
        ├── XCBR2.ST.Pos.stVal    ← 2号線 遮断器
        ├── XCBR3.ST.Pos.stVal    ← 3号線 遮断器
        ├── XCBR4.ST.Pos.stVal    ← 4号線 遮断器
        ├── XSWI1〜4              ← 断路器
        └── MMXU1〜4              ← 計測
```

### Step 5: SCADA 画面

`substation_control.html` を Cogent DataHub の WebServer フォルダにコピー:
```
C:\Program Files\Cogent\Cogent DataHub\Plugin\WebServer\html\substation_control.html
```

ブラウザでアクセスし、設定バーの Prefix を実際のドメイン名に合わせてください。

> **タグ名セパレータ**: Windows 7 → `.` (デフォルト), Windows 11 → `$` (設定バーの SEP で切替)

## トラブルシューティング

- **genmodel エラー**: `java -version` で Java 8+ を確認
- **SCADA に値が出ない**: F12 コンソール→「ポイント確認」→ Cogent Data Browser と名前を照合
- **制御が効かない**: 書き込み先が `CF.Pos.PosCmd` であることを確認（`ST.Pos.stVal` は読み取り専用）
