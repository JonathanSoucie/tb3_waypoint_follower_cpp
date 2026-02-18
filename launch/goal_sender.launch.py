from launch import LaunchDescription
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory
import os

def generate_launch_description():
    pkg_share = get_package_share_directory("tb3_waypoint_follower_cpp")
    params_file = os.path.join(pkg_share, "config", "goal_sender.yaml")

    return LaunchDescription([
        Node(
            package="tb3_waypoint_follower_cpp",
            executable="goal_sender",
            name="goal_sender",
            output="screen",
            parameters=[params_file],

            
        )
        
    ])
