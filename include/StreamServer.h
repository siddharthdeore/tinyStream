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
#endif

#include <vector>
#include <thread> // std::thread
#include <mutex>  // std::mutex
#include <atomic> // atomic
#include <iostream>
#include <cstring> // strlen
#include <iomanip> // put_time
#define MAX_CONNECTIONS 32

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

            std::cout << YELLOW << "\nServer started at http://localhost:" << port << "\n"
                      << CLR << std::endl;

            keep_accepting = true;

            _server_thread = std::thread(&StreamServer::handle_new_connections, this);
            _client_thread = std::thread(&StreamServer::handle_clients, this);
        }

        ~StreamServer()
        {
        }

        void start()
        {
            _server_thread.join();
            _client_thread.join();
        }

        /**
         * @brief send video stream to each connected client,
         * if client closes, remove descriptor from client list
         */
        void handle_clients()
        {

            std::vector<unsigned char> jpg;

            while (keep_accepting)
            {
                // grab jpeg image from camera
                jpg = _camera->get_frame();

                // update jpg size in header for individual frame
                std::string head = "--frame\r\nContent-Type: image/jpeg\r\n"
                                   "Content-Length: " +
                                   std::to_string(jpg.size()) + "\r\n\r\n";

                char *frame_header = (char *)head.c_str();

                // send to all active clients
                for (std::vector<int>::iterator it = _client_fds.begin(); it != _client_fds.end();)
                {
                    current_client_fd = *it;

                    // send frame header
                    int ret = send(current_client_fd, frame_header, strlen(frame_header), 0x4000);
                    // send frame
                    ret = send(current_client_fd, jpg.data(), jpg.size(), 0x4000);
                    // on rerror remove client
                    if (ret < 0)
                    {
                        const int socklen = sizeof(struct sockaddr_in);

                        getpeername(current_client_fd, (struct sockaddr *)&_client_address, (socklen_t *)&socklen);

                        _cl_mtx.lock();
                        it = _client_fds.erase(it);
                        _cl_mtx.unlock();

                        PRINT_TIMESTAMP;
                        std::cout << RED << " [ - ] IP: " << inet_ntoa(_client_address.sin_addr)
                                  << " PORT: " << ntohs(_client_address.sin_port)
                                  << ", Total clients " << _client_fds.size() << CLR << std::endl;

                        _close_socket(current_client_fd);
                    }

                    else
                    {
                        ++it;
                    }
                }

                usleep(20000);
            }
        }
        void handle_new_connections()
        {
            const std::string header = "HTTP/1.0 200 OK\r\n"
                                       "Cache-Control: no-cache\r\n"
                                       "Pragma: no-cache\r\n"
                                       "Connection: close\r\n"
                                       "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

            const int header_size = header.size();
            char *header_data = (char *)header.data();

            const int socklen = sizeof(struct sockaddr_in);

            int new_connection_fd;

            while (keep_accepting && (new_connection_fd = accept(_server_sock_fd, (struct sockaddr *)&_client_address, (socklen_t *)&socklen)))
            {
                // read from client
                char msg[512] = "\0";
                int readBytes = recv(new_connection_fd, msg, 512, MSG_PEEK);

                // send primary header response
                send(new_connection_fd, header_data, header_size, 0x4000);

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
                usleep(100000);
            }
        }
        void stop()
        {
            std::cout << YELLOW << "\nServer Closing " << CLR << std::endl;

            _cl_mtx.lock();
            keep_accepting = false;
            _cl_mtx.unlock();

            for (std::vector<int>::iterator it = _client_fds.begin(); it != _client_fds.end(); it++)
            {
                _close_socket(*it);
            }
            _close_socket(_server_sock_fd);

            std::terminate();
        }

    private:
        // socket descriptors
        int _server_sock_fd, current_client_fd;
        std::vector<int> _client_fds;

        // internet socket address
        struct sockaddr_in _server_address, _client_address;

        std::thread _server_thread, _client_thread;

        std::atomic<bool> keep_accepting;
        std::mutex _cl_mtx;

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
