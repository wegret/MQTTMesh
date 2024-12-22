#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

#include <vector>
#include <stdint.h>

#include <sys/resource.h>
#include <unordered_map>

void client_remove(int fd);

#define TASK_ENABLE 1

#include <cstdio>
#include <cstdarg>
#include <string>
#include <unordered_map>

// ANSI 颜色代码
#define RESET_COLOR "\033[0m"
#define RED_COLOR "\033[31m"
#define GREEN_COLOR "\033[32m"
#define YELLOW_COLOR "\033[33m"
#define BLUE_COLOR "\033[34m"

// 辅助函数：根据前缀返回颜色代码
const char* getColorPrefix(const std::string& prefix);

// cfprintf 函数声明
void cfprintf(FILE* stream, const char* format, ...);


#endif // __MAIN_H__