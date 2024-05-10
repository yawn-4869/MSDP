#ifndef __HeartBeat_H__
#define __HeartBeat_H__

#include <sys/time.h>
#include <memory>
#include <vector>
#include <map>
#include "netsocket.h"
#include "EventLoop.h"

#define HB_IP_LEN 40
#define HB_MAX_NODE_COUNT 5

typedef enum ERRTYPE
{
	HB_ERR_SOCK_INIT = -100,
	HB_ERR_SOCK_BIND,
	HB_ERR_SOCK_SETOPT,
	HB_ERR_SOCK_SEND,
	HB_ERR_SOCK_RECV,
	HB_ERR_INVALID_PARAM,
	HB_OK = 0
} ERRTYPE;

typedef enum HBRUNTYPE
{
    HB_CONNECTED = 0,
    HB_DISCONNECTED = 1
} HBRUNTYPE;

typedef enum HBMSGTYPE
{
    HB_MSG_REG = 0, // 注册
    HB_MSG_RUNNING, // 运行
    HB_MSG_QUIT, // 退出
    HB_MSG_ERR // 错误
} HBMSGTYPE;

// 发送的心跳包结构体
typedef struct HBMSG
{
    HBMSGTYPE type; // 包类型: 注册/运行/退出/错误
    int64_t timestamp; // 时间戳信息 ms
    int64_t alive_time; // 在网时长信息 ms
    char worker_ip[HB_IP_LEN]; // 工作机ip地址
    int next_track_number; // 下一个tracknum
    u_int len; // 包长度, 结构体+content
    // char content[8]; // 消息内容
} HBMSG;

// 存储的服务器结点结构体
typedef struct NODE
{
    char ip[HB_IP_LEN]; // ip
    u_int port; // 端口
    bool is_worker{ false }; // 是否是工作机
    int64_t join_time; // 入网时间
    int64_t last_time; // 上一次接收心跳包时间
    u_int lost_count{ 0 };
    bool is_alive{ false }; // 是否连接
    bool is_disconnected{ false }; // 自身是否断开连接
} NODE;

class HeartBeat
{
public:
    HeartBeat();
    HeartBeat(HBRUNTYPE hb_type, const char* client_addr, const char* local_addr, u_int port);
    void init(HBRUNTYPE hb_type, const char* client_addr, const char* local_addr, u_int port);
    ~HeartBeat();
public:
    // 发送心跳包
    int sendMsg(HBMSGTYPE type, char* worker_ip, int next_track_number);
    // 接收心跳包并根据心跳包的类型进行处理
    int recvMsg();
    // 循环检查节点是否有效
    void loopCheck();
    // 节点是否有效
    bool isAlive(const char* ip) {
        if(!m_node_map.count(ip)) return false;
        return m_node_map[ip].is_alive;
    }
    // 返回节点在网时长
    int64_t getAliveTime(const char* ip) {
        if(!m_node_map.count(ip) || !m_node_map[ip].is_alive) return 0;
        return m_node_map[ip].last_time - m_node_map[ip].join_time;
    }
    int getRecvFd() {
        return m_receive_sock->GetReceiveFd();
    }
    const char* getWorkerAddr() {
        for(auto it = m_node_map.begin(); it != m_node_map.end(); it++) {
            if(it->second.is_worker) {
                return it->first.c_str();
            }
        }
        return NULL;
    }

    bool isDisconnected() {
        return m_node_map[m_local_addr].is_disconnected;
    }

private:
    // 返回除工作机外在网时长最久的节点ip
    const char* getMaxAliveNode() {
        const char* target_ip = NULL;
        for(auto it = m_node_map.begin(); it != m_node_map.end(); it++) {
            // printf("[%s] [%d] [%lld] [%lld]\n", it->first.c_str(), it->second.is_alive, it->second.join_time, it->second.last_time);
            // printf("[%lld]\n", getAliveTime(it->first.c_str()));
            if(!it->second.is_alive) {
                continue;
            }

            // if(it->second.is_worker) {
            //     return it->first.c_str();
            // }

            if(target_ip == NULL) {
                target_ip = it->first.c_str();
            } else {
                int64_t target_alive_time = getAliveTime(target_ip);
                int64_t new_alive_time = getAliveTime(it->first.c_str());
                if(target_alive_time < new_alive_time) {
                    target_ip = it->first.c_str();
                }
            }
        }

        return target_ip;
    }
private:
	HBRUNTYPE m_hb_type;	// 作为服务器/客户端
	char m_local_addr[HB_IP_LEN];	// 发送地址
    char m_client_addr[HB_IP_LEN]; // 发送地址
	u_int m_port;			// 监听端口
    std::map<std::string, NODE> m_node_map; // 服务器节点列表
    CNetSocket* m_send_sock;
    CNetSocket* m_receive_sock;
};


#endif