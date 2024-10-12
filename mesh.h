#ifndef __MESH_H__
#define __MESH_H__

#include <asio.hpp>

using asio::ip::tcp;

/* 服务器配置信息 */
struct MeshInfo
{
    int port = 1883;
    int debug_alive_time = 5;

    bool init(const char file[]);
};
extern MeshInfo mesh_info;

/* Session类：管理单个客户端的TCP连接 */
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket)
        : socket_(std::move(socket)) {}
    void start()
    {
        do_read(); // 开始读取数据
    } // 开始会话的公共接口

private:
    tcp::socket socket_;                // 与客户端的连接套接字
    static const int max_length = 1024; // 最大读取长度
    char data_[max_length];             // 数据缓冲区

    void do_read();                    // 开始异步读取数据
    void do_write(std::size_t length); // 开始异步发送数据
};

/* Server类管理服务器的操作，接受新的连接 */
class Server
{
public:
    Server(asio::io_context &io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        do_accept();
    }

private:
    tcp::acceptor acceptor_;

    void do_accept();
};

#endif // __MESH_H__