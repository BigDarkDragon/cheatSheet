# 変電所 IED シミュレータ (v5 — 4IED 対応版)

## 概要

Industroyer 攻撃対象の変電所を模擬する IEC 61850 IED シミュレータ。
1つの実行体を**4プロセス起動**し、4台の IED（4フィーダー分の遮断器+断路器）を模擬する。

### 構成

| IED | ポート | 対応フィーダー | KEPserver チャンネル名 |
|-----|--------|---------------|----------------------|
| IED1 | 102  | 1号線 | IED1_Ch1 |
| IED2 | 1102 | 2号線 | IED2_Ch1 |
| IED3 | 2102 | 3号線 | IED3_Ch1 |
| IED4 | 3102 | 4号線 | IED4_Ch1 |

### 各 IED のデータモデル

```
IED1 / CBIED (論理デバイス)
├── LLN0   — IED 全体の状態
├── XCBR1  — 遮断器 (52R) ★ Industroyer の攻撃対象
│   ├── Pos.stVal  → 位置 (1=Open, 2=Closed)
│   ├── Pos.PosCmd → 制御コマンド (CF, Read/Write)
│   └── OpCnt.stVal → 動作回数
├── XSWI1  — 断路器 (89R)
│   ├── Pos.stVal  → 位置 (1=Open, 2=Closed)
│   └── Pos.PosCmd → 制御コマンド (CF, Read/Write)
└── MMXU1  — 計測ユニット
    ├── A.phsA.cVal.mag.f → A相電流 [A]
    ├── PhV.phsA.cVal.mag.f → A相電圧 [kV]
    └── Hz.mag.f → 周波数 [Hz]
```

> **電流シミュレーション**: 遮断器 AND 断路器が共に Closed の場合のみ電流が流れる（物理動作を模擬）

## v5 修正履歴

| 変更 | 内容 |
|------|------|
| 4IED 対応 | XCBR2 を削除。1 IED = XCBR1 + XSWI1 + MMXU1 に簡素化 |
| 断路器連動 | XSWI1 Open 時は電流 = 0A（遮断器と断路器の AND 条件） |
| タグ名 | クローズ系は `.` 区切り (例: `XCBR1.ST.Pos.stVal`) |

## 前提条件

- libIEC61850 がビルド済みであること
- Java Runtime（genmodel.jar の実行に必要）
- CMake 3.0 以上
- C コンパイラ（GCC / Visual Studio / MinGW）

## ディレクトリ構成

```
<作業ディレクトリ>/
├── libiec61850/              ← ビルド済みの libIEC61850
│   └── tools/model_generator/genmodel.jar
└── substation_sim/           ← 本ディレクトリ
    ├── substation.icd        ← IEC 61850 データモデル定義 (v5)
    ├── substation_server.c   ← サーバアプリケーション (v5)
    ├── CMakeLists.txt        ← ビルド設定
    ├── substation_control.html ← SCADA 画面 (4IED対応)
    ├── static_model.h        ← (genmodel が生成)
    └── static_model.c        ← (genmodel が生成)
```

## 手順

### Step 1: genmodel.jar でデータモデルの C コードを生成

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

### Step 3: 4台同時起動

```bash
# Linux（port 102 は root 権限が必要）
sudo ./substation_server 102  &   # IED1 (1号線)
./substation_server 1102 &         # IED2 (2号線)
./substation_server 2102 &         # IED3 (3号線)
./substation_server 3102 &         # IED4 (4号線)
```

起動スクリプト例:
```bash
#!/bin/bash
# start_all_ieds.sh
echo "Starting 4 IED instances..."
sudo ./substation_server 102  &
sleep 0.5
./substation_server 1102 &
sleep 0.5
./substation_server 2102 &
sleep 0.5
./substation_server 3102 &
echo "All 4 IEDs started."
echo "  IED1: port 102  (1号線)"
echo "  IED2: port 1102 (2号線)"
echo "  IED3: port 2102 (3号線)"
echo "  IED4: port 3102 (4号線)"
wait
```

### Step 4: KEPserver 設定

KEPserver に 4つの IEC 61850 チャンネルを作成:

| チャンネル名 | 接続先 IP | ポート |
|------------|----------|--------|
| IED1_Ch1 | (IED の IP) | 102 |
| IED2_Ch1 | (IED の IP) | 1102 |
| IED3_Ch1 | (IED の IP) | 2102 |
| IED4_Ch1 | (IED の IP) | 3102 |

### Step 5: SCADA 画面

`substation_control.html` を Cogent DataHub の WebServer フォルダにコピー:
```
C:\Program Files\Cogent\Cogent DataHub\Plugin\WebServer\html\substation_control.html
```

ブラウザでアクセス:
```
http://<Cogent IP>/substation_control.html
```

画面上部の設定バーで各 IED のドメインプレフィックスを確認・調整してください。

> **タグ名セパレータ**: Windows 7 環境では `.` (ドット) がデフォルト。
> Windows 11 環境では `$` (ドル) の場合があります。設定バーの SEP で切り替え可能。

## トラブルシューティング

### genmodel.jar が動かない
- `java -version` で Java 8 以上がインストールされているか確認

### ポート競合
- 4プロセスが異なるポートを使っているか確認
- `ss -tlnp | grep substation` でリスニング状態を確認

### SCADA 画面に値が出ない
1. 接続ステータスが「接続中」か確認
2. F12 コンソールでエラーを確認
3. 「ポイント確認(F12)」ボタンで登録されたポイント名を確認
4. Cogent DataHub の Data Browser でポイント名を確認し、HTML のプレフィックスを合わせる
5. SEP (`.` vs `$`) が正しいか確認

### 全IEDを停止する
```bash
pkill -f substation_server
```
