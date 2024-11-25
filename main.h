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

#endif // __MAIN_H__