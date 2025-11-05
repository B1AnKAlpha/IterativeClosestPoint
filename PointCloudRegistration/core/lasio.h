#ifndef LASIO_H
#define LASIO_H

#include <string>
#include <functional>
#include "pointcloud.h"

/**
 * @brief LAS文件输入输出工具类
 */
class LASIO
{
public:
    /**
     * @brief 读取LAS文件
     * @param filename 文件路径
     * @param cloud 输出点云对象
     * @param maxPoints 最大读取点数 (0表示读取所有点)
     * @return 是否成功
     */
    static bool readLAS(const std::string& filename, PointCloud& cloud, size_t maxPoints = 0);
    
    /**
     * @brief 写入LAS文件
     * @param filename 文件路径
     * @param cloud 输入点云对象
     * @return 是否成功
     */
    static bool writeLAS(const std::string& filename, const PointCloud& cloud);
    
    /**
     * @brief 批量读取LAS文件
     * @param filename 文件路径
     * @param batch_size 每批读取的点数
     * @param process_func 处理每批数据的函数
     * @return 总共读取的点数
     */
    static size_t readLASBatch(const std::string& filename, 
                               size_t batch_size,
                               std::function<void(const std::vector<Point3D>&)> process_func);

private:
    // LAS文件头部结构（简化版）
    struct LASHeader {
        char signature[4];
        uint16_t file_source_id;
        uint16_t global_encoding;
        uint32_t project_id1;
        uint16_t project_id2;
        uint16_t project_id3;
        uint64_t project_id4;
        uint8_t version_major;
        uint8_t version_minor;
        char system_id[32];
        char software[32];
        uint16_t creation_day;
        uint16_t creation_year;
        uint16_t header_size;
        uint32_t offset_to_data;
        uint32_t num_variable_records;
        uint8_t point_format;
        uint16_t point_record_length;
        uint32_t num_point_records;
        uint32_t num_points_by_return[5];
        double x_scale;
        double y_scale;
        double z_scale;
        double x_offset;
        double y_offset;
        double z_offset;
        double max_x;
        double min_x;
        double max_y;
        double min_y;
        double max_z;
        double min_z;
    };
};

#endif // LASIO_H
