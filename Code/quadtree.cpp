#include "quadtree.h"
#include <float.h>
#include <cmath>
#include <iomanip>
#include <iostream>
#include "common.h"
#include <fstream>
GPSdata::GPSdata() {}

GPSdata::GPSdata(int _id, string _time, double _lon, double _lat)
    : id(_id), time(_time), longitude(_lon), latitude(_lat) {}

void GPSdata::print() {
    std::ofstream outFile("output.txt", std::ios::app); // 追加写入

    if (!outFile.is_open()) {
        std::cerr << "无法打开输出文件\n";
        return;
    }

    outFile << "id:" << this->id <<std::setw(20)<< "时间:" << this->time<<std::setw(20)<<"坐标:" << std::fixed << std::setprecision(5)
            << this->longitude << ',' << std::fixed << std::setprecision(5)
            << this->latitude << '\n';
    //outFile << "--------------------------\n";

    outFile.close();
}

Rectangle::Rectangle() {}

Rectangle::Rectangle(point2d _top_right, point2d _bottom_left)
    : top_right(_top_right), bottom_left(_bottom_left) {}

Rectangle::Rectangle(double top_right_x,
                     double top_right_y,
                     double bottom_left_x,
                     double bottom_left_y) {
    top_right = std::make_pair(top_right_x, top_right_y);
    bottom_left = std::make_pair(bottom_left_x, bottom_left_y);
}

Rectangle::Rectangle(point2d _top_right,
                     double bottom_left_x,
                     double bottom_left_y)
    : top_right(_top_right) {
    bottom_left = std::make_pair(bottom_left_x, bottom_left_y);
}
Rectangle::Rectangle(double top_right_x,
                     double top_right_y,
                     point2d _bottom_left)
    : bottom_left(_bottom_left) {
    top_right = std::make_pair(top_right_x, top_right_y);
}

bool Rectangle::Inside(point2d p) const {
    auto x = p.first;
    auto y = p.second;
    return x >= bottom_left.first && x <= top_right.first &&
           y <= top_right.second && y >= bottom_left.second;
}

bool Rectangle::Inside(double x, double y) const {
    return (x >= bottom_left.first || equal(x, bottom_left.first)) &&
           (x <= top_right.first || equal(x, top_right.first)) &&
           (y <= top_right.second || equal(y, top_right.second)) &&
           (y >= bottom_left.second || equal(y, bottom_left.second));
}

bool Rectangle::Intersects(const Rectangle& other) const {
    const bool separated = other.top_right.first < bottom_left.first ||
                           other.bottom_left.first > top_right.first ||
                           other.top_right.second < bottom_left.second ||
                           other.bottom_left.second > top_right.second;
    return !separated;
}

QuadNode::QuadNode() {}

QuadNode::QuadNode(Rectangle* bounding) : range(bounding) {}

QuadNode::~QuadNode() {
    for (auto childNode : child) {
        delete childNode;
    }
    delete range;
}

void QuadNode::InsertNode(GPSdata* gpsdata, int depth) {
    // 空节点不用插入
    if (gpsdata == nullptr) {
        return;
    }

    double node_x = gpsdata->longitude;
    double node_y = gpsdata->latitude;

    // 要插入的节点不在范围内
    if (!this->range->Inside(node_x, node_y)) {
        return;
    }

    double root_x1 = this->range->top_right.first;
    double root_y1 = this->range->top_right.second;
    double root_x2 = this->range->bottom_left.first;
    double root_y2 = this->range->bottom_left.second;
    double x_center = (root_x1 + root_x2) / 2;
    double y_center = (root_y1 + root_y2) / 2;

    if (depth == 0) {
        this->data.push_back(gpsdata);
        return;
    }

    // 在根节点范围的左半部分
    if (node_x <= x_center) {
        // 在左上角
        if (node_y >= y_center) {
            if (this->child[LEFTTOP] == nullptr) {
                Rectangle* rect =
                    new Rectangle(x_center, root_y1, root_x2, y_center);
                this->child[LEFTTOP] = new QuadNode(rect);
                this->child[LEFTTOP]->father = this;
            }
            this->child[LEFTTOP]->InsertNode(gpsdata, depth - 1);
        }
        // 在左下角
        else {
            if (this->child[LEFTBOT] == nullptr) {
                Rectangle* rect =
                    new Rectangle(x_center, y_center, root_x2, root_y2);
                this->child[LEFTBOT] = new QuadNode(rect);
                this->child[LEFTBOT]->father = this;
            }
            this->child[LEFTBOT]->InsertNode(gpsdata, depth - 1);
        }
    }
    // 在根节点范围的右半部分
    else {
        // 在右上角
        if (node_y >= y_center) {
            if (this->child[RIGHTTOP] == nullptr) {
                Rectangle* rect =
                    new Rectangle(root_x1, root_y1, x_center, y_center);
                this->child[RIGHTTOP] = new QuadNode(rect);
                this->child[RIGHTTOP]->father = this;
            }
            this->child[RIGHTTOP]->InsertNode(gpsdata, depth - 1);
        }
        // 在右下角
        else {
            if (this->child[RIGHTBOT] == nullptr) {
                Rectangle* rect =
                    new Rectangle(root_x1, y_center, x_center, root_y2);
                this->child[RIGHTBOT] = new QuadNode(rect);
                this->child[RIGHTBOT]->father = this;
            }
            this->child[RIGHTBOT]->InsertNode(gpsdata, depth - 1);
        }
    }
}

