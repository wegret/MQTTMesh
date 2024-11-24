#ifndef __TRIETREE_H__
#define __TRIETREE_H__

#include <vector>
#include <set>
#include <stdlib.h>
#include <string.h>

class client_t;

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

    TrieNode(const char *data, int len): pat_len(len), clients(), clients_wildcard(), arbitrary(nullptr){
        pat = (const char*)(malloc(len));
        memcpy((void*)pat, data, len);
    }
    ~TrieNode(){
        for (int i = 0; i < children.size(); i++)
            delete children[i];
    }
    TrieNode *insert(const char *data, const int len);

    TrieNode *getchild(const char *data, const int len);

    bool match(const char *data, int len);
    void subscribe(client_t *client, bool istree = false);
};

class TopicTree
{
private:
    TrieNode *root;
public:
    TopicTree(): root(new TrieNode("", 0)){}
    
    int subscribe(client_t *client, char *topic, int len);

    int publish(const char *topic, const int & topic_len, const char *message, const int message_len, client_t *publisher , int DUP = 0, int QoS = 0, int RETAIN = 0);
};

extern TopicTree topic_tree;

#endif