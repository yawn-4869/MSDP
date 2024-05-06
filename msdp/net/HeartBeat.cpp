#include <string.h>
#include "HeartBeat.h"
#include "Logger.h"
#include "Config.h"
#include "utils.h"

HeartBeat::HeartBeat() {

}

HeartBeat::HeartBeat(HBRUNTYPE hb_type, const char* client_addr, const char* local_addr, u_int port) {
    init(hb_type, client_addr, local_addr, port);
}

HeartBeat::~HeartBeat() {
    if(m_receive_sock) {
        delete m_receive_sock;
        m_receive_sock = nullptr;
    }

    if(m_send_sock) {
        delete m_send_sock;
        m_send_sock = nullptr;
    }
}

void HeartBeat::init(HBRUNTYPE hb_type, const char* client_addr, const char* local_addr, u_int port) {
    m_hb_type = hb_type;
    m_port = port;
    strcpy(m_client_addr, client_addr);
    strcpy(m_local_addr, local_addr);
    m_receive_sock = new CNetSocket();
    m_send_sock = new CNetSocket();
    m_receive_sock->InitNetMultiSocket('R', (char*)client_addr, m_port);
    m_send_sock->InitNetMultiSocket('S', (char*)client_addr, m_port);
}

int HeartBeat::sendMsg(HBMSGTYPE type, char* worker_ip, int next_track_number) {
    if(m_node_map[m_local_addr].is_disconnected) {
        return -1;
    }
    HBMSG msg;
    msg.type = type;
    msg.len = sizeof(HBMSG);
    msg.next_track_number = next_track_number;
    const char* local_worker_ip = getWorkerAddr();
    const char* node_ip = local_worker_ip == NULL ? worker_ip : local_worker_ip;
    strcpy(msg.worker_ip, node_ip);

    msg.timestamp = getNowMs();
    msg.alive_time = m_node_map[m_local_addr].last_time - m_node_map[m_local_addr].join_time;
    // msg->content
    // memcpy(msg->content, content, len);
    // msg->content[len] = '\0';

    // 组播发送
    bool is_send = m_send_sock->SendData((unsigned char*)&msg, msg.len);
    if(!is_send) {
        ERRORLOG("IP:PORT [%s:%d] socket send error! ERR: %d, error: %s", m_local_addr, m_port, errno, strerror(errno));
        if(m_node_map.count(m_local_addr)) {
            // 工作机掉线
            m_node_map[m_local_addr].lost_count++;
            if(m_node_map[m_local_addr].lost_count > Config::get_instance()->m_hb_lost_max_count) {
                // 将自身结点标记为失联
                m_node_map[m_local_addr].is_disconnected = true;
                m_node_map[m_local_addr].join_time = 0;
                m_node_map[m_local_addr].last_time = 0;
                m_hb_type = HB_DISCONNECTED;
            }
        }
        return HB_ERR_SOCK_SEND;
    }

    DEBUGLOG("IP:PORT [%s:%d] send HB msg to [%s:%d] success, node status: %d, alive time = %lld, current worker[%s]", 
    m_local_addr, m_port, m_client_addr, m_port, m_node_map[m_local_addr].is_disconnected, msg.alive_time, msg.worker_ip);
    return HB_OK;
}

