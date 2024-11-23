#ifndef __TRIETREE_H__
#define __TRIETREE_H__

#include <set>
#include <vector>
#include "mqtt_client.h"
#include "mqtt_protocol.h"
#include "main.h"

class TrieNode
{
private:
    const char *pat;
    int pat_len;

public:
    std::vector<TrieNode*> children;
    TrieNode *arbitrary;    // 单层符 + 的节点

    std::set<client_t*> clients; // 准确匹配的客户端，如果到这里结尾了就要发送消息
    std::set<client_t*> clients_wildcard; // # 通配符匹配的客户端，前缀能匹配到这里就要发送消息

    TrieNode(const char *data, int len): pat(data), pat_len(len), children(), clients(), clients_wildcard(), arbitrary(nullptr){}
    ~TrieNode(){
        for (int i = 0; i < children.size(); i++)
            delete children[i];
    }
    TrieNode *insert(const char *data, const int len){  // 插入一个节点，或者返回已有的节点
        for (int i = 0; i < children.size(); i++)
            if (children[i]->match(data, len))
                return children[i];
        if (len == 1 && data[0] == '+'){    // 单层符 + 和任何字符都视为匹配
            if (arbitrary == nullptr)
                arbitrary = new TrieNode(data, len);
            return arbitrary;
        }
        TrieNode *node = new TrieNode(data, len);
        children.push_back(node);
        return node;
    }

    TrieNode *getchild(const char *data, const int len){    // 获取除了单层符 + 以外的子节点
        for (int i = 0; i < children.size(); i++)
            if (children[i]->match(data, len))
                return children[i];
        return nullptr;
    }

    bool match(const char *data, int len){
        // if (pat_len == 1 && pat[0] == '+')  // 单层符 + 和任何字符都视为匹配
        //     return true; 
        // 好像现在不需要了，我在getchild里面单独处理，这样少一些无效遍历
        // 不对了
        // TODO 这里有bug，当成暂不支持 + 通配符
        if (pat_len != len)
            return false;
        return memcmp(pat, data, len) == 0;
    }
    void subscribe(client_t *client, bool istree = false){
        if (istree)
            clients_wildcard.insert(client);
        else
            clients.insert(client);
    }
};

class TopicTree
{
private:
    TrieNode *root;
public:
    TopicTree(): root(new TrieNode("", 0)){}
    
    int subscribe(client_t *client, char *topic, int len){
        int last = 0;
        TrieNode *node = root;
        for (int i = 0; i < len; i++){
            if (topic[i] == '/'){
                if (i - last == 1 && topic[last] == '#')    // 格式错误，#只能在最后
                    return MQTT_ERROR;
                node = node->insert(topic + last, i - last);
                last = i + 1;
            }
        }
        if (len - last == 1 && topic[last] == '#') // # 通配符订阅
            node->subscribe(client, true);
        else{
            // "xxx/" 不等于 "xxx"
            node = node->insert(topic + last, len - last);
            node->subscribe(client);
        }
        return MQTT_OK;
    }

    int publish(const char *topic, const int & topic_len, const char *message, const int message_len, int DUP = 0, int QoS = 0, int RETAIN = 0){
        std::set<client_t*> subscribers; // 订阅者集合
        int last = 0;

        std::vector<TrieNode*> nodes[2];    // 交替做一个二级队列 
        uint8_t op = 0;
        nodes[op].push_back(root);  

        for (int i = 0; i < topic_len; i++){
            if (topic[i] == '/'){
                if (i - last == 1 && topic[last] == '#')    // 格式错误，#只能在最后
                    return MQTT_ERROR;
                nodes[op^1].clear();
                for (auto it = nodes[op].begin(); it != nodes[op].end(); it++){
                    subscribers.insert((*it)->clients_wildcard.begin(), (*it)->clients_wildcard.end());

                    TrieNode *node = (*it)->getchild(topic + last, i - last);
                    
                    if (node != nullptr)
                        nodes[op^1].push_back(node);
                    // if ((*it)->arbitrary != nullptr){
                    //     nodes[op^1].push_back((*it)->arbitrary);    // 单层符 + 匹配
                    //     subscribers.insert((*it)->arbitrary->clients_wildcard.begin(), (*it)->arbitrary->clients_wildcard.end());
                    // }
                }
                op ^= 1;
                last = i + 1;
            }
        }
        for (auto it = nodes[op].begin(); it != nodes[op].end(); it++){
            subscribers.insert((*it)->clients.begin(), (*it)->clients.end());

            TrieNode *node = (*it)->getchild(topic + last, topic_len - last);
            
            subscribers.insert(node->clients.begin(), node->clients.end()); // 准确匹配
            if (node != nullptr)
                subscribers.insert(node->clients_wildcard.begin(), node->clients_wildcard.end());   // # 通配符 + 匹配
            // if ((*it)->arbitrary != nullptr)
            //     subscribers.insert((*it)->arbitrary->clients_wildcard.begin(), (*it)->arbitrary->clients_wildcard.end());
        }

        send_t send_buffer;
        send_buffer.insert(0x30 | (DUP << 3) | (QoS << 1) | RETAIN);
        // 剩余长度 = 2 + topic_len + message_len
        LengthDecoder length_decoder;
        int len;
        uint8_t *length = length_decoder.write(2 + topic_len + message_len, len);
        send_buffer.insert(length, len);
        delete length;

        send_buffer.insert(topic_len >> 8);
        send_buffer.insert(topic_len & 0xFF);
        send_buffer.insert((uint8_t*)topic, topic_len);
        send_buffer.insert((uint8_t*)message, message_len);

        for (auto it = subscribers.begin(); it != subscribers.end(); it++)
            if (send_buffer.send((*it)->fd) == MQTT_ERROR){ // 针对这个发送失败了
                fprintf(stderr, "发送消息给客户端 %d 失败\n", (*it)->fd);
                client_remove((*it)->fd);
            }
        return MQTT_OK;
    }
};

extern TopicTree topic_tree;

#endif