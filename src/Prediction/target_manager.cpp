#include "third_party/Prediction/target_manager.hpp"
using namespace DAP;
using namespace Predictor;



TargetManager::TargetManager(bool useKetiVel,int queue_size,float z_value,int poly_order,int index):
queue_size(queue_size),z_value(z_value),poly_order(poly_order),managerIdx(index),predictWithKetiVel(useKetiVel) {

    assert(poly_order > 0 or queue_size >0 && "Invalid initialization of target manager" );
    //obsrv_traj_for_predict_total = TXYZTraj(4,100000);

};

/**
 * @brief renew the polynomial models
 */
void TargetManager::update_predict(){
//    if (observations.size() > queue_size-1){
////        ROS_INFO("step0");
//        VectorXf t_vals(observations.size());
//        VectorXf x_vals(observations.size());
//        VectorXf y_vals(observations.size());
//
//        VectorXf qx_vals(observations.size());
//        VectorXf qy_vals(observations.size());
//        VectorXf qz_vals(observations.size());
//        VectorXf qw_vals(observations.size());
//
//        int i =0 ;
//        for (list<tuple<float,Vector6f>>::iterator it = observations.begin() ; it != observations.end() ; it++, i++){
//            t_vals(i) = std::get<0>(*it);
//            x_vals(i) = std::get<1>(*it)(0);
//            y_vals(i) = std::get<1>(*it)(1);
//
//            qx_vals(i) = std::get<1>(*it)(2);
//            qy_vals(i) = std::get<1>(*it)(3);
//            qz_vals(i) = std::get<1>(*it)(4);
//            qw_vals(i) = std::get<1>(*it)(5);
//        }
//
//        ROS_DEBUG_STREAM("Observation queue for prediction: ");
//        ROS_DEBUG_STREAM("t : ");
//        ROS_DEBUG_STREAM(t_vals.transpose());
//        ROS_DEBUG_STREAM("x : ");
//        ROS_DEBUG_STREAM(x_vals.transpose()) ;
//        ROS_DEBUG_STREAM("y : ") ;
//        ROS_DEBUG_STREAM(y_vals.transpose());
////        ROS_INFO("step1");
//        fit_coeff_x = polyfit(t_vals,x_vals,poly_order);
//        fit_coeff_y = polyfit(t_vals,y_vals,poly_order);
//
//        fit_coeff_qx  = polyfit(t_vals,qx_vals,poly_order);
//        fit_coeff_qy  = polyfit(t_vals,qy_vals,poly_order);
//        fit_coeff_qz  = polyfit(t_vals,qz_vals,poly_order);
//        fit_coeff_qw  = polyfit(t_vals,qw_vals,poly_order);
//
//        int N_pnt = observations.size();
//        obsrv_traj_for_predict = DAP::TXYZQuatTraj(8,N_pnt);
//        obsrv_traj_for_predict.block(0,0,1,N_pnt) = t_vals.transpose();
//        obsrv_traj_for_predict.block(1,0,1,N_pnt) = x_vals.transpose();
//        obsrv_traj_for_predict.block(2,0,1,N_pnt) = y_vals.transpose();
//        obsrv_traj_for_predict.block(3,0,1,N_pnt).array() = z_value; // use ref height as z value
//
//        obsrv_traj_for_predict.block(5,0,1,N_pnt) = qx_vals.transpose();
//        obsrv_traj_for_predict.block(6,0,1,N_pnt) = qy_vals.transpose();
//        obsrv_traj_for_predict.block(7,0,1,N_pnt) = qz_vals.transpose();
//        obsrv_traj_for_predict.block(4,0,1,N_pnt) = qw_vals.transpose();
//
//        is_predicted = true;

//        ROS_INFO("step2");
//
    list<tuple<float,Vector8f>> observationsBuffer = observations;
    if (observationsBuffer.size() > queue_size-1){
//        ROS_INFO("step0");
            VectorXf t_vals(observationsBuffer.size());
            VectorXf x_vals(observationsBuffer.size());
            VectorXf y_vals(observationsBuffer.size());

            VectorXf qx_vals(observationsBuffer.size());
            VectorXf qy_vals(observationsBuffer.size());
            VectorXf qz_vals(observationsBuffer.size());
            VectorXf qw_vals(observationsBuffer.size());

            VectorXf vx_vals(observationsBuffer.size());
            VectorXf vy_vals(observationsBuffer.size());



        int i =0 ;
            for (list<tuple<float,Vector8f>>::iterator it = observationsBuffer.begin() ; it != observationsBuffer.end() ; it++, i++){
                t_vals(i) = std::get<0>(*it);
                x_vals(i) = std::get<1>(*it)(0);
                y_vals(i) = std::get<1>(*it)(1);

                qx_vals(i) = std::get<1>(*it)(2);
                qy_vals(i) = std::get<1>(*it)(3);
                qz_vals(i) = std::get<1>(*it)(4);
                qw_vals(i) = std::get<1>(*it)(5);

                vx_vals(i) = std::get<1>(*it)(6);
                vy_vals(i) = std::get<1>(*it)(7);

            }

            ROS_DEBUG_STREAM("Observation queue for prediction: ");
            ROS_DEBUG_STREAM("t : ");
            ROS_DEBUG_STREAM(t_vals.transpose());
            ROS_DEBUG_STREAM("x : ");
            ROS_DEBUG_STREAM(x_vals.transpose()) ;
            ROS_DEBUG_STREAM("y : ") ;
            ROS_DEBUG_STREAM(y_vals.transpose());
//        ROS_INFO("step1");
            fit_coeff_x = polyfit(t_vals,x_vals,poly_order);
            fit_coeff_y = polyfit(t_vals,y_vals,poly_order);

            fit_coeff_qx  = polyfit(t_vals,qx_vals,poly_order);
            fit_coeff_qy  = polyfit(t_vals,qy_vals,poly_order);
            fit_coeff_qz  = polyfit(t_vals,qz_vals,poly_order);
            fit_coeff_qw  = polyfit(t_vals,qw_vals,poly_order);

            int N_pnt = observationsBuffer.size();
            obsrv_traj_for_predict = DAP::TXYZQuatTraj(8,N_pnt);
            obst_traj_velocity_keti = DAP::TXYZTraj(4,N_pnt);

            obsrv_traj_for_predict.block(0,0,1,N_pnt) = t_vals.transpose();
            obsrv_traj_for_predict.block(1,0,1,N_pnt) = x_vals.transpose();
            obsrv_traj_for_predict.block(2,0,1,N_pnt) = y_vals.transpose();
            obsrv_traj_for_predict.block(3,0,1,N_pnt).array() = z_value; // use ref height as z value

            obsrv_traj_for_predict.block(5,0,1,N_pnt) = qx_vals.transpose();
            obsrv_traj_for_predict.block(6,0,1,N_pnt) = qy_vals.transpose();
            obsrv_traj_for_predict.block(7,0,1,N_pnt) = qz_vals.transpose();
            obsrv_traj_for_predict.block(4,0,1,N_pnt) = qw_vals.transpose();

            obst_traj_velocity_keti.block(0,0,1,N_pnt)  = t_vals.transpose();
            obst_traj_velocity_keti.block(1,0,1,N_pnt)  = vx_vals.transpose();
            obst_traj_velocity_keti.block(2,0,1,N_pnt)  = vy_vals.transpose();




        is_predicted = true;

//        ROS_INFO("step2");
    }
}

