#include "octree.h"
#include <algorithm>
#include <cmath>
#include <limits>

// OctreeNode Implementation
OctreeNode::OctreeNode(double minx, double maxx, double miny, double maxy, double minz, double maxz)
    : min_x(minx), max_x(maxx), min_y(miny), max_y(maxy), min_z(minz), max_z(maxz)
    , is_leaf(true)
{
    for (int i = 0; i < 8; i++) {
        children[i] = nullptr;
    }
}

OctreeNode::~OctreeNode()
{
    for (int i = 0; i < 8; i++) {
        if (children[i]) {
            delete children[i];
        }
    }
}

bool OctreeNode::contains(const Point3D& p) const
{
    return p.x >= min_x && p.x <= max_x &&
           p.y >= min_y && p.y <= max_y &&
           p.z >= min_z && p.z <= max_z;
}

double OctreeNode::minDistanceTo(const Point3D& p) const
{
    double dx = std::max(0.0, std::max(min_x - p.x, p.x - max_x));
    double dy = std::max(0.0, std::max(min_y - p.y, p.y - max_y));
    double dz = std::max(0.0, std::max(min_z - p.z, p.z - max_z));
    return std::sqrt(dx*dx + dy*dy + dz*dz);
}

// Octree Implementation
Octree::Octree(const std::vector<Point3D>& pts, int max_pts, int max_d)
    : points(&pts), max_points_per_node(max_pts), max_depth(max_d), root(nullptr)
{
    if (pts.empty()) return;
    
    // 计算边界
    double min_x = pts[0].x, max_x = pts[0].x;
    double min_y = pts[0].y, max_y = pts[0].y;
    double min_z = pts[0].z, max_z = pts[0].z;
    
    for (const auto& p : pts) {
        if (p.x < min_x) min_x = p.x;
        if (p.x > max_x) max_x = p.x;
        if (p.y < min_y) min_y = p.y;
        if (p.y > max_y) max_y = p.y;
        if (p.z < min_z) min_z = p.z;
        if (p.z > max_z) max_z = p.z;
    }
    
    // 稍微扩大边界
    double eps = 0.001;
    min_x -= eps; max_x += eps;
    min_y -= eps; max_y += eps;
    min_z -= eps; max_z += eps;
    
    // 创建根节点
    root = new OctreeNode(min_x, max_x, min_y, max_y, min_z, max_z);
    
    // 建立索引列表
    std::vector<int> all_indices(pts.size());
    for (size_t i = 0; i < pts.size(); i++) {
        all_indices[i] = static_cast<int>(i);
    }
    
    // 构建树
    buildTree(root, all_indices, 0);
}

Octree::~Octree()
{
    if (root) {
        delete root;
    }
}

void Octree::buildTree(OctreeNode* node, const std::vector<int>& indices, int depth)
{
    if (indices.size() <= static_cast<size_t>(max_points_per_node) || depth >= max_depth) {
        node->point_indices = indices;
        node->is_leaf = true;
        return;
    }
    
    node->is_leaf = false;
    
    // 计算中心点
    double mid_x = (node->min_x + node->max_x) / 2;
    double mid_y = (node->min_y + node->max_y) / 2;
    double mid_z = (node->min_z + node->max_z) / 2;
    
    // 为8个子节点分配点
    std::vector<std::vector<int>> child_indices(8);
    for (int idx : indices) {
        const Point3D& p = (*points)[idx];
        int octant = 0;
        if (p.x > mid_x) octant |= 1;
        if (p.y > mid_y) octant |= 2;
        if (p.z > mid_z) octant |= 4;
        child_indices[octant].push_back(idx);
    }
    
    // 递归创建子节点
    for (int i = 0; i < 8; i++) {
        if (!child_indices[i].empty()) {
            double minx = (i & 1) ? mid_x : node->min_x;
            double maxx = (i & 1) ? node->max_x : mid_x;
            double miny = (i & 2) ? mid_y : node->min_y;
            double maxy = (i & 2) ? node->max_y : mid_y;
            double minz = (i & 4) ? mid_z : node->min_z;
            double maxz = (i & 4) ? node->max_z : mid_z;
            
            node->children[i] = new OctreeNode(minx, maxx, miny, maxy, minz, maxz);
            buildTree(node->children[i], child_indices[i], depth + 1);
        }
    }
}

void Octree::searchNearest(OctreeNode* node, const Point3D& query, 
                          int& best_idx, double& best_dist_sq) const
{
    if (!node) return;
    
    // 如果节点的最小距离大于当前最佳距离，剪枝
    double min_dist = node->minDistanceTo(query);
    if (min_dist * min_dist >= best_dist_sq) return;
    
    if (node->is_leaf) {
        // 叶节点：检查所有点
        for (int idx : node->point_indices) {
            const Point3D& p = (*points)[idx];
            double dx = p.x - query.x;
            double dy = p.y - query.y;
            double dz = p.z - query.z;
            double dist_sq = dx*dx + dy*dy + dz*dz;
            
            if (dist_sq < best_dist_sq) {
                best_dist_sq = dist_sq;
                best_idx = idx;
            }
        }
    } else {
        // 内部节点：按距离排序子节点，优先搜索近的
        struct ChildDist {
            int index;
            double dist;
        };
        std::vector<ChildDist> child_dists;
        
        for (int i = 0; i < 8; i++) {
            if (node->children[i]) {
                double dist = node->children[i]->minDistanceTo(query);
                child_dists.push_back({i, dist});
            }
        }
        
        std::sort(child_dists.begin(), child_dists.end(), 
                 [](const ChildDist& a, const ChildDist& b) { return a.dist < b.dist; });
        
        for (const auto& cd : child_dists) {
            searchNearest(node->children[cd.index], query, best_idx, best_dist_sq);
        }
    }
}

int Octree::findNearest(const Point3D& query) const
{
    if (!root || points->empty()) return 0;
    
    int best_idx = 0;
    double best_dist_sq = std::numeric_limits<double>::max();
    
    searchNearest(root, query, best_idx, best_dist_sq);
    return best_idx;
}
