#include <arpa/inet.h>
#include "buttonrpc.hpp"
#include "Config.h"
#include "Logger.h"
#include "Server.h"
#include "Raft.h"

// int main() 
// {
// 	// // 初始化配置类和日志类
//     Config::setGlobalConfig("../conf/settings.json");
// 	Logger::InitGlobalLogger();

//     // Server fusion_server;
//     // fusion_server.start();

//     Raft raft_server(Config::get_instance()->m_server_id);
//     raft_server.start();

// 	return 0;
// }

#define COMMOM_PORT 5001

// int main(int argc, char* argv[]) {
//     if (argc < 2) {
//         printf("loss parameter of peersNum\n");
//         exit(-1);
//     }
//     int peersNum = atoi(argv[1]);
//     if (peersNum % 2 == 0) {
//         printf("the peersNum should be odd\n");
//         exit(-1);
//     }
//     srand((unsigned)time(NULL));
//     vector<PeersInfo> peers(peersNum);
//     for (int i = 0; i < peersNum; i++) {
//         peers[i].m_peerId = i;
//         peers[i].m_port.first = COMMOM_PORT + i;
//         peers[i].m_port.second = COMMOM_PORT + i + peers.size();
//         printf(" id : %d port1 : %d, port2 : %d\n", peers[i].m_peerId, peers[i].m_port.first, peers[i].m_port.second);
//     }

//     Raft* raft = new Raft[peers.size()];
//     for(int i = 0; i < peers.size(); i++){
//         raft[i].Make(peers, i);
//     }

//     while(1) {
//         usleep(400000);
//         for (int i = 0; i < peers.size(); i++) {
//             if (raft[i].getState().second) {
//                 printf("current server %d send log append\n", i);
//                 for (int j = 0; j < 100; j++) {
//                     Operation opera;
//                     opera.op = "put"; opera.key = to_string(j); opera.value = to_string(j);
//                     raft[i].start(opera);
//                     usleep(50000);
//                 }
//             } else continue;
//         }
//         usleep(400000);
//         int killed_raft = -1;
//         for (int i = 0; i < peers.size(); i++) {
//             if (raft[i].getState().second) {
//                 raft[i].kill();
//                 killed_raft = i;
//                 break;
//             }
//         }
//         usleep(400000);
//         raft[killed_raft].activate();
//     }


//     // while (1);
// }


