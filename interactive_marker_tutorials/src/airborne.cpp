//
// Created by lch on 19-6-12.
//

/*
 * Copyright (c) 2011, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */


#include <ros/ros.h>

#include <interactive_markers/interactive_marker_server.h>
#include <interactive_markers/menu_handler.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/InteractiveMarkerUpdate.h>
#include <visualization_msgs/InteractiveMarkerInit.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/Polygon.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/String.h>

#include <tf/transform_broadcaster.h>
#include <tf/tf.h>

#include <math.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <Eigen/Eigen>
#include <decomp_ros_msgs/cmd.h>
#include <ground_station_msgs/Cmd.h>
#include <rviz_visual_tools/mav_cmd_enum.h>

using namespace visualization_msgs;
using namespace Eigen;
using namespace std;

// %Tag(vars)%
boost::shared_ptr<interactive_markers::InteractiveMarkerServer> server;
interactive_markers::MenuHandler menu_handler;
InteractiveMarker interactiveCubeMarker;
InteractiveMarker interactiveQuadrotor;
static std::string mesh_resource;
//double start_x,start_y,start_z;

ros::Publisher drone_target_point, markerPub;
ros::Subscriber markerHandle1,markerHandle2, odometry_sub, gui_state_sub;
ros::Subscriber marker_pub_trigger;
ros::Timer marker_pub_timer;
ros::Time t_marker;
geometry_msgs::PoseStamped drone_target_pose_msg;
Vector3d virfence_min, virfence_max;
std_msgs::String gui_state;

bool marker_flag, joyContrlFlag, marker_holding_flag;
bool mesh_marker_flag, remap_flag;
bool marker_trigger_state;
int marker_type,seq;
double start_x, start_y, start_z;
double transit_x, transit_y, transit_z;

// vis text
//Marker text_marker;


// %EndTag(vars)%

// %Tag(Box)%

Marker makestaticQuadrotor(visualization_msgs::Marker &msg)
{
    Marker marker;
    marker.mesh_resource = mesh_resource;
    marker.type = visualization_msgs::Marker::MESH_RESOURCE;
    marker.scale.x = 1;
    marker.scale.y = 1;
    marker.scale.z = 1;
    marker.color.r = 0.5;
    marker.color.g = 0.5;
    marker.color.b = 0.5;
    marker.color.a = 1.0;
    return marker;
}

Marker makeQuadrotor(InteractiveMarker &msg )
{
    Marker marker;
    marker.mesh_resource = mesh_resource;
    marker.type = visualization_msgs::Marker::MESH_RESOURCE;
    marker.scale.x = msg.scale ;
    marker.scale.y = msg.scale ;
    marker.scale.z = msg.scale ;
    marker.color.r = 0.5;
    marker.color.g = 0.5;
    marker.color.b = 0.5;
    marker.color.a = 1.0;
    return marker;
}

InteractiveMarkerControl& makeQuadrotorControl(InteractiveMarker &msg)
{
    InteractiveMarkerControl control;
    control.always_visible = true;
    control.markers.push_back(makeQuadrotor(msg));
    msg.controls.push_back(control);
    return msg.controls.back();
}

Marker makeBox( InteractiveMarker &msg )
{
    Marker marker;

    marker.type = Marker::CUBE;
    marker.scale.x = msg.scale * 0.45;
    marker.scale.y = msg.scale * 0.45;
    marker.scale.z = msg.scale * 0.45;
    marker.color.r = 0.5;
    marker.color.g = 0.5;
    marker.color.b = 0.5;
    marker.color.a = 1.0;

    return marker;
}

InteractiveMarkerControl& makeBoxControl( InteractiveMarker &msg )
{
    InteractiveMarkerControl control;
    control.always_visible = true;
    control.markers.push_back( makeBox(msg) );
    msg.controls.push_back( control );
    return msg.controls.back();
}
// %EndTag(Box)%


