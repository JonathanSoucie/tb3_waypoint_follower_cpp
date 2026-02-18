from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node

from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    use_sim_time = LaunchConfiguration("use_sim_time")

    declare_use_sim_time = DeclareLaunchArgument(
        "use_sim_time",
        default_value="True",
        description="Use simulation (Gazebo) clock if true",
    )

    # Package share dirs (reliable)
    tb3_gazebo_share = get_package_share_directory("turtlebot3_gazebo")
    slam_toolbox_share = get_package_share_directory("slam_toolbox")
    tb3_nav2_share = get_package_share_directory("turtlebot3_navigation2")
    my_pkg_share = get_package_share_directory("tb3_waypoint_follower_cpp")

    # --- Gazebo ---
    gazebo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(tb3_gazebo_share, "launch", "turtlebot3_world.launch.py")
        )
    )

    # --- SLAM Toolbox ---
    slam_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(slam_toolbox_share, "launch", "online_async_launch.py")
        ),
        launch_arguments={"use_sim_time": use_sim_time}.items(),
    )

    # --- Nav2 ---
    nav2_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(tb3_nav2_share, "launch", "navigation2.launch.py")
        ),
        launch_arguments={"use_sim_time": use_sim_time}.items(),
    )

    # --- Your goal_sender launch (loads YAML) ---
    goal_sender_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(my_pkg_share, "launch", "goal_sender.launch.py")
        )
    )

    # --- RViz2 (force open) ---
    rviz_config = os.path.join(tb3_nav2_share, "rviz", "tb3_navigation2.rviz")
    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        output="screen",
        arguments=["-d", rviz_config],
        parameters=[{"use_sim_time": use_sim_time}],
    )

    return LaunchDescription(
        [
            declare_use_sim_time,
            gazebo_launch,
            slam_launch,
            nav2_launch,
            goal_sender_launch,
            rviz_node,
        ]
    )