int HeartBeat::recvMsg() {
    unsigned char buf[sizeof(HBMSG)];
    memset(buf, 0, sizeof(buf));
    m_receive_sock->ReceiveData(buf);
    HBMSG msg;
    memcpy(&msg, buf, sizeof(HBMSG));
    sockaddr_in client_addr = m_receive_sock->getClientAddr();
    
    // 服务端需要处理客户端的连接注册、连接退出、连接失败
    char *ip = inet_ntoa(client_addr.sin_addr);
    u_int port = client_addr.sin_port;
    int len = msg.len - sizeof(HBMSG);	
    // char descript[HB_NODE_DESCRIPT] = {0};		
    // memcpy(descript, msg->content, MIN(HB_NODE_DESCRIPT, len));
    switch (msg.type)
    {
    case HB_MSG_REG:
        DEBUGLOG("IP:PORT [%s:%d] receive node [%s:%d] register message", m_local_addr, m_port, ip, port);
        if(m_node_map.count(ip)) {
            break;
        }
        if(m_node_map.size() < HB_MAX_NODE_COUNT) {
            NODE node;
            strcpy(node.ip, ip);
            node.port = port;
            node.join_time = msg.timestamp;
            node.last_time = msg.timestamp;
            node.is_worker = strcasecmp(ip, msg.worker_ip) == 0 ? true : false;;
            node.is_alive = true;
            m_node_map.emplace(node.ip, node);
            DEBUGLOG("IP:PORT [%s:%d] add node [%s:%d] success", m_local_addr, m_port, ip, port);
        } else {
            ERRORLOG("node list size greater than HB_MAX_NODE_COUNT!");
        }
        break;

    case HB_MSG_RUNNING:
        if(!m_node_map.count(ip)) {
            // 新结点加入
            if(m_node_map.size() < HB_MAX_NODE_COUNT) {
                NODE node;
                strcpy(node.ip, ip);
                node.port = port;
                node.join_time = msg.timestamp;
                node.last_time = msg.timestamp;
                node.is_worker = strcasecmp(ip, msg.worker_ip) == 0 ? true : false;;
                node.is_alive = true;
                m_node_map.emplace(node.ip, node);
                DEBUGLOG("IP:PORT [%s:%d] add node [%s:%d] success", m_local_addr, m_port, ip, port);
            } else {
                ERRORLOG("node list size greater than HB_MAX_NODE_COUNT!");
                break;
            }
        } else {
            // 接收到已有结点的网络包
            if(m_node_map[m_local_addr].is_disconnected) {
                // 断连状态
                if(ip != m_local_addr) {
                    // 重新连接, 接收到来自其他机器的网络包
                    if(!m_node_map[msg.worker_ip].is_worker) {
                        // 更新工作机信息
                        m_node_map[msg.worker_ip].is_worker = true;
                        m_node_map[m_local_addr].is_worker = false;
                    }

                    // 更新其他结点的信息
                    m_node_map[ip].join_time = msg.timestamp - msg.alive_time;
                    m_node_map[ip].last_time = msg.timestamp;

                    // 恢复连接标志
                    m_hb_type = HB_CONNECTED;
                    m_node_map[m_local_addr].is_disconnected = false;
                } else {
                    // 接到自身网络包, 重置自身在网时长
                    m_node_map[ip].join_time = msg.timestamp;
                    m_node_map[ip].last_time = msg.timestamp;
                }
            } else {
                // 连接状态, 更新列表结点时间戳
                if(!m_node_map[ip].is_alive) {
                    // 节点之前已经因为超时而被标记为失连
                    // 需要重新计算在网时间
                    m_node_map[ip].is_alive = true;
                    m_node_map[ip].join_time = msg.timestamp;
                }
                m_node_map[ip].last_time = msg.timestamp;
            }

            DEBUGLOG("IP:PORT [%s:%d] update node [%s:%d] success, status[%d], join time: %lld, last time: %lld", 
            m_local_addr, m_port, ip, port, m_node_map[m_local_addr].is_disconnected, m_node_map[ip].join_time, m_node_map[ip].last_time);
        }
        break;
    case HB_MSG_QUIT:
        DEBUGLOG("Receive node quit message, IP:PORT = %s:%d", ip, port);
        m_node_map[ip].is_alive = false;
        break;
    case HB_MSG_ERR:
        break;
    default:
        break;
    }
    return msg.next_track_number;
}

void HeartBeat::loopCheck() {
    for(auto it = m_node_map.begin(); it != m_node_map.end(); it++) {
        // 更新结点列表
        if(!it->second.is_alive) continue;

        int64_t ts_recv = it->second.last_time;
        int64_t ts_now = getNowMs();
        int64_t delta_ts = ts_now - ts_recv;
        
        if(delta_ts > Config::get_instance()->m_hb_lost_tolerance) {
            if(it->second.lost_count >= Config::get_instance()->m_hb_lost_max_count) {
                if(it->second.is_worker) {
                    // 工作机掉线
                    // const char* ip = getMaxAliveNode();
                    // if(ip != nullptr) {
                    //     it->second.is_worker = false;
                    //     m_node_map[ip].is_worker = true;
                    // }
                    // DEBUGLOG("worker server changed, from [%s] to [%s]", it->second.ip, ip);
                    it->second.is_worker = false;
                }
                it->second.is_alive = false;
                // 相关参数重置, 等待下次连接
                it->second.join_time = 0;
                it->second.last_time = 0;
                it->second.lost_count = 0;
                printf("a node server from ip:%s disconnected\n", it->second.ip);
            } else {
                it->second.lost_count++;
                printf("a node server from ip:%s timeout, delta_ts=%lld, count: %d\n", it->second.ip, delta_ts, it->second.lost_count);
            }
        }
    }

    // 工作机更新
    const char* ip = getMaxAliveNode();
    if(ip == nullptr) {
        ip = m_local_addr;
    }
    m_node_map[ip].is_worker = true;
    DEBUGLOG("current worker server [%s]", ip);
}