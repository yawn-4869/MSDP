#include "Raft.h"
#include "utils.h"
#include "Config.h"

void Raft::init() {
    Config* g_config = Config::get_instance();
    m_id = g_config->m_server_id;

    for(int i = 0; i < g_config->m_server_count; ++i) {
        m_peers.emplace_back(4001, 5001, i);
        m_next_index.push_back(1);
        m_match_index.push_back(0);
    }

    m_last_wake = getNowMs();
    
    pthread_t listen_tid1;
    pthread_t listen_tid2;
    pthread_t listen_tid3;
    pthread_create(&listen_tid1, NULL, listenForVote, this);
    pthread_detach(listen_tid1);
    pthread_create(&listen_tid2, NULL, listenForAppend, this);
    pthread_detach(listen_tid2);
}

int64_t Raft::getMyDuration(int64_t last) {
    return getNowMs() - last;
}

void Raft::setBroadcastTime() {
    m_last_broadcast -= 200;
}

void* Raft::listenForVote(void* arg) {
    Raft* raft = static_cast<Raft*>(arg);
    buttonrpc server;
    // TODO: vote端口
    server.as_server(4001);
    server.bind("replyToVote", &Raft::replyToVote, raft);

    pthread_t wait_tid;
    pthread_create(&wait_tid, NULL, electionLoop, raft);
    pthread_detach(wait_tid);

    server.run();
    printf("exit\n");
}

void* Raft::listenForAppend(void* arg){
    Raft* raft = (Raft*)arg;
    buttonrpc server;
    // TODO: log append端口
    server.as_server(5001);
    server.bind("replyToAppend", &Raft::replyToAppend, raft);
    pthread_t heart_tid;
    pthread_create(&heart_tid, NULL, processEntriesLoop, raft);
    pthread_detach(heart_tid);

    server.run();
    printf("exit!\n");
}

void* Raft::electionLoop(void* arg) {
    Raft* raft = (Raft*)arg;
    bool reset_flg = false;

    while(!raft->m_is_dead) {
        int time_out = rand() % 200 + 200;
        while(1) {
            raft->m_lock.lock();
            int64_t duration = raft->getMyDuration(raft->m_last_wake);
            
            if(raft->m_state == FOLLOWER && duration > time_out) {
                raft->m_state = CANDIDATE;
            }

            if(raft->m_state == CANDIDATE && duration > time_out) {
                printf(" %d attempt election at term %d, time_out is %d\n", raft->m_id, raft->m_cur_term, time_out);
                raft->m_last_wake = getNowMs();
                reset_flg = true;

                raft->m_cur_term++;
                raft->m_voted_for = raft->m_id;
                // raft->saveRaftState();
                raft->m_recv_votes = 1;
                raft->m_finished_vote = 1;
                raft->m_cur_peer_id = 0;
                pthread_t tid[raft->m_peers.size() - 1];
                int i = 0;
                for(auto server : raft->m_peers){
                    if(server.m_id == raft->m_id) continue;
                    pthread_create(tid + i, NULL, callRequestVote, raft);
                    pthread_detach(tid[i]);
                    i++;
                }

                while(raft->m_recv_votes <= raft->m_peers.size() / 2 && raft->m_finished_vote != raft->m_peers.size()){
                    raft->m_cond.wait(raft->m_lock.getMutex());
                }

                if(raft->m_state != CANDIDATE){
                    raft->m_lock.unlock();
                    continue;
                }

                if(raft->m_recv_votes > raft->m_peers.size() / 2) {
                    raft->m_state = LEADER;

                    for(int i = 0; i < raft->m_peers.size(); i++){
                        // raft->m_next_index[i] = raft->lastIndex() + 1;
                        raft->m_next_index[i] = raft->m_logs.size() + 1;
                        raft->m_match_index[i] = 0;
                    }

                    printf(" %d become new leader at term %d\n", raft->m_id, raft->m_cur_term);
                    raft->setBroadcastTime();
                }
            }

            raft->m_lock.unlock();
            if(reset_flg){
                reset_flg = false;
                break;
            }
        }
    }
}

