cmake_minimum_required(VERSION 3.8)
project(udp_com)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0 gstreamer-app-1.0)

# Include GStreamer headers
include_directories(${GSTREAMER_INCLUDE_DIRS} ${GSTREAMER_APP_INCLUDE_DIRS})

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

# Declare the Executable (Link your source file)
add_executable(h264_udp_streamer src/h264_udp_streamer.cpp)

# Link ROS 2 dependencies to your executable
ament_target_dependencies(h264_udp_streamer 
  rclcpp 
  sensor_msgs 
  cv_bridge
)

# Link GStreamer libraries to your executable
target_link_libraries(h264_udp_streamer 
  ${GSTREAMER_LIBRARIES} 
  ${GSTREAMER_APP_LIBRARIES}
)

# Install the executable so 'ros2 run' can find it
install(TARGETS
  h264_udp_streamer
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
