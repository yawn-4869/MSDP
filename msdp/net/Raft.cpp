#include <random>
// #include <google/protobuf/service.h>
// #include <rocket/net/rpc/rpc_channel.h>
// #include <rocket/net/rpc/rpc_controller.h>
// #include <rocket/net/rpc/rpc_closure.h>
// #include <rocket/common/log.h>
// #include <rocket/common/error_code.h>
// #include <rocket/net/tcp/net_addr.h>
// #include <rocket/net/tcp/tcp_server.h>
#include <math.h>
#include "Config.h"
#include "Raft.h"
// #include "raft_server.h"
#include "raft.pb.h"
#include "fusion.pb.h"

Raft::Raft(uint8_t id) : m_id(id) {
    // 初始化指针
    m_main_event_loop = EventLoop::GetCurrentEventloop();

    m_rpc_timeout = 2000;

    m_vote_client.set_timeout(m_rpc_timeout);
    m_log_client.set_timeout(m_rpc_timeout);

    for(int i = 0; i < Config::get_instance()->m_server_count; ++i) {
        m_peers_info.emplace_back(4001+i, 5001+i);
    }
    
    // 定时器设定在200-400ms随机
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    m_timer_event = std::make_shared<TimerEvent>(std::rand() % 201 + 200, false, std::bind(&Raft::handleTimeout, this));
    m_main_event_loop->addTimerEvent(m_timer_event);
    pthread_create(&m_t_vote_id, NULL, listenForVote, this);
    pthread_create(&m_t_log_id, NULL, listenForLogAppend, this);
}

void Raft::start() {
    m_main_event_loop->loop();
}

Raft::~Raft() {
    if(m_main_event_loop) {
        delete m_main_event_loop;
    }
}

void Raft::handleTimeout() {
    if(m_role == ServerRole::FOLLOWER) {
        // follower超时, 成为candidate
        m_role = ServerRole::CANDIDATE;
        // 自增任期号, 给自己投票, 重置计时器, 发送 vote request
        m_current_term++;
        m_vote_req_args.id = m_id;
        m_vote_req_args.term = m_current_term;
        // TODO: last_log_idx 和 last_log_term 两个参数的值是否是这两个？
        m_vote_req_args.last_log_idx = m_log_entries.size() - 1;
        m_vote_req_args.last_log_term = m_log_entries.back().m_term;
        m_voted_for = m_id;
        m_recv_votes = 1; // 每次发起选举的时候都需要重置选票
        for(int peer_id : m_peers) {
            if(peer_id == m_id) {
                continue;
            }
            callRequestVote(peer_id, m_vote_req_args);
        }

    } else if(m_role == ServerRole::CANDIDATE) {
        // candidate 超时
        // 重新发起选举
        // 重新发起选举的话, 需要增加当前任期, 以表示这是新一轮的选举, 以免混淆
        m_current_term++;
        m_vote_req_args.term = m_current_term;
        m_vote_req_args.last_log_idx = m_log_entries.size() - 1;
        m_vote_req_args.last_log_term = m_log_entries.back().m_term;
        m_voted_for = m_id;
        m_recv_votes = 1; // 每次发起选举的时候都需要重置选票
        for(int peer_id : m_peers) {
            if(peer_id == m_id) {
                continue;
            }
            callRequestVote(peer_id, m_vote_req_args);
        }
    } else {
        // leader 需要定时发送 logAppendRequest 给所有服务器
        for(int peer_id : m_peers) {
            if(peer_id == m_id) {
                continue;
            }
            m_append_log_args.id = m_id;
            m_append_log_args.term = m_current_term;
            // m_append_log_args.prev_log_idx = m_log_entries.size();
            // m_append_log_args.prev_log_term = m_log_entries.back().m_term;
            callRequestAppendLog(peer_id, m_append_log_args);
        }
    }

    // 重置时间
    resetTimeout(m_main_event_loop, m_timer_event);
}