void display_text()
{
    std::ostringstream text_str;
    Marker text_marker;
    text_marker.header.frame_id="world";
    text_marker.header.stamp = ros::Time::now();
    text_marker.ns = "basic_shapes";
    text_marker.action = visualization_msgs::Marker::ADD;
    text_marker.pose.orientation.x = 0.0;
    text_marker.pose.orientation.y = 0.0;
    text_marker.pose.orientation.z = 0.0;
    text_marker.pose.orientation.w = 1.0;
    text_marker.id =0;
    text_marker.type = visualization_msgs::Marker::TEXT_VIEW_FACING;

    text_marker.scale.z = 0.2;
    text_marker.color.b = 0;
    text_marker.color.g = 0;
    text_marker.color.r = 255;
    text_marker.color.a = 1;

    text_str<< std::fixed;
    text_str<< std::setprecision(2);
    text_str<<drone_target_pose_msg.pose.position.x<<","
       <<drone_target_pose_msg.pose.position.y<<","<<drone_target_pose_msg.pose.position.z;
    text_marker.text = text_str.str();
    text_marker.pose.position.x = drone_target_pose_msg.pose.position.x;
    text_marker.pose.position.y = drone_target_pose_msg.pose.position.y;
    text_marker.pose.position.z = drone_target_pose_msg.pose.position.z + 2;
    markerPub.publish(text_marker);
}

void geofence()
{
    if (drone_target_pose_msg.pose.position.x<virfence_min(0)){
        drone_target_pose_msg.pose.position.x = virfence_min(0);
    }
    else if (drone_target_pose_msg.pose.position.x > virfence_max(0)){
        drone_target_pose_msg.pose.position.x = virfence_max(0);
    }
    if (drone_target_pose_msg.pose.position.y < virfence_min(1)){
        drone_target_pose_msg.pose.position.y = virfence_min(1);
    }
    else if (drone_target_pose_msg.pose.position.y > virfence_max(1)){
        drone_target_pose_msg.pose.position.y = virfence_max(1);
    }
    if (drone_target_pose_msg.pose.position.z < virfence_min(2)){
        drone_target_pose_msg.pose.position.z = virfence_min(2);
    }
    else if (drone_target_pose_msg.pose.position.z > virfence_max(2)){
        drone_target_pose_msg.pose.position.z = virfence_max(2);
    }
}

void markerpubCallback(const ros::TimerEvent&)
{
//    ROS_INFO("pub cb 1111111111111111111");
    ros::Time time = ros::Time::now();
    ros::Duration delta_t = time-t_marker;

    if(delta_t > ros::Duration(0.2) && delta_t < ros::Duration(0.3)
        &&remap_flag){
        geofence();
        drone_target_point.publish(drone_target_pose_msg);
        ROS_INFO("pub target pose");
//        display_text();
    }

}

// %Tag(frameCallback)%
void frameCallback(const ros::TimerEvent&)
{
    ROS_INFO("fcb 11111111111111111");
    static uint32_t counter = 0;

    static tf::TransformBroadcaster br;

    tf::Transform t;

    ros::Time time = ros::Time::now();

    t.setOrigin(tf::Vector3(0.0, 0.0, 0.0));
    t.setRotation(tf::Quaternion(0.0, 0.0, 0.0, 1.0));
    br.sendTransform(tf::StampedTransform(t, time, "base_link", "world"));

    t.setOrigin(tf::Vector3(0.0, 0.0, 0.0));
    t.setRotation(tf::createQuaternionFromRPY(0.0, 0.0, 0.0));
    br.sendTransform(tf::StampedTransform(t, time, "base_link", "world"));

    counter++;
    ROS_INFO("fcb 222222222222222222");



}
// %EndTag(frameCallback)%

