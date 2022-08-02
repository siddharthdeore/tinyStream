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

#define MAX_CONNECTIONS 3

#define LOG(x) std::cout << x << std::endl
#include <opencv2/opencv.hpp>

class StreamServer
{
public:
    StreamServer(const int &port)
    {
        // initialize socket
        _server_sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        _server_address.sin_family = AF_INET;
        _server_address.sin_addr.s_addr = INADDR_ANY;
        _server_address.sin_port = htons(port);

        const int opt = 1;
        setsockopt(_server_sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
        // bind port
        bind(_server_sock_fd, (struct sockaddr *)&_server_address, sizeof(_server_address));
        // start listneing client requests
        listen(_server_sock_fd, MAX_CONNECTIONS);
        std::cout << "server started at http://localhost:" << port << std::endl;

        cap.open("http://10.240.29.68:8080/video", cv::CAP_ANY);

        keep_accepting = true;

        _server_thread = std::thread(&StreamServer::handle_new_connections, this);
        _client_thread = std::thread(&StreamServer::handle_clients, this);

        _server_thread.join();
        _client_thread.join();
    }
    ~StreamServer()
    {
        _close_socket(_server_sock_fd);
    }

    void handle_clients()
    {
        cv::Mat frame;
        std::vector<uchar> jpg;

        std::vector<int> params;
        params.push_back(cv::IMWRITE_JPEG_QUALITY);
        params.push_back(50);

        while (keep_accepting)
        {
            cap.read(frame);
            cv::imencode(".jpg", frame, jpg, params);
            frame.release();
            const int len_buf = jpg.size();
            std::string head = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + std::to_string(len_buf) + "\r\n\r\n";
            char *frame_header = (char *)head.c_str();

            for (std::vector<int>::iterator it = client_fds.begin(); it != client_fds.end();)
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
                    std::cout << "[ADD] ip: " << inet_ntoa(_client_address.sin_addr) << " port: " << ntohs(_client_address.sin_port) << " Total Clients " << client_fds.size() << std::endl;
                    _close_socket(current_client_fd);
                    _cl_mtx.lock();
                    it = client_fds.erase(it);
                    _cl_mtx.unlock();
                }
                else
                {
                    ++it;
                }
            }
            usleep(16666);
        }
    }
    void handle_new_connections()
    {
        const std::string header = "HTTP/1.0 200 OK\r\nCache-Control: no-cache\r\nPragma: no-cache\r\nConnection: close\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

        const int header_size = header.size();
        char *header_data = (char *)header.data();

        const int socklen = sizeof(struct sockaddr_in);
        int new_connection_fd;
        while ((new_connection_fd = accept(_server_sock_fd, (struct sockaddr *)&_client_address, (socklen_t *)&socklen)))
        {

            char msg[512] = "\0";
            int readBytes = recv(new_connection_fd, msg, 512, MSG_PEEK);
            send(new_connection_fd, header_data, header_size, 0x4000);
            _cl_mtx.lock();
            client_fds.push_back(new_connection_fd);
            _cl_mtx.unlock();
            std::cout << "[REM] ip: " << inet_ntoa(_client_address.sin_addr) << " port: " << ntohs(_client_address.sin_port) << " Total Clients " << client_fds.size() << std::endl;
        }
    }

private:
    int _server_sock_fd, current_client_fd;
    std::vector<int> client_fds;
    struct sockaddr_in _server_address, _client_address;
    std::thread _server_thread, _client_thread;
    std::atomic<bool> keep_accepting;
    std::mutex _cl_mtx;
    cv::VideoCapture cap;
    void _close_socket(const int &fd)
    {
#ifdef _WIN32
        closesocket(fd);
#elif __linux__
        close(fd);
#endif
    }
};

#endif
