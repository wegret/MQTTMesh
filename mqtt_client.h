#ifndef __MQTT_CLIENT_H__
#define __MQTT_CLIENT_H__

#include "main.h"
#include <vector>
#include "mqtt_protocol.h"
#include "trietree.h"

class TrieNode;

class client_t
{
private:
    char ip[INET_ADDRSTRLEN]; // 客户端 IP 地址

    bool connect_init = false; // 是否初始化（是否已经建立连接）
    uint16_t keep_alive;       // 保持连接时间

    Message_t msg_recv; // 当前收到的消息

    char *username = nullptr; // 用户名
    char *password = nullptr; // 密码（暂时没打算实现）

public:
    char *clientid; // 客户端ID
    int fd;                   // 客户端文件描述符
    std::vector<std::set<client_t*>*> nodes;
    
    client_t(int fd, const char *ip);

    ~client_t(){
        if (username != nullptr)
            free(username);
        if (password != nullptr)
            free(password);
        if (clientid != nullptr)
            free(clientid);
        fprintf(stderr, "客户端 %s 断开连接，正在清空订阅...\n", clientid);
        int cnt=0;
        for (auto it = nodes.begin(); it != nodes.end(); it++){
            (*it)->erase(this);
            cnt++;
        }
        fprintf(stderr, "清空 %d 个订阅完成\n", cnt);
        
        free(clientid);
    }

    int read(const uint8_t &ch);

    int send(ControlType control_type, const uint8_t *buffer = nullptr, int buffer_len = 0);
};

class send_t 
{
private:
    uint8_t* buffer;
    int buffer_size;    // 当前空间长度
    int buffer_len;     // 当前数据长度

public:
    send_t(){
        buffer_size = 64;
        buffer = (uint8_t*)malloc(buffer_size);
        buffer_len = 0;
    }
    ~send_t(){
        if (buffer != nullptr)
            free(buffer);
        buffer = nullptr;
    }
    void clear(){
        buffer_len = 0;
    }
    void insert(uint8_t ch){
        if (buffer_len == buffer_size){
            buffer_size = buffer_size * 2 + 1;  // 2倍扩容
            buffer = (uint8_t*)realloc(buffer, buffer_size);
        }
        buffer[buffer_len++] = ch;
    }
    void insert(const uint8_t* data, int len){
        if (buffer_len + len > buffer_size){
            buffer_size = (buffer_len + len) * 2 + 1;  // 2倍扩容
            buffer = (uint8_t*)realloc(buffer, buffer_size);
        }
        memcpy(buffer + buffer_len, data, len);
        buffer_len += len;
    }
    int send(const int& fd){
        int bytes_send = ::send(fd, buffer, buffer_len, 0);
        if (bytes_send < 0){
            perror("send 失败");
            return MQTT_ERROR;
        }
        return MQTT_OK;
    }
};

#endif // __MQTT_CLIENT_H__