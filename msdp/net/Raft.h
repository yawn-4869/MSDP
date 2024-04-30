#ifndef __RAFT_H__
#define __RAFT_H__

#include <vector>
#include <unordered_map>
#include <string>
#include <string.h>
#include "TimerEvent.h"
#include "IOThread.h"
#include <locker.h>

enum ServerRole {
    FOLLOWER = 1,
    CANDIDATE = 2,
    LEADER = 3
};

class LogEntry {
public:
    LogEntry(int term, std::string& command, int idx) : m_term(term), m_idx(idx), m_command(command) {}
    int getLogTerm() {
        return m_term;
    }

private:
    uint32_t m_term; // 任期号
    std::string m_command; // 状态机指令
    uint32_t m_idx; // 日志索引
};

class Raft {
public:
    Raft(uint8_t id);
    ~Raft();
    void handleTimeout();
    void callRequestVote(int peer_id);
    void callRequestAppendLog(int peer_id);
    void responseToRequest();
    void resetTimeout(EventLoop* event_loop, TimerEvent::s_ptr time_event);
private:
    ServerRole m_role {ServerRole::FOLLOWER}; // 服务器类别, 初始化时均为FOLLOWER
    uint8_t m_id; // 服务器id

    // 所有服务器的持久性状态
    int32_t m_current_term {0}; // 服务器已知最新的任期
    uint8_t m_voted_for {0}; // 当前任期内收到选票的 candidate 的id
    std::vector<LogEntry> m_log_entries; // 日志条目, 用于修改状态机
    std::vector<int> m_peers;

    // 所有服务器的易失性状态
    int32_t m_commit_idx {0}; // 已知已提交的最高的日志条目的索引
    int32_t m_last_applied {0}; // 已经被应用到状态机的最高的日志条目的索引
    uint8_t m_recv_votes {0}; // 已收到的选票数量

    // leader服务器的易失性状态
    std::unordered_map<int, int> m_next_idx_for_nodes; // 对于每一台服务器，发送到该服务器的下一个日志条目的索引
    std::unordered_map<int, int> m_match_idx_for_nodes; // 对于每一台服务器，已知的已经复制到该服务器的最高日志条目的索引

    EventLoop* m_main_event_loop;
    TimerEvent::s_ptr m_election_timer_event; // 定时器
    IOThread* m_election_thread; // 选举线程

    // TODO: Vote RPC实现
    // 参数
    // | term         | 候选人的任期号               |
    // | candidateId  | 请求选票的候选人的 ID        |
    // | lastLogIndex | 候选人的最后日志条目的索引值 |
    // | lastLogTerm  | 候选人最后日志条目的任期号   |
    // 返回值
    // | term        | 当前任期号，以便于候选人去更新自己的任期号 |
    // | voteGranted | 候选人赢得了此张选票时为真                |

    // candidate 服务器调用 Vote Request
    // 对于candiate:
    // 1. 初始：自增curent_term, 给自己投票, 重置
    // 2. 给自己投票
    // 3. 重置选举超时计时器
    // 4. 发送请求投票RPC给其他服务器（组播/广播实现）
    // 对于follower:
    // 只要在超时时间内收到了来自candidate的Vote Request或者来自leader的AppendEntries Request, 都需要重置超时计时器
    // 校验 leader 合法性: 
    // 1. term < current_term false reject
    // voted_for == 0 || voted == candidate_id voteGranted = true

    // TODO: AppendEntries RPC实现
    // 参数
    // | term         | 领导人的任期                                                 |
    // | leaderId     | 领导人 ID 因此跟随者可以对客户端进行重定向（译者注：跟随者根据领导人 ID 把客户端的请求重定向到领导人，比如有时客户端把请求发给了跟随者而不是领导人） |
    // | prevLogIndex | 紧邻新日志条目之前的那个日志条目的索引                       |
    // | prevLogTerm  | 紧邻新日志条目之前的那个日志条目的任期                       |
    // | log_entries[]    | 需要被保存的日志条目（被当做心跳使用时，则日志条目内容为空；为了提高效率可能一次性发送多个） |
    // | leaderCommit | 领导人的已知已提交的最高的日志条目的索引                     |
    // 返回值
    // | term    | 当前任期，对于领导人而言 它会更新自己的任期                  |
    // | success | 如果跟随者所含有的条目和 prevLogIndex 以及 prevLogTerm 匹配上了，则为 true |

    // leader服务器调用 AppendEntries RPC Request
    // 对于leader: 
    // 
    // followers 调用的 AppendEntries RPC Response 
    // 对于follower: 
    // 如果leader的任期小于follower的当前任期, false, reject
    // 跟随follower日志中找不到包含相同索引位置和任期号的log_entry(pre_log_idx 以及 pre_log_term) false, reject
    // 已存在的条目和leader发送的条目冲突, 删除这个条目及其之后的所有条目
    // 日志中尚未存在该条目: accepted
    // 更新m_commit_idx为min(leader_commit_idx, pre_log_idx)
};

#endif