QuadNode* QuadNode::Search(point2d p) const {
    if (!range || !range->Inside(p)) {
        return nullptr;
    }

    if (isLeaf()) {
        return const_cast<QuadNode*>(this);
    }

    for (auto childNode : child) {
        if (childNode && childNode->range && childNode->range->Inside(p)) {
            return childNode->Search(p);
        }
    }
    return const_cast<QuadNode*>(this);
}

bool QuadNode::isLeaf() const {
    return child[0] == nullptr && child[1] == nullptr && child[2] == nullptr &&
           child[3] == nullptr;
}

void QuadNode::findPointLeaf(point2d p, vector<GPSdata*>& leaf) const {
    if (!range || !range->Inside(p)) {
        return;
    }

    if (this->isLeaf()) {
        leaf = this->data;
    } else {
        if (this->child[LEFTTOP] != nullptr &&
            this->child[LEFTTOP]->range->Inside(p)) {
            this->child[LEFTTOP]->findPointLeaf(p, leaf);
        } else if (this->child[LEFTBOT] != nullptr &&
                   this->child[LEFTBOT]->range->Inside(p)) {
            this->child[LEFTBOT]->findPointLeaf(p, leaf);
        } else if (this->child[RIGHTTOP] != nullptr &&
                   this->child[RIGHTTOP]->range->Inside(p)) {
            this->child[RIGHTTOP]->findPointLeaf(p, leaf);
        } else if (this->child[RIGHTBOT] != nullptr &&
                   this->child[RIGHTBOT]->range->Inside(p)) {
            this->child[RIGHTBOT]->findPointLeaf(p, leaf);
        }
    }
}

void QuadNode::collectRange(const Rectangle& rect, vector<GPSdata*>& out) const {
    if (!range || !range->Intersects(rect)) {
        return;
    }

    if (isLeaf()) {
        for (auto dat : data) {
            if (dat && rect.Inside(dat->longitude, dat->latitude)) {
                out.emplace_back(dat);
            }
        }
        return;
    }

    for (auto childNode : child) {
        if (childNode) {
            childNode->collectRange(rect, out);
        }
    }
}

vector<GPSdata*> QuadNode::PointSearch(point2d p) const {
    vector<GPSdata*> leaf;
    this->findPointLeaf(p, leaf);
    vector<GPSdata*> result;
    // std::cout << leaf.size();
    //printVector(leaf, "E:/leaf.txt");
    for (auto dat : leaf) {
        if (equal(dat->longitude, p.first) && equal(dat->latitude, p.second)) {
            result.emplace_back(dat);
        }
    }

    return result;
}

vector<GPSdata*> QuadNode::AreaSearch(const Rectangle* rect) const {
    if (!rect) {
        return {};
    }
    vector<GPSdata*> result;
    collectRange(*rect, result);
    return result;
}

vector<GPSdata*> QuadNode::TrajectorySearch(const Rectangle* rect, int target_idx) const {
    if (!rect) {
        return {};
    }
    vector<GPSdata*> area;
    collectRange(*rect, area);
    vector<GPSdata*> result;
    result.reserve(area.size());
    for (auto dat : area) {
        if (dat && dat->id == target_idx) {
            result.emplace_back(dat);
        }
    }

    return result;
}

