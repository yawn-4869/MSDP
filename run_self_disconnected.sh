# 模拟本机网络断开
# 1. 不接收其余虚拟机数据包
iptables -I INPUT -s 192.168.58.147 -j DROP
iptables -I INPUT -s 192.168.58.148 -j DROP

# 2. 限制组播发送数据包
iptables -I OUTPUT -d 239.0.0.48 -j DROP
iptables -I OUTPUT -d 239.0.0.1 -j DROP
iptables -I OUTPUT -p udp -d 192.168.1.136 -j DROP