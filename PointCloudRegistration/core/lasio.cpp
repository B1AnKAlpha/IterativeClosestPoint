#include "lasio.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <functional>

bool LASIO::readLAS(const std::string& filename, PointCloud& cloud, size_t maxPoints)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return false;
    }
    
    // 设置更大的I/O缓冲区(1MB)以提高读取速度
    const int IO_BUFFER_SIZE = 1024 * 1024;
    std::vector<char> io_buffer(IO_BUFFER_SIZE);
    file.rdbuf()->pubsetbuf(io_buffer.data(), IO_BUFFER_SIZE);
    
    // 读取LAS文件头(227字节 - LAS 1.2标准)
    char header[227];
    file.read(header, 227);
    
    if (!file) {
        std::cerr << "无法读取文件头" << std::endl;
        file.close();
        return false;
    }
    
    // 验证签名
    if (std::strncmp(header, "LASF", 4) != 0) {
        std::cerr << "不是有效的LAS文件" << std::endl;
        file.close();
        return false;
    }
    
    // 从头部提取关键信息(使用指针直接访问,避免结构体对齐问题)
    uint32_t offset_to_data = *reinterpret_cast<uint32_t*>(header + 96);
    uint32_t num_point_records = *reinterpret_cast<uint32_t*>(header + 107);
    uint16_t point_record_length = *reinterpret_cast<uint16_t*>(header + 105);
    
    // 获取缩放因子和偏移量
    double x_scale = *reinterpret_cast<double*>(header + 131);
    double y_scale = *reinterpret_cast<double*>(header + 139);
    double z_scale = *reinterpret_cast<double*>(header + 147);
    double x_offset = *reinterpret_cast<double*>(header + 155);
    double y_offset = *reinterpret_cast<double*>(header + 163);
    double z_offset = *reinterpret_cast<double*>(header + 171);
    
    std::cout << "LAS文件信息:" << std::endl;
    std::cout << "  点数: " << num_point_records << std::endl;
    std::cout << "  点记录长度: " << point_record_length << std::endl;
    std::cout << "  缩放因子: (" << x_scale << ", " << y_scale << ", " << z_scale << ")" << std::endl;
    std::cout << "  偏移量: (" << x_offset << ", " << y_offset << ", " << z_offset << ")" << std::endl;
    
    // 跳转到点数据开始位置
    file.seekg(offset_to_data, std::ios::beg);
    
    // 确定要读取的点数
    size_t numToRead = num_point_records;
    if (maxPoints > 0 && maxPoints < numToRead) {
        numToRead = maxPoints;
    }
    
    cloud.clear();
    cloud.points.reserve(numToRead);
    
    std::cout << "开始批量读取点数据..." << std::endl;
    
    // 批量读取点数据 - 每次读取10000个点
    const int BATCH_SIZE = 10000;
    std::vector<char> buffer(BATCH_SIZE * point_record_length);
    
    int read_count = 0;
    int last_progress = 0;
    
    for (size_t batch_start = 0; batch_start < numToRead; batch_start += BATCH_SIZE) {
        int points_to_read = std::min(BATCH_SIZE, static_cast<int>(numToRead - batch_start));
        int bytes_to_read = points_to_read * point_record_length;
        
        // 一次性读取一批点
        file.read(buffer.data(), bytes_to_read);
        
        if (!file && !file.eof()) {
            std::cerr << "警告: 读取批次时失败,已读取 " << read_count << " 个点" << std::endl;
            break;
        }
        
        // 解析批次中的点
        for (int i = 0; i < points_to_read; i++) {
            int offset = i * point_record_length;
            int32_t x = *reinterpret_cast<int32_t*>(buffer.data() + offset);
            int32_t y = *reinterpret_cast<int32_t*>(buffer.data() + offset + 4);
            int32_t z = *reinterpret_cast<int32_t*>(buffer.data() + offset + 8);
            
            Point3D p;
            p.x = x * x_scale + x_offset;
            p.y = y * y_scale + y_offset;
            p.z = z * z_scale + z_offset;
            
            cloud.points.push_back(p);
            read_count++;
        }
        
        // 显示进度(每50000个点显示一次)
        int progress = (read_count / 50000) * 50000;
        if (progress > last_progress && progress > 0) {
            std::cout << "  已读取 " << read_count << " / " << numToRead << " 个点 ("
                     << (read_count * 100 / numToRead) << "%)" << std::endl;
            last_progress = progress;
        }
    }
    
    file.close();
    
    // 计算边界
    cloud.computeBounds();
    
    std::cout << "成功读取 " << cloud.points.size() << " 个点" << std::endl;
    std::cout << "边界: X[" << cloud.minX << ", " << cloud.maxX << "]" << std::endl;
    std::cout << "      Y[" << cloud.minY << ", " << cloud.maxY << "]" << std::endl;
    std::cout << "      Z[" << cloud.minZ << ", " << cloud.maxZ << "]" << std::endl;
    
    return true;
}

