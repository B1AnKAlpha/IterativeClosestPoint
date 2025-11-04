# ICP点云配准算法实现# ICP点云精匹配程序



![C++](https://img.shields.io/badge/C++-11-blue.svg) ![Language](https://img.shields.io/badge/Language-中文-red.svg) ![License](https://img.shields.io/badge/License-Educational-green.svg) ![Status](https://img.shields.io/badge/Status-Complete-success.svg)## 功能说明

这是一个使用C++实现的ICP(Iterative Closest Point)算法，用于对两个LAS格式的点云文件进行精确配准。

基于C++和Eigen库实现的ICP（Iterative Closest Point）点云配准算法，支持LAS格式点云文件的读取与配准。

## 算法流程

## 项目简介1. 读取两个LAS点云文件

2. 点云下采样(可选，提高速度)

本项目实现了经典的ICP点云配准算法，并加入了多项优化：3. ICP迭代配准:

- 八叉树加速最近邻搜索   - 找到最近点对应

- SVD最优变换求解   - 计算变换矩阵(旋转+平移)

- 3-sigma原则剔除离群点   - 应用变换

- 支持LAS格式点云文件读写   - 检查收敛条件

4. 输出配准结果到CSV文件

## 目录结构

## 编译方法

```

.### 方法1: 使用CMake

├── icp_registration.cpp    # ICP配准主程序```bash

├── test_icp.cpp           # ICP算法测试程序mkdir build

├── Eigen/                 # Eigen线性代数库cd build

├── README.md              # 项目说明文档cmake ..

└── .gitignore            # Git忽略配置cmake --build .

``````



## 核心功能### 方法2: 直接使用编译器

```bash

### ICP配准算法# 使用g++

g++ -o icp_registration icp_registration.cpp -std=c++11

实现了完整的ICP点云配准流程：

1. **八叉树构建**：对目标点云构建八叉树索引，加速最近邻搜索# 使用MSVC (Visual Studio开发人员命令提示符)

2. **最近点匹配**：使用八叉树快速查找对应点对cl /EHsc /std:c++11 icp_registration.cpp

3. **离群点剔除**：采用3-sigma原则剔除距离过大的误匹配点对```

4. **变换求解**：使用SVD分解计算最优旋转矩阵和平移向量

5. **迭代优化**：迭代更新变换直至收敛## 运行

```bash

### 离群点剔除策略./icp_registration

```

针对低重合度点云配准问题（重合度30%-50%），引入3-sigma原则：

- 计算所有点对距离的均值和标准差## 输入文件

- 设置阈值为 `threshold = mean + 3 * std_dev`- Scannew_096.las (源点云)

- 剔除距离大于阈值的点对，仅用有效点计算变换- Scannew_099.las (目标点云)

- 显著提高配准鲁棒性和精度

## 输出文件

### 文件格式支持- icp_result_source.csv (配准后的源点云)

- icp_result_target.csv (目标点云)

- **输入**：LAS格式点云文件（激光雷达标准格式）

- **输出**：配准后的LAS文件和变换参数文本文件## 参数调整

在main函数中可以调整:

## 使用方法- `sample_rate`: 下采样率(默认10，每10个点取1个)

- `max_iterations`: 最大迭代次数(默认50)

### 环境要求- `tolerance`: 收敛阈值(默认1e-6)

- C++11 或更高版本

- g++/MinGW 编译器## 注意事项

- Eigen 3.x 线性代数库（已包含）1. 确保LAS文件格式正确(标准LAS 1.2格式)

2. 此程序假设已经进行了粗匹配

### 编译3. 对于大型点云，建议使用下采样加速处理

4. 简化版本未使用SVD计算最优旋转，可以进一步改进

```bash
# 编译ICP配准程序
g++ -o icp_registration.exe icp_registration.cpp -std=c++11 -O2 -I.

# 编译测试程序
g++ -o test_icp.exe test_icp.cpp -std=c++11 -O2 -I.
```

### 运行示例

```bash
# ICP点云配准（源点云 目标点云）
./icp_registration.exe source.las target.las

# 测试程序
./test_icp.exe
```

### 输出文件

- `registered_source.las`：配准后的源点云
- `icp_transformation.txt`：变换参数（旋转矩阵R和平移向量t）

## 技术说明

### 八叉树加速

采用空间划分的八叉树结构，将最近邻搜索的时间复杂度从O(n)降低到O(log n)，显著提升配准速度。

### SVD变换求解

使用奇异值分解（SVD）计算刚体变换：
1. 计算点云质心
2. 去质心化
3. 构建协方差矩阵 H = A^T * B
4. SVD分解求解旋转矩阵 R = V * U^T
5. 计算平移向量 t = centroid_B - R * centroid_A

### 3-Sigma离群点剔除

在每次迭代中：
1. 计算所有点对的距离 d_i
2. 计算均值 μ 和标准差 σ
3. 剔除满足 d_i > μ + 3σ 的点对
4. 仅用剩余有效点对计算变换矩阵

这种方法能有效处理以下场景：
- 点云部分重合（重合度30%-50%）
- 点云拼接任务
- 存在较多非对应点的配准问题

### 适用场景

**适合**：
- 点云部分重合的配准/拼接
- 有较好初始对齐的精配准
- 激光雷达点云处理

**不适合**：
- 完全无初始对齐的点云
- 重合度极低（<20%）的场景
- 存在大量噪声和形变的点云

## 算法参数

可在代码中调整的关键参数：

```cpp
int max_iterations = 50;           // 最大迭代次数
double tolerance = 1e-5;           // 收敛阈值
int max_points_per_octree = 10;   // 八叉树叶节点最大点数
int max_octree_depth = 20;        // 八叉树最大深度
double sigma_multiplier = 3.0;     // sigma倍数（3-sigma原则）
```

## 实验结果

使用3-sigma离群点剔除后的改进效果：
- RMSE显著降低
- 配准成功率提升
- 对非重合区域更鲁棒
- 每次迭代显示有效点数和剔除的离群点数

## 许可

本项目仅用于教学和学习目的。

---

该项目是西南交通大学《数字摄影测量》课程实验项目
