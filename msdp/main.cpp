#include <arpa/inet.h>
#include "Config.h"
#include "Logger.h"
#include "Server.h"

int main() 
{
	// // 初始化配置类和日志类
    Config::setGlobalConfig("../conf/settings.json");
	Logger::InitGlobalLogger();

    Server fusion_server;
    fusion_server.start();

	return 0;
}