void* Raft::callRequestVote(void* arg) {
    Raft* raft = static_cast<Raft*>(arg);
    buttonrpc client;
    raft->m_lock.lock();
    RequestVoteArgs args;
    args.candidateId = raft->m_id;
    args.term = raft->m_cur_term;
    args.lastLogIndex = raft->m_logs.size();
    args.lastLogTerm = raft->m_logs.empty() ? 0 : raft->m_logs.back().m_term;
    if(raft->m_cur_peer_id == raft->m_id) {
        raft->m_cur_peer_id++;
    }

    int client_peer_id = raft->m_cur_peer_id;
    // TODO: rpc组播地址, vote 端口
    client.as_client("239.0.0.64", 4001);

    if(raft->m_cur_peer_id == raft->m_peers.size() || 
        (raft->m_cur_peer_id == raft->m_peers.size() - 1 && raft->m_id == raft->m_cur_peer_id)) {
        raft->m_cur_peer_id = 0;
    }

    raft->m_lock.unlock();

    RequestVoteReply reply = client.call<RequestVoteReply>("replyToVote", args).val();
    raft->m_lock.lock();
    raft->m_finished_vote++;
    raft->m_cond.signal();
    if(reply.term > raft->m_cur_term){
        raft->m_state = FOLLOWER;
        raft->m_cur_term = reply.term;
        raft->m_voted_for = -1;
        raft->m_lock.unlock();
        return NULL;
    }

    if(reply.VoteGranted){
        raft->m_recv_votes++;
    }

    raft->m_lock.unlock();
}

bool Raft::checkLogUptodate(int term, int index){
    int last_term = m_logs.empty() ? 0 : m_logs.back().m_term;

    if(term > last_term){
        return true;
    }

    if(term == last_term && index >= m_logs.size()){
        return true;
    }

    return false;
}

RequestVoteReply Raft::replyToVote(RequestVoteArgs args) {
    RequestVoteReply reply;
    reply.VoteGranted = false;

    if(m_is_dead) {
        return reply;
    }

    m_lock.lock();
    reply.term = m_cur_term;

    if(m_cur_term > args.term) {
        m_lock.unlock();
        return reply;
    }

    if(m_cur_term < args.term) {
        m_state = FOLLOWER;
        m_cur_term = args.term;
        m_voted_for = -1;
    }

    if(m_voted_for == -1 || m_voted_for == args.candidateId) {
        if(!checkLogUptodate(args.lastLogTerm, args.lastLogIndex)) {
            m_lock.unlock();
            return reply;
        }

        m_voted_for = args.candidateId;
        reply.VoteGranted = true;
        printf("[%d] vote to [%d] at %d, duration is %d\n", m_id, m_voted_for, m_cur_term, getMyDuration(m_last_wake));
        m_last_wake = getNowMs();
    }
    m_lock.unlock();
    return reply;
}

void* Raft::processEntriesLoop(void* arg) {
    Raft* raft = static_cast<Raft*>(arg);

    while(!raft->m_is_dead) {
        usleep(1000);
        raft->m_lock.lock();
        if(raft->m_state != LEADER) {
            raft->m_lock.unlock();
            continue;
        }

        int during_time = raft->getMyDuration(raft->m_last_broadcast);
        // TODO: 心跳包设置：100ms
        if(during_time < 100) {
            raft->m_lock.unlock();
            continue;
        }

        raft->m_last_broadcast = getNowMs();

        pthread_t tid[raft->m_peers.size() - 1];
        int i = 0;
        for(auto& server : raft->m_peers){
            if(server.m_id == raft->m_id) continue;
            // if(raft->m_next_index[server.m_id] <= raft->m_lastIncludedIndex){        //进入install分支的条件，日志落后于leader的快照
            //     printf("%d send install rpc to %d, whose nextIdx is %d, but leader's lastincludeIdx is %d\n", 
            //         raft->m_peerId, server.m_peerId, raft->m_nextIndex[server.m_peerId], raft->m_lastIncludedIndex);
            //     server.isInstallFlag = true;
            //     pthread_create(tid + i, NULL, sendInstallSnapShot, raft);
            //     pthread_detach(tid[i]);
            // }else{
                // printf("%d send append rpc to %d, whose nextIdx is %d, but leader's lastincludeIdx is %d\n", 
                    // raft->m_peerId, server.m_peerId, raft->m_nextIndex[server.m_peerId], raft->m_lastIncludedIndex);
            pthread_create(tid + i, NULL, sendAppendEntries, raft);
            pthread_detach(tid[i]);
            // }
            i++;
        }
        raft->m_lock.unlock();
    }
}

