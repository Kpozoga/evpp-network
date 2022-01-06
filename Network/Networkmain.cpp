// Network.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Network.h"
#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>


int nmain(int argc, char* argv[]) {
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(2, 2);
    int err = WSAStartup(wVersionRequested, &wsaData);
    if (0 != err) {
        return 0;
    }
    
    google::SetLogDestination(google::GLOG_INFO, "./glog/info/");
    google::SetLogDestination(google::GLOG_WARNING, "./glog/warning/");
    google::SetLogDestination(google::GLOG_ERROR, "./glog/error/");
    google::SetLogDestination(google::GLOG_FATAL, "./glog/fatal/");
    google::InitGoogleLogging(argv[0]);

    std::string addr = "0.0.0.0:9099";
    int thread_num = 4;
    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "TCPEchoServer", thread_num);
    server.SetMessageCallback([](const evpp::TCPConnPtr& conn,
        evpp::Buffer* msg) {
            std::cout << msg->ToString();
            conn->Send(msg);
        });
    server.SetConnectionCallback([](const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            LOG_INFO << "A new connection from " << conn->remote_addr();
            std::cout << "A new connection from " << conn->remote_addr();
        }
        else {
            LOG_INFO << "Lost the connection from " << conn->remote_addr();
            std::cout << "Lost the connection from " << conn->remote_addr();
        }
    });
    server.Init();
    server.Start();
    loop.Run();
    return 0;
}

