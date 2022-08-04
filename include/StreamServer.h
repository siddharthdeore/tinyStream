/**
 * @file StreamServer.h
 * @author Siddharth Deore (siddharthdeore@gmail.com)
 * @brief
 * @version 0.1
 * @date 2019-08-02
 *
 * @copyright Copyright (c)
 *
 */
#ifndef STREAMSERVER_H
#define STREAMSERVER_H

#pragma once

#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#elif __linux__
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h> // non-blocking
#endif

#include <vector>
#include <thread> // std::thread
#include <mutex>  // std::mutex
#include <atomic> // atomic
#include <iostream>
#include <cstring> // strlen
#include <iomanip> // put_time
#define MAX_CONNECTIONS 64

#include "ICamera.h"

#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"
#define CLR "\033[0m"

#define PRINT_TIMESTAMP std::cout << std::put_time(std::localtime(&time_now), "%Y-%m-%d %OH:%OM:%OS")
static std::time_t time_now = std::time(nullptr);

static void set_nonblock(int socket)
{
    int flags;
    flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

namespace iit
{
    class StreamServer
    {
    public:
        StreamServer(const int &port, const ICamera::Ptr &Camera)
        {
            _camera = std::move(Camera);

            // initialize socket
            _server_sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            _server_address.sin_family = AF_INET;
            _server_address.sin_addr.s_addr = INADDR_ANY;
            _server_address.sin_port = htons(port);

            // force to bind port enable
            const int opt = 1;
            setsockopt(_server_sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

            // bind port
            bind(_server_sock_fd, (struct sockaddr *)&_server_address, sizeof(_server_address));

            // start listneing client requests
            listen(_server_sock_fd, MAX_CONNECTIONS);
            set_nonblock(_server_sock_fd);

            std::cout << YELLOW << "\nServer started at http://localhost:" << port << "\n"
                      << CLR << std::endl;
            _flg_mtx.lock();
            keep_accepting = true;
            _flg_mtx.unlock();

            //_server_thread = std::thread(&StreamServer::handle_new_connections, this);
            _client_thread = std::thread(&StreamServer::handle_clients, this);
        }

        ~StreamServer()
        {
            std::cout << GREEN << "Server Closed" << CLR << std::endl;
        }

        void start()
        {
            //_server_thread.join();
            _client_thread.join();
        }

        /**
         * @brief send video stream to each connected client,
         * if client closes, remove descriptor from client list
         */
        void handle_clients()
        {

            std::vector<unsigned char> jpg;
            int master_socket, activity;
            int max_sd;
            // set of socket descriptors
            fd_set readfds;
            const int socklen = sizeof(struct sockaddr_in);

            struct timeval timeout_val = {0, 10000}; // 0 sec, 1 milisec
            //////////////////
            while (keep_accepting)
            {
                // clear the socket set
                FD_ZERO(&readfds);
                // add master socket to set
                FD_SET(_server_sock_fd, &readfds);

                max_sd = _server_sock_fd;

                // update max socket fd
                for (int current_client_fd : _client_fds)
                {
                    // if valid socket descriptor then add to read list
                    if (current_client_fd > 0)
                    {
                        FD_SET(current_client_fd, &readfds);
                    }
                    // highest file descriptor number, need it for the select function
                    if (current_client_fd > max_sd)
                        max_sd = current_client_fd;
                }

                activity = select(max_sd + 1, &readfds, NULL, (fd_set *)0, &timeout_val);
                // handle new connection on activity
                if (FD_ISSET(_server_sock_fd, &readfds))
                {
                    FD_CLR(_server_sock_fd, &readfds);
                    handle_new_connections();
                }

                std::string frame_header;

                // grab jpeg image from camera
                jpg = _camera->get_frame();

                // update jpg size in header for individual frame
                frame_header = "--frame\r\nContent-Type: image/jpeg\r\n"
                               "Content-Length: " +
                               std::to_string(jpg.size()) + "\r\n\r\n";

                // send to all active clients
                for (std::vector<int>::iterator it = _client_fds.begin(); it != _client_fds.end(); ++it)
                {
                    int current_client_fd = *it;
                    char buffer[1024];
                    // check if current client is closing and read message
                    /*
                    if (FD_ISSET(current_client_fd, &writefds))
                    {
                        FD_CLR(current_client_fd,&writefds);
                        if ((activity = recv(current_client_fd, buffer, 1024,MSG_PEEK)) == 0)
                        {
                        }
                    }
                    */
                    // send frame header
                    int ret = send(current_client_fd, (char *)frame_header.data(), frame_header.size(), 0x4000);
                    ret = send(current_client_fd, jpg.data(), jpg.size(), 0x4000);
                    // send frame
                    if (ret < 0)
                    {
                        getpeername(current_client_fd, (struct sockaddr *)&_client_address, (socklen_t *)&socklen);

                        // remove cleint fd from list
                        _client_fds.erase(it--);

                        PRINT_TIMESTAMP;
                        std::cout << RED << " [ - ] IP: " << inet_ntoa(_client_address.sin_addr)
                                  << " PORT: " << ntohs(_client_address.sin_port)
                                  << ", Total clients " << _client_fds.size() << CLR << std::endl;
                    }
                }
                usleep(20000); // 50 Hz loop
            }
        }
        void handle_new_connections()
        {
            const std::string header = "HTTP/1.0 200 OK\r\n"
                                       "Cache-Control: no-cache\r\n"
                                       "Pragma: no-cache\r\n"
                                       "Connection: close\r\n"
                                       "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

            const int socklen = sizeof(struct sockaddr_in);

            int new_connection_fd;

            if (keep_accepting && (new_connection_fd = accept(_server_sock_fd, (struct sockaddr *)&_client_address, (socklen_t *)&socklen)))
            {
                set_nonblock(new_connection_fd);

                // read from client
                // char msg[512] = "\0";
                // int readBytes = recv(new_connection_fd, msg, 512, MSG_PEEK);

                // send primary header response
                send(new_connection_fd, header.data(), header.size(), 0x4000);

                // add new connection to client list
                _cl_mtx.lock();
                _client_fds.push_back(new_connection_fd);
                _cl_mtx.unlock();

                // verbose output
                PRINT_TIMESTAMP;
                std::cout << GREEN << " [ + ] IP: " << inet_ntoa(_client_address.sin_addr)
                          << " PORT: " << ntohs(_client_address.sin_port)
                          << ", Total clients " << _client_fds.size() << CLR << std::endl;
                // 10Hz
                // usleep(1000);
            }
        }
        void stop()
        {
            std::cout << YELLOW << "\nClosing connections " << CLR << std::endl;

            _flg_mtx.lock();
            keep_accepting = false;
            _flg_mtx.unlock();

            _cl_mtx.lock();
            for (std::vector<int>::iterator it = _client_fds.begin(); it != _client_fds.end(); it++)
            {
                getpeername(*it, (struct sockaddr *)&_client_address, (socklen_t *)sizeof(socklen_t));
                PRINT_TIMESTAMP;
                std::cout << YELLOW << " [ closing ] IP: " << inet_ntoa(_client_address.sin_addr)
                          << " PORT: " << ntohs(_client_address.sin_port) << CLR << std::endl;
                _close_socket(*it);
            }
            _cl_mtx.unlock();
        }

    private:
        // socket descriptors
        int _server_sock_fd;
        std::vector<int> _client_fds;

        // internet socket address
        struct sockaddr_in _server_address, _client_address;

        // std::thread _server_thread, _client_thread;
        std::thread _client_thread;

        std::atomic<bool> keep_accepting;
        std::mutex _cl_mtx, _flg_mtx;

        // Camera pointer
        ICamera::Ptr _camera;

        void _close_socket(const int &fd)
        {
#ifdef _WIN32
            closesocket(fd);
#elif __linux__
            close(fd);
#endif
        }
    };
}

#endif
