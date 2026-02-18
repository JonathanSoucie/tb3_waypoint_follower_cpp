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

## Running the System (Multi-Terminal Setup)

Each component is launched in a separate terminal.
Make sure all terminals use the same ROS_DOMAIN_ID.

### Terminal 1 — Gazebo (Simulation)

```bash
export ROS_DOMAIN_ID=30
export TURTLEBOT3_MODEL=burger
source /opt/ros/humble/setup.bash
ros2 launch turtlebot3_gazebo turtlebot3_world.launch.py
```

### Terminal 2 — SLAM Toolbox

```bash
export ROS_DOMAIN_ID=30
source /opt/ros/humble/setup.bash
ros2 launch slam_toolbox online_async_launch.py use_sim_time:=True
```
### Terminal 3 — Nav2

```bash
export ROS_DOMAIN_ID=30
export TURTLEBOT3_MODEL=burger
source /opt/ros/humble/setup.bash
ros2 launch turtlebot3_navigation2 navigation2.launch.py use_sim_time:=True
```

This will also launch RViz with the Nav2 configuration.

### Terminal 4 — Waypoint Follower Node

```bash
export ROS_DOMAIN_ID=30
source /opt/ros/humble/setup.bash
source ~/turtlebot3_ws/install/setup.bash
ros2 launch tb3_waypoint_follower_cpp goal_sender.launch.py
```

This starts the goal_sender node and loads parameters from goal_sender.yaml.

### Sending a Navigation Goal

Open a new terminal and publish a goal:

```bash
export ROS_DOMAIN_ID=30
source /opt/ros/humble/setup.bash

ros2 topic pub --once /goal_pose geometry_msgs/msg/PoseStamped "{
  header: {frame_id: 'map'},
  pose: {
    position: {x: 0.5, y: 0.0, z: 0.0},
    orientation: {w: 1.0}
  }
}"
```

The robot should begin navigating to the specified pose.
