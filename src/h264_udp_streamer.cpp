#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <cv_bridge/cv_bridge.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <string>

class H264UdpStreamer {
public:
    H264UdpStreamer(ros::NodeHandle& nh, ros::NodeHandle& pnh) {
        gst_init(nullptr, nullptr);

        std::string input_topic, target_ip;
        int target_port, width, height, fps, bitrate;

        // Pull settings from Parameter Server
        pnh.param<std::string>("input_topic", input_topic, "/camera/realsense_camera/infra1/image_rect_raw");
        pnh.param<std::string>("target_ip", target_ip, "127.0.0.1");
        pnh.param<int>("target_port", target_port, 5000);
        pnh.param<int>("width", width, 848);
        pnh.param<int>("height", height, 480);
        pnh.param<int>("fps", fps, 30);
        pnh.param<int>("bitrate", bitrate, 2000000);

        ROS_INFO("Streaming topic %s to %s:%d", input_topic.c_str(), target_ip.c_str(), target_port);

        // GStreamer factory string setup
        std::string pipeline_str = 
            "appsrc name=mysource emit-signals=true is-live=true caps=video/x-raw,format=GRAY8,width=" 
            + std::to_string(width) + ",height=" + std::to_string(height) + ",framerate=" + std::to_string(fps) + "/1 ! "
            "videoconvert ! video/x-raw,format=I420 ! "
            "nvvidconv ! video/x-raw(memory:NVMM),format=NV12 ! "
            "nvv4l2h264enc maxperf-enable=1 iframeinterval=15 preset-level=1 control-rate=1 bitrate=" + std::to_string(bitrate) + " insert-sps-pps=true ! "
            "h264parse ! rtph264pay config-interval=1 pt=96 ! "
            "udpsink host=" + target_ip + " port=" + std::to_string(target_port) + " sync=false async=false";

        GError *error = nullptr;
        pipeline_ = gst_parse_launch(pipeline_str.c_str(), &error);
        if (!pipeline_) {
            ROS_ERROR("GStreamer init pipeline crash error: %s", error->message);
            g_error_free(error);
            return;
        }

        appsrc_ = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(pipeline_), "mysource"));
        gst_element_set_state(pipeline_, GST_STATE_PLAYING);

        subscription_ = nh.subscribe(input_topic, 10, &H264UdpStreamer::imageCallback, this);
    }

    ~H264UdpStreamer() {
        if (pipeline_) {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
            gst_object_unref(pipeline_);
        }
    }

private:
    void imageCallback(const sensor_msgs::Image::ConstPtr& msg) {
        cv_bridge::CvImagePtr cv_ptr;
        try {
            cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::MONO8);
        } catch (cv_bridge::Exception& e) {
            ROS_ERROR("cv_bridge failed processing: %s", e.what());
            return;
        }

        int size = cv_ptr->image.total() * cv_ptr->image.elemSize();
        GstBuffer *buffer = gst_buffer_new_allocate(nullptr, size, nullptr);
        
        GstMapInfo map;
        if (gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
            memcpy(map.data, cv_ptr->image.data, size);
            gst_buffer_unmap(buffer, &map);
            
            GstFlowReturn ret;
            g_signal_emit_by_name(appsrc_, "push-buffer", buffer, &ret);
            gst_buffer_unref(buffer);
        }
    }

    ros::Subscriber subscription_;
    GstElement *pipeline_;
    GstAppSrc *appsrc_;
};

int main(int argc, char **argv) {
    ros::init(argc, argv, "h264_udp_streamer");
    ros::NodeHandle nh;
    ros::NodeHandle pnh("~");
    
    H264UdpStreamer streamer(nh, pnh);
    ros::spin();
    return 0;
}