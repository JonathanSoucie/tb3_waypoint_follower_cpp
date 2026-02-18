# tb3_waypoint_follower_cpp

A ROS 2 (Humble) C++ package that provides a configurable waypoint / goal interface for TurtleBot3 navigation using Nav2.

The package implements a dedicated node, `goal_sender`, which listens for pose goals on a topic and forwards them to Nav2’s `NavigateToPose` action server. This decouples goal input from navigation execution and demonstrates proper ROS 2 design patterns using actions, parameters, and runtime configuration.

---

## Features

- C++ ROS 2 node for Nav2 goal handling
- Subscribes to `geometry_msgs/PoseStamped` goals
- Sends goals to Nav2 via an action client
- Fully parameterized using YAML
- Supports:
  - Goal frame validation
  - Configurable action server name
  - Configurable goal topic
  - Optional cancellation of active goals
- Designed for TurtleBot3 simulation with Gazebo, SLAM Toolbox, and Nav2

---

## Package Structure

tb3_waypoint_follower_cpp/
├── src/
│ └── goal_sender.cpp
├── launch/
│ └── goal_sender.launch.py
├── config/
│ └── goal_sender.yaml
├── CMakeLists.txt
├── package.xml
└── README.md

## Dependencies

- ROS 2 Humble
- TurtleBot3 packages
- Nav2
- SLAM Toolbox
- geometry_msgs
- nav2_msgs
- rclcpp
- rclcpp_action

Make sure the TurtleBot3 packages are installed and sourced correctly.

---

## Build Instructions

From your ROS 2 workspace:

```bash
cd ~/turtlebot3_ws
source /opt/ros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash
```
