#include "TCPServer.hpp"
#include <iostream>
#include <string>

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

    int port = atoi(argv[1]);
    TCPServer *srv = TCPServer::getinstance(port);

    for (;;){}
    return 0;
}


