
#include "mqtt_client.h"

/**
 * 函数功能：构造client_t对象
 * 传入参数：int fd，const char *ip
 * 返回参数：无
 */
client_t::client_t(int fd, const char *ip)
{
    this->fd = fd;
    strncpy(this->ip, ip, INET_ADDRSTRLEN);
    this->connect_init = false;

    
}

/**
 * 函数功能：接收一个字符数据
 * 传入参数：const uint8_t ch
 * 返回参数：是否存在错误（按照mqtt协议，出现差错应该立即disconnect）
 *          1：存在错误
 */
int client_t::read(const uint8_t& ch){
    if (msg_recv.read(ch) == MQTT_ERROR)
        return MQTT_ERROR;
    // 一条消息结束了
    if (msg_recv.over){
        if (!connect_init){
            if (msg_recv.control_type != CONNECT)
                return MQTT_ERROR;
            connect_init = true;
            // TODO: 一些处理
            fprintf(stderr, "客户端 %s:%d 建立正确的MQTT Connection\n", ip, fd);
        }
        else{
            if (msg_recv.control_type == CONNECT)   // 禁止再次发送CONNECT
                return MQTT_ERROR;
            // TODO: 一些处理
        }

        msg_recv.clear();
    }
    
    return MQTT_OK;
}