void Raft::callRequestVote(int peer_id, VoteRequestArgs args) {

    m_vote_client.as_client("127.0.0.1", m_peers_info[peer_id - 1].first);
    RequestVoteReply reply = m_vote_client.call<RequestVoteReply>("replyToVote", args).val();
    if(reply.term > m_current_term) {
        // 回退到 follower 后重置计时器
        m_role = FOLLOWER;
        m_current_term = reply.term;
        m_voted_for = 0;
        resetTimeout(m_main_event_loop, m_timer_event);
        return;
    }
    if(reply.vote_granted) {
        m_recv_votes++;
    }

    if(m_recv_votes > m_peers.size() / 2) {
        // 半数通过, 成为领导人, 重置定时器
        m_role = LEADER;
        resetTimeout(m_main_event_loop, m_timer_event);
    }
}

RequestVoteReply Raft::replyToVote(VoteRequestArgs args) {
    RequestVoteReply reply;
    reply.term = m_current_term;
    if(m_current_term > args.term) {
        return reply;
    }

    if(m_current_term < args.term) {
        m_role = FOLLOWER;
        m_current_term = args.term;
        m_voted_for = 0;
    }

    if(m_voted_for == 0 || m_voted_for == args.id) {
        if(!checkLogUpdate(args.last_log_term, args.last_log_idx)) {
            m_voted_for = 0;
            return reply;
        }
    }

    // 合法性校验通过, 重置定时器
    resetTimeout(m_main_event_loop, m_timer_event);

    m_voted_for = args.id;
    reply.vote_granted = true;
    return reply;
}

void Raft::callRequestAppendLog(int peer_id, AppendLogArgs args) {
    m_log_client.as_client("127.0.0.1", m_peers_info[peer_id-1].second);
    LogAppendReply reply = m_log_client.call<LogAppendReply>("replyToLogAppend", args).val();
    if(reply.term > m_current_term) {
        // leader 过期, 回退到 follower, 重置定时器
        m_role = FOLLOWER;
        m_current_term = reply.term;
        m_voted_for = 0;
        resetTimeout(m_main_event_loop, m_timer_event);
        return;
    }

    if(reply.success) {
        // 成功, 获取 nextIndex 后的所有日志, 发送给该 follower 
        // 目前的实现方式：
        // leader 需要将 m_log_entries nextIndex 后的日志条目序列化, 序列化后存储到 AppendLogArgs commands 中发送
        // follower 需要对接收到的 AppendLogArgs commands 进行解序列化
        // 序列化和解序列化可能需要 protobuf 实现
        // 更新 leader 中存储的 peer 对应的 nextIndex 和 matchIndex 信息
        m_next_idx_for_nodes[peer_id] = reply.last_idx + 1;
        m_match_idx_for_nodes[peer_id] = reply.last_idx;

    } else {
        // 失败, 根据 reply 中的冲突索引和冲突任期进行调整
        if(reply.last_term != 0) {
            // 任期冲突调整, 找到索引和任期都匹配的项
            int leader_conflict_idx = -1;
            for(int i = args.prev_log_idx; i >= 1; --i) {
                if(m_log_entries[i - 1].m_term == reply.last_term) {
                    leader_conflict_idx = i;
                    break;
                }
            }

            if(leader_conflict_idx != -1) {
                // 找到匹配项
                m_next_idx_for_nodes[peer_id] = leader_conflict_idx + 1;
            } else {
                // 未找到, 直接跳到 last_idx, 下一次再进行匹配
                m_next_idx_for_nodes[peer_id] = reply.last_idx;
            }
            
        } else {
            m_next_idx_for_nodes[peer_id] = reply.last_idx + 1;
        }
    }
}

void* Raft::listenForVote(void* arg) {
    Raft* raft = static_cast<Raft*>(arg);
    raft->m_vote_server.as_server(raft->m_peers_info[raft->m_id-1].first);
    raft->m_vote_server.bind("replyToVote", &Raft::replyToVote, raft);
    raft->m_vote_server.run();
}

void* Raft::listenForLogAppend(void* arg) {
    Raft* raft = static_cast<Raft*>(arg);
    raft->m_log_server.as_server(raft->m_peers_info[raft->m_id-1].first);
    raft->m_log_server.bind("replyToLogAppend", &Raft::replyToLogAppend, raft);
    raft->m_log_server.run();
}

