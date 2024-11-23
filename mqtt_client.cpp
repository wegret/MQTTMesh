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
            /* CONNECT */
            connect_init = true;

            if (msg_recv.buffer_len  < 10){    // payload小于0了
                fprintf(stderr, "CONNECT剩余长度错误\n");
                return MQTT_ERROR;
            }

            static uint8_t variable_header[] = {0, 4, 'M', 'Q', 'T', 'T', 4};    // CONNECT的可变头部的协议和等级部分
            if (memcmp(msg_recv.buffer, variable_header, 7) != 0){
                fprintf(stderr, "CONNECT可变包头协议错误\n");
                fprintf(stderr, "收到的协议：%d\n", msg_recv.buffer[6]);
                return MQTT_ERROR;
            }
            uint8_t connect_flags = msg_recv.buffer[7]; // 连接标志
            keep_alive = (msg_recv.buffer[8] << 8) + msg_recv.buffer[9]; // 保持连接时间

            // TODO: flags的处理

            uint8_t *payload = msg_recv.buffer + 10;
            int payload_len = msg_recv.buffer_len - 10;
            if (payload_len < 2){
                fprintf(stderr, "CONNECT payload长度错误\n");
                return MQTT_ERROR;
            }
            uint16_t client_id_len = (payload[0] << 8) + payload[1];
            if (client_id_len == 0){
                // TODO 必须同时将清理会话标志设置为1 
            //    fprintf(stderr, "CONNECT client_id长度为0\n");
                clientid = generate_clientid();
            }
            else{
                if (payload_len < 2 + client_id_len){
                    fprintf(stderr, "CONNECT client_id长度错误\n");
                    return MQTT_ERROR;
                }
                if (!isvalid_clientid((const char*)(payload + 2), client_id_len)){
                    fprintf(stderr, "CONNECT client_id不合法\n");
                    // TODO: 这里应该发一个0x02的CONNACK
                    return MQTT_ERROR;
                }
                clientid = (char*)(payload + 2);
            }
            fprintf(stderr, "客户端 %s:%d 建立正确的MQTT Connection\n", ip, fd);

            if (send(CONNACK) == MQTT_ERROR)// 发送连接确认包
                return MQTT_ERROR;   
        }
        else{
            if (msg_recv.control_type == CONNECT)   // 禁止再次发送CONNECT
                return MQTT_ERROR;
            else if (msg_recv.control_type == PUBLISH){ // 发布消息
                uint8_t *buffer = msg_recv.buffer;
                int buffer_len = msg_recv.buffer_len;

                uint8_t *topic; // 主题
                int topic_len;
                uint8_t *payload;   // 有效载荷
                int payload_len;
                
                if (buffer_len < 2){
                    fprintf(stderr, "PUBLISH长度错误\n");
                    return MQTT_ERROR;
                }
                topic_len = (msg_recv.buffer[0] << 8) + msg_recv.buffer[1];
                if (buffer_len < 2 + topic_len){
                    fprintf(stderr, "PUBLISH主题长度错误\n");
                    return MQTT_ERROR;
                }
                topic = buffer + 2;

                if (msg_recv.QoS > 0){  // QoS > 0
                    // TODO: QoS > 0的处理
                    // if (buffer_len < 2 + topic_len + 2){
                    //     fprintf(stderr, "PUBLISH消息ID长度错误\n");
                    //     return MQTT_ERROR;
                    // }
                    // msg_recv.id = (buffer[2 + topic_len] << 8) + buffer[2 + topic_len + 1];
                    // payload = buffer + 2 + topic_len + 2;
                    // payload_len = buffer_len - 2 - topic_len - 2;
                }
                else{
                    payload = buffer + 2 + topic_len;
                    payload_len = buffer_len - 2 - topic_len;
                }
                // TODO: 要把消息发送给所有订阅了这个主题的客户端
                std::string topic_str(reinterpret_cast<char*>(topic), topic_len);
                std::string message_str(reinterpret_cast<char*>(payload), payload_len);
                fprintf(stderr, "客户端 %s:%d 发布主题 %s 消息 %s\n", ip, fd, topic_str.c_str(), message_str.c_str());
            }
            else
                // TODO: 一些处理
                ;
        }

        msg_recv.clear();
    }
    return MQTT_OK;
}

/**
 * 函数功能：发送一个数据包
 * 传入参数：ControlType control_type 控制类型
 *          const uint8_t *buffer, int buffer_len
 * 返回参数：是否发送成功
 */
int client_t::send(ControlType control_type, const uint8_t *buffer, int buffer_len){
    send_t send_buffer;
    /* 连接确认 */
    if (control_type == CONNACK){
        send_buffer.insert(0x20);    // 固定头部
        send_buffer.insert(2);       // 剩余长度

        if (buffer == nullptr || buffer_len != 2){
            send_buffer.insert(0);       // 连接确认标志
            send_buffer.insert(ACCEPTED);       // 返回码
        }
        else{
            send_buffer.insert(buffer[0]);       // 连接确认标志
            send_buffer.insert(buffer[1]);       // 返回码
        }
    }
    else
        return MQTT_ERROR;
    return send_buffer.send(fd);
}