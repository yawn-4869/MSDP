/*
==========================================================================
* 类Logger为本项目的日志封装，用于将日志信息输出到指定的文件中

* 单例模式实现
* 可选择同步日志或者异步日志输出
* 日志命名：日志文件前缀名_date_log_日志文件序号

==========================================================================
*/

#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdio.h>
#include <string>
#include <mutex>
#include <queue>
#include <memory>
#include <semaphore.h>
#include "TimerEvent.h"

template<typename... Args>
std::string formatString(const char* str, Args&&... args) {

  int size = snprintf(nullptr, 0, str, args...);

  std::string result;
  if (size > 0) {
    result.resize(size);
    snprintf(&result[0], size + 1, str, args...);
  }

  return result;
}

#define DEBUGLOG(str, ...) \
    if(!Logger::GetInstance()->isLogClose() && Logger::GetInstance()->getLogLevel() <= Debug){ \
		Logger::GetInstance()->pushCommonLog(LoggerEvent(LogLevel::Debug).toString() \
		+ "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" \
		+ formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

#define INFOLOG(str, ...) \
    if(!Logger::GetInstance()->isLogClose() && Logger::GetInstance()->getLogLevel() <= Info){ \
		Logger::GetInstance()->pushCommonLog(LoggerEvent(LogLevel::Info).toString() \
		+ "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" \
		+ formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

#define ERRORLOG(str, ...) \
    if(!Logger::GetInstance()->isLogClose() && Logger::GetInstance()->getLogLevel() <= Error){ \
		Logger::GetInstance()->pushCommonLog(LoggerEvent(LogLevel::Error).toString() \
		+ "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" \
		+ formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

#define APPDEBUGLOG(str, ...) \
	if(!Logger::GetInstance()->isLogClose() && Logger::GetInstance()->getLogLevel() <= Debug){ \
		Logger::GetInstance()->pushAppLog(LoggerEvent(LogLevel::Debug).toString() \
		+ "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" \
		+ formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

#define APPINFOLOG(str, ...) \
	if(!Logger::GetInstance()->isLogClose() && Logger::GetInstance()->getLogLevel() <= Info){ \
		Logger::GetInstance()->pushAppLog(LoggerEvent(LogLevel::Info).toString() \
		+ "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" \
		+ formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

#define APPERRORLOG(str, ...) \
	if(!Logger::GetInstance()->isLogClose() && Logger::GetInstance()->getLogLevel() <= Error){ \
		Logger::GetInstance()->pushAppLog(LoggerEvent(LogLevel::Error).toString() \
		+ "[" + std::string(__FILE__) + ":" + std::to_string(__LINE__) + "]\t" \
		+ formatString(str, ##__VA_ARGS__) + "\n"); \
    } \

enum LogLevel
{
	Unknown = 0,
	Debug = 1,
	Info = 2,
	Error = 3
};

std::string LogLevelToString(LogLevel& level);
LogLevel StringToLogLevel(const std::string& log_level);

class AsyncLogger {
public:
	typedef std::shared_ptr<AsyncLogger> s_ptr;
	AsyncLogger(std::string file_path, std::string file_name, int max_log_size);
	~AsyncLogger();
	void pushLogToBuffer(std::vector<std::string>& vec);
	void stop();
	void flush();

public:
	static void* Loop(void* args);

private:
	int m_no{ 0 }; // 日志序号
	FILE* m_file_handler{ NULL }; // 文件句柄
	bool m_reopen_flag{ false }; // 是否重新打开日志
	bool m_stop_flag{ false }; // 是否停止记录
	int m_max_log_size{ 0 }; // 最大容量
	std::string m_file_path; // 存储路径
	std::string m_file_name; // 存储文件名
	std::string m_date; // 日志日期
	std::queue<std::vector<std::string>> m_buffer;
	// 线程同步与互斥
	pthread_t m_thread;
	sem_t m_sem;
	pthread_mutex_t m_mutex;
	pthread_cond_t m_cond;
};

class Logger {
public:
	void pushCommonLog(std::string msg);
	void pushAppLog(const std::string msg);
	void log();
	LogLevel getLogLevel() {
		return m_set_level;
	}

	bool isLogClose() {
		return m_close_log == 1;
	}

private:
	void asyncLoop();
	void init();

public:
	static Logger* GetInstance();
	static void InitGlobalLogger();

private:
	Logger(LogLevel& level, int close_log);

private:
	int m_close_log; // 是否关闭日志 0 -- false 1--true
	LogLevel m_set_level; // 日志记录级别
	std::vector<std::string> m_common_buffer; // 程序日常运行日志缓冲队列
	std::vector<std::string> m_app_buffer; // 融合日志日志缓冲队列
	pthread_mutex_t m_common_buffer_mutex; // m_common_buffer访问互斥
	pthread_mutex_t m_app_buffer_mutex; // m_app_buffer访问互斥
	AsyncLogger::s_ptr m_app_logger; // 记录融合日志
	AsyncLogger::s_ptr m_common_logger; // 记录程序日常运行日志
	TimerEvent::s_ptr m_timer_event; // 定时器
};

class LoggerEvent {
public:
	LoggerEvent(LogLevel level) : m_level(level){}
	std::string toString();
private:
	std::string m_file_name; // 输出文件
    int32_t m_file_line; // 行号
    int32_t m_pid; // 进程号
    int32_t m_tid; // 线程号
    std::string m_timestamp; // 时间
	LogLevel m_level; // 日志事件级别
};

#endif
