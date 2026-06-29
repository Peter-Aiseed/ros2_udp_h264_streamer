from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package='realsense2_camera',
            executable='realsense2_camera_node',
            name='realsense_camera',
            output='screen',
            parameters=[{
                # Enable only infrared
                'enable_infra1': True,
                'enable_infra2': False,
                'enable_color': False,
                'enable_depth': False,

                # Infra1 settings
                'infra_width': 848,
                'infra_height': 480,
                'infra_fps': 30,

                # Optional stability settings
                'enable_auto_exposure': True,
                'rgb_camera.profile': '0,0,0,0,0',
            }]
        )

    ])