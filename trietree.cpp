#include "mqtt_protocol.h"
#include "main.h"
#include "mqtt_client.h"
#include "trietree.h"

TopicTree topic_tree;

/**
 * 函数功能：把一个主题字符串加入TopicTree
 * 传入参数：client_t *client, char *topic, int len
 * 返回参数：是否成功
 * 备注：    插入一个节点，或者返回已有的节点
 */
TrieNode *TrieNode::insert(const char *data, const int len)
{ 
    for (int i = 0; i < children.size(); i++)
        if (children[i]->match(data, len))
            return children[i];
    if (len == 1 && data[0] == '+')
    { // 单层符 + 和任何字符都视为匹配
        if (arbitrary == nullptr)
            arbitrary = new TrieNode(data, len);
        return arbitrary;
    }
    TrieNode *node = new TrieNode(data, len);
    children.push_back(node);
    return node;
}

/**
 * 函数功能：获取对应主题的子节点，如果没有则返回nullptr
 * 传入参数：const char *data, const int len
 * 返回参数：TrieNode *
 * 备注：    获取除了单层符 + 以外的子节点
 */
TrieNode *TrieNode::getchild(const char *data, const int len)
{ 
    for (int i = 0; i < children.size(); i++)
        if (children[i]->match(data, len))
            return children[i];
    return nullptr;
}

/**
 * 函数功能：判断节点是否与对应的主题串匹配
 * 传入参数：const char *data, int len
 * 返回参数：是否匹配
 */
bool TrieNode::match(const char *data, int len)
{
    // if (pat_len == 1 && pat[0] == '+')  // 单层符 + 和任何字符都视为匹配
    //     return true;
    // 好像现在不需要了，我在getchild里面单独处理，这样少一些无效遍历
    // 不对了
    // TODO 这里有bug，当成暂不支持 + 通配符
    if (pat_len != len)
        return false;
    return memcmp(pat, data, len) == 0;
}

/**
 * 函数功能：把一个客户端加入到节点的订阅者集合中
 * 传入参数：client_t *client, bool istree
 * 返回参数：无
 * 备注：    istree为真表示是#通配符订阅
 *          同时会添加到client_t的nodes中，方便删除
 */
void TrieNode::subscribe(client_t *client, bool istree)
{
    if (istree){
        clients_wildcard.insert(client);
        client->nodes.push_back(&clients_wildcard);
    }
    else{
        clients.insert(client);
        client->nodes.push_back(&clients);
    }
}

/**
 * 函数功能：把一个主题字符串加入TopicTree
 * 传入参数：client_t *client, char *topic, int len
 * 返回参数：是否成功
 */
int TopicTree::subscribe(client_t *client, char *topic, int len)
{
    int last = 0;
    TrieNode *node = root;
    for (int i = 0; i < len; i++)
    {
        if (topic[i] == '/')
        {
            if (i - last == 1 && topic[last] == '#') // 格式错误，#只能在最后
                return MQTT_ERROR;
            node = node->insert(topic + last, i - last);
            last = i + 1;
        }
    }
    if (len - last == 1 && topic[last] == '#') // # 通配符订阅
        node->subscribe(client, true);
    else
    {
        // "xxx/" 不等于 "xxx"
        node = node->insert(topic + last, len - last);
        node->subscribe(client);
    }
    return MQTT_OK;
}


/**
 * 函数功能：分发主题消息
 * 传入参数：const char *topic, const int &topic_len, const char *message, const int message_len, client_t *publisher, int DUP, int QoS, int RETAIN
 * 返回参数：是否成功
 * 备注：    会把消息发送给所有订阅了这个主题的客户端
 */
int TopicTree::publish(const char *topic, const int &topic_len, const char *message, const int message_len, client_t *publisher, int DUP, int QoS, int RETAIN)
{
    // fprintf(stderr, "查找订阅者....\n");
    std::set<client_t *> subscribers; // 订阅者集合
    int last = 0;

    std::vector<TrieNode *> nodes[2]; // 交替做一个二级队列
    uint8_t op = 0;
    nodes[op].push_back(root);

    for (int i = 0; i < topic_len; i++)
    {
        if (topic[i] == '/')
        {
            if (i - last == 1 && topic[last] == '#') // 格式错误，#只能在最后
                return MQTT_ERROR;
            nodes[op ^ 1].clear();
            for (auto it = nodes[op].begin(); it != nodes[op].end(); it++)
            {
                subscribers.insert((*it)->clients_wildcard.begin(), (*it)->clients_wildcard.end());

                TrieNode *node = (*it)->getchild(topic + last, i - last);

                if (node != nullptr)
                    nodes[op ^ 1].push_back(node);
                // if ((*it)->arbitrary != nullptr){
                //     nodes[op^1].push_back((*it)->arbitrary);    // 单层符 + 匹配
                //     subscribers.insert((*it)->arbitrary->clients_wildcard.begin(), (*it)->arbitrary->clients_wildcard.end());
                // }
            }
            op ^= 1;
            last = i + 1;
        }
    }
    for (auto it = nodes[op].begin(); it != nodes[op].end(); it++)
    {
        subscribers.insert((*it)->clients.begin(), (*it)->clients.end());

        TrieNode *node = (*it)->getchild(topic + last, topic_len - last);

        if (node != nullptr)
        {
            subscribers.insert(node->clients.begin(), node->clients.end());                   // 准确匹配
            subscribers.insert(node->clients_wildcard.begin(), node->clients_wildcard.end()); // # 通配符 + 匹配
        }

        // if ((*it)->arbitrary != nullptr)
        //     subscribers.insert((*it)->arbitrary->clients_wildcard.begin(), (*it)->arbitrary->clients_wildcard.end());
    }

    fprintf(stderr, "找到 %d 个订阅者\n", (int)subscribers.size());
    for (auto it = subscribers.begin(); it != subscribers.end(); it++)
        fprintf(stderr, "\t订阅者 %d\n", (*it)->fd);

    // fprintf(stderr, "准备消息....\n");
    send_t send_buffer;
    send_buffer.insert(0x30 | (DUP << 3) | (QoS << 1) | RETAIN);
    // 剩余长度 = 2 + topic_len + message_len
    LengthDecoder length_decoder;
    int len;
    uint8_t *length = length_decoder.write(2 + topic_len + message_len, len);
    send_buffer.insert(length, len);
    free(length);

    send_buffer.insert(topic_len >> 8);
    send_buffer.insert(topic_len & 0xFF);
    send_buffer.insert((uint8_t *)topic, topic_len);
    send_buffer.insert((uint8_t *)message, message_len);

    // fprintf(stderr, "准备完成，发送消息....\n");

    for (auto it = subscribers.begin(); it != subscribers.end(); it++)
    {
        // fprintf(stderr, "发送消息给客户端 %d ...\n", (*it)->fd);
        if ((*it) != publisher && send_buffer.send((*it)->fd) == MQTT_ERROR)
        { // 针对这个发送失败了
            // fprintf(stderr, "发送消息给客户端 %d 失败\n", (*it)->fd);
            client_remove((*it)->fd);
        }
        // fprintf(stderr, "发送消息给客户端 %d 完成\n", (*it)->fd);
    }
    fprintf(stderr, "发送完成\n");
    return MQTT_OK;
}