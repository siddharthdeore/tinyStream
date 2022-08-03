#include "StreamServer.h"
#include <signal.h>
#define CAMERA_ID -1
// #define CAMERA_ID "http://10.240.29.68:8080/video"

std::unique_ptr<StreamServer> Server;

void signal_intrupt_handle(int var)
{
    std::cout << "\n" << std::endl;
    // blocks at accept call, TODO: convert to non blocking
    Server->stop(); 
}

int main(int argc, char const *argv[])
{

    signal(SIGINT, signal_intrupt_handle);

    std::unique_ptr<BaseCamera> Camera = std::make_unique<BaseCamera>(CAMERA_ID);
    Server = std::make_unique<StreamServer>(8080, Camera);

    Server->start();

    return 0;
}
