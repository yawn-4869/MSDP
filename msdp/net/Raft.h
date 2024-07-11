#ifndef __RAFT_H__
#define __RAFT_H__

#include <vector>
#include <unordered_map>
#include <string>
#include <string.h>
#include "TimerEvent.h"
#include "IOThread.h"
#include <locker.h>
#include <buttonrpc.hpp>

enum ServerRole {
    FOLLOWER = 1,
    CANDIDATE = 2,
    LEADER = 3
};

struct VoteRequestArgs {
    uint32_t term;
    uint8_t id;
    uint32_t last_log_idx;
    uint32_t last_log_term;
    VoteRequestArgs() : term(0), id(0), last_log_idx(0), last_log_term(0) {}
    VoteRequestArgs(uint32_t _term, uint8_t _id, uint32_t _last_log_idx, uint32_t _last_log_term)
            : term(_term), id(_id), last_log_idx(_last_log_idx), last_log_term(_last_log_term) {}
    // must implement
	friend Serializer& operator >> (Serializer& in, VoteRequestArgs& d) {
		in >> d.term >> d.id >> d.last_log_idx >> d.last_log_term;
		return in;
	}
	friend Serializer& operator << (Serializer& out, VoteRequestArgs d) {
		out << d.term << d.id << d.last_log_idx << d.last_log_term;
		return out;
	}
};

struct RequestVoteReply {
    uint32_t term;
    bool vote_granted;
    RequestVoteReply() : term(0), vote_granted(false) {}
    RequestVoteReply(uint32_t _term, uint8_t _vote_granted)
            : term(_term), vote_granted(_vote_granted) {}
    // must implement
	friend Serializer& operator >> (Serializer& in, RequestVoteReply& d) {
		in >> d.term >> d.vote_granted;
		return in;
	}
	friend Serializer& operator << (Serializer& out, RequestVoteReply d) {
		out << d.term << d.vote_granted;
		return out;
	}
};

struct AppendLogArgs {
    uint32_t term;
    uint8_t id;
    uint32_t prev_log_idx;
    uint32_t prev_log_term;
    int32_t leader_commit_idx;
    std::string commands;
    AppendLogArgs() : term(0), id(), prev_log_idx(0), prev_log_term(0), leader_commit_idx(0) {}
    AppendLogArgs(uint8_t _id, uint32_t _term, uint32_t _log_idx, uint32_t _log_term, uint32_t _commit_idx) : 
                term(_term), id(_id), prev_log_idx(_log_idx), prev_log_term(_log_term), leader_commit_idx(_commit_idx) {}
    friend Serializer& operator >> (Serializer& in, AppendLogArgs& d) {
		in >> d.term >> d.id >> d.prev_log_idx >> d.prev_log_term >> d.leader_commit_idx;
		return in;
	}
	friend Serializer& operator << (Serializer& out, AppendLogArgs d) {
		out << d.term << d.id << d.prev_log_idx << d.prev_log_term << d.leader_commit_idx;
		return out;
	}
};

struct LogAppendReply {
    uint32_t term;
    bool success;
    uint32_t last_idx; // success == true: 向 leader 指示 nextIndex, success == false: 向 leader 传递发生冲突的日志索引, 便于快速匹配
    uint32_t last_term; // success == true: 向 leader 指示 term, success == false: 传递发生冲突的冲突的日志任期, 便于 leader 快速定位
    LogAppendReply() : term(0), success(false), last_idx(0), last_term(0) {}
    LogAppendReply(uint32_t _term, uint8_t _success, uint32_t idx, uint32_t conflict)
            : term(_term), success(_success), last_idx(idx), last_term(conflict) {}
    // must implement
	friend Serializer& operator >> (Serializer& in, LogAppendReply& d) {
		in >> d.term >> d.success >> d.last_idx;
		return in;
	}
	friend Serializer& operator << (Serializer& out, LogAppendReply d) {
		out << d.term << d.success << d.last_idx;
		return out;
	}
};

struct LogEntry {
    LogEntry() : m_term(0), m_idx(0) {}
    LogEntry(uint32_t term, const std::string& command, uint32_t idx) : m_term(term), m_idx(idx), m_command(command) {}

    uint32_t m_term; // 任期号
    std::string m_command; // 状态机指令
    uint32_t m_idx; // 日志索引
};

class Raft {
public:
    Raft(uint8_t id);
    ~Raft();
    void start();
    void handleTimeout();
    void callRequestVote(int peer_id, VoteRequestArgs args);
    void callRequestAppendLog(int peer_id, AppendLogArgs args);
    static void* listenForVote(void* arg);
    static void* listenForLogAppend(void* arg);
    RequestVoteReply replyToVote(VoteRequestArgs args);
    LogAppendReply replyToLogAppend(AppendLogArgs args);

private:
    void resetTimeout(EventLoop* event_loop, TimerEvent::s_ptr time_event); // TODO: 可能需要修改
    bool checkLogUpdate(int term, int log_idx);
    int findLastMatched(AppendLogArgs& args);
    void logSerialize(const std::vector<LogEntry>& input, std::string& output);
    void logDeserialize(const std::string& input, std::vector<LogEntry>& output);

private:
    ServerRole m_role {ServerRole::FOLLOWER}; // 服务器类别, 初始化时均为FOLLOWER
    uint8_t m_id; // 服务器id

    // 所有服务器的持久性状态
    int32_t m_current_term {0};             // 服务器已知最新的任期
    uint8_t m_voted_for {0};                // 当前任期内收到选票的 candidate 的id
    std::vector<LogEntry> m_log_entries;    // 日志条目, 用于修改状态机
    std::vector<int> m_peers;
    std::vector<pair<int, int>> m_peers_info; // 集群服务器的端口信息, first -> vote port second -> log port

    // 所有服务器的易失性状态
    int32_t m_commit_idx {0};   // 已知已提交的最高的日志条目的索引
    int32_t m_last_applied {0}; // 已经被应用到状态机的最高的日志条目的索引
    uint8_t m_recv_votes {0};   // 已收到的选票数量

    // leader服务器的易失性状态
    std::unordered_map<int, int> m_next_idx_for_nodes; // 对于每一台服务器，发送到该服务器的下一个日志条目的索引
    std::unordered_map<int, int> m_match_idx_for_nodes; // 对于每一台服务器，已知的已经复制到该服务器的最高日志条目的索引

    EventLoop* m_main_event_loop;
    TimerEvent::s_ptr m_timer_event; // 定时器
    

    // rpc客户端与服务端
    buttonrpc m_vote_client;
    buttonrpc m_vote_server;
    buttonrpc m_log_client;
    buttonrpc m_log_server;

    // 线程
    pthread_t m_t_vote_id;
    pthread_t m_t_log_id;

    // 端口
    int m_vote_port;
    int m_log_port;

    // rpc 超时时间
    int m_rpc_timeout;

    // 选举参数
    VoteRequestArgs m_vote_req_args;
    RequestVoteReply m_vote_reply;
    AppendLogArgs m_append_log_args;
};

#endif