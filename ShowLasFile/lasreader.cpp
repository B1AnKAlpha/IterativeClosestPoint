#include "lasreader.h"
#include <algorithm>

bool LASReader::readLASFile(const std::string& filename, PointCloud& cloud, int sample_rate) {
    std::cout << "正在打开文件: " << filename << std::endl;
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "错误: 无法打开文件: " << filename << std::endl;
        return false;
    }
    
    // 设置I/O缓冲区
    const int BUFFER_SIZE = 1024 * 1024;
    std::vector<char> io_buffer(BUFFER_SIZE);
    file.rdbuf()->pubsetbuf(io_buffer.data(), BUFFER_SIZE);
    
    // 读取LAS文件头
    char header[227];
    file.read(header, 227);
    
    if (!file) {
        std::cerr << "错误: 无法读取文件头" << std::endl;
        file.close();
        return false;
    }
    
    // 获取点数据偏移
    unsigned int offset_to_points = *((unsigned int*)(header + 96));
    
    // 获取点数
    unsigned int num_points = *((unsigned int*)(header + 107));
    std::cout << "点数量: " << num_points << std::endl;
    
    // 获取点数据格式和记录长度
    unsigned short point_record_length = *((unsigned short*)(header + 105));
    
    if (num_points == 0 || num_points > 100000000) {
        std::cerr << "警告: 点数量异常: " << num_points << std::endl;
        file.close();
        return false;
    }
    
    // 获取比例因子和偏移量
    cloud.x_scale = *((double*)(header + 131));
    cloud.y_scale = *((double*)(header + 139));
    cloud.z_scale = *((double*)(header + 147));
    cloud.x_offset = *((double*)(header + 155));
    cloud.y_offset = *((double*)(header + 163));
    cloud.z_offset = *((double*)(header + 171));
    
    // 获取边界值
    cloud.x_max = *((double*)(header + 179));
    cloud.x_min = *((double*)(header + 187));
    cloud.y_max = *((double*)(header + 195));
    cloud.y_min = *((double*)(header + 203));
    cloud.z_max = *((double*)(header + 211));
    cloud.z_min = *((double*)(header + 219));
    
    // 预分配内存空间
    cloud.reserve(num_points / sample_rate + 1);
    
    // 定位到点数据
    file.seekg(offset_to_points, std::ios::beg);
    
    if (!file) {
        std::cerr << "错误: 无法定位到点数据" << std::endl;
        file.close();
        return false;
    }
    
    std::cout << "开始读取点数据..." << std::endl;
    
    // 批量读取点数据
    const int BATCH_SIZE = 10000;
    std::vector<char> buffer(BATCH_SIZE * point_record_length);
    
    int read_count = 0;
    int last_progress = 0;
    
    for (unsigned int batch_start = 0; batch_start < num_points; batch_start += BATCH_SIZE) {
        int points_to_read = std::min(BATCH_SIZE, (int)(num_points - batch_start));
        int bytes_to_read = points_to_read * point_record_length;
        
        // 一次性读取一批点
        file.read(buffer.data(), bytes_to_read);
        
        if (!file && !file.eof()) {
            std::cerr << "警告: 读取失败,已读取 " << read_count << " 个点" << std::endl;
            break;
        }
        
        // 解析批次中的点
        for (int i = 0; i < points_to_read; i++) {
            // 应用采样率
            if ((batch_start + i) % sample_rate != 0) continue;
            
            int offset = i * point_record_length;
            int x_raw = *((int*)(buffer.data() + offset));
            int y_raw = *((int*)(buffer.data() + offset + 4));
            int z_raw = *((int*)(buffer.data() + offset + 8));
            
            Point3D point;
            point.x = x_raw * cloud.x_scale + cloud.x_offset;
            point.y = y_raw * cloud.y_scale + cloud.y_offset;
            point.z = z_raw * cloud.z_scale + cloud.z_offset;
            
            cloud.points.push_back(point);
            read_count++;
        }
        
        // 显示进度
        int progress = ((batch_start + points_to_read) * 100 / num_points);
        if (progress > last_progress && progress % 10 == 0) {
            std::cout << "读取进度: " << progress << "%" << std::endl;
            last_progress = progress;
        }
    }
    
    file.close();
    std::cout << "成功读取 " << read_count << " 个点" << std::endl;
    
    return read_count > 0;
}
