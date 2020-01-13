
#include "server_sock.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>    
#include <cstring>
#include <sys/ioctl.h>

ServerSocket::ServerSocket()
{
    server_sock_info = {0};
}

ServerSocket::~ServerSocket()
{
    if(client_socks != nullptr){
        //close the client sockets
        for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
            if(client_socks[i]){
                shutdown(client_socks[i], SHUT_RDWR);
                close(client_socks[i]);
            }
        }
        //free the allocated memory.
        free(client_socks);
    }
    //close the server socket
    if(sock_fd){
        shutdown(sock_fd, SHUT_RDWR);
        close(sock_fd);
    }   
}

void ServerSocket::CreateSocket(const SERVER_SOCK_INFO_T *socket_config_)
{
    int opt_enable;
    int sock_options = 0;
    //update server_sock_info
    server_sock_info = *socket_config_;
    //create socket
    if(server_sock_info.TCP_SOCKET)
        sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    else
        sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if(sock_fd < 0){
        perror("Socket Creation!");
        exit(EXIT_FAILURE);
    }
    //set socket options
    opt_enable = 1;
    sock_options |= (server_sock_info.REUSE_ADDR ? SO_REUSEADDR : 0);
    sock_options |= (server_sock_info.REUSE_PORT ? SO_REUSEPORT : 0);
    if(setsockopt(sock_fd, SOL_SOCKET, sock_options, (const void *)&opt_enable, (socklen_t)sizeof(opt_enable)) < 0){
        perror("Set Socket Options!");
        exit(EXIT_FAILURE);
    }
    printf("Server socket created!\r\n");
}

void ServerSocket::StartServer(void)
{
    fd_set read_fds;  
    int max_socket_fd;
    if(sock_fd <= 0){
        printf("Firstly, socket should be configured, ConfigureSocket!\r\n");
        return;
    }
    //set socket address
    bzero((void *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_sock_info.PORT_NO);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    //binding
    if(bind(sock_fd, (const sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("Binding Error!");
        exit(EXIT_FAILURE);
    }
    //start listening
    if (listen(sock_fd, server_sock_info.LISTEN_BACKLOG) < 0) { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    //allocate buf to handle client sockets..
    client_socks = (int *) calloc(server_sock_info.MAX_CLIENT_COUNT, sizeof(int));
    if(!client_socks){
        perror("Memory Allocation Error for Client Sockets!");
        exit(EXIT_FAILURE);
    }

    //accept the incoming connections 
    printf("Waiting for clients\r\n");
    while(true){
        FD_ZERO(&read_fds); //clear set
        FD_SET(sock_fd, &read_fds); //set with the socket of server.
        max_socket_fd = sock_fd;  //by default we assume sock_fd is the highest number of all sockets including client sockets.
        //update max socket fd.
        for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
            if(client_socks[i] > 0){
                FD_SET(client_socks[i], &read_fds);
                if(client_socks[i] > max_socket_fd)
                    max_socket_fd = client_socks[i];
            }
        }
        printf("Select..\r\n");
        if(select(max_socket_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
            printf("Select Error!");
        else{   //select returns because of having data ready in the socket.
            printf("Conn. Req Received!\r\n");
            //check for server socket / any connection request?
            if(FD_ISSET(sock_fd, &read_fds)){
                struct sockaddr_in client_addr;
                int client_addr_len = sizeof(client_addr);
                int new_client_fd = accept(sock_fd, (sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
                if(new_client_fd < 0){
                    perror("Accept Error");
                    exit(EXIT_FAILURE);
                }
                printf("*** New Client, socket fd is %d , ip is : %s , port : %d  \r\n" , new_client_fd , inet_ntoa(client_addr.sin_addr) , ntohs (client_addr.sin_port));   
                //update client socket array with new client socket socket descriptor.
                for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
                    if(client_socks[i] == 0){
                        client_socks[i] = new_client_fd;
                        break;
                    }
                }
            }
            //check other client sockets
            for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
                if(FD_ISSET(client_socks[i], &read_fds)){
                    uint16_t byte_count = 0;
                    ioctl (client_socks[i], FIONREAD, &byte_count);
                    if(byte_count){
                        unsigned char *buf = (unsigned char *)calloc(byte_count, sizeof(uint8_t));
                        read(client_socks[i], buf, byte_count);
                        printf("Recvd Message from Client:%d, Msg:%s\r\n", client_socks[i], buf);
                    }else if(byte_count == 0){  //disconnection request
                        printf("Disconnection Req. Recvd, Client sock fd:%d\r\n", client_socks[i]);
                        shutdown(client_socks[i], SHUT_RDWR);
                        close(client_socks[i]);
                        client_socks[i] = 0;
                    }

                }
            }
        }
    }
}


SERVER_SOCK_INFO_T ServerSocket::GetSocketOptions(void)const
{
    return this->server_sock_info;
}

