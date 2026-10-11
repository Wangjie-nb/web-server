#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include <string>
#include <vector>
#include <mutex>
#include "thread_pool.h"

class HttpServer {
private:
    int port;
    int server_fd;
    int epoll_fd;
    ThreadPool* pool;
    std::string wwwRoot;

    // 聊天室消息历史
    std::vector<std::string> msgHistory;
    std::mutex msgMtx;

    void acceptClient();
    void handleRead(int client_fd);
    std::string parseRequest(const std::string& request, const std::string& body);
    std::string getContentType(const std::string& path);
    std::string readFile(const std::string& path);

public:
    HttpServer(int p, int threadNum, const std::string& root);
    ~HttpServer();

    void start();
};

#endif
