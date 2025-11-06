# 数字摄影测量课程设计 - ICP点云配准系统



![C++](https://img.shields.io/badge/C++-17-blue.svg) ![Qt](https://img.shields.io/badge/Qt-6.8.3-green.svg) ![Eigen](https://img.shields.io/badge/Eigen-3.x-orange.svg) ![Language](https://img.shields.io/badge/Language-中文-red.svg) ![License](https://img.shields.io/badge/License-Educational-green.svg) ![Status](https://img.shields.io/badge/Status-Complete-success.svg)




## 功能说明

这是一个基于Qt框架开发的ICP(Iterative Closest Point)点云配准系统，提供完整的图形界面和3D可视化功能。支持LAS格式点云文件的导入、配准和结果可视化。

这是一个使用C++实现的ICP(Iterative Closest Point)算法，用于对两个LAS格式的点云文件进行精确配准。

## 项目结构



```

Photo/

├── PointCloudRegistration/      # Qt可视化应用程序├── icp_registration.cpp    # ICP配准主程序```bash

│   ├── core/                    # 核心算法模块

│   │   ├── icpengine.h/cpp     # ICP配准引擎├── test_icp.cpp           # ICP算法测试程序mkdir build

│   │   ├── lasio.h/cpp         # LAS文件读写

│   │   └── pointcloud.h        # 点云数据结构├── Eigen/                 # Eigen线性代数库cd build

│   ├── services/                # 业务逻辑层

│   │   ├── registrationservice.h/cpp  # 配准服务├── README.md              # 项目说明文档cmake ..

│   │   └── settingsservice.h/cpp      # 设置服务

│   ├── ui/                      # 用户界面└── .gitignore            # Git忽略配置cmake --build .

│   │   ├── pages/              # 各功能页面

│   │   │   ├── dashboardpage.cpp      # 概览页``````

│   │   │   ├── datamanagerpage.cpp    # 数据管理页

│   │   │   ├── registrationpage.cpp   # 配准操作页

│   │   │   ├── visualizationpage.cpp  # 3D可视化页

│   │   │   └── settingspage.cpp       # 设置页

│   │   └── mainwindow.h/cpp    # 主窗口## 运行示例

│   ├── widgets/                 # 自定义控件

│   │   └── pointcloudviewer.h/cpp  # 3D点云查看器```bash

│   └── CMakeLists.txt# ICP点云配准（源点云 目标点云）

├── ElaWidgetTools/              # UI组件库（第三方）./icp_registration.exe source.las target.las

├── icp/                         # ICP算法核心实现

│   ├── icp.h/cpp               # ICP算法# 测试程序

│   └── Eigen/                  # Eigen线性代数库./test_icp.exe

└── README.md```

```

## 编译方法

### 方法1: 使用Qt Creator（推荐）



1. 打开Qt Creator

2. 选择 `文件` → `打开文件或项目`

3. 选择 `PointCloudRegistration/CMakeLists.txt`---

4. 配置构建套件（选择Qt 6.8.3 MinGW 64-bit）

5. 点击左下角的 `▶️ 运行` 按钮

### 方法2: 使用CMake命令行

```bash
cd PointCloudRegistration
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

Windows运行：
```powershell
.\Release\PointCloudRegistration.exe
```

Linux/macOS运行：
```bash
./PointCloudRegistration
```



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


### 测试数据

项目包含两个测试点云文件：
- `Scannew_096.las`：源点云
- `Scannew_099.las`：目标点云

### 预期结果

配准成功后：
- RMSE < 0.01（具体取决于点云质量）
- 迭代次数：10-30次
- 处理时间：数秒到数分钟（取决于点云大小）


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
