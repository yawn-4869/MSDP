# 重新连接, 清除上述定义的防火墙规则
iptables -D INPUT 1
iptables -D INPUT 1
iptables -D OUTPUT 1
iptables -D OUTPUT 1
iptables -D OUTPUT 1