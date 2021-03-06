/**
 * This script simulates Lane re-generation pipeline in Airsim
 * Inputs: Current state of the car, occupancy grid
 */

#include <atypical_planner/Wrapper.h>
#include <third_party/Vectormap.h>
#include "nav_msgs/Odometry.h"
#include "nav_msgs/OccupancyGrid.h"
#include <ros/ros.h>
#include <occupancy_grid_utils/coordinate_conversions.h>

#include "Eigen/Core"


using namespace Planner;
using namespace Eigen;

nav_msgs::OccupancyGrid curOccupancyGrid;
CarState curCarState;

bool isMapReceived = false;
/**
 * @brief callback for occupancy grid. the gridmap is rolling window manner.
 * @param msg
 */
void cbOccupancy(const nav_msgs::OccupancyGridPtr msg){
    curOccupancyGrid = *msg;
    isMapReceived = true;
}
/**
 * @brief receiving the odometry informtion. The info is ned
 * @param pose_ned
 */
void cbCarPose(const nav_msgs::Odometry& pose_ned){

    // Convert ned to enu pose

    Eigen::Quaternionf quat;
    Eigen::Vector3f transl;

    transl(0) = pose_ned.pose.pose.position.x;
    transl(1) = -pose_ned.pose.pose.position.y;
    transl(2) = pose_ned.pose.pose.position.z;
    quat.x() =  pose_ned.pose.pose.orientation.x;
    quat.y() =  pose_ned.pose.pose.orientation.y;
    quat.z() =  pose_ned.pose.pose.orientation.z;
    quat.w() =  pose_ned.pose.pose.orientation.w;

    Eigen::Affine3f transform;

    quat.normalize();
    transform.setIdentity();
    transform.translate(transl);
    transform.rotate(quat);

    // Putting msg into CurState
    Vector3f e1 = transform.rotation().matrix().col(0); // w.r.t NED

    Matrix3f Ren;
    Ren << 0,1,0,
            1,0,0,
            0,0,1;
    Vector3f e1e = Ren*e1;
    double theta = atan2(e1e(2), e1e(1));
    curCarState.theta = theta;
    curCarState.x =transform.translation()(0);
    curCarState.y = transform.translation()(1);
    curCarState.v = -1; // velocity is not important here (TODO)
}

int main(int argc, char ** argv){

    /**
     * Define lane
     */

    Lane laneOrig;
    // This code mock the vector map cases

    int N1 = 20, N2 = 10;

    VectorXf l1X(N1); l1X.setZero();
    VectorXf l1Y(N1); l1Y.setLinSpaced(N1,0,73);
    VectorXf l2X(N2); l2X.setLinSpaced(N2,0,49);
    VectorXf l2Y(N2); l2Y.setConstant(73);

    for (int n = 0 ; n < N1 ; n++){
        laneOrig.points.emplace_back(l1X(n),l1Y(n));
    }

    for (int n = 0 ; n < N2 ; n++){
        laneOrig.points.emplace_back(l2X(n),l2Y(n));
    }

    /**
     * ROS
     */

    ros::init(argc,argv,"lane_tester");
    ros::NodeHandle nh("~");
    ros::Subscriber subOccupancyGrid = nh.subscribe("occupancy_grid",2,cbOccupancy);
    ros::Subscriber subCarPose = nh.subscribe("car_odom",1,cbCarPose);
    ros::Publisher pubOrigLane = nh.advertise<nav_msgs::Path>("orignal_lane",1);
    ros::Publisher pubSlicedLane = nh.advertise<nav_msgs::Path>("sliced_lane",1);
    ros::Publisher pubFreeCells = nh.advertise<visualization_msgs::Marker>("free_cells",1);

    string map_frame = "map";

    visualization_msgs::Marker freeCells;
    freeCells.type = visualization_msgs::Marker::CUBE_LIST;
    freeCells.pose.orientation.w = 1.0;
    freeCells.header.frame_id = map_frame;
    freeCells.color.a = 1.0;
    freeCells.color.r = 1.0;
    freeCells.color.g = 1.0;
    freeCells.color.b = 1.0;
    freeCells.scale.x = 0.5;
    freeCells.scale.y = 0.5;
    freeCells.scale.z = 0.5;


    nav_msgs::Path origLane = laneOrig.getPath(map_frame);

    ros::Rate rl(20);
    int Nx = 20;
    int Ny = 20;
    while (ros::ok()){

        if (isMapReceived){
            Vector2d windowOrig(curOccupancyGrid.info.origin.position.x,curOccupancyGrid.info.origin.position.y);
            double windowWidth = curOccupancyGrid.info.width*curOccupancyGrid.info.resolution;
            double windowHeight = curOccupancyGrid.info.height*curOccupancyGrid.info.resolution ;

            printf("Current window: origin = [%f,%f] / dimension = [%f,%f]\n",windowOrig(0),windowOrig(1),windowWidth,windowHeight);
            curCarState.print();


            // Test occupancy
            VectorXd xs = VectorXd::LinSpaced(Nx,windowOrig(0)+0.4,windowOrig(0) + windowWidth-0.4);
            VectorXd ys = VectorXd::LinSpaced(Ny,windowOrig(1)+0.4,windowOrig(1) + windowHeight-0.4);
            freeCells.points.clear();

            for (int i = 0 ; i < Nx ; i++)
                for (int j = 0; j < Ny ; j++){
                    Vector2d point(xs(i),ys(j));

                    geometry_msgs::Point queryPoint3;
                    queryPoint3.x = point(0);
                    queryPoint3.y = point(1);
                    queryPoint3.z = 0;

                    occupancy_grid_utils::index_t idx = occupancy_grid_utils::pointIndex(curOccupancyGrid.info,queryPoint3);
                    if (curOccupancyGrid.data[idx] <50 ){
                       freeCells.points.push_back(queryPoint3);
                    }
                }



            pubFreeCells.publish(freeCells);
            vector<Vector2d> pathSliced = laneOrig.slicing(curCarState,windowOrig,windowWidth,windowHeight);
            pubSlicedLane.publish(getPath(pathSliced,map_frame));

        }
        pubOrigLane.publish(origLane);

        ros::spinOnce();
        rl.sleep();
    }


    return 0;

}


