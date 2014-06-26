/**
 * vnc2rdp: proxy for RDP client connect to VNC server
 *
 * Copyright 2014 Yiwei Li <leeyiw@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _V2R_LOG_H_
#define _V2R_LOG_H_

#include <errno.h>
#include <stdint.h>
#include <string.h>

#define V2R_LOG_MAX_LEN		2048

#define V2R_LOG_DEBUG		0
#define V2R_LOG_INFO		1
#define V2R_LOG_WARN		2
#define V2R_LOG_ERROR		3
#define V2R_LOG_FATAL		4

#ifndef NDEBUG
#define v2r_log_debug(...) \
	v2r_log(V2R_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#else
#define v2r_log_debug(...)
#endif

#define v2r_log_info(...) \
	v2r_log(V2R_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define v2r_log_warn(...) \
	v2r_log(V2R_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define v2r_log_error(...) \
	v2r_log(V2R_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define v2r_log_fatal(...) \
	v2r_log(V2R_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#define ERRMSG strerror(errno)

void v2r_log(uint8_t level, const char *file, int line, const char *fmt, ...);

#endif  // _V2R_LOG_H_
