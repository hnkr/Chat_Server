/**
 * server_sock.cpp
 * Hunkar Ciplak, hunkarciplak@hotmail.com
*/


#include "server_sock.hpp"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>    
#include <cstring>
#include <sys/ioctl.h>
#include <mutex>
#include <queue>



ServerSocket::ServerSocket()
{
    server_sock_info = {0};
}

ServerSocket::~ServerSocket()
{
    if(client_socks != nullptr){
        //close the client sockets
        for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
            if(client_socks[i].client_fd){
                shutdown(client_socks[i].client_fd, SHUT_RDWR);
                close(client_socks[i].client_fd);
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
    std::uint16_t connected_client_count = 0;
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
    mclient_lock.lock();
    client_socks = (CLIENT_INFO_T *) calloc(server_sock_info.MAX_CLIENT_COUNT, sizeof(CLIENT_INFO_T));
    mclient_lock.unlock();
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
        this->mclient_lock.lock();
        for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
            if(client_socks[i].client_fd > 0){
                FD_SET(client_socks[i].client_fd, &read_fds);
                if(client_socks[i].client_fd > max_socket_fd)
                    max_socket_fd = client_socks[i].client_fd;
            }
        }
        this->mclient_lock.unlock();
        //printf("Select..\r\n");
        if(select(max_socket_fd + 1, &read_fds, NULL, NULL, NULL) < 0)
            printf("Select Error!");
        else{   //select returns because of having data ready in the socket.
            
            //check for server socket / any connection request?
            if(FD_ISSET(sock_fd, &read_fds)){
                printf("Conn. Req Received!\r\n");
                struct sockaddr_in client_addr;
                int client_addr_len = sizeof(client_addr);
               
                int new_client_fd = accept(sock_fd, (sockaddr *)&client_addr, (socklen_t *)&client_addr_len);
                if(new_client_fd < 0){
                    perror("Accept Error");
                    exit(EXIT_FAILURE);
                }
                if(connected_client_count < server_sock_info.MAX_CLIENT_COUNT){ //if MAX_CLIENT_COUNT client are already connected, then slightly discard the new connection request.
                    connected_client_count++;
                    //printf("*** New Client, socket fd is %d , ip is : %s , port : %d  \r\n" , new_client_fd , inet_ntoa(client_addr.sin_addr) , ntohs (client_addr.sin_port));   
                    //update client socket array with new client socket socket descriptor.
                    this->mclient_lock.lock();
                    for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
                        if(client_socks[i].client_fd == 0){
                            client_socks[i].client_fd = new_client_fd;
                            client_socks[i].client_addr = client_addr;
                            std::string msg_rcvd("## NEW CONNECTION: "); 
                            msg_rcvd = msg_rcvd + inet_ntoa(client_socks[i].client_addr.sin_addr) + "\r\n";
                            this->mbuf_lock.lock();
                            qmessage_to_send.push(msg_rcvd);
                            this->mbuf_lock.unlock();
                            break;
                        }
                    }
                    this->mclient_lock.unlock();
                }else{  //if the client limit reached, shutdown and close the socket..
                    printf("New client socket, shut down and closed!\r\n");
                    shutdown(new_client_fd, SHUT_RDWR);
                    close(new_client_fd);
                }
            }
            //check other client sockets
            for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
                if(FD_ISSET(client_socks[i].client_fd, &read_fds)){
                    uint16_t byte_count = 0;
                    this->mclient_lock.lock();
                    ioctl (client_socks[i].client_fd, FIONREAD, &byte_count);
                    this->mclient_lock.unlock();
                    if(byte_count){
                        char *buf = (char *)calloc(byte_count, sizeof(uint8_t));
                        read(client_socks[i].client_fd, buf, byte_count);
                        //printf("Recvd Message from Client:%s, Msg:%s\r\n", inet_ntoa(client_socks[i].client_addr.sin_addr), buf);
                        std::string msg_rcvd(inet_ntoa(client_socks[i].client_addr.sin_addr));
                        msg_rcvd = msg_rcvd + ":" + buf;
                        this->mbuf_lock.lock();
                        qmessage_to_send.push(msg_rcvd);
                        //printf("Recvd Message Pushed to Queue:%s\r\n", this->qmessage_to_send.front().c_str());
                        //printf("Size of Queue:%d\r\n", this->qmessage_to_send.size());
                        this->mbuf_lock.unlock();
                        free(buf);
                    }else if(byte_count == 0){  //disconnection request
                        printf("Disconn. Req Received!\r\n");
                        std::string msg_rcvd("## DISCONNECTION: "); 
                        msg_rcvd = msg_rcvd + inet_ntoa(client_socks[i].client_addr.sin_addr) + "\r\n";
                        this->mbuf_lock.lock();
                        qmessage_to_send.push(msg_rcvd);
                        this->mbuf_lock.unlock();
                        this->mclient_lock.lock();
                        //printf("Disconnection Req. Recvd, Client ip:%s\r\n", inet_ntoa(client_socks[i].client_addr.sin_addr));
                        shutdown(client_socks[i].client_fd, SHUT_RDWR);
                        close(client_socks[i].client_fd);
                        client_socks[i].client_fd = 0;
                        client_socks[i].client_addr = {0};
                        this->mclient_lock.unlock();
                    }
                }
            }
        }
    }
}

void ServerSocket::SendQueueToAllClients(void)
{
    //printf("SendQueueToAllClients\r\n");
    while(1){
        if(qmessage_to_send.size()){
            mbuf_lock.lock();
            std::string msg_buf (qmessage_to_send.front());
            qmessage_to_send.pop();
            mbuf_lock.unlock();
            mclient_lock.lock();
            for(int i = 0; i < server_sock_info.MAX_CLIENT_COUNT; i++){
                if(client_socks[i].client_fd)
                    write(client_socks[i].client_fd, msg_buf.c_str(), msg_buf.length() + 1);
                
            }
            mclient_lock.unlock();
        }
    }
}

SERVER_SOCK_INFO_T ServerSocket::GetSocketOptions(void)const
{
    return this->server_sock_info;
}

CLIENT_INFO_T * ServerSocket::GetClientSocketBuf(void)const
{
    return this->client_socks;
}



