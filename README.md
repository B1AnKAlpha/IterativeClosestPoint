# 数字摄影测量课程设计 - ICP点云配准系统



![C++](https://img.shields.io/badge/C++-17-blue.svg) ![Qt](https://img.shields.io/badge/Qt-6.8.3-green.svg) ![Eigen](https://img.shields.io/badge/Eigen-3.x-orange.svg) ![Language](https://img.shields.io/badge/Language-中文-red.svg) ![License](https://img.shields.io/badge/License-Educational-green.svg) ![Status](https://img.shields.io/badge/Status-Complete-success.svg)




## 功能说明

这是一个基于Qt框架开发的ICP(Iterative Closest Point)点云配准系统，提供完整的图形界面和3D可视化功能。支持LAS格式点云文件的导入、配准和结果可视化。

## 项目结构

```
├── PointCloudRegistration/    # Qt可视化应用程序
│   ├── core/                  # 核心算法（ICP引擎、LAS文件读写）
│   ├── services/              # 业务逻辑（配准服务、设置服务）
│   ├── ui/                    # 用户界面（主窗口、各功能页面）
│   ├── widgets/               # 自定义控件（3D点云查看器）
│   └── CMakeLists.txt
├── ElaWidgetTools/            # UI组件库
├── Eigen/                     # Eigen线性代数库
├── icp_registration.cpp       # 命令行版ICP配准程序
├── test_icp.cpp               # ICP算法测试程序
├── CMakeLists.txt             # 项目构建配置
└── README.md
```

## 编译方法

### 方法1: Qt可视化版本（推荐）

使用Qt Creator：
1. 打开Qt Creator
2. 选择 `文件` → `打开文件或项目`
3. 选择 `PointCloudRegistration/CMakeLists.txt`
4. 配置构建套件（选择Qt 6.8.3 MinGW 64-bit）
5. 点击左下角的 `▶️ 运行` 按钮

或使用CMake命令行：
```bash
cd PointCloudRegistration
mkdir build
cd build
cmake ..
cmake --build . --config Release
./Release/PointCloudRegistration.exe  # Windows
```

### 方法2: 命令行版本

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 运行示例

### Qt可视化版本
直接运行程序，通过图形界面操作：
- 数据管理页面导入点云
- 配准页面设置参数并执行
- 可视化页面查看结果和迭代过程

### 命令行版本
```bash
# ICP点云配准（源点云 目标点云）
./icp_registration.exe source.las target.las

# 测试程序
./test_icp.exe
```

### 输出文件

- `registered_source.las`：配准后的源点云
- `icp_transformation.txt`：变换参数（旋转矩阵R和平移向量t）



## 算法参数说明

```cpp
// ICP配准参数
int maxIterations = 50;           // 最大迭代次数
double tolerance = 1e-6;          // 收敛阈值（RMSE变化量）
double sigmaMultiplier = 3.0;     // 离群点剔除倍数（3-sigma原则）

// 八叉树参数
int octreeMaxPoints = 10;         // 叶节点最大点数
int octreeMaxDepth = 20;          // 最大深度

// 渲染参数
float sourcePointSize = 2.0f;     // 源点云点大小
float targetPointSize = 2.0f;     // 目标点云点大小
QColor sourceColor = Qt::red;     // 源点云颜色
QColor targetColor = Qt::blue;    // 目标点云颜色
```



## 依赖项说明

### ElaWidgetTools

- **用途**：提供Material Design风格的UI组件
- **许可证**：MIT License（已包含在项目中）
- **包含组件**：按钮、滑块、开关、卡片等

### Eigen

- **用途**：矩阵运算和SVD分解
- **版本**：3.x
- **许可证**：MPL2（已包含在项目中）



## 致谢

- Eigen团队提供的优秀线性代数库
- ElaWidgetTools作者提供的UI组件
- Qt框架提供的跨平台开发支持

---

**如有问题或建议，欢迎提Issue！**
