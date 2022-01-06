// Modified from https://github.com/chenshuo/muduo/blob/master/examples/pingpong/client.cc

#include <evpp/tcp_client.h>
#include <evpp/event_loop_thread_pool.h>
#include <evpp/buffer.h>
#include <evpp/tcp_conn.h>

class Client;

class Session {
public:
    Session(evpp::EventLoop* loop,
        const std::string& serverAddr/*ip:port*/,
        const std::string& name,
        Client* owner)
        : client_(loop, serverAddr, name),
        owner_(owner),
        bytes_read_(0),
        bytes_written_(0),
        messages_read_(0) {
        client_.SetConnectionCallback(
            std::bind(&Session::OnConnection, this, std::placeholders::_1));
        client_.SetMessageCallback(
            std::bind(&Session::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    }

    void Start() {
        client_.Connect();
    }

    void Stop() {
        client_.Disconnect();
    }

    int64_t bytes_read() const {
        return bytes_read_;
    }

    int64_t messages_read() const {
        return messages_read_;
    }

private:
    void OnConnection(const evpp::TCPConnPtr& conn);

    void OnMessage(const evpp::TCPConnPtr& conn, evpp::Buffer* buf) {
        LOG_TRACE << "bytes_read=" << bytes_read_ << " bytes_writen=" << bytes_written_;
        ++messages_read_;
        bytes_read_ += buf->size();
        bytes_written_ += buf->size();
        conn->Send(buf);
    }

private:
    evpp::TCPClient client_;
    Client* owner_;
    int64_t bytes_read_;
    int64_t bytes_written_;
    int64_t messages_read_;
};

class Client {
public:
    Client(evpp::EventLoop* loop,
        const std::string& serverAddr, // ip:port
        int blockSize,
        int sessionCount,
        int timeout_sec,
        int threadCount)
        : loop_(loop),
        session_count_(sessionCount),
        timeout_(timeout_sec),
        connected_count_(0) {
        loop->RunAfter(evpp::Duration(double(timeout_sec)), std::bind(&Client::HandleTimeout, this));
        tpool_.reset(new evpp::EventLoopThreadPool(loop, threadCount));
        tpool_->Start(true);

        for (int i = 0; i < blockSize; ++i) {
            message_.push_back(static_cast<char>(i % 128));
        }

        for (int i = 0; i < sessionCount; ++i) {
            char buf[32];
            snprintf(buf, sizeof buf, "C%05d", i);
            Session* session = new Session(tpool_->GetNextLoop(), serverAddr, buf, this);
            session->Start();
            sessions_.push_back(session);
        }
    }

    ~Client() {
    }

    const std::string& message() const {
        return message_;
    }

    void OnConnect() {
        if (++connected_count_ == session_count_) {
            LOG_WARN << "all connected";
        }
    }

    void OnDisconnect(const evpp::TCPConnPtr& conn) {
        if (--connected_count_ == 0) {
            LOG_WARN << "all disconnected";

            int64_t totalBytesRead = 0;
            int64_t totalMessagesRead = 0;
            for (auto& it : sessions_) {
                totalBytesRead += it->bytes_read();
                totalMessagesRead += it->messages_read();
            }
            LOG_WARN << totalBytesRead << " total bytes read";
            LOG_WARN << totalMessagesRead << " total messages read";
            LOG_WARN << static_cast<double>(totalBytesRead) / static_cast<double>(totalMessagesRead)
                << " average message size";
            LOG_WARN << static_cast<double>(totalBytesRead) / (timeout_ * 1024 * 1024)
                << " MiB/s throughput";
            loop_->QueueInLoop(std::bind(&Client::Quit, this));
        }
    }

private:
    void Quit() {
        tpool_->Stop();
        loop_->Stop();
        for (auto& it : sessions_) {
            delete it;
        }
        sessions_.clear();
        while (!tpool_->IsStopped() || !loop_->IsStopped()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        tpool_.reset();
    }

    void HandleTimeout() {
        LOG_WARN << "stop";
        for (auto& it : sessions_) {
            it->Stop();
        }
    }
private:
    evpp::EventLoop* loop_;
    std::shared_ptr<evpp::EventLoopThreadPool> tpool_;
    int session_count_;
    int timeout_;
    std::vector<Session*> sessions_;
    std::string message_;
    std::atomic<int> connected_count_;
};

void Session::OnConnection(const evpp::TCPConnPtr& conn) {
    if (conn->IsConnected()) {
        conn->SetTCPNoDelay(true);
        conn->Send(owner_->message());
        owner_->OnConnect();
    }
    else {
        owner_->OnDisconnect(conn);
    }
}

int pmain(int argc, char* argv[]) {
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
    if (argc == 7) {
        fprintf(stderr, "Usage: client <host_ip> <port> <threads> <blocksize> <sessions> <time_seconds>\n");
        return -1;
    }

    const char* ip = "127.0.0.1";
    uint16_t port = 9099;
    int threadCount = 4;
    int blockSize = 16;
    int sessionCount = 1;
    int timeout = 10;

    evpp::EventLoop loop;
    //std::string serverAddr = std::string(ip) + ":" + std::to_string(port);"127.0.0.1:9099";
    std::string serverAddr = "127.0.0.1:9099";

    Client client(&loop, serverAddr, blockSize, sessionCount, timeout, threadCount);
    loop.Run();
    return 0;
}



