#include <random>
#include <google/protobuf/service.h>
#include <rocket/net/rpc/rpc_channel.h>
#include <rocket/net/rpc/rpc_controller.h>
#include <rocket/net/rpc/rpc_closure.h>
#include <rocket/common/log.h>
#include <rocket/common/error_code.h>
#include <rocket/net/tcp/net_addr.h>
#include <rocket/net/tcp/tcp_server.h>
#include "Raft.h"
#include "raft_server.h"

Raft::Raft(uint8_t id) : m_id(id) {
    // 定时器设定在200-400ms随机
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    m_election_timer_event = std::make_shared<TimerEvent>(std::rand() % 201 + 200, false, std::bind(&Raft::handleTimeout, this));
}

void Raft::handleTimeout() {
    if(m_role == ServerRole::FOLLOWER) {
        // follower超时, 成为candidate
        m_role = ServerRole::CANDIDATE;
        // 自增任期号, 给自己投票, 重置计时器, 发送vote request
        m_current_term++;
        m_voted_for = m_id;
        for(int peer_id : m_peers) {
            if(peer_id == m_id) {
                continue;
            }
            callRequestVote(peer_id);
        }

    } else if(m_role == ServerRole::CANDIDATE) {
        // candidate 超时
        // 重新发起选举
        for(int peer_id : m_peers) {
            if(peer_id == m_id) {
                continue;
            }
            callRequestVote(peer_id);
        }
    } else {
        // leader 超时

    }

    // 重置时间
    resetTimeout(m_election_thread->getEventLoop(), m_election_timer_event);
}

void Raft::callRequestVote(int peer_id) {
    NEWMESSAGE(::callVoteRequest, request);
    NEWMESSAGE(::callVoteResponse, response);

    request->set_term(m_current_term);
    request->set_candidate_id(m_id);
    request->set_last_log_idx(m_log_entries.size());
    if(!m_log_entries.empty()) {
        request->set_last_log_term(m_log_entries.back().getLogTerm());
    }

    NEWRPCCONTROLLER(controller);
    controller->SetTimeout(2000);
    std::shared_ptr<rocket::RpcClosure> closure = std::make_shared<rocket::RpcClosure>(nullptr, [this, request, response, controller]() mutable {
        if (controller->GetErrorCode() == 0) {
            INFOLOG("call vote request rpc success, request[%s], response[%s]", request->ShortDebugString().c_str(), response->ShortDebugString().c_str());
            if(response->term() > m_current_term) {
                // 当前服务器落后于其他服务器, 回到follower状态
                m_role = ServerRole::FOLLOWER;
                // 更新term
                m_current_term = response->term();
                // 初始化
                m_voted_for = -1;
            } else {
                if(response->vote_granted()) {
                    m_recv_votes++;
                }
            }
        } else {
            if(controller->GetErrorCode() == 7) {
                // rpc调用超时
                
            }
            ERRORLOG("call rpc failed, request[%s], error code[%d], error info[%s]", 
            request->ShortDebugString().c_str(), 
            controller->GetErrorCode(), 
            controller->GetErrorInfo().c_str());
        }
    });
    std::string addr = "127.0.0.1:4001";
    CALLRPRC(addr, RaftServer_Stub, call_vote, controller, request, response, closure);
}

void Raft::responseToRequest() {
    rocket::Config::SetGlobalConfig("../../conf/rocket.xml");
    rocket::Logger::InitGlobalLogger();
    rocket::RpcDispatcher::GetRpcDispatcher()->registerService(std::make_shared<raft_server::RaftServerImpl>());
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("", rocket::Config::GetGlobalConfig()->m_port);
    rocket::TcpServer tcp_server(addr);
    tcp_server.start();
    printf("exit\n");
}

void Raft::callRequestAppendLog(int peer_id) {

}

void Raft::resetTimeout(EventLoop* event_loop, TimerEvent::s_ptr timer_event) {
    event_loop->deleteTimerEvent(timer_event);
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    timer_event->resetInterval(std::rand() % 201 + 200);
    event_loop->addTimerEvent(timer_event);
}