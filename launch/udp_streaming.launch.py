from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='udp_com',
            executable='h264_udp_streamer',
            name='h264_udp_streamer_node',
            output='screen',
            # Pass custom parameters here without modifying the C++ binary!
            parameters=[{
                'input_topic': '/camera/realsense_camera/infra1/image_rect_raw',   # Universal Topic Parameter
                'target_ip': '192.168.0.65',                                       # Target Destination IP
                'target_port': 5000,                                               # Target Destination Port
                'width': 848,
                'height': 480,
                'fps': 30,
                'bitrate': 5000000,
            }]
        )
    ])