GPSdata* QuadNode::AdjacentSearch(Rectangle* rect,
                                  vector<int> time1,
                                  vector<int> time2,
                                  int idx) {
    vector<GPSdata*> area = AreaSearch(rect);
    vector<GPSdata*> taxi_this;
    vector<GPSdata*> taxi_other;
    GPSdata* result;
    double min_dis = DBL_MAX;
    for (auto gps : area) {
        if (later(gps->time, time1) && earlier(gps->time, time2)) {
            if (gps->id == idx) {
                taxi_this.emplace_back(gps);
            } else {
                taxi_other.emplace_back(gps);
            }
        }
    }

    for (auto other_taxi : taxi_other) {
        for (auto this_taxi : taxi_this) {
            double d = distance(other_taxi, this_taxi);

            if (d < min_dis) {
                min_dis = d;
                result = other_taxi;
            }
        }
    }
    return result;
}

bool QuadNode::PointDelete(int id, double lon, double lat) {
    // 如果当前区域不包含该点，直接返回失败
    if (!range->Inside(lon, lat)) {
        return false;
    }

    // 如果是叶子节点，直接在当前节点的data中查找并删除
    if (isLeaf()) {
        for (auto it = data.begin(); it != data.end(); ++it) {
            if ((*it)->id == id && equal((*it)->longitude, lon) && equal((*it)->latitude, lat)) {
                delete *it; // 释放内存
                data.erase(it);
                return true;
            }
        }
        return false; // 未找到匹配的点
    } else {
        // 确定子节点位置
        double x_center = (range->top_right.first + range->bottom_left.first) / 2;
        double y_center = (range->top_right.second + range->bottom_left.second) / 2;

        int index = -1;
        if (lon <= x_center) {
            index = (lat >= y_center) ? LEFTTOP : LEFTBOT;
        } else {
            index = (lat >= y_center) ? RIGHTTOP : RIGHTBOT;
        }

        // 如果子节点存在，递归删除
        if (child[index] != nullptr) {
            return child[index]->PointDelete(id, lon, lat);
        } else {
            // 子节点不存在，说明数据存储在当前节点（可能未分割）
            for (auto it = data.begin(); it != data.end(); ++it) {
                if ((*it)->id == id && equal((*it)->longitude, lon) && equal((*it)->latitude, lat)) {
                    delete *it;
                    data.erase(it);
                    return true;
                }
            }
            return false;
        }
    }
}

bool QuadNode::PointDelete(int id, string time) {
    bool deleted = false;

    if (!isLeaf()) {
        for (int i = 0; i < 4; ++i) {
            if (child[i] != nullptr) {
                deleted = child[i]->PointDelete(id, time);
                if (deleted) return true;
            }
        }
    }

    for (auto it = data.begin(); it != data.end(); ++it) {
        if ((*it)->id == id && (*it)->time == time) {
            delete *it;
            data.erase(it);
            return true;
        }
    }

    return deleted;
}

bool QuadNode::PointChange(int id, point2d p, double new_lon, double new_lat) {
    // 旧坐标p对应的点
    GPSdata* old_data = nullptr;

    // 找到旧坐标p的叶子节点
    vector<GPSdata*> leaf_data;
    findPointLeaf(p, leaf_data);

    // 查找旧数据点
    for (auto data : leaf_data) {
        if (data->id == id && equal(data->longitude, p.first) && equal(data->latitude, p.second)) {
            old_data = data;
            break;
        }
    }

    if (old_data == nullptr) {
        return false; // 未找到旧数据点
    }

    // 删除旧数据点
    if (!PointDelete(id, p.first, p.second)) {
        return false;
    }

    // 创建新数据点并插入四叉树
    GPSdata* new_data = new GPSdata(id, old_data->time, new_lon, new_lat);
   // delete old_data; // 释放旧数据内存

    // 插入新数据点（需要重新构建四叉树插入逻辑）
    // 假设根节点为this，最大深度为某个固定值（如10）
    InsertNode(new_data, 10);
    return true;
}

//bool QuadNode::PointChange(int id, point2d p, double lon, double lat) {}

//bool QuadNode::PointDelete(int id, double lon, double lat) {}

//bool QuadNode::PointDelete(int id, string time) {}
