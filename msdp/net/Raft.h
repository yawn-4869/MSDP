#ifndef __RAFT_H__
#define __RAFT_H__

#include <vector>
#include <unordered_set>
#include <string>
#include <buttonrpc.hpp>
#include "locker.h"

struct PeersInfo{
    PeersInfo() : m_port(0, 0), m_id(-1){}
    PeersInfo(int port1, int port2, int id) : m_port(port1, port2), m_id(id) {}
    pair<int, int> m_port;
    int m_id;
};

struct LogEntry{
    LogEntry(string cmd = "", int term = -1):m_command(cmd),m_term(term){}
    string m_command;
    int m_term;
};

struct RequestVoteArgs{
    int term;
    int candidateId;
    int lastLogTerm;
    int lastLogIndex;

    friend Serializer& operator >> (Serializer& in, RequestVoteArgs& d) {
		in >> d.term >> d.candidateId >> d.lastLogTerm >> d.lastLogIndex;
		return in;
	}
	friend Serializer& operator << (Serializer& out, RequestVoteArgs d) {
		out << d.term << d.candidateId << d.lastLogTerm << d.lastLogIndex;
		return out;
	}
};

struct AppendEntriesArgs{
    int m_term;
    int m_leaderId;
    int m_prevLogIndex;
    int m_prevLogTerm;
    int m_leaderCommit;
    std::string m_sendLogs;

    friend Serializer& operator >> (Serializer& in, AppendEntriesArgs& d) {
		in >> d.m_term >> d.m_leaderId >> d.m_prevLogIndex >> d.m_prevLogTerm >> d.m_leaderCommit >> d.m_sendLogs;
		return in;
	}
	friend Serializer& operator << (Serializer& out, AppendEntriesArgs d) {
		out << d.m_term << d.m_leaderId << d.m_prevLogIndex << d.m_prevLogTerm << d.m_leaderCommit << d.m_sendLogs;
		return out;
	}
};

struct RequestVoteReply{
    int term;
    bool VoteGranted;
};

struct AppendEntriesReply{
    int m_term;
    bool m_success;
    int m_conflict_term;
    int m_conflict_index;
};

class Raft {

public:
    void init();
    static void* listenForVote(void* arg);
    static void* listenForAppend(void* arg);
    static void* processEntriesLoop(void* arg);
    static void* electionLoop(void* arg);
    static void* callRequestVote(void* arg);
    static void* sendAppendEntries(void* arg);

    enum RAFT_STATE{ LEADER = 0, CANDIDATE, FOLLOWER};
    void kill();
    void activate();
    bool isDead();
    std::pair<int, bool> getState();
    RequestVoteReply replyToVote(RequestVoteArgs args);
    AppendEntriesReply replyToAppend(AppendEntriesArgs args);

private:
    int64_t getMyDuration(int64_t last);
    void setBroadcastTime();
    bool checkLogUptodate(int term, int index);
    std::vector<LogEntry> getCmdAndTerm(string text);

private:
    int m_id;
    int m_leader_id {-1};
    std::vector<PeersInfo> m_peers;
    std::unordered_set<int> m_is_exist;
    int m_peer_id;
    bool m_is_dead {false};

    int m_cur_term {0};
    int m_voted_for {-1};
    std::vector<LogEntry> m_logs;

    Locker m_lock;
    Cond m_cond;

    std::vector<int> m_next_index;
    std::vector<int> m_match_index;
    int m_last_applied {0};
    int m_commit_index {0};

    int m_recv_votes {0};
    int m_finished_vote {0};
    int m_cur_peer_id {0};
    RAFT_STATE m_state {FOLLOWER};
    int64_t m_last_wake;
    int64_t m_last_broadcast;

    // 改进Raft: 用于选举 leader 时的特征参数
    int m_latentcy {0}; // 网络时延
    int m_off_times {0}; // 掉线次数
    int m_online_time {0}; // 在线时长
    int m_leader_cnt {0}; // 成为 leader 的次数
    int m_cpu_capacity {0}; // cpu 能力
    int m_threads_cnt {0}; // 线程量, 可能是次选指标
};

#endif