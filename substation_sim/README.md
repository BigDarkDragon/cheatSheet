# 変電所 IED シミュレータ ビルド手順

## 前提条件

- libIEC61850 がビルド済みであること（`server_example_basic` が動作確認済み）
- Java Runtime（genmodel.jar の実行に必要）
- CMake 3.0 以上
- C コンパイラ（GCC / Visual Studio / MinGW）

## ディレクトリ構成

```
<作業ディレクトリ>/
├── libiec61850/              ← ビルド済みの libIEC61850
│   ├── build/
│   ├── src/
│   ├── hal/
│   └── tools/
│       └── model_generator/
│           └── genmodel.jar
│
└── substation_sim/           ← 本ディレクトリ
    ├── substation.icd        ← IEC 61850 データモデル定義
    ├── substation_server.c   ← サーバアプリケーション
    ├── CMakeLists.txt        ← ビルド設定
    ├── static_model.h        ← (生成される)
    └── static_model.c        ← (生成される)
```

## 手順

### Step 1: substation_sim ディレクトリを VM にコピー

本ディレクトリ (`substation_sim/`) を VM1 上の libiec61850 と同じ階層にコピーする。

```
例:
  /home/user/libiec61850/          ← ビルド済み
  /home/user/substation_sim/       ← ここにコピー
```

### Step 2: genmodel.jar でデータモデルの C コードを生成

```bash
# Linux
cd substation_sim/
java -jar ../libiec61850/tools/model_generator/genmodel.jar substation.icd

# Windows
cd substation_sim
java -jar ..\libiec61850\tools\model_generator\genmodel.jar substation.icd
```

成功すると以下のファイルが生成される:
- `static_model.h` — データモデルのシンボル定義（変数名）
- `static_model.c` — データモデルの静的初期化コード

生成されるシンボル例:
```
IEDMODEL_CBIED_XCBR1_Pos          — 遮断器1の Position DO
IEDMODEL_CBIED_XCBR1_Pos_stVal    — 遮断器1の位置状態値 (Dbpos)
IEDMODEL_CBIED_XCBR1_Pos_ctlModel — 遮断器1の制御モデル
IEDMODEL_CBIED_XCBR1_OpCnt_stVal  — 遮断器1の動作回数
IEDMODEL_CBIED_MMXU1_A_phsA_cVal_mag_f — A相電流の大きさ
IEDMODEL_CBIED_MMXU1_Hz_mag_f     — 周波数
```

### Step 3: ビルド

```bash
# Linux
cd substation_sim/
mkdir build && cd build
cmake .. -DLIBIEC61850_HOME=../../libiec61850
make

# Windows (Visual Studio)
cd substation_sim
mkdir build && cd build
cmake .. -DLIBIEC61850_HOME=..\..\libiec61850 -G "Visual Studio 16 2019"
cmake --build . --config Release

# Windows (MinGW)
cd substation_sim
mkdir build && cd build
cmake .. -DLIBIEC61850_HOME=..\..\libiec61850 -G "MinGW Makefiles"
mingw32-make
```

### Step 4: 実行

```bash
# Linux（port 102 は root 権限が必要）
sudo ./substation_server

# 別のポートを使う場合（root 不要）
./substation_server 10102

# Windows
substation_server.exe
# または
substation_server.exe 10102
```

起動成功時の出力:
```
============================================================
  Substation IED Simulator (Industroyer Target Environment)
============================================================
  Logical Device : CBIED
  Breakers       : XCBR1 (Bay1), XCBR2 (Bay2)
  Disconnector   : XSWI1
  Measurement    : MMXU1 (3-phase I/V/Hz)
  MMS Port       : 102
============================================================

[+] MMS server started on port 102
[+] Waiting for connections... (Ctrl+C to stop)
```

### Step 5: 接続確認

別ターミナルから libIEC61850 付属のクライアントサンプルで接続:

```bash
# libiec61850/examples/iec61850_client_example1/ から
./iec61850_client_example1 192.168.56.10

# ポートを変えた場合
./iec61850_client_example1 192.168.56.10 10102
```

