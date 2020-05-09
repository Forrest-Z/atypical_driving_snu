//
// Created by jbs on 20. 4. 6..
//

#ifndef ATYPICAL_DRIVING_PLANNERBASE_H
#define ATYPICAL_DRIVING_PLANNERBASE_H

#include <octomap/OcTree.h>
#include <vector>
#include <memory>
#include <geometry_msgs/Twist.h>
#include <mutex>
#include <thread>
#include <string>
#include <Eigen/Core>
#include <third_party/Prediction/target_manager.hpp>


using namespace std;
using namespace Eigen;


namespace Planner {
    /**
     * @brief
     */
    struct Corridor {
        double xl;
        double yl;
        double xu;
        double yu;
        double t_start;
        double t_end;
    };

    /**
     * @brief Parameters for local planner
     */
    struct ParamLocal {
        double horizon;
        double tStep;
        double obstRadiusNominal;
        double goalReachingThres;
    };

    /**
     * @brief Parameters for global planner
     */
    struct ParamGlobal {
        double horizon;
        double car_width;
        double car_z_min;
        double car_z_max;
        double car_speed;
        double road_width;
        double world_x_min;
        double world_y_min;
        double world_x_max;
        double world_y_max;
        double grid_resolution;
        double box_resolution;
        double box_max_size;
    };

    struct ParamPredictor{
        int queueSize;
        float  zHeight;
        int polyOrder;

    };

    /**
     * @brief Parameters for both planner.
     * @details This is retrieved from launch-param
     */
    struct Param {
        ParamGlobal g_param;
        ParamLocal l_param;
        ParamPredictor p_param;
    };
    /**
     * @brief State feed for MPC
     * @todo Conversion btw ROS messages / covariance
     */
    struct CarState{
        double x;
        double y;
        double v; // sign?
        double theta;
    };

    struct ObstacleEllipse{
        Vector2d q;
        Matrix2d Q;
    };

    struct ObstaclePath{
        int id;
        vector<ObstacleEllipse> obstPath;
    };

    struct ObstaclePathArray{
        vector<ObstaclePath> obstPathArray;
    };

    /**
     * @brief Input term of MPC
     */
    struct CarInput{
        double v; // linear velocity
        double delta; // steering angle
    };

    /**
      * @brief Final output of MPC : trajectory of inputs.
      * @details We interpolate (ts,us,xs) to calculate current state or input
    */
    struct MPCResultTraj{
        vector<double> ts;
        vector<CarInput> us;
        vector<CarState> xs;
    };

    /**
     * @brief The planning results and ingredients
     */
    class PlannerBase{

    private:
        // to be updated from callback
        shared_ptr<octomap::OcTree> octo_global_ptr;
        shared_ptr<octomap::OcTree> octo_local_ptr;
        CarState cur_state;
        CarState desired_state; //jungwon

        // to be updated by planners
        vector<CarState> navigation_path; //jungwon navigation planning output from Dabin Kim
        vector<pair<double, double>> skeleton_path; //jungwon: debug purpose TODO: delete this after debugging
        vector<Corridor> corridor_seq;
        Corridor search_range;
        MPCResultTraj mpc_result;

        ObstaclePathArray obstaclePathArray;

    public:

        // prediction module
        vector<Predictor::TargetManager> predictorSet;
        // Get
        CarState getCarState() {return cur_state;};
        CarState getDesiredState() {return desired_state;}; //jungwon
        CarInput getCurInput() { return CarInput(); }; // do some interpolation

        vector<pair<double, double>> getSkeletonPath() {return skeleton_path;}; //TODO: delete this after debugging
        vector<Corridor> getCorridorSeq() {return corridor_seq;};
        Corridor getSearchRange() {return search_range;};
        octomap::OcTree* getGlobalOctoPtr() {return octo_global_ptr.get();}
        octomap::OcTree* getLocalOctoPtr() {return octo_local_ptr.get();}
        MPCResultTraj getMPCResultTraj() {return mpc_result;}

        // Set from subscriber
        void setCarState(const CarState& carState_) { cur_state = carState_;};
        void setDesiredState(const CarState& desiredState_) {desired_state = desiredState_;};
        void setGlobalMap(octomap::OcTree* octoGlobalPtr_) {octo_global_ptr.reset(octoGlobalPtr_);};
        void setLocalMap(octomap::OcTree* octoLocalPtr_) {octo_local_ptr.reset(octoLocalPtr_);};

        // Set from planner
        void setSkeletonPath(const vector<pair<double, double>>& skeleton_in_) {skeleton_path = skeleton_in_;}//TODO: delete this after debugging
        void setCorridorSeq(const vector<Corridor>& corridor_in_) {corridor_seq = corridor_in_;}
        void setSearchRange(const Corridor& search_range_in_) {search_range = search_range_in_;}
        void setMPCResultTraj(const MPCResultTraj& mpc_result_in_) {mpc_result = mpc_result_in_;}

        // Set from predictor
        // update obstaclePathArray so that it is prediction from [tPredictionStart,tPredictionStart+ horizon[
        // prepare prediction sequence for MPC
        void updatePrediction(VectorXd tSeq_,double rNominal = 0) {
            VectorXf tSeq = tSeq_.cast<float>();
            // TODO predictor should be multiple
            obstaclePathArray.obstPathArray.clear();

            for(auto predictor : predictorSet){
                predictor.update_predict(); // this will do nothing if observation is not enough
                if (predictor.is_prediction_available()) {
                    vector<geometry_msgs::Pose> obstFuturePose = predictor.eval_pose_seq(tSeq);
                    ObstaclePath obstPath;
                    for (auto obstPose : obstFuturePose) {
                        // construct one obstaclePath
                        ObstacleEllipse obstE;
                        obstE.q = Vector2d(obstPose.position.x, obstPose.position.y);

                        // TODO shape should be consider later
                        obstE.Q.setZero();
                        obstE.Q(0, 0) = 1 / pow(rNominal, 2);
                        obstE.Q(1, 1) = 1 / pow(rNominal, 2);

                        obstPath.obstPath.push_back(obstE);
                    }
                    obstaclePathArray.obstPathArray.push_back(obstPath);
                }
            }


        }
    };
    /**
     * @brief Abstract class. The shared attributes to be inherited to the derived classes
     */
    class AbstractPlanner{

    protected:
        // Shared resource
        shared_ptr<PlannerBase> p_base; /**< This is the shared resource. Multiple thread will use this data. */

    public:
        AbstractPlanner(shared_ptr<PlannerBase> p_base_):p_base(p_base_) {};
        virtual bool plan() = 0;
    };

}


#endif //ATYPICAL_DRIVING_PLANNERBASE_H
