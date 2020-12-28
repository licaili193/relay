# Relay
Relay Server

## Dependency
### Dependencies for the relay

* practical socket

### Dependencies for the stream

* libavcodec-dev
```
sudo apt install -y libavcodec-dev libavformat-dev libavdevice-dev libavfilter-dev
```

* opencv
In OpenCV folder
```
mkdir build
cd build
cmake -DBUILD_LIST=core,imgproc,imgcodecs,videoio,highgui,video ..
sudo make install
```

* glog
```
sudo apt-get install libgoogle-glog-dev
```

* gflags
```
sudo apt-get install libgflags-dev
```