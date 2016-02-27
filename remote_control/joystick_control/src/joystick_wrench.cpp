// %Tag(FULL)%
// %Tag(INCLUDE)%
#include <ros/ros.h>
#include <ros/console.h>
#include <geometry_msgs/WrenchStamped.h>
#include <geometry_msgs/Twist.h>
#include <sensor_msgs/Joy.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Bool.h>
#include "navigator_msg_multiplexer/wrench_arbiter.h"
#include <tf/transform_datatypes.h>
// %EndTag(INCLUDE)%

// %Tag(CLASSDEF)%
class JoystickWrench
{
   public:
     JoystickWrench();

   private:
     void joyCallback(const sensor_msgs::Joy::ConstPtr& joy);
     void odom_cb(const nav_msgs::Odometry::ConstPtr& odom);
     ros::NodeHandle nh_;
     double force_scale_;
     double torque_scale_;
     int axis_x_force_;
     int axis_y_force_;
     int axis_torque_;
     ros::Publisher wrench_pub_;
     ros::Publisher des_pose_pub;
     ros::Publisher kill_pub;
     ros::Subscriber joy_sub_;
     ros::Subscriber odom_sub_;
     ros::Publisher vel_pub_;
     ros::ServiceClient client;
     bool wrench_controller;
     int last_controller_state;
     int last_kill_state;
     int killed;
     nav_msgs::Odometry cur_pose;


};
// %EndTag(CLASSDEF)%

JoystickWrench::JoystickWrench()
{
// %Tag(Param)%
   nh_.param("force_scale", force_scale_, force_scale_);
   nh_.param("torque_scale", torque_scale_, torque_scale_);
// %EndTag(PARAMS)%

// %Tag(PUB)%
   wrench_pub_= nh_.advertise<geometry_msgs::WrenchStamped>("/wrench/rc", 1);
   kill_pub = nh_.advertise<std_msgs::Bool>("/killed", 1);
   des_pose_pub = nh_.advertise<geometry_msgs::Point>("/set_desired_pose", 1);
   client = nh_.serviceClient<navigator_msg_multiplexer::wrench_arbiter>("change_wrench");
   wrench_controller = true;
   killed = false;
   last_controller_state = 0;
   last_kill_state = 0;
// %EndTag(PUB)%

// %Tag(SUB)%
   joy_sub_ = nh_.subscribe<sensor_msgs::Joy>("joy", 10, &JoystickWrench::joyCallback, this);
   odom_sub_ = nh_.subscribe<nav_msgs::Odometry>("odom", 10, &JoystickWrench::odom_cb, this);
// %EndTag(SUB)%

}

void JoystickWrench::odom_cb(const nav_msgs::Odometry::ConstPtr& odom){

  cur_pose = *odom;
}

// %Tag(CALLBACK)%
void JoystickWrench::joyCallback(const sensor_msgs::Joy::ConstPtr& joy)
{


  //Motor::Wrench wrench
  geometry_msgs::WrenchStamped wrench;
  navigator_msg_multiplexer::wrench_arbiter w_control;


  if (joy->buttons[7] == 1 && joy->buttons[7] != last_controller_state)
  {
    wrench_controller = !(wrench_controller);
    if (wrench_controller == false)
    {
      w_control.request.str = "rc";
      if (!client.call(w_control))
      {
        ROS_ERROR("Failed to change controller");
      }
    }
    if (wrench_controller == true)
    {
      w_control.request.str = "autonomous";
      if (!client.call(w_control))
      {
        ROS_ERROR("Failed to change controller");
      }
    }
  }

  if (killed == false)
  {
    wrench.header.frame_id = "/base_link";
    wrench.wrench.force.x = force_scale_*joy->axes[1];
    wrench.wrench.force.y = -1*force_scale_*joy->axes[0];
    wrench.wrench.torque.z = -1*torque_scale_*joy->axes[3];
    std_msgs::Bool var;
    var.data = true;
    kill_pub.publish(var);
  } else{
    std_msgs::Bool var;
    var.data = false;
    kill_pub.publish(var);
  }

  if (joy->buttons[0] ==1)
  {
    geometry_msgs::Point point;



    tf::Quaternion q(cur_pose.pose.pose.orientation.x, cur_pose.pose.pose.orientation.y, cur_pose.pose.pose.orientation.z, cur_pose.pose.pose.orientation.w);
    tf::Matrix3x3 m(q);
    double roll, pitch, yaw;
    m.getRPY(roll, pitch, yaw);

    point.x = cur_pose.pose.pose.position.x;
    point.y = cur_pose.pose.pose.position.y;
    point.z = yaw;


    des_pose_pub.publish(point);

    w_control.request.str = "autonomous";
    if (!client.call(w_control))
    {
      ROS_ERROR("Failed to change controller");
    }

  }

  if (joy->buttons[8] == 1 && joy->buttons[8] != last_controller_state)
  {
    killed = !(killed);
  }
  last_controller_state = joy->buttons[7];
  last_kill_state = joy->buttons[8];
  wrench_pub_.publish(wrench);

}
// %EndTag(CALLBACK)%

// %Tag(MAIN)%
int main(int argc, char** argv)
{
  ros::init(argc, argv, "joystick_wrench");
  JoystickWrench joystick_wrench;

  ros::spin();
}
// %EndTag(MAIN)%
// %EndTag(FULL)%
