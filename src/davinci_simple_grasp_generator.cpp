/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2018, Case Western Reserve University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of SRI International nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/* Author: Su Lu <sxl924@case.edu>
   Desc:   Grasp generator for generating simple grasping trajectory
*/

#include <cwru_davinci_grasp/davinci_simple_grasp_generator.h>
#include <cwru_davinci_grasp/davinci_simple_grasp_constants.h>

namespace cwru_davinci_grasp
{

DavinciSimpleGraspGenerator::DavinciSimpleGraspGenerator(moveit_visual_tools::MoveItVisualToolsPtr visual_tools,
                                                         bool verbose) :
  visual_tools_(visual_tools), verbose_(verbose)
{
  ROS_INFO("DavinciSimpleGraspGenerator, loaded simple needle generator");
}

DavinciSimpleGraspGenerator::~DavinciSimpleGraspGenerator()
{
  convenience_ros_functions::ROSFunctions::destroySingleton();
}

bool DavinciSimpleGraspGenerator::generateSimpleNeedleGrasps(
  const geometry_msgs::PoseStamped &needle_pose,
  const DavinciNeeldeGraspData &needleGraspData,
  std::vector<moveit_msgs::Grasp> &possible_grasps)
{
  // it's advisable to call initSingleton() before you need it for the first
  // time in a time-critical context.
  convenience_ros_functions::ROSFunctions::initSingleton();

  // ---------------------------------------------------------------------------------------------
  // first, transform from the object's frame (center of object) to /base_link
  geometry_msgs::PoseStamped _needle_pose = needle_pose;
  std::string base_frame = needleGraspData.base_link_;

  _needle_pose.header.stamp = ros::Time(0); // most recent time for transformPose
  geometry_msgs::PoseStamped needle_pose_in_base_frame_;

  if (convenience_ros_functions::ROSFunctions::Singleton()->transformPose(
    _needle_pose, base_frame, needle_pose_in_base_frame_, 1) != 0)
  {
    ROS_ERROR("Test transform failed.");
    return false;
  }

  tf::poseMsgToEigen(needle_pose_in_base_frame_.pose, needle_pose_wrt_base_frame_);

  ROS_INFO("object pose is now transformed to base frame %s", base_frame.c_str());

  // ---------------------------------------------------------------------------------------------
  // Grasp parameters

  // Create re-usable approach motion
  moveit_msgs::GripperTranslation pre_grasp_approach;
  pre_grasp_approach.direction.header.stamp = ros::Time::now();

  // The distance the origin of a robot link needs to travel
  pre_grasp_approach.desired_distance = needleGraspData.approach_retreat_desired_dist_;
  pre_grasp_approach.min_distance = needleGraspData.approach_retreat_min_dist_;

  // Create re-usable retreat motion
  moveit_msgs::GripperTranslation post_grasp_retreat;
  post_grasp_retreat.direction.header.stamp = ros::Time::now();

  // The distance the origin of a robot link needs to travel
  post_grasp_retreat.desired_distance = needleGraspData.approach_retreat_desired_dist_;
  post_grasp_retreat.min_distance = needleGraspData.approach_retreat_min_dist_;

  // Create re-usable blank pose
  geometry_msgs::PoseStamped grasp_pose_msg;
  grasp_pose_msg.header.stamp = ros::Time::now();
  grasp_pose_msg.header.frame_id = needleGraspData.base_link_;

//  double grasp_theta_0;
//  double grasp_theta_1;
//  double grasp_theta_2;
//  double grasp_theta_3;

  for(int i = 0; i < needleGraspData.grasp_theta_0_list_.size(); i++)
  {
    //  inside first loop
    double grasp_theta_0 = needleGraspData.grasp_theta_0_list_[i];

    for (int j = 0; j < needleGraspData.grasp_theta_1_list_.size(); j++)
    {
      //  inside second loop
      double grasp_theta_1 = needleGraspData.grasp_theta_1_list_[j];

      for (int k = 0; k < needleGraspData.grasp_theta_2_list_.size(); k++)
      {
        //  inside third loop
        double grasp_theta_2 = needleGraspData.grasp_theta_2_list_[k];

        for (int l = 0; l < needleGraspData.grasp_theta_3_list_.size(); l++)
        {
          //  inside fourth loop
          double grasp_theta_3 = needleGraspData.grasp_theta_3_list_[l];

          ROS_INFO_STREAM("All four grasping parameters: " << grasp_theta_0 << " "
                                                           << grasp_theta_1 << " "
                                                           << grasp_theta_2 << " "
                                                           << grasp_theta_3);
          double grasping_parameters[] = {grasp_theta_0, grasp_theta_1, grasp_theta_2, grasp_theta_3};

          moveit_msgs::Grasp new_grasp;
          // A name for this grasp
          static int grasp_id = 0;
          new_grasp.id = "Grasp" + boost::lexical_cast<std::string>(grasp_id);
          ++grasp_id;

          // PreGrasp and Grasp Postures --------------------------------------------------------------------------

          // The internal posture of the hand for the pre-grasp only positions are used
          new_grasp.pre_grasp_posture = needleGraspData.pre_grasp_posture_;

          // The internal posture of the hand for the grasp positions and efforts are used
          new_grasp.grasp_posture = needleGraspData.grasp_posture_;

          // Grasp ------------------------------------------------------------------------------------------------

          Eigen::Affine3d grasp_pose;  // needle wrt to gripper tool tip frame frame
          grasp_pose = calNeedleToGripperPose(grasping_parameters, needleGraspData.needle_radius_);

          if( verbose_ )
          {
            // Convert pose to global frame (base_link)
            tf::poseEigenToMsg(needle_pose_wrt_base_frame_ * grasp_pose.inverse(), grasp_pose_msg.pose);
            visual_tools_->publishArrow(grasp_pose_msg.pose, rviz_visual_tools::GREEN);
          }

          // ------------------------------------------------------------------------
          // Convert pose to global frame (base_link)
          tf::poseEigenToMsg(needle_pose_wrt_base_frame_ * grasp_pose.inverse(), grasp_pose_msg.pose);

          // The position of the end-effector for the grasp relative to a reference frame (that is always specified elsewhere, not in this message)
          new_grasp.grasp_pose = grasp_pose_msg;

          // Other ------------------------------------------------------------------------------------------------

          // the maximum contact force to use while grasping (<=0 to disable)
          new_grasp.max_contact_force = 0;

          // -------------------------------------------------------------------------------------------------------
          // -------------------------------------------------------------------------------------------------------
          // Approach and retreat
          // -------------------------------------------------------------------------------------------------------
          // -------------------------------------------------------------------------------------------------------

          // Straight down ---------------------------------------------------------------------------------------
          // With respect to the base link/world frame

          // Approach
          pre_grasp_approach.direction.header.frame_id = needleGraspData.base_link_;
          pre_grasp_approach.direction.vector.x = 0;
          pre_grasp_approach.direction.vector.y = 0;
          pre_grasp_approach.direction.vector.z = -1; // Approach direction (negative z axis)  // TODO: document this assumption
          new_grasp.pre_grasp_approach = pre_grasp_approach;

          // Retreat
          post_grasp_retreat.direction.header.frame_id = needleGraspData.base_link_;
          post_grasp_retreat.direction.vector.x = 0;
          post_grasp_retreat.direction.vector.y = 0;
          post_grasp_retreat.direction.vector.z = 1; // Retreat direction (pos z axis)
          new_grasp.post_grasp_retreat = post_grasp_retreat;

          // Add to vector
          possible_grasps.push_back(new_grasp);
        }
      }
    }
  }
  ROS_INFO_STREAM_NAMED("grasp", "Generated " << possible_grasps.size() << " grasps." );
  return true;
}


bool DavinciSimpleGraspGenerator::generateDefinedSimpleNeedleGrasp(const geometry_msgs::PoseStamped &needle_pose,
                                                                   const DavinciNeeldeGraspData &needleGraspData,
                                                                   moveit_msgs::Grasp &possible_grasp)
{
  // it's advisable to call initSingleton() before you need it for the first
  // time in a time-critical context.
  convenience_ros_functions::ROSFunctions::initSingleton();

  // ---------------------------------------------------------------------------------------------
  // first, transform from the object's frame (center of object) to /base_link
  geometry_msgs::PoseStamped _needle_pose = needle_pose;
  std::string base_frame = needleGraspData.base_link_;

  _needle_pose.header.stamp = ros::Time(0); // most recent time for transformPose
  geometry_msgs::PoseStamped needle_pose_in_base_frame_;

  if (convenience_ros_functions::ROSFunctions::Singleton()->transformPose(
    _needle_pose, base_frame, needle_pose_in_base_frame_, 1) != 0)
  {
    ROS_ERROR("Test transform failed.");
    return false;
  }

  tf::poseMsgToEigen(needle_pose_in_base_frame_.pose, needle_pose_wrt_base_frame_);

  ROS_INFO("object pose is now transformed to base frame %s", base_frame.c_str());

  // ---------------------------------------------------------------------------------------------
  // Grasp parameters

  // Create re-usable approach motion
  moveit_msgs::GripperTranslation pre_grasp_approach;
  pre_grasp_approach.direction.header.stamp = ros::Time::now();

  // The distance the origin of a robot link needs to travel
  pre_grasp_approach.desired_distance = needleGraspData.approach_retreat_desired_dist_;
  pre_grasp_approach.min_distance = needleGraspData.approach_retreat_min_dist_;

  // Create re-usable retreat motion
  moveit_msgs::GripperTranslation post_grasp_retreat;
  post_grasp_retreat.direction.header.stamp = ros::Time::now();

  // The distance the origin of a robot link needs to travel
  post_grasp_retreat.desired_distance = needleGraspData.approach_retreat_desired_dist_;
  post_grasp_retreat.min_distance = needleGraspData.approach_retreat_min_dist_;

  // Create re-usable blank pose
  geometry_msgs::PoseStamped grasp_pose_msg;
  grasp_pose_msg.header.stamp = ros::Time::now();
  grasp_pose_msg.header.frame_id = needleGraspData.base_link_;

  double grasp_theta_0 = needleGraspData.grasp_theta_0_;
  double grasp_theta_1 = needleGraspData.grasp_theta_1_;
  double grasp_theta_2 = needleGraspData.grasp_theta_2_;
  double grasp_theta_3 = needleGraspData.grasp_theta_3_;

  ROS_INFO_STREAM("All four grasping parameters: " << grasp_theta_0 << " "
                                                   << grasp_theta_1 << " "
                                                   << grasp_theta_2 << " "
                                                   << grasp_theta_3);
  double grasping_parameters[] = {grasp_theta_0, grasp_theta_1, grasp_theta_2, grasp_theta_3};

  moveit_msgs::Grasp new_grasp;
  // A name for this grasp
  static int grasp_id = 0;
  new_grasp.id = "Grasp" + boost::lexical_cast<std::string>(grasp_id);
  ++grasp_id;

  // PreGrasp and Grasp Postures --------------------------------------------------------------------------

  // The internal posture of the hand for the pre-grasp only positions are used
  new_grasp.pre_grasp_posture = needleGraspData.pre_grasp_posture_;

  // The internal posture of the hand for the grasp positions and efforts are used
  new_grasp.grasp_posture = needleGraspData.grasp_posture_;

  // Grasp ------------------------------------------------------------------------------------------------

  Eigen::Affine3d grasp_pose;  // needle wrt to gripper tool tip frame frame
  grasp_pose = calNeedleToGripperPose(grasping_parameters, needleGraspData.needle_radius_);

  if (verbose_)
  {
    // Convert pose to global frame (base_link)
    tf::poseEigenToMsg(needle_pose_wrt_base_frame_ * grasp_pose.inverse(), grasp_pose_msg.pose);
    visual_tools_->publishArrow(grasp_pose_msg.pose, rviz_visual_tools::GREEN);
  }

  // ------------------------------------------------------------------------
  // Convert pose to global frame (base_link)
  tf::poseEigenToMsg(needle_pose_wrt_base_frame_ * grasp_pose.inverse(), grasp_pose_msg.pose);

  // The position of the end-effector for the grasp relative to a reference frame (that is always specified elsewhere, not in this message)
  new_grasp.grasp_pose = grasp_pose_msg;

  // Other ------------------------------------------------------------------------------------------------

  // the maximum contact force to use while grasping (<=0 to disable)
  new_grasp.max_contact_force = 0;

  // -------------------------------------------------------------------------------------------------------
  // -------------------------------------------------------------------------------------------------------
  // Approach and retreat
  // -------------------------------------------------------------------------------------------------------
  // -------------------------------------------------------------------------------------------------------

  // Straight down ---------------------------------------------------------------------------------------
  // With respect to the base link/world frame

  // Approach
  pre_grasp_approach.direction.header.frame_id = needleGraspData.base_link_;
  pre_grasp_approach.direction.vector.x = 0;
  pre_grasp_approach.direction.vector.y = 0;
  pre_grasp_approach.direction.vector.z = -1; // Approach direction (negative z axis)  // TODO: document this assumption
  new_grasp.pre_grasp_approach = pre_grasp_approach;

  // Retreat
  post_grasp_retreat.direction.header.frame_id = needleGraspData.base_link_;
  post_grasp_retreat.direction.vector.x = 0;
  post_grasp_retreat.direction.vector.y = 0;
  post_grasp_retreat.direction.vector.z = 1; // Retreat direction (pos z axis)
  new_grasp.post_grasp_retreat = post_grasp_retreat;

  // Add to vector
  possible_grasp = new_grasp;
  return true;
}

Eigen::Affine3d DavinciSimpleGraspGenerator::calNeedleToGripperPose(const double (&grasping_parameters)[4],
                                                                    const double &needle_radius)
{
  double theta_0 = grasping_parameters[0];
  double theta_1 = grasping_parameters[1];
  double theta_2 = grasping_parameters[2];
  double theta_3 = grasping_parameters[3];

  Eigen::Affine3d desired_needle_to_gripper_pose;

  Eigen::Matrix3d orientaion; // orientation part
  orientaion <<
             cos(theta_1) * sin(theta_3) - cos(theta_3) * sin(theta_1) * sin(theta_2),
    -cos(theta_1) * cos(theta_3) - sin(theta_1) * sin(theta_2) * sin(theta_3),
    cos(theta_2) * sin(theta_1), -cos(theta_2) * cos(theta_3),
    -cos(theta_2) * sin(theta_3), -sin(theta_2),
    sin(theta_1) * sin(theta_3) + cos(theta_1) * cos(theta_3) * sin(theta_2),
    cos(theta_1) * sin(theta_2) * sin(theta_3) - cos(theta_3) * sin(theta_1),
    -cos(theta_1) * cos(theta_2);

  Eigen::Vector3d translation; // translation part
  translation << needle_radius * sin(theta_1) * sin(theta_2),
    needle_radius * cos(theta_2),
    -theta_0 - needle_radius * cos(theta_1) * sin(theta_2);

  desired_needle_to_gripper_pose.linear() =
    orientaion; // initialize linear part by Eigen::Matrix3d type orientation
  desired_needle_to_gripper_pose.translation() = translation;

  return desired_needle_to_gripper_pose;
}

}  // namespace
