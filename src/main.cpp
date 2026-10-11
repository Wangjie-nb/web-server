#include "http_server.h"
#include <iostream>

int main() {
    HttpServer server(8080, 4, "./www");
    server.start();
    return 0;
}
