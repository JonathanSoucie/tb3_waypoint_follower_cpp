#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include <geometry_msgs/msg/pose_stamped.hpp>
#include <nav2_msgs/action/navigate_to_pose.hpp>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>

using namespace std::chrono_literals;

class goal_sender : public rclcpp::Node
{
  using NavigateToPose = nav2_msgs::action::NavigateToPose;
  using RetrieveGoal = geometry_msgs::msg::PoseStamped;
  using GoalHandleNavigateToPose = rclcpp_action::ClientGoalHandle<NavigateToPose>;

public:
  goal_sender()
  : Node("goal_sender")
  {
    // Default Parameters
    this->declare_parameter("action_name", "/navigate_to_pose");
    this->declare_parameter("goal_topic", "/goal_pose");
    this->declare_parameter("goal_frame", "map");
    this->declare_parameter("server_timeout_sec", 50.0);

    this->declare_parameter("cancel_on_new_goal", true);
    this->declare_parameter("require_goal_frame", false);

    // Get Parameters from yaml
    action_name_ = this->get_parameter("action_name").as_string();
    goal_topic_ = this->get_parameter("goal_topic").as_string();
    goal_frame_ = this->get_parameter("goal_frame").as_string();
    server_timeout_sec_ = this->get_parameter("server_timeout_sec").as_double();

    cancel_on_new_goal_ = this->get_parameter("cancel_on_new_goal").as_bool();
    require_goal_frame_ = this->get_parameter("require_goal_frame").as_bool();

    // Print parameters
    RCLCPP_INFO(this->get_logger(), "action_name: %s", action_name_.c_str());
    RCLCPP_INFO(this->get_logger(), "goal_topic: %s", goal_topic_.c_str());
    RCLCPP_INFO(this->get_logger(), "goal_frame: %s", goal_frame_.c_str());
    RCLCPP_INFO(this->get_logger(), "server_timeout_sec: %.2f", server_timeout_sec_);
    RCLCPP_INFO(this->get_logger(), "cancel_on_new_goal: %s", cancel_on_new_goal_ ? "true" : "false");
    RCLCPP_INFO(this->get_logger(), "require_goal_frame: %s", require_goal_frame_ ? "true" : "false");

    // Action Client: Sends goal to Nav2
    nav2_client_ = rclcpp_action::create_client<NavigateToPose>(this, action_name_);

    auto timeout = std::chrono::duration<double>(server_timeout_sec_);

    RCLCPP_INFO(
      this->get_logger(),
      "Waiting for Nav2 action server '%s' (timeout %.1f s)...",
      action_name_.c_str(),
      server_timeout_sec_
    );

    server_ready_ = nav2_client_->wait_for_action_server(timeout);

    if (!server_ready_) {
      RCLCPP_ERROR(
        this->get_logger(),
        "Nav2 action server '%s' not available after %.1f seconds",
        action_name_.c_str(),
        server_timeout_sec_
      );
    } else {
      RCLCPP_INFO(this->get_logger(), "Nav2 action server is ready: %s", action_name_.c_str());
    }

    // Goal Subscriber (NOW calls your member callback)
    goal_sub_ = this->create_subscription<RetrieveGoal>(
      goal_topic_,
      rclcpp::QoS(10),
      std::bind(&goal_sender::on_goal_pose, this, std::placeholders::_1)
    );

    RCLCPP_INFO(this->get_logger(), "Subscribed to goal topic: %s", goal_topic_.c_str());
  }

private:
  // ---- Callback that runs when a PoseStamped goal arrives ----
  void on_goal_pose(const RetrieveGoal::SharedPtr msg)
  {
    RCLCPP_INFO(
      this->get_logger(),
      "Got goal in frame '%s': x=%.3f y=%.3f",
      msg->header.frame_id.c_str(),
      msg->pose.position.x,
      msg->pose.position.y
    );

    if (!server_ready_) {
      RCLCPP_WARN(this->get_logger(), "Received goal but Nav2 action server not ready yet. Ignoring.");
      return;
    }

    if (msg->header.frame_id.empty()) {
      RCLCPP_ERROR(this->get_logger(), "Goal header.frame_id is empty. Rejecting.");
      return;
    }

    if (require_goal_frame_ && msg->header.frame_id != goal_frame_) {
      RCLCPP_ERROR(
        this->get_logger(),
        "Goal must be in frame '%s' but got '%s'. Rejecting (no TF transform implemented yet).",
        goal_frame_.c_str(),
        msg->header.frame_id.c_str()
      );
      return;
    }

    // If already navigating: cancel or ignore
    if (current_goal_handle_) {
      if (cancel_on_new_goal_) {
        RCLCPP_WARN(this->get_logger(), "New goal received: canceling previous goal...");
        (void)nav2_client_->async_cancel_goal(current_goal_handle_);
        current_goal_handle_.reset();
      } else {
        RCLCPP_WARN(this->get_logger(), "Already navigating and cancel_on_new_goal=false. Ignoring new goal.");
        return;
      }
    }

    // Build Nav2 goal
    NavigateToPose::Goal goal_msg;
    goal_msg.pose = *msg;
    goal_msg.pose.header.stamp = this->now();  // keep it "fresh"

    // Action callbacks
    rclcpp_action::Client<NavigateToPose>::SendGoalOptions options;

    options.goal_response_callback =
      [this](std::shared_ptr<GoalHandleNavigateToPose> goal_handle)
      {
        if (!goal_handle) {
          RCLCPP_ERROR(this->get_logger(), "Nav2 rejected the goal.");
          current_goal_handle_.reset();
        } else {
          RCLCPP_INFO(this->get_logger(), "Nav2 accepted the goal.");
          current_goal_handle_ = goal_handle;
        }
      };

    options.feedback_callback =
      [this](
        std::shared_ptr<GoalHandleNavigateToPose> /*goal_handle*/,
        const std::shared_ptr<const NavigateToPose::Feedback> feedback)
      {
        RCLCPP_INFO_THROTTLE(
          this->get_logger(),
          *this->get_clock(),
          1000,
          "Feedback: distance_remaining=%.3f",
          feedback->distance_remaining
        );
      };

    options.result_callback =
      [this](const GoalHandleNavigateToPose::WrappedResult & result)
      {
        switch (result.code) {
          case rclcpp_action::ResultCode::SUCCEEDED:
            RCLCPP_INFO(this->get_logger(), "Navigation SUCCEEDED.");
            break;
          case rclcpp_action::ResultCode::ABORTED:
            RCLCPP_ERROR(this->get_logger(), "Navigation ABORTED.");
            break;
          case rclcpp_action::ResultCode::CANCELED:
            RCLCPP_WARN(this->get_logger(), "Navigation CANCELED.");
            break;
          default:
            RCLCPP_ERROR(this->get_logger(), "Navigation finished with unknown result code.");
            break;
        }
        current_goal_handle_.reset();
      };

    // Send goal
    (void)nav2_client_->async_send_goal(goal_msg, options);
    RCLCPP_INFO(this->get_logger(), "Sent goal to Nav2.");
  }

private:
  // Parameters
  std::string action_name_;
  std::string goal_topic_;
  std::string goal_frame_;
  double server_timeout_sec_{50.0};
  bool cancel_on_new_goal_{true};
  bool require_goal_frame_{false};

  // Subscriber
  rclcpp::Subscription<RetrieveGoal>::SharedPtr goal_sub_;

  // Nav2 action client
  rclcpp_action::Client<NavigateToPose>::SharedPtr nav2_client_;
  bool server_ready_{false};

  // Track active goal
  std::shared_ptr<GoalHandleNavigateToPose> current_goal_handle_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<goal_sender>());
  rclcpp::shutdown();
  return 0;
}