bool LASIO::writeLAS(const std::string& filename, const PointCloud& cloud)
{
    if (cloud.empty()) {
        std::cerr << "点云为空，无法写入" << std::endl;
        return false;
    }
    
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法创建文件: " << filename << std::endl;
        return false;
    }
    
    // 创建227字节的LAS 1.2头部
    char header[227];
    std::memset(header, 0, 227);
    
    // 文件签名
    std::strncpy(header, "LASF", 4);
    
    // 版本号 (offset 24-25)
    header[24] = 1;  // major
    header[25] = 2;  // minor
    
    // 头部大小 (offset 94-95)
    *reinterpret_cast<uint16_t*>(header + 94) = 227;
    
    // 点数据偏移 (offset 96-99)
    *reinterpret_cast<uint32_t*>(header + 96) = 227;
    
    // 点格式 (offset 104)
    header[104] = 0;
    
    // 点记录长度 (offset 105-106)
    *reinterpret_cast<uint16_t*>(header + 105) = 20;
    
    // 点数 (offset 107-110)
    *reinterpret_cast<uint32_t*>(header + 107) = static_cast<uint32_t>(cloud.points.size());
    
    // 缩放因子
    *reinterpret_cast<double*>(header + 131) = 0.001;  // x_scale
    *reinterpret_cast<double*>(header + 139) = 0.001;  // y_scale
    *reinterpret_cast<double*>(header + 147) = 0.001;  // z_scale
    
    // 偏移量
    *reinterpret_cast<double*>(header + 155) = cloud.minX;  // x_offset
    *reinterpret_cast<double*>(header + 163) = cloud.minY;  // y_offset
    *reinterpret_cast<double*>(header + 171) = cloud.minZ;  // z_offset
    
    // 边界
    *reinterpret_cast<double*>(header + 179) = cloud.maxX;
    *reinterpret_cast<double*>(header + 187) = cloud.minX;
    *reinterpret_cast<double*>(header + 195) = cloud.maxY;
    *reinterpret_cast<double*>(header + 203) = cloud.minY;
    *reinterpret_cast<double*>(header + 211) = cloud.maxZ;
    *reinterpret_cast<double*>(header + 219) = cloud.minZ;
    
    // 写入头部
    file.write(header, 227);
    
    // 写入点数据
    double x_scale = 0.001;
    double y_scale = 0.001;
    double z_scale = 0.001;
    
    for (const auto& p : cloud.points) {
        int32_t x = static_cast<int32_t>((p.x - cloud.minX) / x_scale);
        int32_t y = static_cast<int32_t>((p.y - cloud.minY) / y_scale);
        int32_t z = static_cast<int32_t>((p.z - cloud.minZ) / z_scale);
        
        file.write(reinterpret_cast<const char*>(&x), sizeof(int32_t));
        file.write(reinterpret_cast<const char*>(&y), sizeof(int32_t));
        file.write(reinterpret_cast<const char*>(&z), sizeof(int32_t));
        
        // 写入其他字段（填充0）
        char padding[8] = {0};
        file.write(padding, 8);
    }
    
    file.close();
    
    std::cout << "成功写入 " << cloud.points.size() << " 个点到 " << filename << std::endl;
    return true;
}

size_t LASIO::readLASBatch(const std::string& filename, 
                           size_t batch_size,
                           std::function<void(const std::vector<Point3D>&)> process_func)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "无法打开文件: " << filename << std::endl;
        return 0;
    }
    
    // 设置I/O缓冲区
    const int IO_BUFFER_SIZE = 1024 * 1024;
    std::vector<char> io_buffer(IO_BUFFER_SIZE);
    file.rdbuf()->pubsetbuf(io_buffer.data(), IO_BUFFER_SIZE);
    
    // 读取LAS文件头
    char header[227];
    file.read(header, 227);
    
    if (!file || std::strncmp(header, "LASF", 4) != 0) {
        std::cerr << "不是有效的LAS文件" << std::endl;
        file.close();
        return 0;
    }
    
    // 提取头部信息
    uint32_t offset_to_data = *reinterpret_cast<uint32_t*>(header + 96);
    uint32_t num_point_records = *reinterpret_cast<uint32_t*>(header + 107);
    uint16_t point_record_length = *reinterpret_cast<uint16_t*>(header + 105);
    
    double x_scale = *reinterpret_cast<double*>(header + 131);
    double y_scale = *reinterpret_cast<double*>(header + 139);
    double z_scale = *reinterpret_cast<double*>(header + 147);
    double x_offset = *reinterpret_cast<double*>(header + 155);
    double y_offset = *reinterpret_cast<double*>(header + 163);
    double z_offset = *reinterpret_cast<double*>(header + 171);
    
    // 跳转到点数据
    file.seekg(offset_to_data, std::ios::beg);
    
    size_t total_read = 0;
    std::vector<Point3D> batch;
    batch.reserve(batch_size);
    
    // 使用批量读取缓冲区
    const int READ_BUFFER_SIZE = 10000;
    std::vector<char> read_buffer(READ_BUFFER_SIZE * point_record_length);
    
    for (uint32_t batch_start = 0; batch_start < num_point_records; batch_start += READ_BUFFER_SIZE) {
        int points_to_read = std::min(READ_BUFFER_SIZE, static_cast<int>(num_point_records - batch_start));
        int bytes_to_read = points_to_read * point_record_length;
        
        file.read(read_buffer.data(), bytes_to_read);
        
        if (!file && !file.eof()) {
            break;
        }
        
        // 解析读取的数据
        for (int i = 0; i < points_to_read; i++) {
            int offset = i * point_record_length;
            int32_t x = *reinterpret_cast<int32_t*>(read_buffer.data() + offset);
            int32_t y = *reinterpret_cast<int32_t*>(read_buffer.data() + offset + 4);
            int32_t z = *reinterpret_cast<int32_t*>(read_buffer.data() + offset + 8);
            
            Point3D p;
            p.x = x * x_scale + x_offset;
            p.y = y * y_scale + y_offset;
            p.z = z * z_scale + z_offset;
            
            batch.push_back(p);
            
            if (batch.size() >= batch_size) {
                process_func(batch);
                total_read += batch.size();
                batch.clear();
            }
        }
    }
    
    // 处理剩余数据
    if (!batch.empty()) {
        process_func(batch);
        total_read += batch.size();
    }
    
    file.close();
    return total_read;
}
