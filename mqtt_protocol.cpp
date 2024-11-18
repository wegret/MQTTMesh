#include "mqtt_protocol.h"

static uint8_t fixed_header_vis[] = {0, 0, 0, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0};   // 必须匹配的标志位，除了PUBLISH需要特判

/**
 * 函数功能：接收一个字符数据
 * 传入参数：const uint8_t ch
 * 返回参数：是否存在错误（按照mqtt协议，出现差错应该立即disconnect）
 *          1：存在错误
 */ 
int Message_t::read(const uint8_t& ch){
    if (remaining_length == 0)  // 还没有计算出剩余长度，固定头没处理结束
    {
        if (control_type == 0){ // 类型尚未确定
            control_type = (ControlType) (ch >> 4);
            if (!(CONNECT<=control_type && control_type<=DISCONNECT))   // 不是合法的控制类型
                return MQTT_ERROR;
            
            // 检查标志位
            if (control_type == PUBLISH){   // PUBLISH，需要特判
                DUP = (ch & 0x08) >> 3;
                QoS = (ch & 0x06) >> 1;
                RETAIN = ch & 0x01;

                if (QoS == 3)   // QoS=3是非法的
                    return MQTT_ERROR;
            }
            else if ((ch & 0x0F) != fixed_header_vis[control_type])
                return MQTT_ERROR;
            return MQTT_OK;
        }
        // 类型已经确定，开始计算剩余长度
        int res = length_decoder.read(ch);
        if (res == -2)  // 发生错误
            return MQTT_ERROR;
        if (res >= 0)   // 解码成功
        {
            remaining_length = buffer_len = res;

            if (remaining_length > buffer_size){    // 扩容
                buffer_size = remaining_length;
                buffer = (uint8_t*)realloc(buffer, buffer_size);
                if (buffer == nullptr)
                    return MQTT_ERROR;
            }
            write_idx = buffer;
        }
    }
    else    // 剩余长度存在，是可变头部或者有效载荷
    {
        remaining_length--;
        *write_idx++ = ch;  // 这里先不处理，只读取完数据包

        if (remaining_length == 0) // 一个数据包结束了
            over = true, *write_idx = '\0';
    }
    return MQTT_OK;
}

/**
 * 函数功能：结束了一个数据包，清空并等待下一个
 * 传入参数：无
 * 返回参数：无
 */
void Message_t::clear(){
    control_type = RESERVED;
    remaining_length = 0;
    length_decoder.clear();
    DUP = QoS = RETAIN = 0;
    id = 0;
    over = false;
    if (buffer != nullptr)
        free(buffer);
    buffer = nullptr;
    buffer_size = buffer_len = 0;
    write_idx = nullptr;
}