void* Raft::sendAppendEntries(void* arg) {
    Raft* raft = static_cast<Raft*>(arg);

    buttonrpc client;
    AppendEntriesArgs args;
    int clientPeerId;
    raft->m_lock.lock();

    for(int i = 0; i < raft->m_peers.size(); i++) {
        if(raft->m_peers[i].m_id == raft->m_id) continue;
        if(raft->m_is_exist.count(i)) continue;
        clientPeerId = i;
        raft->m_is_exist.insert(i);
        // printf("%d in append insert index : %d, size is %d\n", raft->m_peerId, i, raft->isExistIndex.size());
        break;
    }

    // TODO: LogAppend rpc 组播地址 组播端口
    client.as_client("239.0.0.64", 5001);
    // printf("%d send to %d's append port is %d\n", raft->m_peerId, clientPeerId, raft->m_peers[clientPeerId].m_port.second);

    if(raft->m_is_exist.size() == raft->m_peers.size() - 1){
        // printf("append clear size is %d\n", raft->isExistIndex.size());
        // for(int i = 0; i < raft->m_peers.size(); i++){
        //     raft->m_peers[i].isInstallFlag = false;
        // }
        raft->m_is_exist.clear();
    }
    
    args.m_term = raft->m_cur_term;
    args.m_leaderId = raft->m_id;
    args.m_prevLogIndex = raft->m_next_index[clientPeerId] - 1;
    args.m_leaderCommit = raft->m_commit_index;

    for(int i = args.m_prevLogIndex + 1; i < raft->m_logs.size(); i++){
        args.m_sendLogs += (raft->m_logs[i].m_command + "," + to_string(raft->m_logs[i].m_term) + ";");
    }


    //用作自己调试可能，因为如果leader的m_prevLogIndex为0，follower的size必为0，自己调试直接赋日志给各个server看选举情况可能需要这段代码
    // if(args.m_prevLogIndex == 0){
    //     args.m_prevLogTerm = 0;
    //     if(raft->m_logs.size() != 0){
    //         args.m_prevLogTerm = raft->m_logs[0].m_term;
    //     }
    // }
    args.m_prevLogTerm = raft->m_logs[args.m_prevLogIndex-1].m_term;

    // printf("[%d] -> [%d]'s prevLogIndex : %d, prevLogTerm : %d\n", raft->m_peerId, clientPeerId, args.m_prevLogIndex, args.m_prevLogTerm); 
    
    raft->m_lock.unlock();
    AppendEntriesReply reply = client.call<AppendEntriesReply>("replyToAppend", args).val();

    raft->m_lock.lock();
    if(raft->m_cur_term != args.m_term){
        raft->m_lock.unlock();
        return NULL;
    }
    if(reply.m_term > raft->m_cur_term){
        raft->m_state = FOLLOWER;
        raft->m_cur_term = reply.m_term;
        raft->m_voted_for = -1;
        raft->m_lock.unlock();
        return NULL;
    }

    if(reply.m_success){
        raft->m_next_index[clientPeerId] = args.m_prevLogIndex + raft->getCmdAndTerm(args.m_sendLogs).size() + 1;  //可能RPC调用完log又增加了，但那些是不应该算进去的，不能直接取m_logs.size() + 1
        raft->m_match_index[clientPeerId] = raft->m_next_index[clientPeerId] - 1;
        raft->m_match_index[raft->m_id] = raft->m_logs.size();

        vector<int> tmpIndex = raft->m_match_index;
        sort(tmpIndex.begin(), tmpIndex.end());
        int realMajorityMatchIndex = tmpIndex[tmpIndex.size() / 2];
        if(realMajorityMatchIndex > raft->m_commit_index) {
            raft->m_commit_index = realMajorityMatchIndex;
        }
    }

    if(!reply.m_success) {
        if(reply.m_conflict_term != -1 && reply.m_conflict_term != -100) {
            int leader_conflict_index = -1;
            for(int index = args.m_prevLogIndex; index > 0; index--) {
                if(raft->m_logs[index-1].m_term == reply.m_conflict_term) {
                    leader_conflict_index = index;
                    break;
                }
            }

            if(leader_conflict_index != -1) {
                raft->m_next_index[clientPeerId] = leader_conflict_index + 1;
            } else {
                raft->m_next_index[clientPeerId] = reply.m_conflict_index; //这里加不加1都可，无非是多一位还是少一位，此处指follower对应index为空
            }
        } else {
            if(reply.m_conflict_term == -100) {

            }
            //-------------------很关键，运行时不能注释下面这段，因为我自己调试bug强行增加bug，没有专门的测试程序-----------------
            else raft->m_next_index[clientPeerId] = reply.m_conflict_index;
        }
        
    }
    raft->m_lock.unlock();
}