void TargetManager::update_observation(float t, geometry_msgs::Pose target_pose, Vector3f dimensions_,float vxKeti  , float vyKeti ) {

    dimensions = dimensions_;
    geometry_msgs::Point position = target_pose.position;
    geometry_msgs::Quaternion quat = target_pose.orientation;

    Vector6f state; state << position.x,position.y,quat.x,quat.y,quat.z,quat.w;
    Vector8f stateWithDim; stateWithDim << state , dimensions_(0),dimensions_(1) ;
    Vector8f stateWithVel; stateWithVel << state, vxKeti, vyKeti ;
    observations.push_back(std::make_tuple(t,stateWithVel));
    // total history for logging
    observationHistory.push_back(stateWithDim);

    lastObservationTime = t;

    if (observations.size() > queue_size)
        observations.pop_front();
    if (observationHistory.size() > size_history)
        observationHistory.pop_front();



}
// x dir = velocity
geometry_msgs::Pose TargetManager::eval_pose(float t){
    geometry_msgs::Pose pose;


    if (predictWithKetiVel){

        double tLastObsv = obsrv_traj_for_predict.coeff(0,observations.size()-1); // time stamp of the last observation
        double xLastObsv = obsrv_traj_for_predict.coeff(1,observations.size()-1);
        double yLastObsv = obsrv_traj_for_predict.coeff(2,observations.size()-1);



        Vector2f meanKetiVel = getAvgKetiVelocity();

//        cout << "mean KETI velocity " << meanKetiVel.transpose() << endl;

        pose.position.x = xLastObsv + (t-tLastObsv)*meanKetiVel(0);
        pose.position.y = yLastObsv + (t-tLastObsv)*meanKetiVel(1);

    }else{
        // translation
        pose.position.x = polyeval(fit_coeff_x,t);
        pose.position.y = polyeval(fit_coeff_y,t);
    }

    pose.position.z = z_value;

    // In case of orientation, we just use first order regression
    Eigen:Quaternionf q;
    q.x() = polyeval(fit_coeff_qx,t);
    q.y() = polyeval(fit_coeff_qy,t);
    q.z() = polyeval(fit_coeff_qz,t);
    q.w() = polyeval(fit_coeff_qw,t);
    q.normalize();
    pose.orientation.x = q.x();
    pose.orientation.y = q.y();
    pose.orientation.z = q.z();
    pose.orientation.w = q.w();

    // Rotation based on the velocity
    /**
    float vel_x = polyeval_derivative(fit_coeff_x,t);
    float vel_y = polyeval_derivative(fit_coeff_y,t);

    float theta = atan2(vel_y, vel_x);
    DAP::TransformMatrix mat = DAP::TransformMatrix::Identity(); 
    mat.rotate(Eigen::AngleAxisf(theta,Eigen::Vector3f::UnitZ()));

    Eigen::Quaternionf quat(mat.rotation());
    pose.orientation.x = quat.x();
    pose.orientation.y = quat.y();
    pose.orientation.z = quat.z();
    pose.orientation.w = quat.w();
     **/

    // Rotation based on interpolation

    return pose;
}

