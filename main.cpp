
#include "server_sock.hpp"
#include <iostream>

int main(void)
{
    std::cout << "TCP Server" << std::endl;
    ServerSocket serv;  //create the object
    SERVER_SOCK_INFO_T server_config;   //to configure the socket, if you don't want to use the default settings.
    server_config.TCP_SOCKET = 1;
    server_config.LISTEN_BACKLOG = 3;
    server_config.MAX_CLIENT_COUNT = 5;
    server_config.PORT_NO = 56789;
    server_config.REUSE_ADDR = 1;
    server_config.REUSE_PORT = 1;
    server_config.BLOCKING = 1;

    serv.CreateSocket(&server_config);   //create the socket with the configuration.
    serv.StartServer();     //start server
    return 0;
}
