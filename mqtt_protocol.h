#ifndef __MQTT_PROTOCOL_H__
#define __MQTT_PROTOCOL_H__

#include "stdint.h"
#include <string>

#define MQTT_OK 0    // 正常
#define MQTT_ERROR 1 // 发生错误

#define SIZE_MORE (5)

typedef enum ReturnCode
{
    ACCEPTED = 0,
    UNACCEPTABLE_PROTOCOL_VERSION = 1,
    IDENTIFIER_REJECTED = 2,
    SERVER_UNAVAILABLE = 3,
    BAD_USERNAME_OR_PASSWORD = 4,
    NOT_AUTHORIZED = 5
} ReturnCode;

/* 首部控制符 */
typedef enum ControlType
{
    RESERVED = 0,
    CONNECT = 1,
    CONNACK = 2,
    PUBLISH = 3,
    PUBACK = 4,
    PUBREC = 5,
    PUBREL = 6,
    PUBCOMP = 7,
    SUBSCRIBE = 8,
    SUBACK = 9,
    UNSUBSCRIBE = 10,
    UNSUBACK = 11,
    PINGREQ = 12,
    PINGRESP = 13,
    DISCONNECT = 14
} ControlType;

/* 长度解码器 */
struct LengthDecoder
{
    int multipler; // 乘数
    int value;     // 临时值

    LengthDecoder() : multipler(1), value(0) {}

    // 返回-1就是还没有解码成功，返回-2是发生错误了
    // 返回正常的非负整数就是解码成功
    int read(const uint8_t &ch)
    {
        value += (ch & 0x7F) * multipler;
        multipler *= 128;
        if (multipler > 128 * 128 * 128)
            return -2;
        if (ch & 0x80)
            return -1;
        return value;
    }
    void clear(void)
    {
        multipler = 1;
        value = 0;
    }
};

/* 一条MQTT消息 */
struct Message_t
{
    ControlType control_type; // 控制包类型

    int remaining_length;         // 剩余长度（应该不会爆int吧，小概率）
    LengthDecoder length_decoder; // 长度解码器

    uint8_t DUP, QoS, RETAIN; // 标志位
    uint8_t id;               // 消息ID（）

    bool over; // 这个信息是否已经结束了

    uint8_t *buffer;    // 缓冲区（可变报头+有效载荷）
    int buffer_size;    // 缓冲区大小（后面会扩大，只增不减）
    int buffer_len;     // 是实际上的可变报头+有效载荷的长度，在写入的时候不会变
    uint8_t *write_idx; // 写指针

    Message_t() : control_type(RESERVED), remaining_length(0), over(false), buffer(nullptr), buffer_size(0), buffer_len(0), write_idx(nullptr) {}
    ~Message_t()
    {
        if (buffer != nullptr)
            free(buffer);
    }

    int read(const uint8_t &ch);
    void clear(void);
};

bool isvalid_clientid(const char *client_id, uint16_t len);

char* generate_clientid(void);

#endif