// %Tag(processFeedback)%
void processFeedback( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback )
{
    /*
    std::ostringstream s;
    s << "Feedback from marker '" << feedback->marker_name << "' "
      << " / control '" << feedback->control_name << "'";

    std::ostringstream mouse_point_ss;
    if( feedback->mouse_point_valid )
    {
        mouse_point_ss << " at " << feedback->mouse_point.x
                       << ", " << feedback->mouse_point.y
                       << ", " << feedback->mouse_point.z
                       << " in frame " << feedback->header.frame_id;
    }

//    switch ( feedback->event_type )
//    {
//        case visualization_msgs::InteractiveMarkerFeedback::BUTTON_CLICK:
//            ROS_INFO_STREAM( s.str() << ": button click" << mouse_point_ss.str() << "." );
//            break;
//
//        case visualization_msgs::InteractiveMarkerFeedback::MENU_SELECT:
//            ROS_INFO_STREAM( s.str() << ": menu item " << feedback->menu_entry_id << " clicked" << mouse_point_ss.str() << "." );
//            break;
//
//        case visualization_msgs::InteractiveMarkerFeedback::POSE_UPDATE:
//            ROS_INFO_STREAM( s.str() << ": pose changed"
//                                     << "\nposition = "
//                                     << feedback->pose.position.x
//                                     << ", " << feedback->pose.position.y
//                                     << ", " << feedback->pose.position.z
//                                     << "\norientation = "
//                                     << feedback->pose.orientation.w
//                                     << ", " << feedback->pose.orientation.x
//                                     << ", " << feedback->pose.orientation.y
//                                     << ", " << feedback->pose.orientation.z
//                                     << "\nframe: " << feedback->header.frame_id
//                                     << " time: " << feedback->header.stamp.sec << "sec, "
//                                     << feedback->header.stamp.nsec << " nsec" );
//            break;
//
//        case visualization_msgs::InteractiveMarkerFeedback::MOUSE_DOWN:
//            ROS_INFO_STREAM( s.str() << ": mouse down" << mouse_point_ss.str() << "." );
//            break;
//
//        case visualization_msgs::InteractiveMarkerFeedback::MOUSE_UP:
//            ROS_INFO_STREAM( s.str() << ": mouse up" << mouse_point_ss.str() << "." );
//            break;
//    }
    */
    server->applyChanges();

}
// %EndTag(processFeedback)%
/*
// %Tag(alignMarker)%
void alignMarker( const visualization_msgs::InteractiveMarkerFeedbackConstPtr &feedback )
{
    geometry_msgs::Pose pose = feedback->pose;

    pose.position.x = round(pose.position.x-0.5)+0.5;
    pose.position.y = round(pose.position.y-0.5)+0.5;

    ROS_INFO_STREAM( feedback->marker_name << ":"
                                           << " aligning position = "
                                           << feedback->pose.position.x
                                           << ", " << feedback->pose.position.y
                                           << ", " << feedback->pose.position.z
                                           << " to "
                                           << pose.position.x
                                           << ", " << pose.position.y
                                           << ", " << pose.position.z );

    server->setPose( feedback->marker_name, pose );
    server->applyChanges();
}
// %EndTag(alignMarker)%

double rand( double min, double max )
{
    double t = (double)rand() / (double)RAND_MAX;
    return min + t*(max-min);
}
*/
////////////////////////////////////////////////////////////////////////////////////

