/*
    Took from https://github.com/ProfessorX737/ErrorLogging/blob/master/ErrorLogging/logging.h
*/
#pragma once 
#include <string>
#include <sstream>

enum severity_level {
	INFO = 0,
	WARNING = 1,
	PROBLEM = 2,
	FATAL = 3,
	NUM_SEVERITIES = 4
};

static const char* severity_names[NUM_SEVERITIES] = { "INFO","WARNING","PROBLEM","FATAL" };

class log_message : public std::ostringstream {
public:
	log_message(severity_level severity, const char* file_name, const char* func_name, int line);
	~log_message();
protected:
	void print_log_message();
private:
	severity_level severity;
	const char* file_name;
	const char* func_name;
	int line;
};

class log_message_fatal : public log_message {
public:
	log_message_fatal(const char* file_name, const char* func_name, int line);
	~log_message_fatal();
};

#define FHTTP_LOG_INFO log_message(INFO,__FILE__,__FUNCTION__,__LINE__).flush()
#define FHTTP_LOG_WARNING log_message(WARNING,__FILE__,__FUNCTION__,__LINE__).flush()
#define FHTTP_LOG_PROBLEM log_message(PROBLEM,__FILE__,__FUNCTION__,__LINE__).flush()
#define FHTTP_LOG_FATAL log_message_fatal(__FILE__,__FUNCTION__,__LINE__).flush()
#define FHTTP_LOG(severity) FHTTP_LOG_##severity

#define FHTTP_CHECK(expr) \
	if(!expr) log_message_fatal(__FILE__,__FUNCTION__,__LINE__).flush() << "Check failed: " << #expr << " "

#define FHTTP_CHECK_OP(val1,op,val2) \
	if(!(val1 op val2)) log_message_fatal(__FILE__,__FUNCTION__,__LINE__).flush() << "Check failed: " << #val1 << " " << #op << " " << #val2 << " "

#define FHTTP_CHECK_EQ(val1,val2) CHECK_OP(val1,==,val2)
#define FHTTP_CHECK_NE(val1,val2) CHECK_OP(val1,!=,val2)
#define FHTTP_CHECK_LE(val1,val2) CHECK_OP(val1,<=,val2)
#define FHTTP_CHECK_LT(val1,val2) CHECK_OP(val1,<,val2)
#define FHTTP_CHECK_GE(val1,val2) CHECK_OP(val1,>=,val2)
#define FHTTP_CHECK_GT(val1,val2) CHECK_OP(val1,>,val2)
#define FHTTP_CHECK_NOT_NULL(val) CHECK_NE(val,nullptr)
#define FHTTP_CHECK_POW2(val) \
	FHTTP_CHECK((val != 0) && ((val & (val - 1)) == 0))

#ifndef NDEBUG

#define FHTTP_DCHECK(expr) FHTTP_CHECK(expr)
#define FHTTP_DCHECK_EQ(val1,val2) FHTTP_CHECK_OP(val1,==,val2)
#define FHTTP_DCHECK_NE(val1,val2) FHTTP_CHECK_OP(val1,!=,val2)
#define FHTTP_DCHECK_LE(val1,val2) FHTTP_CHECK_OP(val1,<=,val2)
#define FHTTP_DCHECK_LT(val1,val2) FHTTP_CHECK_OP(val1,<,val2)
#define FHTTP_DCHECK_GE(val1,val2) FHTTP_CHECK_OP(val1,>=,val2)
#define FHTTP_DCHECK_GT(val1,val2) FHTTP_CHECK_OP(val1,>,val2)
#define FHTTP_DCHECK_NOT_NULL(val) FHTTP_CHECK_NE(val,nullptr)
#define FHTTP_DCHECK_POW2(val) \
	FHTTP_CHECK((val != 0) && ((val & (val - 1)) == 0))

#else 

#define FHTTP_DUD_STREAM if(false) FHTTP_LOG(FATAL)

#define FHTTP_DCHECK(expr) DUD_STREAM
#define FHTTP_DCHECK_EQ(val1,val2) DUD_STREAM
#define FHTTP_DCHECK_NE(val1,val2) DUD_STREAM 
#define FHTTP_DCHECK_LE(val1,val2) DUD_STREAM 
#define FHTTP_DCHECK_LT(val1,val2) DUD_STREAM 
#define FHTTP_DCHECK_GE(val1,val2) DUD_STREAM 
#define FHTTP_DCHECK_GT(val1,val2) DUD_STREAM 
#define FHTTP_DCHECK_NOT_NULL(val) DUD_STREAM 
#define FHTTP_DCHECK_POW2(val) DUD_STREAM

#endif

// ==================== logging.cpp ======================
#include <stdio.h>
#include <chrono>
#include <iomanip>
#include <ctime>

std::string get_current_time() {
    std::stringstream ss;
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm now_tm = *std::localtime(&now_time_t);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S") << '.' 
              << std::setw(3) << std::setfill('0') << now_ms.count();
    return ss.str();
}

log_message::log_message(severity_level severity, const char* file_name, const char* func_name, int line) :
	severity(severity), file_name(file_name), func_name(func_name), line(line) {
}

log_message::~log_message() {
	print_log_message();
}

void log_message::print_log_message() {
	fprintf(stderr, "%s %s: %s '%s' {%s:%d}\n", get_current_time().c_str(), severity_names[severity], func_name, str().c_str(), file_name, line);
}

log_message_fatal::log_message_fatal(const char* file_name, const char* func_name, int line) :
	log_message(FATAL, file_name, func_name, line) {
}

log_message_fatal::~log_message_fatal() {
	print_log_message();
	abort();
}