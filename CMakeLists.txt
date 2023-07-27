cmake_minimum_required(VERSION 3.5)
project(game_controller)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(bfc_msgs REQUIRED)

include_directories(include)

add_executable(game_controller src/game_controller.cpp)
ament_target_dependencies(game_controller rclcpp std_msgs bfc_msgs)

install(TARGETS
  game_controller
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()