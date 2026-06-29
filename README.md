# udp_com

An ultra-low-latency, hardware-accelerated H.264 video streaming pipeline designed for ROS 2 and optimized for long-range robotic/drone field deployments (e.g., via Microhard industrial radio links). 

This package captures raw ROS 2 image topics on an NVIDIA Jetson Orin, utilizes onboard hardware-accelerated memory encoding (`nvvidconv` and `nvv4l2h264enc`), and streams the video over UDP to a ground control station.

---

## Features
* **Universal Parameterization:** Change input topic, resolution, frame rate, bitrate, target IP, and port directly from a launch file without recompiling.
* **Hardware Accelerated:** Performs direct memory (`NVMM`) encoding natively on the Jetson GPU/NVENC engine to save CPU cycles.
* **Long-Range Optimized:** Configured with tight Constant Bitrate (CBR) controls and frequent Keyframe (GOP) intervals to survive high packet-loss RF environments.

---

## 🚀 Installation & Setup

### 1. Jetson Orin (Sender) Prerequisites
Ensure your Jetson environment has NVIDIA's GStreamer plugins installed:
```bash
sudo apt update
sudo apt install -y gstreamer1.0-tools gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav ros-$ROS_DISTRO-cv-bridge
```

### 2. Build the Workspace
```bash
cd ~/ego_ws_ros2
colcon build --packages-select udp_com
source install/setup.bash
```

## 🛸 How to Use
### Step 1: Configure & Run the Sender (Jetson)
Open launch/udp_streaming.launch.py and set your ground station's IP, target port, and desired image topic.

### Step 2: Launch the streamer
```bash
ros2 launch udp_com udp_streaming.launch.py
```

### Step 3: Run the Receiver (Ground Station Desktop)
For real-time viewing, execute this optimized, zero-buffered GStreamer pipeline natively on your terminal:
```bash
gst-launch-1.0 udpsrc port=5000 caps="application/x-rtp, media=(string)video, clock-rate=(int)90000, encoding-name=(string)H264, payload=(int)96" ! rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! autovideosink sync=false
```
