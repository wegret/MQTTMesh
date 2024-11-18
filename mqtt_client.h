#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#include "main.h"
#include "mqtt_protocol.h"

class client_t
{
private:
    int fd; // 客户端文件描述符
    char ip[INET_ADDRSTRLEN];   // 客户端 IP 地址

    bool connect_init = false;  // 是否初始化（是否已经建立连接）

    Message_t msg_recv;  // 当前收到的消息    

public:
    client_t(int fd, const char *ip);
    
    int read(const uint8_t& ch);
};

#endif // __MQTT_CLIENT_H__