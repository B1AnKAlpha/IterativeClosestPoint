#ifndef OCTREE_H
#define OCTREE_H

#include "pointcloud.h"
#include <vector>

/**
 * @brief 八叉树节点
 */
class OctreeNode {
public:
    double min_x, max_x, min_y, max_y, min_z, max_z;
    std::vector<int> point_indices;
    OctreeNode* children[8];
    bool is_leaf;
    
    OctreeNode(double minx, double maxx, double miny, double maxy, double minz, double maxz);
    ~OctreeNode();
    
    bool contains(const Point3D& p) const;
    double minDistanceTo(const Point3D& p) const;
};

/**
 * @brief 八叉树类 - 用于加速最近邻搜索
 */
class Octree {
public:
    Octree(const std::vector<Point3D>& pts, int max_pts = 10, int max_d = 20);
    ~Octree();
    
    int findNearest(const Point3D& query) const;
    
private:
    OctreeNode* root;
    const std::vector<Point3D>* points;
    int max_points_per_node;
    int max_depth;
    
    void buildTree(OctreeNode* node, const std::vector<int>& indices, int depth);
    void searchNearest(OctreeNode* node, const Point3D& query, 
                      int& best_idx, double& best_dist_sq) const;
};

#endif // OCTREE_H
