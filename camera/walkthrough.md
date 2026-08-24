# Walkthrough: HIKVISION カメラ向け MITM セキュリティ検証ツール

## 作成したファイル

### 1. [hik_arp_spoof.py](file:///c:/Users/kikii/OneDrive/デスクトップ/camera/hik_arp_spoof.py) — ARP Spoofing

カメラと監視端末の ARP テーブルを書き換え、通信を攻撃 PC 経由にする。

**既存 [arp_spoofing.py](file:///c:/Users/kikii/OneDrive/デスクトップ/camera/arp_spoofing.py) からの改善点:**

| 改善 | 詳細 |
|:---|:---|
| コマンドライン引数 | `--camera-ip`, `--monitor-ip`, `--iface` 等で柔軟に設定 |
| MAC 自動取得 | ARP リクエストで自動解決（手動指定も可能） |
| 終了時復元 | Ctrl+C で ARP テーブルを正常な状態に復元 |
| 情報表示 | 起動時に設定一覧表示、定期的にパケット数表示 |

---

### 2. [hik_relay_corrupt.py](file:///c:/Users/kikii/OneDrive/デスクトップ/camera/hik_relay_corrupt.py) — データ中継・改ざん

ARP Spoofing で中間者になった状態で、カメラからの映像データを改ざんして転送する。

**既存 [relay_0_1_ramdom.py](file:///c:/Users/kikii/OneDrive/デスクトップ/camera/relay_0_1_ramdom.py) からの改善点:**

| 改善 | 詳細 |
|:---|:---|
| マルチプロトコル | HTTP/ISAPI (port 80), RTP/UDP, HIKVISION独自 (port 8000) の3種に対応 |
| H.264/H.265 対応 | NAL ユニットタイプを判別し、映像データ部のみを破壊 |
| SPS/PPS/VPS 保護 | デコーダ初期化パラメータは破壊しない → 映像が完全停止せず「乱れる」効果に |
| RTP ヘッダ解析 | RTP v2 ヘッダ、Extension、CSRC を正しくスキップ |
| FU-A 対応 | H.264/H.265 の分割 NAL (Fragmentation Unit) にも対応 |
| 統計表示 | プロトコル別の改ざん数をリアルタイム表示 |
| 改ざん率調整 | `--corrupt-ratio` で改ざん確率を調整可能 |
| 既存コードのバグ修正 | `elif` の不正なインデントを修正 |

**改ざんロジック (プロトコル別):**

```
HTTP (port 80) — Webブラウザ閲覧時のメインターゲット
├── JPEG (0xFFD8) → 先頭+10byte から 200byte 破壊
├── H.264 NAL    → NALヘッダ直後から 200byte 破壊
├── H.265 NAL    → NALヘッダ直後から 200byte 破壊
└── 汎用バイナリ  → HTTPボディ先頭+50byte から 200byte 破壊

RTP (UDP) — RTSP クライアント利用時
├── 通常 NAL    → RTPヘッダ+NALヘッダの後を破壊
└── FU-A/FU     → FUヘッダの後を破壊

HIKVISION独自 (port 8000) — iVMS-4200 利用時
└── 先頭 32byte (推定ヘッダ) 以降を破壊
```

---

## 使い方

### 前提条件
- Python 3 + scapy がインストールされていること
- 攻撃 PC、カメラ、監視端末が同一 L2 ネットワーク上にあること
- OS の IP フォワーディングが無効であること

```bash
pip install scapy
```

### Step 1: ARP Spoofing を開始

```bash
python hik_arp_spoof.py \
    --camera-ip 192.168.1.100 \
    --monitor-ip 192.168.1.200 \
    --iface "イーサネット"
```

### Step 2: 別のターミナルでデータ中継・改ざんを開始

```bash
python hik_relay_corrupt.py \
    --camera-ip 192.168.1.100 \
    --monitor-ip 192.168.1.200 \
    --camera-mac AA:BB:CC:DD:EE:01 \
    --monitor-mac AA:BB:CC:DD:EE:02 \
    --iface "イーサネット"
```

> [!TIP]
> `hik_arp_spoof.py` の起動時に MAC アドレスが自動表示されるので、
> その値を `hik_relay_corrupt.py` の `--camera-mac` / `--monitor-mac` に指定してください。

### オプション調整

```bash
# 100% 改ざん、300byte 破壊
python hik_relay_corrupt.py \
    --camera-ip 192.168.1.100 \
    --monitor-ip 192.168.1.200 \
    --camera-mac AA:BB:CC:DD:EE:01 \
    --monitor-mac AA:BB:CC:DD:EE:02 \
    --corrupt-ratio 1.0 \
    --corrupt-size 300
```

---

## 技術的な設計判断

1. **SPS/PPS/VPS を保護する理由**: これらはデコーダ初期化パラメータ。破壊するとデコーダが完全停止し「映像が乱れる」ではなく「映像が映らない」になる。セキュリティ検証としては「映っているが乱れている」方が脆弱性のインパクトを示しやすい。

2. **改ざん確率 (corrupt_ratio) のデフォルトが 0.7 の理由**: 100% だとストリームが完全に壊れてブラウザ側が接続を切断する可能性がある。70% にすることで一部フレームは正常にデコードされ、映像が「乱れている」状態を維持しやすい。

3. **BPF フィルタで `host` を使う理由**: MAC アドレスだけでフィルタすると、同一ネットワーク上の無関係なトラフィックも拾ってしまう。カメラ・監視端末の IP で絞ることで処理対象を最小限に。