// %Tag(6DOF)%
void make6DofMarker( bool fixed, unsigned int interaction_mode, const tf::Vector3& position, bool show_6dof )
{
    InteractiveMarker int_marker;
    int_marker.header.frame_id = "world";
    tf::pointTFToMsg(position, int_marker.pose.position);
    int_marker.scale = 1;

    int_marker.name = "simple_6dof";
    int_marker.description = "Simple 6-DOF Control";

    // insert a box
    makeBoxControl(int_marker);
    int_marker.controls[0].interaction_mode = interaction_mode;

    InteractiveMarkerControl control;

    if ( fixed )
    {
        int_marker.name += "_fixed";
        int_marker.description += "\n(fixed orientation)";
        control.orientation_mode = InteractiveMarkerControl::FIXED;
    }

    if (interaction_mode != visualization_msgs::InteractiveMarkerControl::NONE)
    {
        std::string mode_text;
        if( interaction_mode == visualization_msgs::InteractiveMarkerControl::MOVE_3D )         mode_text = "MOVE_3D";
        if( interaction_mode == visualization_msgs::InteractiveMarkerControl::ROTATE_3D )       mode_text = "ROTATE_3D";
        if( interaction_mode == visualization_msgs::InteractiveMarkerControl::MOVE_ROTATE_3D )  mode_text = "MOVE_ROTATE_3D";
        int_marker.name += "_" + mode_text;
        int_marker.description = std::string("3D Control") + (show_6dof ? " + 6-DOF controls" : "") + "\n" + mode_text;
    }

    if(show_6dof)
    {
        tf::Quaternion orien(1.0, 0.0, 0.0, 1.0);
        orien.normalize();
        tf::quaternionTFToMsg(orien, control.orientation);
        control.name = "rotate_x";
        control.interaction_mode = InteractiveMarkerControl::ROTATE_AXIS;
        int_marker.controls.push_back(control);
        control.name = "move_x";
        control.interaction_mode = InteractiveMarkerControl::MOVE_AXIS;
        int_marker.controls.push_back(control);

        orien = tf::Quaternion(0.0, 1.0, 0.0, 1.0);
        orien.normalize();
        tf::quaternionTFToMsg(orien, control.orientation);
        control.name = "rotate_z";
        control.interaction_mode = InteractiveMarkerControl::ROTATE_AXIS;
        int_marker.controls.push_back(control);
        control.name = "move_z";
        control.interaction_mode = InteractiveMarkerControl::MOVE_AXIS;
        int_marker.controls.push_back(control);

        orien = tf::Quaternion(0.0, 0.0, 1.0, 1.0);
        orien.normalize();
        tf::quaternionTFToMsg(orien, control.orientation);
        control.name = "rotate_y";
        control.interaction_mode = InteractiveMarkerControl::ROTATE_AXIS;
        int_marker.controls.push_back(control);
        control.name = "move_y";
        control.interaction_mode = InteractiveMarkerControl::MOVE_AXIS;
        int_marker.controls.push_back(control);
    }

    server->insert(int_marker);
    server->setCallback(int_marker.name, &processFeedback);
    if (interaction_mode != visualization_msgs::InteractiveMarkerControl::NONE)
        menu_handler.apply( *server, int_marker.name );
}
// %EndTag(6DOF)%

void makeQuadrotorMarker(const tf::Vector3& position)
{
    interactiveQuadrotor.header.frame_id = "world";
    tf::pointTFToMsg(position, interactiveQuadrotor.pose.position);
    interactiveQuadrotor.description = "virDrone";
    interactiveQuadrotor.name = "virDrone";
    interactiveQuadrotor.scale = 1;

    makeQuadrotorControl(interactiveQuadrotor);

    InteractiveMarkerControl control;

    tf::Quaternion orien(0.0, 1.0, 0.0, 1.0);
    orien.normalize();
    tf::quaternionTFToMsg(orien, control.orientation);
    control.interaction_mode = InteractiveMarkerControl::MOVE_PLANE;
    interactiveQuadrotor.controls.push_back(control);
    control.interaction_mode = InteractiveMarkerControl::MOVE_AXIS;
    interactiveQuadrotor.controls.push_back(control);

    server->insert(interactiveQuadrotor);
    server->setCallback(interactiveQuadrotor.name, &processFeedback);

}

// %Tag(Quadrocopter)%
void makeCubeInteractMarker( const tf::Vector3& position )
{
    interactiveCubeMarker.header.frame_id = "world";
    tf::pointTFToMsg(position, interactiveCubeMarker.pose.position);
    interactiveCubeMarker.scale = 0.5;

    interactiveCubeMarker.name = "virDrone";
    interactiveCubeMarker.description = "virDrone";

    makeBoxControl(interactiveCubeMarker);

    InteractiveMarkerControl control;

    tf::Quaternion orien(0.0, 1.0, 0.0, 1.0);
    orien.normalize();
    tf::quaternionTFToMsg(orien, control.orientation);
    control.interaction_mode = InteractiveMarkerControl::MOVE_PLANE;
    interactiveCubeMarker.controls.push_back(control);
    control.interaction_mode = InteractiveMarkerControl::MOVE_AXIS;
    interactiveCubeMarker.controls.push_back(control);

    server->insert(interactiveCubeMarker);
    server->setCallback(interactiveCubeMarker.name, &processFeedback);
//    server->erase(int_marker.name);
}
// %EndTag(Quadrocopter)%

