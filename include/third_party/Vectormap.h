//
// Created by jbs on 20. 5. 8..
//

#ifndef ATYPICAL_DRIVING_VECTORMAP_H
#define ATYPICAL_DRIVING_VECTORMAP_H

#include <vector>
#include <geometry_msgs/Point.h>
#include <Eigen/Core>
#include <nav_msgs/Path.h>

using namespace std;
using namespace geometry_msgs;
using namespace Eigen;

namespace Planner{
    struct LaneNode{
        vector<Point> laneCenters;
        double width;
    };

    struct LanePath{
        vector<LaneNode> lanes;
        void applyTransform(const Matrix4d& Tab){
            for (auto & lane :lanes){
                for(auto it = lane.laneCenters.begin() ; it!= lane.laneCenters.end(); it++){
                       Vector4d xb(it->x,it->y,0,1);
                       Vector4d xa = Tab*xb;
                       it->x = xa(0);
                       it->y = xa(1);
                }
            }
        }

        nav_msgs::Path getPath(string frame_id){
            nav_msgs::Path nodeNavPath;
            nodeNavPath.header.frame_id = frame_id.c_str();
            for (auto & lane :lanes){
                for(auto it = lane.laneCenters.begin() ; it!= lane.laneCenters.end(); it++){
                    geometry_msgs::PoseStamped aPoint;
                    aPoint.pose.position.x = it->x;
                    aPoint.pose.position.y = it->y;
                    nodeNavPath.poses.push_back(aPoint);
                }
            }
            return nodeNavPath;
        }


    };
}
#endif //ATYPICAL_DRIVING_VECTORMAP_H
