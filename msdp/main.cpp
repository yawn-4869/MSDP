#include <arpa/inet.h>
#include "buttonrpc.hpp"
#include "Config.h"
#include "Logger.h"
#include "Server.h"
#include "Raft.h"

int main() 
{
	// // 初始化配置类和日志类
    Config::setGlobalConfig("../conf/settings.json");
	Logger::InitGlobalLogger();

    // Server fusion_server;
    // fusion_server.start();

    Raft raft_server(Config::get_instance()->m_server_id);
    

	return 0;
}