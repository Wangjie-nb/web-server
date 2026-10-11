# 多线程Web服务器

基于epoll事件驱动 + 线程池实现的简单HTTP服务器，支持返回静态网页。

## 功能特性
- 基于epoll事件驱动模型，单线程监听所有客户端连接
- 线程池处理客户端请求，支持并发访问
- 支持HTTP GET方法，返回html、图片等静态资源
- 404错误处理

## 技术栈
C++17 / Linux epoll / 线程池 / 多线程 / HTTP协议

## 目录结构
```
web-server/
├── src/
│   ├── thread_pool.h/cpp    # 线程池实现
│   ├── http_server.h/cpp    # HTTP服务器
│   └── main.cpp             # 程序入口
├── www/                     # 静态文件目录
│   └── index.html
├── CMakeLists.txt
└── README.md
```

## 编译运行
```bash
mkdir build && cd build
cmake ..
make
cd ..
./bin/webserver
```

## 测试
打开浏览器访问：
```
http://127.0.0.1:8080
```

就能看到测试页面。

## 设计思路
1. **epoll**：负责监听所有socket事件，有新连接就accept，有数据就丢给线程池
2. **线程池**：固定数量的工作线程，从任务队列里取客户端请求处理
3. **HTTP解析**：解析请求行，拿到要访问的文件路径
4. **静态文件**：读取www目录下的文件，返回给客户端

## 为什么用epoll+线程池？
- epoll负责IO事件，不阻塞
- 线程池负责处理请求，充分利用多核CPU
- 比多进程或者每连接一线程效率高
