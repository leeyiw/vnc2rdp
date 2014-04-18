#ifndef _R2V_LOG_H_
#define _R2V_LOG_H_

#include <errno.h>
#include <stdint.h>
#include <string.h>

#define R2V_LOG_MAX_LEN		2048

#define R2V_LOG_DEBUG		0
#define R2V_LOG_INFO		1
#define R2V_LOG_WARN		2
#define R2V_LOG_ERROR		3
#define R2V_LOG_FATAL		4

#define r2v_log_debug(...) \
	r2v_log(R2V_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define r2v_log_info(...) \
	r2v_log(R2V_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define r2v_log_warn(...) \
	r2v_log(R2V_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define r2v_log_error(...) \
	r2v_log(R2V_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define r2v_log_fatal(...) \
	r2v_log(R2V_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#define ERRMSG strerror(errno)

void r2v_log(uint8_t level, const char *file, int line, const char *fmt, ...);

#endif  // _R2V_LOG_H_