または IEDScout で接続し、以下のデータモデルが見えることを確認:
```
IED1
└── CBIED
    ├── LLN0
    │   ├── Mod.stVal = 1 (on)
    │   ├── Beh.stVal = 1 (on)
    │   └── Health.stVal = 1 (Ok)
    ├── XCBR1
    │   ├── Pos.stVal = 2 (on/closed)  ← 遮断器位置
    │   ├── Pos.ctlModel = 1 (direct-with-normal-security)
    │   └── OpCnt.stVal = 0            ← 動作回数
    ├── XCBR2
    │   ├── Pos.stVal = 2 (on/closed)
    │   └── OpCnt.stVal = 0
    ├── XSWI1
    │   └── Pos.stVal = 2 (on/closed)  ← 断路器位置
    └── MMXU1
        ├── A.phsA.cVal.mag.f = ~250.0  ← A相電流
        ├── A.phsB.cVal.mag.f = ~248.0
        ├── A.phsC.cVal.mag.f = ~251.0
        ├── PhV.phsA.cVal.mag.f = ~110.0  ← A相電圧
        ├── PhV.phsB.cVal.mag.f = ~110.0
        ├── PhV.phsC.cVal.mag.f = ~110.0
        └── Hz.mag.f = ~50.0            ← 周波数
```

## ICD ファイルのデータモデル構成

```
substation.icd が定義するデータモデル:

IED1 / CBIED (論理デバイス)
│
├── LLN0 (論理ノードゼロ)
│   ├── Mod   [ENC] — 動作モード (on/blocked/test/off)
│   ├── Beh   [ENS] — 動作状態
│   ├── Health[ENS] — 健全性 (Ok/Warning/Alarm)
│   └── NamPlt[LPL] — 銘板情報
│
├── XCBR1 (遮断器 #1 — Bay 1) ★ Industroyer の主要攻撃対象
│   ├── Pos   [DPC] — 位置 (ST: stVal=Dbpos, CO: ctlVal+Oper)
│   │   ├── stVal  → 現在位置 (1=Open, 2=Closed)
│   │   └── Oper   → 制御コマンド (ctlVal: false=Open, true=Close)
│   ├── Beh   [ENS] — 動作状態
│   ├── Health[ENS] — 健全性
│   ├── OpCnt [INS] — 動作回数カウンタ
│   └── CBOpCap[INS] — 遮断能力
│
├── XCBR2 (遮断器 #2 — Bay 2)
│   └── (XCBR1 と同構成)
│
├── XSWI1 (断路器 #1)
│   ├── Pos   [DPC] — 位置
│   ├── Beh   [ENS]
│   └── Health[ENS]
│
└── MMXU1 (計測ユニット)
    ├── A   [WYE] — 三相電流
    │   ├── phsA.cVal.mag.f [A]
    │   ├── phsB.cVal.mag.f [A]
    │   └── phsC.cVal.mag.f [A]
    ├── PhV [WYE] — 三相電圧
    │   ├── phsA.cVal.mag.f [kV]
    │   ├── phsB.cVal.mag.f [kV]
    │   └── phsC.cVal.mag.f [kV]
    └── Hz  [MV]  — 周波数
        └── mag.f [Hz]
```

## 動作の特徴

- 計測値 (MMXU1) は 100ms 周期で正弦波ベースのシミュレーション値を更新
- 遮断器が Open (stVal=1) の場合、電流値は自動的に 0A になる（物理動作を模擬）
- 制御コマンド（Pos の Oper）を受信すると、コンソールに送信元 IP とコマンド内容をログ出力
- DataSet `dsStatus` と BufferedReportControl `rcbStatus` を定義済み（MMS レポートによる自動通知が可能）

## トラブルシューティング

### genmodel.jar が動かない
- Java がインストールされているか確認: `java -version`
- Java 8 以上が必要

### ビルドエラー: static_model.h が見つからない
- Step 2 の genmodel.jar 実行が完了しているか確認
- 生成ファイルが substation_sim/ 直下にあるか確認

### ビルドエラー: iec61850 ライブラリが見つからない
- `LIBIEC61850_HOME` のパスが正しいか確認
- libiec61850 が事前にビルドされているか確認

### 起動エラー: port 102 にバインドできない
- Linux: `sudo` で実行（102 は特権ポート）
- Windows: 管理者権限で実行
- または別ポートを指定: `./substation_server 10102`

### シンボル名が不一致（コンパイルエラー）
- genmodel.jar の出力する変数名は ICD ファイルの構造に依存する
- `static_model.h` を開き、実際に生成されたシンボル名を確認
- `substation_server.c` 内の `IEDMODEL_CBIED_XCBR1_Pos` 等を生成名に合わせて修正
