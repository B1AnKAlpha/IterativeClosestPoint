# 数字摄影测量课程设计



![C++](https://img.shields.io/badge/C++-11-blue.svg) ![Language](https://img.shields.io/badge/Language-中文-red.svg) ![License](https://img.shields.io/badge/License-Educational-green.svg) ![Status](https://img.shields.io/badge/Status-Complete-success.svg)
## 功能说明

这是一个使用C++实现的ICP(Iterative Closest Point)算法，用于对两个LAS格式的点云文件进行精确配准。

基于C++和Eigen库实现的ICP（Iterative Closest Point）点云配准算法，支持LAS格式点云文件的读取与配准。





## 编译方法

```

.### 方法1: 使用CMake

├── icp_registration.cpp    # ICP配准主程序```bash

├── test_icp.cpp           # ICP算法测试程序mkdir build

├── Eigen/                 # Eigen线性代数库cd build

├── README.md              # 项目说明文档cmake ..

└── .gitignore            # Git忽略配置cmake --build .

``````




## 运行示例

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

**八叉树加速最近邻搜索**  
用八叉树索引目标点云，查找最近点时间从O(n)降到O(log n)。

**SVD求解变换矩阵**  
对匹配点对去质心后，用SVD分解协方差矩阵求旋转矩阵R和平移向量t。

**3-Sigma剔除离群点**  
每次迭代计算点对距离的均值和标准差，剔除距离大于均值+3倍标准差的点对，只用剩下的有效点算变换。这样能处理点云部分重合（30%-50%）的情况，比如点云拼接。



## 算法参数

可在代码中调整的关键参数：

```cpp
int max_iterations = 50;           // 最大迭代次数
double tolerance = 1e-5;           // 收敛阈值
int max_points_per_octree = 10;   // 八叉树叶节点最大点数
int max_octree_depth = 20;        // 八叉树最大深度
double sigma_multiplier = 3.0;     // sigma倍数（3-sigma原则）
```



---

