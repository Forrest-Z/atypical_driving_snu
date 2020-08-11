//
// Created by jbs on 20. 4. 6..
//

#ifndef ATYPICAL_DRIVING_WRAPPER_H
#define ATYPICAL_DRIVING_WRAPPER_H


#include <atypical_planner/LocalPlanner.h>
#include <atypical_planner/GlobalPlanner.h>

#include <optimization_module/parameter.hpp>

#include <ros/ros.h>
#include <nav_msgs/Path.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>
#include <visualization_msgs/MarkerArray.h>
#include <octomap_msgs/Octomap.h>
#include <octomap_msgs/conversions.h>
#include <std_msgs/Float64.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf/tf.h>
#include <tf/transform_broadcaster.h>
#include <tf/transform_listener.h>
#include <driving_msgs/DetectedObjectArray.h>
#include <geometry_msgs/PoseArray.h>
#include <functional>
#include <nav_msgs/Path.h>
#include <map_msgs/OccupancyGridUpdate.h>

namespace Planner{

    bool comparePredictorId(const Predictor::IndexedPredictor& p1,int id);
    /**
     * @brief ROS communicator such as topic advertising/subscription and ros-param parsing.
     */
    class RosWrapper{

    private:
        double t0; // update at the constructor
        /**
         * Shared resource with other thread
         */
        shared_ptr<PlannerBase> p_base; // planning data
        Param param;
        // node handle
        ros::NodeHandle nh;
        tf::TransformBroadcaster tf_br;
        tf::TransformListener tf_ls;

        /**
         * Operation mode
         */

        bool use_nominal_obstacle_radius = false;


        double sibalBeforeOccu  = 0;

        /**
         * Parameters
         */
        string SNUFrameId; // global frame id (JBS)
        string worldFrameId; //world frame id (global UTM)
        string octomapGenFrameId; // the referance frame of octomap
        string baseLinkId; // frame id of car frame
        string detectedObjectId; // frame id of detected objects

        string predictorLogFileDir; // directory of observations of predictor

        int max_marker_id; //count current published markers
        double speed; // current speed of car
        int count_corridors; //count current pulibhsed corridor markers;

        /**
         * Topic to be published
         */
        // topics to be published
        nav_msgs::Path planningPath; // can be obtained from mpcResultTraj
        nav_msgs::Path lanePathVis;

        visualization_msgs::MarkerArray corridorSeq;
        visualization_msgs::MarkerArray obstaclePrediction;
        visualization_msgs::MarkerArray obstacleVelocityText;
        nav_msgs::Path MPCTraj; // msg from mpcResultTraj

        /**
         *  Publisher
         */

        ros::Publisher pubPath; // publisher for result path
        ros::Publisher pubCorridorSeq; // publisher for current corridor sequence
        ros::Publisher pubObservationMarker; // publisher for observed position for obstacles
        ros::Publisher pubObservationPoseArray;
        ros::Publisher pubPredictionArray; // publisher for prediction of the target
        ros::Publisher pubCurCmd; // if MPC has been solved, it emits the command
        ros::Publisher pubMPCTraj; // if MPC has been solved, it pulish mpc traj for local planner horizon
        ros::Publisher pubMPCTrajMarker; // same with pubMPCTraj, but Marker Array Version
        ros::Publisher pubCurGoal; // Publish current goal point (global goal)
        ros::Publisher pubCurPose;
        ros::Publisher pubSlicedLane;
        ros::Publisher pubOrigLane;
        ros::Publisher pubSideLane;
        ros::Publisher pubSmoothLane;
        ros::Publisher pubTextSlider;
        ros::Publisher pubTextTrackingVelocity;
        ros::Publisher pubDetectedObjectsPoseArray;
        ros::Publisher pubCurCmdDabin;
        ros::Publisher pubOurOccu;


        /**
         * Subscriber
         */
        ros::Subscriber subCarPoseCov; /**< car state from KAIST */
        ros::Subscriber subCarSpeed; //
        ros::Subscriber subExampleObstaclePose; //
        ros::Subscriber subDetectedObjects;
        ros::Subscriber subOccuMap; // occupancy grid
        ros::Subscriber subOccuUpdate; // sibal

        /**
         * Callback functions
         */

        void cbCarPoseCov(geometry_msgs::PoseWithCovarianceConstPtr dataPtr);
        void cbCarSpeed(const std_msgs::Float64& speed);
        void cbOccuMap(const nav_msgs::OccupancyGrid& occuMap);
        void cbOccuUpdate(const map_msgs::OccupancyGridUpdateConstPtr& msg );

        void cbObstacles(const geometry_msgs::PoseStamped& obstPose);
        void cbDetectedObjects(const driving_msgs::DetectedObjectArray& objectsArray);

        /**
         * Core routines in while loop of ROS thread
         */
        void publish();
        void prepareROSmsgs();
        void predictionUpdate();
        void processTf();
    public:


        bool isLocalMapReceived = false;
        bool isCarPoseCovReceived = false;
        bool isCarSpeedReceived = false;
        bool isFrameRefReceived = false;
        bool isLaneReceived = false;
        bool isLaneRawReceived = false; // beform transform
        bool isLaneSliceLoaded = false;
        bool isGoalReceived = false;
        bool isOctomapFrameResolved = false; // octomap frame

        RosWrapper(shared_ptr<PlannerBase> p_base_);
        void updateParam(Param& param_);
        void updatePredictionModel();
        void runROS();
        bool isAllInputReceived();
        double curTime() {return (ros::Time::now().toSec()-t0);};
    };

    /**
     * @brief Wrapping module for RosWrapper and the two planners.
     */
    class Wrapper{

    private:
        shared_ptr<PlannerBase> p_base_shared;  /**< pointer for shared resource  */

        thread threadPlanner;
        thread threadRosWrapper;


        ros::NodeHandle nh; // due to map (JBS....)


        Param param; /**< global and local planner parameters  */

        LocalPlanner* lp_ptr; /**< local planner */
        GlobalPlanner* gp_ptr; /**< global planner */
        RosWrapper* ros_wrapper_ptr; /**< ros wrapper */


        // Do two-staged planning
        bool processLane(double tTrigger);
        bool planGlobal(double tTrigger);
        bool planLocal(double tTrigger);
        void updateCorrToBase(); // update the results of the planners to p_base
        void updateMPCToBase(); // update the results of the planners to p_base
        void updatePrediction();
        void runPlanning(); // planning thread.


    public:
        Wrapper();
        void run();
    };
}




#endif //ATYPICAL_DRIVING_WRAPPER_H
