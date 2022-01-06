// Network.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "Network.h"
#include <evpp/tcp_client.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>


int main(int argc, char* argv[]) {
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

    std::string addr = "127.0.0.1:9099";

    if (argc == 2) {
        addr = argv[1];
    }

    evpp::EventLoop loop;
    evpp::TCPClient client(&loop, addr, "TCPPingPongClient");
    client.SetMessageCallback([&loop, &client](const evpp::TCPConnPtr& conn,
        evpp::Buffer* msg) {
            LOG_WARN << "Receive a message [" << msg->ToString() << "]";
            std::cout << "Receive a message [" << msg->ToString() << "]" << std::endl;
            //client.Disconnect();
        });

    client.SetConnectionCallback([](const evpp::TCPConnPtr& conn) {
        if (conn->IsConnected()) {
            LOG_WARN << "Connected to " << conn->remote_addr();
            conn->Send("hello");
        }
        else {
            conn->loop()->Stop();
        }
    });
    client.Connect();
    loop.Run();
    return 0;
}

