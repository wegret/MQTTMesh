#include <iostream>
#include <fstream>
#include <string>

#include <yaml-cpp/yaml.h>
#include "mesh.h"

using std::cerr;
using std::cout;
using std::endl;
using std::string;

MeshInfo mesh_info;

/**
 * 函数功能: 服务器配置初始化
 * 传入参数: 无
 * 传出参数: 无
 * 备注：    读取config.yaml配置
 */
bool MeshInfo::init(const char file[])
{
    try
    {
        YAML::Node config = YAML::LoadFile(file);
        if (config["Mesh"])
        {
            if (config["Mesh"]["port"])
                mesh_info.port = config["Mesh"]["port"].as<int>();
            if (config["Mesh"]["debug_alive_time"])
                mesh_info.debug_alive_time = config["Mesh"]["debug_alive_time"].as<int>();
        }
    }
    catch (const YAML::Exception &e)
    {
        std::cerr << "Error reading YAML file: " << e.what() << std::endl;
    }
    std::cout << "Mesh port: " << mesh_info.port << std::endl;
    std::cout << "Debug alive time: " << mesh_info.debug_alive_time << std::endl;
    return true;
}

/**
 * 函数功能: Session类开始读取数据
 * 传入参数: 无
 * 传出参数: 无
 */
void Session::do_read()
{
    auto self(shared_from_this());
    socket_.async_read_some(asio::buffer(data_, max_length),
                            [this, self](std::error_code ec, std::size_t length)
                            {
                                if (!ec)
                                {
                                    cout << "Received: " << string(data_, length) << endl;
                                    do_write(length);
                                }
                                else
                                {
                                    cerr << "Error on receive: " << ec.message() << endl;
                                    socket_.close();
                                }
                            });
}

/**
 * 函数功能: Session类开始发送数据
 * 传入参数: 发送数据长度
 * 传出参数: 无
 */
void Session::do_write(std::size_t length)
{
    auto self(shared_from_this());
    asio::async_write(socket_, asio::buffer(data_, length),
                      [this, self](std::error_code ec, std::size_t /* length */)
                      {
                          if (!ec)
                          {
                              do_read();
                          }
                          else
                          {
                              cerr << "Error on send: " << ec.message() << endl;
                              socket_.close();
                          }
                      });
}

/**
 * 函数功能: Server类开始接受新的连接
 * 传入参数: 无
 * 传出参数: 无
 */
void Server::do_accept()
{
    acceptor_.async_accept(
        [this](std::error_code ec, tcp::socket socket)
        {
            if (!ec)
            {
                cout << "New connection from "
                     << socket.remote_endpoint().address().to_string()
                     << ":" << socket.remote_endpoint().port() << endl;
                cout.flush();
                std::make_shared<Session>(std::move(socket))->start();
            }
            else
            {
                cerr << "Accept error: " << ec.message() << endl;
            }
            do_accept();
        });
}