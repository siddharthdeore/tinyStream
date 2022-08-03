#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

#include <opencv4/opencv2/opencv.hpp>

int main(int argc, char *argv[])
{

    cv::VideoCapture cap(CAMERA_ID, cv::CAP_ANY);

    int socket_fd, client_fd;
    struct sockaddr_in server, client;

    // Create socket
    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == -1)
    {
        printf("Could not create socket");
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(8080);

    // force port binding option
    const int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    // Bind
    if (bind(socket_fd, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        std::cout << "bind failed" << std::endl;
        return 1;
    }
    std::cout << "bind done" << std::endl;

    listen(socket_fd, 3);

    std::cout << "Waiting for incoming connections..." << std::endl;

    const std::string header = "HTTP/1.0 200 OK\r\nCache-Control: no-cache\r\nPragma: no-cache\r\nConnection: close\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";

    const int header_size = header.size();
    char *header_data = (char *)header.data();
    cv::Mat frame;
    std::vector<uchar> jpg;

    std::vector<int> params;
    params.push_back(cv::IMWRITE_JPEG_QUALITY);
    params.push_back(50);
    int ret = 0;
    const int socklen = sizeof(struct sockaddr_in);
    while ((client_fd = accept(socket_fd, (struct sockaddr *)&client, (socklen_t *)&socklen)))
    {
        std::cout << "Connection accepted" << std::endl;
        char msg[4096] = "\0";
        int readBytes = recv(client_fd, msg, 4096, MSG_PEEK);
        std::cout << msg;
        ret = send(client_fd, header_data, header_size, 0x4000);

        while (ret > -1)
        {
            cap.read(frame);
            cv::imencode(".jpg", frame, jpg, params);
            frame.release();

            const int len_buf = jpg.size();
            std::string head = "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: " + std::to_string(len_buf) + "\r\n\r\n";
            char *frame_header = (char *)head.c_str();

            ret = send(client_fd, frame_header, strlen(frame_header), 0x4000);

            ret = send(client_fd, jpg.data(), len_buf, 0x4000);

            // usleep(16666);
        }

        close(client_fd);
    }

    if (client_fd < 0)
    {
        perror("accept failed");
        return 1;
    }

    return 0;
}