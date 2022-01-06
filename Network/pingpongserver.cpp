#include <evpp/tcp_server.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

void OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->SetTCPNoDelay(true);
        printf("connected \n");
    }
}

void OnMessage(const evpp::TCPConnPtr& conn,
    evpp::Buffer* msg) {
    conn->Send(msg);
}

int psmain(int argc, char* argv[]) {
    std::string addr = "0.0.0.0:9099";
    int thread_num = 4;

    if (argc != 1 && argc != 3) {
        printf("Usage: %s <port> <thread-num>\n", argv[0]);
        printf("  e.g: %s 9099 12\n", argv[0]);
        return 0;
    }

    if (argc == 3) {
        addr = std::string("0.0.0.0:") + argv[1];
        thread_num = atoi(argv[2]);
    }
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

    evpp::EventLoop loop;
    evpp::TCPServer server(&loop, addr, "TCPPingPongServer", thread_num);
    server.SetMessageCallback(&OnMessage);
    server.SetConnectionCallback(&OnConnection);
    server.Init();
    server.Start();
    loop.Run();
    return 0;
}