LogAppendReply Raft::replyToLogAppend(AppendLogArgs args) {
    LogAppendReply reply;
    reply.term = m_current_term;

    // 比较任期
    if(m_current_term > args.term) {
        return reply;
    }


    if(m_current_term < args.term) {
        // 一开始写的 <= , 但是从参数来看, 好像直接 < 就可以
        m_voted_for = 0;
        m_current_term = args.term;
        m_role = FOLLOWER;
    }

    // 重置计时器
    resetTimeout(m_main_event_loop, m_timer_event);

    // 比较日志的任期和索引
    // 1. 日志为空, 直接添加
    if(m_log_entries.empty()) {
        vector<LogEntry> tmp_logs;
        // 解序列化
        logDeserialize(args.commands, tmp_logs);
        for(auto log : tmp_logs) {
            m_log_entries.push_back(log);
        }
        reply.success = true;
        reply.last_idx = m_log_entries.size();
        reply.last_term = m_log_entries.back().m_term;
        return reply;
    }

    // 2. 日志不空, 比较是否有匹配项
    if(m_log_entries.size() < args.prev_log_idx) {
        reply.last_idx = m_log_entries.size();
        return reply;
    }

    // 匹配出现冲突, 传给 leader 处理
    if(args.prev_log_idx > 0 && m_log_entries[args.prev_log_idx - 1].m_term != args.prev_log_term) {
        // 索引号相同但是周期不同, 删除该项及以后的项
        reply.last_term = m_log_entries[args.prev_log_idx - 1].m_term;
        // 找到该 term 的第一个索引
        for(int i = 1; i <= args.prev_log_idx; ++i) {
            if(m_log_entries[i].m_term == args.prev_log_term) {
                reply.last_idx = i;
                break;
            }
        }
        return reply;
    }

    // 匹配成功, 直接添加 
    // for(int i = args.prev_log_idx; i < m_log_entries.size(); ++i) {
    //     m_log_entries
    // }
    m_log_entries.erase(m_log_entries.begin() + args.prev_log_idx, m_log_entries.end());
    vector<LogEntry> recv_log;
    logDeserialize(args.commands, recv_log);
    for(auto log : recv_log) {
        m_log_entries.push_back(log);
    }

    // 更新 commit_idx
    if(m_commit_idx < args.leader_commit_idx) {
        m_commit_idx = min(args.leader_commit_idx, (int)m_log_entries.size());
    }
    reply.success = true;
    reply.last_idx = m_log_entries.size();
    reply.last_term = m_log_entries.back().m_term;
    return reply;
}

void Raft::resetTimeout(EventLoop* event_loop, TimerEvent::s_ptr timer_event) {
    event_loop->deleteTimerEvent(timer_event);
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    timer_event->resetInterval(std::rand() % 201 + 200);
    event_loop->addTimerEvent(timer_event);
}

bool Raft::checkLogUpdate(int term, int log_idx) {
    // 日志为空
    if(m_log_entries.empty()) {
        return true;
    }

    // 比较候选人term和最后一个日志的term
    if(term > m_log_entries.back().m_term) {
        return true;
    }

    // term相同, 比较日志索引
    if(term == m_log_entries.back().m_term && log_idx >= m_log_entries.size()) {
        return true;
    }

    return false;
}

int Raft::findLastMatched(AppendLogArgs& args) {
    for(auto log : m_log_entries) {
        if(log.m_term == args.prev_log_term && log.m_idx == args.prev_log_idx) {
            return log.m_idx;
        }
    }
}

void Raft::logSerialize(const std::vector<LogEntry>& input, std::string& output) {
    LogsMessage logsMessage;
    for (const auto entry : input) {
        LogEntryMessage* logEntryProto = logsMessage.add_entries();
        logEntryProto->set_m_term(entry.m_term);
        logEntryProto->set_m_command(entry.m_command);
        logEntryProto->set_m_idx(entry.m_idx);
    }
    if (!logsMessage.SerializeToString(&output)) {
        throw std::runtime_error("Failed to serialize log entries.");
    }
}
void Raft::logDeserialize(const std::string& input, std::vector<LogEntry>& output) {
    LogsMessage logsMessage;
    if (!logsMessage.ParseFromString(input)) {
        throw std::runtime_error("Failed to parse log entries.");
    }
    for(auto log : logsMessage.entries()) {
        output.emplace_back(log.m_term(), log.m_command(), log.m_idx());
    }
}