vector<geometry_msgs::Pose> TargetManager::eval_pose_seq(vector<float> ts){
    vector<geometry_msgs::Pose> pose_seq(ts.size());
    int ind = 0;
    for (auto it = ts.begin();it != ts.end(); it++,ind++)
        pose_seq[ind] = eval_pose(*it);
    return pose_seq;
}
vector<geometry_msgs::Pose> TargetManager::eval_pose_seq(Eigen::VectorXf ts){
    vector<float> ts_vec(ts.data(),ts.data() + ts.size());
    return eval_pose_seq(ts_vec);
}

visualization_msgs::Marker TargetManager::get_obsrv_marker(string world_frame_id,int ns){
    visualization_msgs::Marker marker;
    marker.header.frame_id = world_frame_id;
    //    marker = TXYZ_traj_to_pnt_marker(obsrv_traj_for_predict,world_frame_id,1.0);
    if(is_predicted){
        marker = TXYZ_traj_to_pnt_marker(TXYZQuat_traj_to_TXYZ_traj(obsrv_traj_for_predict),world_frame_id,1.0);
    }else{
        if (not isNoPredictionWarnEmitted) {
            // cout << "[TargetManager] No prediction performed. empty marker will be returned" << endl;
            isNoPredictionWarnEmitted = true;
        }
    }
    marker.ns = to_string(ns);
    return marker;
}; // observation used for the latest prediction update

geometry_msgs::PoseArray TargetManager::get_obsrv_pose(string world_frame_id) {
    geometry_msgs::PoseArray poseArray;
    poseArray.header.frame_id = world_frame_id;
    for (auto pose : TXYZQuat_to_pose_vector(obsrv_traj_for_predict)){
        poseArray.poses.push_back(pose);
    }
    return poseArray;
}


TargetManager::~TargetManager(){
     //cout << "[TargetManager] Destroyed with log file" << endl;
//     string fileName = logFileDir+"/observation_"+ to_string(managerIdx) +  ".txt";
//    std::ofstream file(fileName);
//    // vec8 = [x,y,qx,qy,qz,qw,dimx,dimy]
//    if(file.is_open()){
//        for (auto vec8 : observationHistory){
//            file << vec8.transpose() << endl;
//        }
//    }
//    else{
//        cerr<< fileName << " was not opened" << endl;
//    }
//    file.close();
}