/*!*******************************************************************************************
 *  \file       basic_motion_references.cpp
 *  \brief      Virtual class for basic motion references implementations
 *  \authors    Miguel Fernández Cortizas
 *              Pedro Arias Pérez
 *              David Pérez Saura
 *              Rafael Pérez Seguí
 *
 *  \copyright  Copyright (c) 2022 Universidad Politécnica de Madrid
 *              All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************************/

#include "motion_reference_handlers/basic_motion_references.hpp"

#include <as2_core/names/topics.hpp>

namespace as2 {
namespace motionReferenceHandlers {
BasicMotionReferenceHandler::BasicMotionReferenceHandler(as2::Node *as2_ptr) : node_ptr_(as2_ptr) {
  if (number_of_instances_ == 0) {
    // Publisher
    command_traj_pub_ = node_ptr_->create_publisher<trajectory_msgs::msg::JointTrajectoryPoint>(
        as2_names::topics::motion_reference::trajectory, as2_names::topics::motion_reference::qos);

    command_pose_pub_ = node_ptr_->create_publisher<geometry_msgs::msg::PoseStamped>(
        as2_names::topics::motion_reference::pose, as2_names::topics::motion_reference::qos);

    command_twist_pub_ = node_ptr_->create_publisher<geometry_msgs::msg::TwistStamped>(
        as2_names::topics::motion_reference::twist, as2_names::topics::motion_reference::qos);

    // Subscriber
    controller_info_sub_ = node_ptr_->create_subscription<as2_msgs::msg::ControllerInfo>(
        as2_names::topics::controller::info, rclcpp::QoS(1),
        [](const as2_msgs::msg::ControllerInfo::SharedPtr msg) {
          current_mode_ = msg->current_control_mode;
        });

    // Set initial control mode
    desired_control_mode_.yaw_mode        = as2_msgs::msg::ControlMode::NONE;
    desired_control_mode_.control_mode    = as2_msgs::msg::ControlMode::UNSET;
    desired_control_mode_.reference_frame = as2_msgs::msg::ControlMode::UNDEFINED_FRAME;
  }

  number_of_instances_++;
  RCLCPP_DEBUG(node_ptr_->get_logger(),
               "There are %d instances of BasicMotionReferenceHandler created",
               number_of_instances_);
};

BasicMotionReferenceHandler::~BasicMotionReferenceHandler() {
  number_of_instances_--;
  if (number_of_instances_ == 0 && node_ptr_ != nullptr) {
    RCLCPP_DEBUG(node_ptr_->get_logger(), "Deleting node_ptr_");
    controller_info_sub_.reset();
    command_traj_pub_.reset();
    command_pose_pub_.reset();
    command_twist_pub_.reset();
  }
};

bool BasicMotionReferenceHandler::sendCommand() {
  // TODO: Check comparation
  // if (this->current_mode_ != desired_control_mode_)
  if (this->current_mode_.yaw_mode != desired_control_mode_.yaw_mode ||
      this->current_mode_.control_mode != desired_control_mode_.control_mode ||
      this->current_mode_.reference_frame != desired_control_mode_.reference_frame) {
    if (!setMode(desired_control_mode_)) {
      return false;
    }
  }
  publishCommands();
  return true;
};

void BasicMotionReferenceHandler::publishCommands() {
  rclcpp::Time stamp = node_ptr_->now();
  command_traj_pub_->publish(command_trajectory_msg_);
  command_pose_pub_->publish(command_pose_msg_);
  command_twist_pub_->publish(command_twist_msg_);
}

bool BasicMotionReferenceHandler::setMode(const as2_msgs::msg::ControlMode &mode) {
  RCLCPP_INFO(node_ptr_->get_logger(), "Setting control mode to [%s]",
              as2::control_mode::controlModeToString(mode).c_str());

  // Set request
  auto request         = as2_msgs::srv::SetControlMode::Request();
  auto response        = as2_msgs::srv::SetControlMode::Response();
  request.control_mode = mode;

  auto set_mode_cli = as2::SynchronousServiceClient<as2_msgs::srv::SetControlMode>(
      as2_names::services::controller::set_control_mode);

  bool out = set_mode_cli.sendRequest(request, response);

  if (out && response.success) {
    this->current_mode_ = mode;
    return true;
  }
  RCLCPP_ERROR(node_ptr_->get_logger(),
               " Controller Control Mode was not able to be settled sucessfully");
  return false;
};

int BasicMotionReferenceHandler::number_of_instances_ = 0;

rclcpp::Subscription<as2_msgs::msg::ControllerInfo>::SharedPtr
    BasicMotionReferenceHandler::controller_info_sub_ = nullptr;

as2_msgs::msg::ControlMode BasicMotionReferenceHandler::current_mode_ =
    as2_msgs::msg::ControlMode();

rclcpp::Publisher<trajectory_msgs::msg::JointTrajectoryPoint>::SharedPtr
    BasicMotionReferenceHandler::command_traj_pub_ = nullptr;
rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr
    BasicMotionReferenceHandler::command_pose_pub_ = nullptr;
rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr
    BasicMotionReferenceHandler::command_twist_pub_ = nullptr;

}  // namespace motionReferenceHandlers
}  // namespace as2