AppendEntriesReply Raft::replyToAppend(AppendEntriesArgs args) {
    vector<LogEntry> recv_log = getCmdAndTerm(args.m_sendLogs);
    AppendEntriesReply reply;
    m_lock.lock();
    reply.m_term = m_cur_term;
    reply.m_success = false;
    reply.m_conflict_index = -1;
    reply.m_conflict_term = -1;

    if(args.m_term < m_cur_term) {
        m_lock.unlock();
        return reply;
    }

    if(args.m_term >= m_cur_term) {
        if(args.m_term > m_cur_term){
            m_voted_for = -1;
        }
        m_cur_term = args.m_term;
        m_state = FOLLOWER;
    }

    m_last_wake = getNowMs();

    if(m_is_dead) {
        reply.m_conflict_term = -100;
        m_lock.unlock();
        return reply;
    }

    int last_index = m_logs.size();
    if(last_index < args.m_prevLogIndex){        
        reply.m_conflict_index = last_index + 1;    
        printf(" [%d]'s logs.size : %d < [%d]'s prevLogIdx : %d, ret conflict idx is %d\n", 
            m_id, last_index, args.m_leaderId, args.m_prevLogIndex, reply.m_conflict_index);
        m_lock.unlock();
        return reply;
    }

    if(m_logs[args.m_prevLogIndex - 1].m_term != args.m_prevLogTerm) {
        printf(" [%d]'s prevLogterm : %d != [%d]'s prevLogTerm : %d\n", m_id, m_logs[args.m_prevLogIndex - 1].m_term, args.m_leaderId, args.m_prevLogTerm);

        reply.m_conflict_term = m_logs[args.m_prevLogIndex - 1].m_term;
        for(int index = 1; index <= args.m_prevLogIndex; index++) {
            if(m_logs[index - 1].m_term == reply.m_conflict_term) {
                reply.m_conflict_index = index;
                break;
            }
        }
        m_lock.unlock();
        return reply;
    }

    for(int i = args.m_prevLogIndex; i < last_index; ++i) {
        m_logs.pop_back();
    }

    for(auto& log : recv_log) {
        m_logs.push_back(log);
    }

    if(m_commit_index < args.m_leaderCommit) {
        m_commit_index = min(args.m_leaderCommit, last_index);
    }
    m_lock.unlock();
    reply.m_success = true;
    return reply;
}

void Raft::kill() {
    m_is_dead = true;
    printf("raft%d is dead\n", m_id);
}
void Raft::activate() {
    m_is_dead = false;
    printf("raft%d is activate\n", m_id);
}

bool Raft::isDead() {
    return m_is_dead;
}

pair<int, bool> Raft::getState() {
    return {m_cur_term, m_state == LEADER};
}

vector<LogEntry> Raft::getCmdAndTerm(string text){
    vector<LogEntry> logs;
    int n = text.size();
    vector<string> str;
    string tmp = "";
    for(int i = 0; i < n; i++){
        if(text[i] != ';'){
            tmp += text[i];
        }else{
            if(tmp.size() != 0) str.push_back(tmp);
            tmp = "";
        }
    }
    for(int i = 0; i < str.size(); i++){
        tmp = "";
        int j = 0;
        for(; j < str[i].size(); j++){
            if(str[i][j] != ','){
                tmp += str[i][j];
            }else break;
        }
        string number(str[i].begin() + j + 1, str[i].end());
        int num = atoi(number.c_str());
        logs.push_back(LogEntry(tmp, num));
    }
    return logs;
}