// %Tag(Moving)%
void makeMovingMarker( const tf::Vector3& position )
{
    InteractiveMarker int_marker;
    int_marker.header.frame_id = "moving_frame";
    tf::pointTFToMsg(position, int_marker.pose.position);
    int_marker.scale = 1;

    int_marker.name = "moving";
    int_marker.description = "Marker Attached to a\nMoving Frame";

    InteractiveMarkerControl control;

    tf::Quaternion orien(1.0, 0.0, 0.0, 1.0);
    orien.normalize();
    tf::quaternionTFToMsg(orien, control.orientation);
    control.interaction_mode = InteractiveMarkerControl::ROTATE_AXIS;
    int_marker.controls.push_back(control);

    control.interaction_mode = InteractiveMarkerControl::MOVE_PLANE;
    control.always_visible = true;
    control.markers.push_back( makeBox(int_marker) );
    int_marker.controls.push_back(control);

    server->insert(int_marker);
    server->setCallback(int_marker.name, &processFeedback);
}
// %EndTag(Moving)%



void odometry_callback(const nav_msgs::Odometry::ConstPtr &msg)
{
//    if(gui_state.data == "AIRBORNE_JOY"){
//        ROS_INFO("3333333");
        transit_x = msg->pose.pose.position.x;
        transit_y = msg->pose.pose.position.y;
        transit_z = msg->pose.pose.position.z;

}

void gui_state_cb(const std_msgs::String::ConstPtr& msg)
{
    gui_state = *msg;

}

void marker_pub_trigger_callback(const geometry_msgs::PoseStamped::ConstPtr& msg)
{
    geofence();
    drone_target_point.publish(drone_target_pose_msg);
}

bool airborne_cmd_callback(ground_station_msgs::Cmd::Request &req,ground_station_msgs::Cmd::Response &res)
{
    switch (req.cmd_code) {
        case MAV_CMD_INIT: {
            tf::Vector3 position;
            position = tf::Vector3(transit_x, transit_y, transit_z);
            if(mesh_marker_flag) {
                makeQuadrotorMarker(position);
//                make6DofMarker(false, visualization_msgs::InteractiveMarkerControl::MOVE_PLANE,position,false);
            }
            else
                makeCubeInteractMarker(position);
            marker_flag = true;
            res.success = true;
            res.message = "initiated";
            break;
        }
        case MAV_CMD_PAUSE: {
            InteractiveMarker tmp_marker;
            tmp_marker.name = "virDrone";
            server->erase(tmp_marker.name);

            marker_flag = false;
            res.success = true;
            res.message = "paused";


            break;
        }
        case MAV_CMD_FINISH: {

            InteractiveMarker tmp_marker;
            tmp_marker.name = "virDrone";
            server->erase(tmp_marker.name);

            transit_x = start_x;
            transit_y = start_y;
            transit_z = start_z;

            marker_flag = false;

/*
            tf::Vector3 position;
            position = tf::Vector3(transit_x, transit_y, transit_z);
            makeQuadrotorMarker(position);
            marker_flag = true;
            */

            res.success = true;
            res.message = "finished";

            break;
        }

//        case MAV_CMD_RESET:{
//            tf::Vector3 position;
//            position = tf::Vector3(start_x,start_y,start_z);
//            makeQuadrotorMarker(position);
//            marker_flag = true;
//            res.success = true;
//            res.message = "reset";
//            break;
//        }
    }
    server->applyChanges();
    return true;
}

