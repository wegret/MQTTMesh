#include <iostream>
#include <fstream>
#include <string>
#include <thread>

#include <asio.hpp>
#include <yaml-cpp/yaml.h>

#include "mesh.h"

int main()
{
    mesh_info.init("config.yaml"); // 读取配置文件

    try
    {
        asio::io_context io_context;               // ASIO的I/O上下文
        Server server(io_context, mesh_info.port); // 创建服务器实例，监听端口7776
        io_context.run();                          // 运行I/O上下文以处理所有异步事件
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl; // 捕获并输出异常信息
    }
    return 0; // 返回程序结束状态码
}
