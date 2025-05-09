// Copyright 2024 Tier IV, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef POINTCLOUD_BASED_OCCUPANCY_GRID_MAP__POINTCLOUD_BASED_OCCUPANCY_GRID_MAP_NODE_HPP_
#define POINTCLOUD_BASED_OCCUPANCY_GRID_MAP__POINTCLOUD_BASED_OCCUPANCY_GRID_MAP_NODE_HPP_

#include "autoware/probabilistic_occupancy_grid_map/costmap_2d/occupancy_grid_map_base.hpp"
#include "autoware/probabilistic_occupancy_grid_map/updater/binary_bayes_filter_updater.hpp"
#include "autoware/probabilistic_occupancy_grid_map/updater/ogm_updater_interface.hpp"
#include "autoware/probabilistic_occupancy_grid_map/utils/cuda_pointcloud.hpp"

#include <autoware_utils/ros/debug_publisher.hpp>
#include <autoware_utils/ros/diagnostics_interface.hpp>
#include <autoware_utils/system/stop_watch.hpp>
#include <autoware_utils/system/time_keeper.hpp>
#include <builtin_interfaces/msg/time.hpp>
#include <laser_geometry/laser_geometry.hpp>
#include <rclcpp/rclcpp.hpp>

#include <sensor_msgs/msg/laser_scan.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>

#include <cuda_runtime.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/transform_listener.h>

#include <memory>
#include <string>

namespace autoware::occupancy_grid_map
{
using builtin_interfaces::msg::Time;
using costmap_2d::OccupancyGridMapInterface;
using costmap_2d::OccupancyGridMapUpdaterInterface;
using laser_geometry::LaserProjection;
using nav2_costmap_2d::Costmap2D;
using nav_msgs::msg::OccupancyGrid;
using sensor_msgs::msg::LaserScan;
using sensor_msgs::msg::PointCloud2;
using tf2_ros::Buffer;
using tf2_ros::TransformListener;

class PointcloudBasedOccupancyGridMapNode : public rclcpp::Node
{
public:
  explicit PointcloudBasedOccupancyGridMapNode(const rclcpp::NodeOptions & node_options);

private:
  void obstaclePointcloudCallback(const PointCloud2::ConstSharedPtr & input_obstacle_msg);
  void rawPointcloudCallback(const PointCloud2::ConstSharedPtr & input_raw_msg);
  void onPointcloudWithObstacleAndRaw();
  void checkProcessingTime(double processing_time_ms);

  OccupancyGrid::UniquePtr OccupancyGridMapToMsgPtr(
    const std::string & frame_id, const Time & stamp, const float & robot_pose_z,
    const Costmap2D & occupancy_grid_map);

private:
  rclcpp::Publisher<OccupancyGrid>::SharedPtr occupancy_grid_map_pub_;
  rclcpp::Subscription<PointCloud2>::SharedPtr obstacle_pointcloud_sub_ptr_;
  rclcpp::Subscription<PointCloud2>::SharedPtr raw_pointcloud_sub_ptr_;
  std::unique_ptr<autoware_utils::StopWatch<std::chrono::milliseconds>> stop_watch_ptr_{};
  std::unique_ptr<autoware_utils::DebugPublisher> debug_publisher_ptr_{};

  std::shared_ptr<Buffer> tf2_{std::make_shared<Buffer>(get_clock())};
  std::shared_ptr<TransformListener> tf2_listener_{std::make_shared<TransformListener>(*tf2_)};

  std::unique_ptr<OccupancyGridMapInterface> occupancy_grid_map_ptr_;
  std::unique_ptr<OccupancyGridMapUpdaterInterface> occupancy_grid_map_updater_ptr_;

  cudaStream_t stream_;
  CudaPointCloud2 raw_pointcloud_;
  CudaPointCloud2 obstacle_pointcloud_;

  autoware::cuda_utils::CudaUniquePtr<Eigen::Matrix3f> device_rotation_;
  autoware::cuda_utils::CudaUniquePtr<Eigen::Vector3f> device_translation_;

  // ROS Parameters
  std::string map_frame_;
  std::string base_link_frame_;
  std::string gridmap_origin_frame_;
  std::string scan_origin_frame_;
  bool use_height_filter_;
  double min_height_;
  double max_height_;
  bool enable_single_frame_mode_;
  bool filter_obstacle_pointcloud_by_raw_pointcloud_;

  // time keeper
  rclcpp::Publisher<autoware_utils::ProcessingTimeDetail>::SharedPtr
    detailed_processing_time_publisher_;
  std::shared_ptr<autoware_utils::TimeKeeper> time_keeper_;
  // diagnostics
  std::unique_ptr<autoware_utils::DiagnosticsInterface> diagnostics_interface_ptr_;
  double processing_time_tolerance_ms_;
  double processing_time_consecutive_excess_tolerance_ms_;
};

}  // namespace autoware::occupancy_grid_map

#endif  // POINTCLOUD_BASED_OCCUPANCY_GRID_MAP__POINTCLOUD_BASED_OCCUPANCY_GRID_MAP_NODE_HPP_
