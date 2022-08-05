#include "StreamServer.h"
#include "BaseCamera.h"
#include <signal.h>

#define CAMERA_ID -1
std::unique_ptr<iit::StreamServer> Server;

//! handle  ctrl+c
void signal_intrupt_handle(int var)
{
    Server->stop();

}

int main(int argc, char const *argv[])
{

    signal(SIGINT, signal_intrupt_handle);

    // Camera
    std::shared_ptr<BaseCamera> Camera = std::make_shared<BaseCamera>(CAMERA_ID, false);

    // http server at port 8080
    Server = std::make_unique<iit::StreamServer>(8080, Camera);

    // keep runing
    Server->start();

    return 0;
}
