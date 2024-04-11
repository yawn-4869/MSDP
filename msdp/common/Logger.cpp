#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include "Logger.h"
#include "Config.h"
#include "EventLoop.h"

std::string LogLevelToString(LogLevel& level)
{
	switch(level)
	{
	case Debug:
		return "DEBUG";
	case Info:
		return "INFO";
	case Error:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}
LogLevel StringToLogLevel(const std::string& log_level)
{
	if(log_level == "DEBUG")
	{
		return Debug;
	}
	else if(log_level == "INFO")
	{
		return Info;
	}
	else if(log_level == "ERROR")
	{
		return Error;
	}
	else
	{
		return Unknown;
	}
}

AsyncLogger::AsyncLogger(std::string file_path, std::string file_name, int max_log_size)
	: m_file_path(file_path), m_file_name(file_name), m_max_log_size(max_log_size) {
	pthread_mutex_init(&m_mutex, NULL);

	sem_init(&m_sem, 0, 0);
	pthread_create(&m_thread, NULL, &AsyncLogger::Loop, this);
	sem_wait(&m_sem);
}

AsyncLogger::~AsyncLogger() {
	if(m_file_handler) {
		fclose(m_file_handler);
		m_file_handler = NULL;
	}

	pthread_mutex_destroy(&m_mutex);
}

void* AsyncLogger::Loop(void* args) {
	AsyncLogger* async_logger = reinterpret_cast<AsyncLogger*>(args);
	pthread_cond_init(&async_logger->m_cond, NULL);
	sem_post(&async_logger->m_sem);

	while(1) {
		pthread_mutex_lock(&async_logger->m_mutex);
		while(async_logger->m_buffer.empty()) {
			pthread_cond_wait(&async_logger->m_cond, &async_logger->m_mutex);
		}

		// printf("pthread_cond_wait end, AsyncLogger loop start\n");
		std::vector<std::string> tmp;
		tmp.swap(async_logger->m_buffer.front());
		async_logger->m_buffer.pop();
		pthread_mutex_unlock(&async_logger->m_mutex);

		// 获取当前日期
        time_t now;
        time(&now);
        struct tm now_time;
        localtime_r(&now, &now_time);
        char date[32];
        strftime(date, 32, "%Y%m%d", &now_time);

        // 判断日志文件是否需要重新被打开: 更换日期?超出大小?文件打开失败?
        if(std::string(date) != async_logger->m_date) {
            async_logger->m_reopen_flag = true;
            async_logger->m_date = std::string(date);
            async_logger->m_no = 0;
        }

        if (async_logger->m_file_handler == NULL) {
            async_logger->m_reopen_flag = true;
        }

        std::stringstream ss;
        ss << async_logger->m_file_path << async_logger->m_file_name << "_"
        << async_logger->m_date;
        
        std::string log_file_name = ss.str() + std::to_string(async_logger->m_no) + ".log";

        if(async_logger->m_reopen_flag) {
            if(async_logger->m_file_handler) {
                fclose(async_logger->m_file_handler);
            }
            async_logger->m_file_handler = fopen(log_file_name.c_str(), "a");
            async_logger->m_reopen_flag = false;
        }
        
        if(ftell(async_logger->m_file_handler) >= async_logger->m_max_log_size) {
            fclose(async_logger->m_file_handler);
            log_file_name = ss.str() + std::to_string(async_logger->m_no++) + ".log";
            async_logger->m_file_handler = fopen(log_file_name.c_str(), "a");

            async_logger->m_reopen_flag = false;
        }

        // 写入日志文件
        for(auto &msg : tmp) {
            if(!msg.empty()) {
                fwrite(msg.c_str(), 1, msg.length(), async_logger->m_file_handler);
            }
        }
        fflush(async_logger->m_file_handler);

        // 输出完毕后判断是否需要停止
        if(async_logger->m_stop_flag) {
            return NULL;
        }
    }
    return NULL;
}

void AsyncLogger::pushLogToBuffer(std::vector<std::string>& vec) {
	pthread_mutex_lock(&m_mutex);
	m_buffer.push(vec);
	pthread_mutex_unlock(&m_mutex);

	// printf("pushLogToBuffer, pthread_cond_signal\n");
	pthread_cond_signal(&m_cond);
}

void AsyncLogger::stop() {
	m_stop_flag = true;
}

void AsyncLogger::flush() {
	if(m_file_handler) {
		fflush(m_file_handler);
	}
}

static Logger* g_logger = NULL;
Logger* Logger::GetInstance() {
	return g_logger;
}

void Logger::InitGlobalLogger() {
    std::string global_log_level_str = Config::get_instance()->m_log_level;
	int global_log_close = Config::get_instance()->m_close_log;
	LogLevel global_log_level = StringToLogLevel(global_log_level_str);
    g_logger = new Logger(global_log_level, global_log_close);
	g_logger->init();
}

Logger::Logger(LogLevel& level, int close_log) : m_set_level(level), m_close_log(close_log) {
	
}

void Logger::pushCommonLog(const std::string msg) {
    // printf(msg.c_str());
	pthread_mutex_lock(&m_common_buffer_mutex);
    m_common_buffer.push_back(msg);
	pthread_mutex_unlock(&m_common_buffer_mutex);
}

void Logger::pushAppLog(const std::string msg) {
    // printf(msg.c_str());
	pthread_mutex_lock(&m_app_buffer_mutex);
    m_app_buffer.push_back(msg);
	pthread_mutex_unlock(&m_app_buffer_mutex);
}

void Logger::asyncLoop() {
	std::vector<std::string> tmp;
	pthread_mutex_lock(&m_common_buffer_mutex);
	tmp.swap(m_common_buffer);
	pthread_mutex_unlock(&m_common_buffer_mutex);

	if(!tmp.empty()) {
		m_common_logger->pushLogToBuffer(tmp);
	}

	// printf("push to common log %d\n", tmp.size());

	tmp.clear();
	pthread_mutex_lock(&m_app_buffer_mutex);
	tmp.swap(m_app_buffer);
	pthread_mutex_unlock(&m_app_buffer_mutex);

	if(!tmp.empty()) {
		m_app_logger->pushLogToBuffer(tmp);
	}

	// printf("push to app log %d\n", tmp.size());
}

void Logger::init() {
	pthread_mutex_init(&m_common_buffer_mutex, NULL);
	pthread_mutex_init(&m_app_buffer_mutex, NULL);
	m_common_logger = std::make_shared<AsyncLogger>(Config::get_instance()->m_common_log_file, 
													Config::get_instance()->m_log_file_name, 
													Config::get_instance()->m_log_max_file_size);
	m_app_logger = std::make_shared<AsyncLogger>(Config::get_instance()->m_fusion_log_file, 
												 Config::get_instance()->m_log_file_name, 
													Config::get_instance()->m_log_max_file_size);
	m_timer_event = std::make_shared<TimerEvent>(Config::get_instance()->m_async_log_interval, true, 
												std::bind(&Logger::asyncLoop, this));
	EventLoop::GetCurrentEventloop()->addTimerEvent(m_timer_event);
}

void Logger::log() {
	// std::unique_lock<std::mutex> lck(m_mutex);
	// std::queue<std::string> tmp;
    // m_buffer.swap(tmp);
	// lck.unlock();

    // while(!tmp.empty()) {
    //     std::string msg = tmp.front();
    //     tmp.pop();

    //     // TODO: 输出到终端要改为输出到日志文件
    //     printf(msg.c_str());
    // }
}

std::string LoggerEvent::toString() {
    // 时间戳字符串
    struct timeval now_time;
    gettimeofday(&now_time, nullptr);
    struct tm now_time_t;
    localtime_r(&(now_time.tv_sec), &now_time_t);
    char buf[128];
    strftime(&buf[0], sizeof(buf), "%y-%m-%d %H:%M:%S", &now_time_t);
    std::string time_str(buf);
    int ms = now_time.tv_usec / 1000;
    time_str = time_str + "." + std::to_string(ms);

    // 进程与线程id
	m_tid = syscall(SYS_gettid);
	m_pid = getpid();
    std::stringstream ss;
    ss << "[" << LogLevelToString(m_level) << "]\t" 
        << "[" << time_str << "]\t"
        << "[" << m_pid << ":" << m_tid << "]\t";
    
    return ss.str();
}