void marker_handle_cb1(const visualization_msgs::InteractiveMarkerUpdate::ConstPtr &msg)
{
//    marker_type = msg->type;
    ros::Time t_now = ros::Time::now();
    if(marker_type ==1 && msg->type == 0 && t_now-t_marker >ros::Duration(0.05)){
        drone_target_point.publish(drone_target_pose_msg);
//        ROS_INFO("previous:%d,current:%d \n",marker_type,msg->type);
//        ROS_INFO("previous:%d,current:%d \n",seq,msg->seq_num);
    }

    t_marker = ros::Time::now();
    marker_type = msg->type;
    seq = msg->seq_num;

}


void marker_handle_cb2(const visualization_msgs::InteractiveMarkerInit::ConstPtr &msg)
{
    if(marker_flag){
//        ROS_INFO("mkcb111");
        drone_target_pose_msg.header.stamp = ros::Time::now();
        drone_target_pose_msg.header.frame_id = msg->markers[0].header.frame_id;
        drone_target_pose_msg.pose.position = msg->markers[0].pose.position;
        drone_target_pose_msg.pose.orientation = msg->markers[0].pose.orientation;

        t_marker = ros::Time::now();
        seq = msg->seq_num;
//        display_text();

//        ROS_INFO("mkcb222");

        ROS_INFO("x:%f, y:%f, z:%f",drone_target_pose_msg.pose.position.x,
                drone_target_pose_msg.pose.position.y,drone_target_pose_msg.pose.position.z);
//        geofence();

    }
}

// %Tag(main)%
int main(int argc, char** argv)
{
    ros::init(argc, argv, "airborne_control");
    ros::NodeHandle n;

    n.getParam("airborne_control/start_x",start_x);
    n.getParam("airborne_control/start_y",start_y);
    n.getParam("airborne_control/start_z",start_z);
    n.getParam("airborne_control/x_range_min",virfence_min(0));
    n.getParam("airborne_control/y_range_min",virfence_min(1));
    n.getParam("airborne_control/z_range_min",virfence_min(2));
    n.getParam("airborne_control/x_range_max",virfence_max(0));
    n.getParam("airborne_control/y_range_max",virfence_max(1));
    n.getParam("airborne_control/z_range_max",virfence_max(2));
    n.getParam("airborne_control/mesh_marker_flag",mesh_marker_flag);
    n.getParam("airborne_control/remap_flag",remap_flag);
    n.getParam("airborne_control/marker_trigger_init",marker_trigger_state);

    transit_x = start_x;
    transit_y = start_y;
    transit_z = start_z;
//    marker_holding_flag = false;

    n.param("mesh_resource", mesh_resource, std::string("package://odom_visualization/meshes/hummingbird.mesh"));
    ros::ServiceServer airborn_cmd_server = n.advertiseService("airborne_cmd",airborne_cmd_callback);
//    markerHandle1 = n.subscribe("/airborne_control/update",1,marker_handle_cb1);
//        gui_state_sub = n.subscribe("/gui_state", 1, gui_state_cb);



    if(remap_flag) {
        markerHandle2 = n.subscribe("/airborne_control/update_full", 1, marker_handle_cb2);
        odometry_sub = n.subscribe("/vins_estimator/odometry", 1, odometry_callback);
        drone_target_point = n.advertise<geometry_msgs::PoseStamped>("/target_pose", 1);
        markerPub = n.advertise<visualization_msgs::Marker>("TEXT_VIEW_FACING", 10);
    }
    // create a timer to update the published transforms
    marker_pub_timer = n.createTimer(ros::Duration(0.1),markerpubCallback);
    if (marker_trigger_state){
        marker_pub_timer.stop();
        marker_pub_trigger = n.subscribe("/goal",1,marker_pub_trigger_callback);
    }

    server.reset( new interactive_markers::InteractiveMarkerServer("airborne_control","",false) );
    marker_flag = false;

    ros::Duration(0.1).sleep();

    menu_handler.insert( "First Entry", &processFeedback );
    interactive_markers::MenuHandler::EntryHandle sub_menu_handle = menu_handler.insert( "Submenu" );
    menu_handler.insert( sub_menu_handle, "First Entry", &processFeedback );

    ros::spin();

    server.reset();
}
// %EndTag(main)%
