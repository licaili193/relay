#include "image_processing.h"

#include "camera_grabber.h"

void CameraGrabber::worker() {
  CHECK(cuda_ctx_);
  CHECK_EQ(CUDA_SUCCESS, cuCtxSetCurrent(*cuda_ctx_));
  cudaSetDevice(0);

  // TODO: Has to be initialized in the worker?
  // 1.  Open the device
  int fd; // A file descriptor to the video device
  fd = open(camera_name_.c_str(), O_RDWR);
  if(fd < 0){
      LOG(FATAL) << "Failed to open device, OPEN";
  }


  // 2. Ask the device if it can capture frames
  v4l2_capability capability;
  if(ioctl(fd, VIDIOC_QUERYCAP, &capability) < 0){
      // something went wrong... exit
      LOG(FATAL) << "Failed to get device capabilities, VIDIOC_QUERYCAP";
  }
  
  // 3. Set Image format
  v4l2_format imageFormat;
  imageFormat.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  imageFormat.fmt.pix.width = width_;
  imageFormat.fmt.pix.height = height_;
  imageFormat.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
  imageFormat.fmt.pix.field = V4L2_FIELD_INTERLACED;
  // tell the device you are using this format
  if(ioctl(fd, VIDIOC_S_FMT, &imageFormat) < 0){
      LOG(FATAL) << "Device could not set format, VIDIOC_S_FMT";
  }

  // 4. Request Buffers from the device
  v4l2_requestbuffers requestBuffer = {0};
  requestBuffer.count = 1; // one request buffer
  requestBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE; // request a buffer which we can use for capturing frames
  requestBuffer.memory = V4L2_MEMORY_MMAP;

  if(ioctl(fd, VIDIOC_REQBUFS, &requestBuffer) < 0){
      LOG(FATAL) << "Could not request buffer from device, VIDIOC_REQBUFS";
  }
  
  // 5. Query the buffer to get raw data ie. ask for the you requested buffer
  // and allocate memory for it
  v4l2_buffer queryBuffer = {0};
  queryBuffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  queryBuffer.memory = V4L2_MEMORY_MMAP;
  queryBuffer.index = 0;
  if(ioctl(fd, VIDIOC_QUERYBUF, &queryBuffer) < 0){
      LOG(FATAL) << "Device did not return the buffer information, VIDIOC_QUERYBUF";
  }
  // use a pointer to point to the newly created buffer
  // mmap() will map the memory address of the device to
  // an address in memory
  char* buffer = (char*)mmap(NULL, queryBuffer.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, queryBuffer.m.offset);
  memset(buffer, 0, queryBuffer.length);


  // 6. Get a frame
  // Create a new buffer type so the device knows which buffer we are talking about
  v4l2_buffer bufferinfo;
  memset(&bufferinfo, 0, sizeof(bufferinfo));
  bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  bufferinfo.memory = V4L2_MEMORY_MMAP;
  bufferinfo.index = 0;

  // Activate streaming
  int type = bufferinfo.type;
  if(ioctl(fd, VIDIOC_STREAMON, &type) < 0){
      LOG(FATAL) << "Could not start streaming, VIDIOC_STREAMON";
  }

  running_.store(true);
  while (running_.load()) {
      // Queue the buffer
      if(ioctl(fd, VIDIOC_QBUF, &bufferinfo) < 0){
          LOG(ERROR) << "Could not queue buffer, VIDIOC_QBUF";
          continue;
      }

      // Dequeue the buffer
      if(ioctl(fd, VIDIOC_DQBUF, &bufferinfo) < 0){
          LOG(ERROR) << "Could not dequeue the buffer, VIDIOC_DQBUF";
          continue;
      }
      // Frames get written after dequeuing the buffer

      memcpy(out_buffer_, buffer, std::min(out_buffer_size_, (size_t)bufferinfo.bytesused));
      cudaMemcpy(d_img_yuyv_, out_buffer_, out_buffer_size_, cudaMemcpyHostToDevice);
      new_image_.store(true);
  }

  // end streaming
  if(ioctl(fd, VIDIOC_STREAMOFF, &type) < 0){
    LOG(ERROR) << "Could not end streaming, VIDIOC_STREAMOFF";
  }

  close(fd);
}
