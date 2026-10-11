#include "http_server.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sys/epoll.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>

static void setNonBlock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

HttpServer::HttpServer(int p, int threadNum, const std::string& root)
    : port(p), server_fd(-1), epoll_fd(-1), wwwRoot(root) {
    pool = new ThreadPool(threadNum);
}

HttpServer::~HttpServer() {
    delete pool;
    if (server_fd > 0) close(server_fd);
    if (epoll_fd > 0) close(epoll_fd);
}

void HttpServer::start() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(server_fd, (struct sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 10);

    epoll_fd = epoll_create1(0);
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);
    setNonBlock(server_fd);

    std::cout << "Web Server start on port " << port << std::endl;

    struct epoll_event events[1024];
    while (true) {
        int nready = epoll_wait(epoll_fd, events, 1024, -1);
        for (int i = 0; i < nready; i++) {
            int fd = events[i].data.fd;
            if (fd == server_fd) {
                acceptClient();
            } else if (events[i].events & EPOLLIN) {
                int client_fd = fd;
                pool->enqueue([this, client_fd] {
                    this->handleRead(client_fd);
                });
            }
        }
    }
}

void HttpServer::acceptClient() {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
    setNonBlock(client_fd);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = client_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev);
}

void HttpServer::handleRead(int client_fd) {
    char buffer[4096];
    std::string request;
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = read(client_fd, buffer, sizeof(buffer));
        if (n > 0) {
            request.append(buffer, n);
        } else {
            break;
        }
    }

    // 分割请求头和请求体
    std::string header, body;
    size_t pos = request.find("\r\n\r\n");
    if (pos != std::string::npos) {
        header = request.substr(0, pos);
        body = request.substr(pos + 4);
    } else {
        header = request;
    }

    std::string response = parseRequest(header, body);
    write(client_fd, response.c_str(), response.size());
    close(client_fd);
}

std::string HttpServer::parseRequest(const std::string& request, const std::string& body) {
    // 解析请求行：GET /index.html HTTP/1.1
    std::istringstream iss(request);
    std::string method, path, version;
    iss >> method >> path >> version;

    // 处理聊天室接口
    if (path == "/sendmsg" && method == "POST") {
        std::lock_guard<std::mutex> lock(msgMtx);
        msgHistory.push_back(body);
        return "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
    }

    if (path == "/getmsg") {
        std::string allMsg;
        {
            std::lock_guard<std::mutex> lock(msgMtx);
            for (auto& msg : msgHistory) {
                allMsg += msg + "\n";
            }
        }
        return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nContent-Length: " + std::to_string(allMsg.size()) + "\r\n\r\n" + allMsg;
    }

    if (path == "/") path = "/index.html";
    if (path == "/chat") path = "/chat.html";

    std::string filepath = wwwRoot + path;
    std::string content = readFile(filepath);

    if (content.empty()) {
        std::string notFound = "<html><body><h1>404 Not Found</h1></body></html>";
        return "HTTP/1.1 404 Not Found\r\nContent-Length: " + std::to_string(notFound.size()) + "\r\n\r\n" + notFound;
    }

    std::string contentType = getContentType(path);
    return "HTTP/1.1 200 OK\r\nContent-Type: " + contentType + "\r\nContent-Length: " + std::to_string(content.size()) + "\r\n\r\n" + content;
}

std::string HttpServer::getContentType(const std::string& path) {
    if (path.find(".html") != std::string::npos) return "text/html";
    if (path.find(".css") != std::string::npos) return "text/css";
    if (path.find(".jpg") != std::string::npos) return "image/jpeg";
    if (path.find(".png") != std::string::npos) return "image/png";
    return "text/plain";
}

std::string HttpServer::readFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return "";
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
