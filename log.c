/**
 * rdp2vnc: proxy for RDP client connect to VNC server
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

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

static const char *r2v_log_level_str[5] = {
	"DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

void r2v_log(uint8_t level, const char *file, int line, const char *fmt, ...)
{
	char log_data[R2V_LOG_MAX_LEN];
	char *ptr = log_data;
	time_t current_time = time(NULL);
	struct tm t;
	va_list ap;

	/* 获取当前时间 */
	localtime_r(&current_time, &t);
	/* 格式化日志头部 */
	ptr += snprintf(log_data, sizeof(log_data),
		"[%4d/%02d/%02d %02d:%02d:%02d] [%u] [%s] [%s:%d] ",
		t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min,
		t.tm_sec, getpid(), r2v_log_level_str[level], file, line);
	/* 添加日志信息 */
	va_start(ap, fmt);
	vsnprintf(ptr, sizeof(log_data) - (ptr - log_data), fmt, ap);
	va_end(ap);
	printf("%s\n", log_data);
}
