#include "HttpServer.hpp"
#include "Log.hpp"

#include <iostream>
#include <string>
#include <memory>

static void Usage(std::string proc)
{
    std::cout << "Usage:\n\t" << proc << " port" << std::endl;
}


int main(int argc, char *argv[])
{
    if (argc != 2){
        Usage(argv[0]);
        exit(4);
    }
    // LOG(INFO, "proc msg");

    int port = atoi(argv[1]);

    std::shared_ptr<HttpServer> http_server(new HttpServer(port));
    // 初始化TCP
    http_server->InitServer();
    http_server->Loop();

    return 0;
}

