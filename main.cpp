
#include "main.h"
#include "mqtt_client.h"

#define DEFAULT_PORT 7776   // 默认端口
#define MAX_EVENTS 10000    // 最大事件数
#define BUFFER_SIZE 1024    // 缓冲区大小

int set_nonblocking(int fd);    // 设置文件描述符为非阻塞

int server_port = DEFAULT_PORT; // 服务器端口

// int client_size = 0;

// int MAX_FD_SIZE;
// client_t** fd_client_ptr;   // f[fd] 指向 client 的指针
std::unordered_map<int, client_t*> fd_client_ptr;

void mesh_init(){
    // 改成哈希表好像没用了这段
    // // 查询最大文件标识符数目
    // struct rlimit rl;
    // if (getrlimit(RLIMIT_NOFILE, &rl) != 0)
    //     MAX_FD_SIZE = 256;  // 默认值
    // else
    //     MAX_FD_SIZE = std::min(static_cast<int>(rl.rlim_max), 1024);

    // fd_client_ptr = (client_t**)malloc(sizeof(client_t*) * MAX_FD_SIZE);
    // for (int i = 0; i < MAX_FD_SIZE; i++)
    //     fd_client_ptr[i] = nullptr;
}

void client_insert(int fd, const char* ip){
    client_t* clnt_ptr = new client_t(fd, ip);
    fd_client_ptr[fd] = clnt_ptr;
}

void client_remove(int fd){
    close(fd);
    if (fd_client_ptr[fd] != nullptr)
        delete fd_client_ptr[fd];
    fd_client_ptr.erase(fd);
}

int main(int argc, char *argv[])
{
    if (argc == 2)  // 如果重设端口
    {
        int expected_port = atoi(argv[1]);
        if (expected_port <= 0)
            fprintf(stderr, "Invalid port number. Using default port %d.\n", DEFAULT_PORT);
        else
            server_port = expected_port;
    }
    
    mesh_init();

    int server_sock;
    struct sockaddr_in server_addr;

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0)
    {
        perror("socket 创建失败");
        exit(EXIT_FAILURE);
    }

    // 设置套接字为非阻塞
    if (set_nonblocking(server_sock) < 0)
    {
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 允许地址重用
    int opt = 1;
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt 失败");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 配置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有接口

    // 绑定套接字
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind 失败");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 开始监听
    if (listen(server_sock, SOMAXCONN) < 0)
    {
        perror("listen 失败");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("服务器正在监听端口 %d ...\n", server_port);

    // 创建 epoll 实例
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1 失败");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    // 将监听套接字添加到 epoll 监控
    struct epoll_event event;
    event.data.fd = server_sock;
    event.events = EPOLLIN | EPOLLET; // 监听可读事件，使用边缘触发
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_sock, &event) == -1)
    {
        perror("epoll_ctl 添加监听套接字失败");
        close(server_sock);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // 分配事件数组，进行强制类型转换
    struct epoll_event *events = (struct epoll_event*)calloc(MAX_EVENTS, sizeof(struct epoll_event));
    if (!events)
    {
        perror("calloc 失败");
        close(server_sock);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1)
        {
            if (errno == EINTR)
                continue; // 被信号中断，继续等待
            perror("epoll_wait 失败");
            break;
        }

        for (int i = 0; i < n; i++)
        {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP) ||
                (!(events[i].events & EPOLLIN)))
            {
                // 出现错误，关闭连接
                fprintf(stderr, "epoll 事件错误，关闭 fd %d\n", events[i].data.fd);
                client_remove(events[i].data.fd);
                continue;
            }
            else if (events[i].data.fd == server_sock)
            {
                // 处理所有新连接
                while (1)
                {
                    struct sockaddr_in client_addr;
                    socklen_t client_addr_len = sizeof(client_addr);
                    int client_fd = accept(server_sock, (struct sockaddr *)&client_addr, &client_addr_len);
                    if (client_fd == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // 所有连接都已处理
                            break;
                        }
                        else
                        {
                            perror("accept 失败");
                            break;
                        }
                    }

                    if (set_nonblocking(client_fd) < 0) // 设置客户端套接字为非阻塞
                    {
                        close(client_fd);
                        continue;
                    }

                    // 将新客户端套接字添加到 epoll 监控
                    event.data.fd = client_fd;
                    event.events = EPOLLIN | EPOLLET;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
                    {
                        perror("epoll_ctl 添加客户端套接字失败");
                        close(client_fd);
                        continue;
                    }

                    // 打印客户端连接信息
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                    printf("接受到连接来自 %s:%d, fd=%d\n", client_ip, ntohs(client_addr.sin_port), client_fd);
                    
                    client_insert(client_fd, client_ip);
                }
                continue;
            }
            else
            {
                // 处理客户端数据
                int client_fd = events[i].data.fd;
                char buffer[BUFFER_SIZE];
                while (1)
                {
                    ssize_t count = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
                    if (count == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // 所有数据都已读取
                            break;
                        }
                        else
                        {
                            perror("recv 失败");
                            client_remove(client_fd);
                            break;
                        }
                    }
                    else if (count == 0)
                    {
                        // 客户端关闭连接
                        printf("客户端 fd=%d 断开连接\n", client_fd);

                        client_remove(client_fd);

                        break;
                    }
                    else
                    {
                        // 处理接收到的数据
                        buffer[count] = '\0'; // 确保字符串终止
                        printf("收到来自 fd=%d 的数据: %s\n", client_fd, buffer);

                        for (int i = 0; i < count; i++){
                            if (fd_client_ptr[client_fd]->read(buffer[i]) == MQTT_ERROR){
                                fprintf(stderr, "MQTT 协议错误，关闭 fd %d\n", client_fd);
                                client_remove(client_fd);
                                break;
                            }
                        }

                        // // 回发消息给客户端
                        // int bytes_sent = send(client_fd, buffer, count, 0);
                        // if (bytes_sent < 0) {
                        //     perror("发送消息失败");
                        //     client_remove(client_fd);
                        // } else {
                        //     printf("回发消息给 fd=%d: %s\n", client_fd, buffer);
                        // }
                    }
                }
            }
        }
    }

    // 清理资源
    free(events);
    close(server_sock);
    close(epoll_fd);

    return 0;
}

/**
 * 函数功能：设置文件描述符为非阻塞
 * 输入参数：fd 文件描述符
 * 返回参数：成功返回 0，失败返回 -1
 */
int set_nonblocking(int fd)
{
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) == -1)
    {
        perror("fcntl F_GETFL");
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        perror("fcntl F_SETFL");
        return -1;
    }
    return 0;
}