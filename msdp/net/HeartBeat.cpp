#include <string.h>
#include "HeartBeat.h"
#include "Logger.h"
#include "Config.h"
#include "utils.h"

HeartBeat::HeartBeat() {

}

HeartBeat::HeartBeat(HBRUNTYPE hb_type, const char* ip, u_int port) {
    init(hb_type, ip, port);
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

void HeartBeat::init(HBRUNTYPE hb_type, const char* ip, u_int port) {
    m_hb_type = hb_type;
    m_port = port;
    strcpy(m_ip, ip);
    m_receive_sock = new CNetSocket();
    m_send_sock = new CNetSocket();
    m_receive_sock->InitNetMultiSocket('R', m_ip, m_port);
    m_send_sock->InitNetMultiSocket('S', m_ip, m_port);
}

int HeartBeat::sendMsg(HBMSGTYPE type, char* worker_ip, int next_track_number) {
    if(m_hb_type == HB_DISCONNECTED) {
        // 断开连接, 不需要发送网络包
        return HB_ERR_INVALID_PARAM;
    }
    // 组包
    // u_int pkg_len = sizeof(HBMSG) + len;
    // HBMSG* msg = (HBMSG*)malloc(sizeof(pkg_len));
    // msg->type = type;
    // msg->len = pkg_len;
    HBMSG msg;
    msg.type = type;
    msg.len = sizeof(HBMSG);
    msg.next_track_number = next_track_number;
    const char* local_worker_ip = getWorkerAddr();
    const char* node_ip = local_worker_ip == NULL ? worker_ip : local_worker_ip;
    strcpy(msg.worker_ip, node_ip);

    msg.timestamp = getNowMs();
    msg.alive_time = m_node_map[m_ip].last_time - m_node_map[m_ip].join_time;
    // msg->content
    // memcpy(msg->content, content, len);
    // msg->content[len] = '\0';

    // 组播发送
    bool is_send = m_send_sock->SendData((unsigned char*)&msg, msg.len);
    if(!is_send) {
        ERRORLOG("Socket send error! ERR: %d, error: %s", errno, strerror(errno));
        if(m_node_map.count(m_ip)) {
            // 工作机掉线
            m_node_map[m_ip].lost_count++;
            if(m_node_map[m_ip].lost_count > Config::get_instance()->m_hb_lost_max_count) {
                // 将自身结点标记为失联
                m_node_map[m_ip].is_disconnected = true;
                m_node_map[m_ip].join_time = 0;
                m_node_map[m_ip].last_time = 0;
                m_hb_type = HB_DISCONNECTED;
            }
        }
        return HB_ERR_SOCK_SEND;
    }
    DEBUGLOG("send hb_msg: %d success, destination: %s:%d", msg.type, m_ip, m_port);
    return HB_OK;
}

int HeartBeat::recvMsg() {
    // if(m_hb_type == HB_CLIENT) {
    //     // 客户端目前只需要发送心跳包, 并不需要接收, 因此不做任何处理
    //     return -1;
    // }
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
        DEBUGLOG("Receive node register message, IP:PORT = %s:%d", ip, port);
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
            DEBUGLOG("Add server node success, IP:PORT = %s:%d", ip, port);
        } else {
            ERRORLOG("node list size greater than HB_MAX_NODE_COUNT!");
        }
        break;

    case HB_MSG_RUNNING:
        if(!m_node_map.count(ip)) {
            DEBUGLOG("Need to add this node to node list, IP:PORT = %s:%d", ip, port);
            if(m_node_map.size() < HB_MAX_NODE_COUNT) {
                NODE node;
                strcpy(node.ip, ip);
                node.port = port;
                node.join_time = msg.timestamp;
                node.last_time = msg.timestamp;
                node.is_worker = strcasecmp(ip, msg.worker_ip) == 0 ? true : false;;
                node.is_alive = true;
                m_node_map.emplace(node.ip, node);
            } else {
                ERRORLOG("node list size greater than HB_MAX_NODE_COUNT!");
                break;
            }
        }

        DEBUGLOG("Receive node running message, IP:PORT = %s:%d", ip, port);
        if(!m_node_map[ip].is_alive) {
            // 节点之前已经因为超时而被标记为失连
            // 需要重新计算在网时间
            m_node_map[ip].is_alive = true;
            m_node_map[ip].join_time = msg.timestamp;
        }

        if(m_node_map[m_ip].is_disconnected && ip != m_ip) {
            // 重新连接, 接收到了其他工作机的心跳包
            if(!m_node_map[msg.worker_ip].is_worker) {
                // 更新工作机信息
                m_node_map[msg.worker_ip].is_worker = true;
                m_node_map[m_ip].is_worker = false;
                m_node_map[m_ip].is_disconnected = false;
            }

            // 更新其他结点的信息
            m_node_map[ip].join_time = msg.timestamp - msg.alive_time;
            m_node_map[ip].last_time = msg.timestamp;

            // 重置自身在网时长
            m_node_map[m_ip].join_time = msg.timestamp;
            m_node_map[m_ip].last_time = msg.timestamp;

            // 恢复连接标志
            m_hb_type = HB_CONNECTED;
        }

        m_node_map[ip].last_time = msg.timestamp;
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
        if(!it->second.is_alive) continue;

        int64_t ts_recv = it->second.last_time;

        int64_t ts_now = getNowMs();

        int64_t delta_ts = ts_now - ts_recv;
        if(delta_ts > Config::get_instance()->m_hb_lost_tolerance) {
            if(it->second.lost_count >= Config::get_instance()->m_hb_lost_max_count) {
                it->second.is_alive = false;
                // 相关参数重置, 等待下次连接
                it->second.join_time = 0;
                it->second.last_time = 0;
                it->second.lost_count = 0;
                printf("a node server from ip:%s disconnected\n", it->second.ip);
                if(it->second.is_worker) {
                    // 工作机掉线
                    const char* ip = getMaxAliveNode();
                    if(ip != nullptr) {
                        it->second.is_worker = false;
                        m_node_map[ip].is_worker = true;
                    }
                }
            } else {
                it->second.lost_count++;
                printf("a node server from ip:%s timeout, count: %d\n", it->second.ip, it->second.lost_count);
            }
        }
    }
}