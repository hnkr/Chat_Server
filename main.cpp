/**
 * main.cpp
 * Hunkar Ciplak, hunkarciplak@hotmail.com
*/

#include "server_sock.hpp"
#include <iostream>
#include <thread>


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
    server_config.BLOCKING = 1; //non-blocking not implemented yet.

    serv.CreateSocket(&server_config);   //create the socket with the configuration.
    std::thread th_server_rx(&ServerSocket::StartServer, &serv);    //start listening & accepting connection req. & receiving
    std::thread th_server_tx(&ServerSocket::SendQueueToAllClients, &serv);  //send the received data to all clients
    th_server_rx.join();
    th_server_tx.join();

    return 0;
}
