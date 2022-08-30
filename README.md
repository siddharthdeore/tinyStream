# tinyStream
<i>tinyStream</i> : Minimal Webcam streaming server grabs camera frames and streams to desired port and supports multiple clients.


<img src="https://upload.wikimedia.org/wikipedia/commons/1/18/ISO_C%2B%2B_Logo.svg" width = "16"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/1/13/Cmake.svg/900px-Cmake.svg.png" width = "16"> <img src="https://upload.wikimedia.org/wikipedia/commons/thumb/b/b0/NewTux.svg/800px-NewTux.svg.png" width ="16">



## Build
```
mkdir build && cd build
cmake ..
make
```
## Run
Stream deafault usb camera by executing `tinyStream`

```
./tinyStream
```
Use multiple usb cameras by passing camera index in command line argument, for example two usb cameras `/dev/video0` and `/dev/video2` be streamed by passing there index '0' and '2' respectively in command line argument shall stream camera id 0 on port `8080` and camera id 2 on `8081`

```
./tinyStream 0 2
```


⚠️ Unter early development, larger chages expected! 
