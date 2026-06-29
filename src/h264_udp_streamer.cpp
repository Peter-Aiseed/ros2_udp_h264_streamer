#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

class H264UdpStreamer : public rclcpp::Node {
public:
    H264UdpStreamer() : Node("h264_udp_streamer") {
        // Initialize GStreamer
        gst_init(nullptr, nullptr);

        // Target Configuration (Change these to match your setup!)
        this->declare_parameter<std::string>("input_topic", "/camera/realsense_camera/infra1/image_rect_raw");
        this->declare_parameter<std::string>("target_ip", "127.0.0.1");
        this->declare_parameter<int>("target_port", 5000);
        this->declare_parameter<int>("width", 848);
        this->declare_parameter<int>("height", 480);
        this->declare_parameter<int>("fps", 30);
        this->declare_parameter<int>("bitrate", 4000000);

        // Retrieve the parameters (can be overridden by launch/command line)
        std::string input_topic = this->get_parameter("input_topic").as_string();
        std::string target_ip = this->get_parameter("target_ip").as_string();
        int target_port = this->get_parameter("target_port").as_int();
        int width = this->get_parameter("width").as_int();
        int height = this->get_parameter("height").as_int();
        int fps = this->get_parameter("fps").as_int();
        int bitrate = this->get_parameter("bitrate").as_int();

        // Build the pipeline string utilizing Orin hardware acceleration
        std::string pipeline_str = 
            "appsrc name=mysource emit-signals=true is-live=true caps=video/x-raw,format=GRAY8,width=" 
            + std::to_string(width) + ",height=" + std::to_string(height) + ",framerate=" + std::to_string(fps) + "/1 ! "
            "videoconvert ! video/x-raw,format=I420 ! "  // Converts GRAY8 to standard I420 raw video first
            "nvvidconv ! video/x-raw(memory:NVMM),format=NV12 ! "  // Uploads to hardware memory safely
            "nvv4l2h264enc maxperf-enable=1 bitrate=" + std::to_string(bitrate) + " insert-sps-pps=true ! "
            "h264parse ! rtph264pay config-interval=1 pt=96 ! "
            "udpsink host=" + target_ip + " port=" + std::to_string(target_port) + " sync=false async=false";

        RCLCPP_INFO(this->get_logger(), "Launching Pipeline: %s", pipeline_str.c_str());

        // Parse and start pipeline
        GError *error = nullptr;
        pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
        if (!pipeline_) {
            RCLCPP_ERROR(this->get_logger(), "GStreamer pipeline error: %s", error->message);
            g_error_free(error);
            return;
        }

        // Get appsrc element
        appsrc_ = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(pipeline_), "mysource"));
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);

        // Subscribe to ROS 2 topic
        subscription_ = this->create_subscription<sensor_msgs::msg::Image>(
            input_topic, 10,
            [this](const sensor_msgs::msg::Image::SharedPtr msg) { image_callback(msg); });
    }

    ~H264UdpStreamer() {
        if (pipeline_) {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
        }
    }

private:
    void image_callback(const sensor_msgs::msg::Image::SharedPtr msg) {
        cv_bridge::CvImagePtr cv_ptr;
        try {
            // Realsense infra is mono8
            cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::MONO8);
        } catch (cv_bridge::Exception& e) {
            RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
            return;
        }

        int size = cv_ptr->image.total() * cv_ptr->image.elemSize();
        GstBuffer *buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
        
        // Copy OpenCV frame data into GStreamer buffer
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
            memcpy(map.data, cv_ptr->image.data, size);
            gst_buffer_unmap(buffer, &map);
            
            // Push buffer into appsrc
            GstFlowReturn ret;
            g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
            gst_buffer_unref(buffer);
            
            if (ret != GST_FLOW_OK) {
                RCLCPP_WARN(this->get_logger(), "Error pushing buffer to appsrc");
            }
        }
    }

    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subscription_;
    GstElement *pipeline_;
    GstAppSrc *appsrc_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<H264UdpStreamer>());
    rclcpp::shutdown();
    return 0;
}