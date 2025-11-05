# 点云配准软件 - 编译指南

## 🎉 项目完成！

所有核心功能已经完整实现，包括：
- ✅ 完整的ICP算法引擎（八叉树加速、3-sigma离群点剔除）
- ✅ OpenGL 3D点云查看器（交互式旋转缩放、迭代回放）
- ✅ 5个功能完整的UI页面
- ✅ 配准服务和设置管理
- ✅ 主窗口集成

## 📋 文件清单

### 已创建的文件（共40个）

**项目配置 (3)**
- CMakeLists.txt
- main.cpp
- README.md

**核心算法层 (8)**
- core/pointcloud.h/cpp
- core/lasio.h/cpp
- core/octree.h/cpp
- core/icpengine.h/cpp

**服务层 (4)**
- services/registrationservice.h/cpp
- services/settingsservice.h/cpp

**组件层 (2)**
- widgets/pointcloudviewer.h/cpp

**UI页面层 (12)**
- ui/mainwindow.h/cpp
- ui/pages/dashboardpage.h/cpp
- ui/pages/datamanagerpage.h/cpp
- ui/pages/registrationpage.h/cpp
- ui/pages/visualizationpage.h/cpp
- ui/pages/settingspage.h/cpp

**文档 (3)**
- README.md
- STATUS.md
- IMPLEMENTATION_STATUS.md
- BUILD_GUIDE.md (本文件)

## 🔧 编译步骤

### 前置要求

1. **Qt 6.x** (推荐 6.5+)
   ```bash
   # 确保Qt已安装并配置环境变量
   qmake --version
   ```

2. **CMake 3.16+**
   ```bash
   cmake --version
   ```

3. **C++17编译器**
   - Windows: MinGW 13.1+ 或 MSVC 2019+
   - Linux: GCC 9+ 或 Clang 10+
   - macOS: Xcode 12+

4. **Eigen 3.x** (已包含在 `../icp/Eigen` 目录)

5. **ElaWidgetTools** (已包含在 `../ElaWidgetTools` 目录)

### Windows编译（推荐使用Qt Creator）

#### 方法1：使用Qt Creator (最简单)

1. 打开Qt Creator
2. File → Open File or Project
3. 选择 `PointCloudRegistration/CMakeLists.txt`
4. 选择合适的Kit (Desktop Qt 6.x MinGW 64-bit)
5. 点击"Configure Project"
6. 点击左下角的"Run"按钮（绿色三角形）

#### 方法2：命令行编译

```powershell
cd E:\Study\Grade\3a\Photo\PointCloudRegistration
mkdir build
cd build

# 配置（替换为你的Qt路径）
cmake .. -G "MinGW Makefiles" -DCMAKE_PREFIX_PATH="E:/Qt/6.8.3/mingw_64"

# 编译
cmake --build . --config Release

# 运行
.\PointCloudRegistration.exe
```

### Linux编译

```bash
cd /path/to/Photo/PointCloudRegistration
mkdir build && cd build

# 配置
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64

# 编译
make -j$(nproc)

# 运行
./PointCloudRegistration
```

### macOS编译

```bash
cd /path/to/Photo/PointCloudRegistration
mkdir build && cd build

# 配置
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/macos

# 编译
make -j$(sysctl -n hw.ncpu)

# 运行
open ./PointCloudRegistration.app
```

## 🐛 常见编译问题

### 问题1: 找不到Qt

**错误信息：**
```
Could NOT find Qt6 (missing: Qt6_DIR)
```

**解决方法：**
```bash
cmake .. -DCMAKE_PREFIX_PATH="你的Qt安装路径/6.x/编译器版本"
```

### 问题2: 找不到Eigen

**错误信息：**
```
fatal error: Eigen/Eigen: No such file or directory
```

**解决方法：**
确保 `../icp/Eigen` 目录存在，包含Eigen库头文件。

### 问题3: 找不到ElaWidgetTools

**错误信息：**
```
Could not find ElaWidgetTools
```

**解决方法：**
1. 确保 `../ElaWidgetTools/ElaWidgetTools` 目录存在
2. 先编译ElaWidgetTools：
   ```bash
   cd ../ElaWidgetTools/ElaWidgetTools
   mkdir build && cd build
   cmake .. -DCMAKE_PREFIX_PATH="你的Qt路径"
   cmake --build .
   ```

### 问题4: OpenGL相关错误

**Windows上缺少OpenGL库：**
```bash
# 确保安装了OpenGL相关包
# MinGW通常自带，如果缺失：
# 1. 更新显卡驱动
# 2. 重新安装Qt，确保选中了OpenGL组件
```

### 问题5: 链接错误

**解决方法：**
```bash
# 清理构建目录重新编译
cd build
rm -rf *  # 或 del /s /q * (Windows)
cmake ..
cmake --build .
```

## ✅ 验证安装

编译成功后，运行程序应该看到：

1. **主窗口打开**，显示"点云配准系统"标题
2. **左侧导航栏**包含5个页面图标：
   - 🏠 概览
   - 📁 数据管理
   - ⚙️ 配准控制台
   - 📦 3D可视化
   - 🔧 设置
3. **默认显示概览页面**，包含欢迎信息和快速开始指南

## 🚀 快速测试

### 测试配准功能

1. **导入测试数据**
   - 点击"数据管理"页面
   - 点击"导入源点云" → 选择 `../Scannew_096.las`
   - 点击"导入目标点云" → 选择 `../Scannew_099.las`

2. **配置参数**（可选）
   - 点击"设置"页面
   - 调整ICP参数（默认值通常已经很好）

3. **开始配准**
   - 点击"配准控制台"页面
   - 点击"开始配准"按钮
   - 观察实时进度和日志输出

4. **查看结果**
   - 点击"3D可视化"页面
   - 使用鼠标左键旋转视图
   - 使用滚轮缩放
   - 使用播放控制查看迭代过程

## 📦 发布打包

### Windows打包

```bash
# 编译Release版本
cmake --build . --config Release

# 使用windeployqt打包
cd Release
windeployqt PointCloudRegistration.exe

# 手动复制ElaWidgetTools.dll到Release目录
copy ..\..\ElaWidgetTools\ElaWidgetTools\build\Release\ElaWidgetTools.dll .
```

### Linux打包

```bash
# 使用linuxdeployqt
linuxdeployqt PointCloudRegistration -appimage
```

### macOS打包

```bash
# 使用macdeployqt
macdeployqt PointCloudRegistration.app -dmg
```

## 📝 开发说明

### 项目结构设计

项目采用分层架构：
```
表现层 (UI Pages) 
    ↓
业务层 (Services)
    ↓
核心层 (Core Algorithms + Widgets)
```

### 添加新功能

1. **添加新的ICP算法变体**
   - 修改 `core/icpengine.cpp` 中的 `runICP()` 方法
   - 添加新参数到 `ICPParameters` 结构

2. **添加新的UI页面**
   - 在 `ui/pages/` 创建新页面类
   - 在 `ui/mainwindow.cpp` 的 `setupPages()` 中注册
   - 使用 `addPageNode()` 添加到导航栏

3. **添加新的点云格式支持**
   - 在 `core/lasio.cpp` 添加新的读写函数
   - 更新 `DataManagerPage` 的文件过滤器

## 🎓 技术栈

- **Qt 6.x** - UI框架
- **ElaWidgetTools** - 现代化UI组件库
- **Eigen 3.x** - 线性代数库（SVD分解）
- **OpenGL** - 3D渲染
- **C++17** - 编程语言
- **CMake** - 构建系统

## 📚 相关文档

- [README.md](README.md) - 项目概述和功能说明
- [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md) - 实现进度
- [Qt Documentation](https://doc.qt.io/qt-6/)
- [Eigen Documentation](https://eigen.tuxfamily.org/dox/)

## 🤝 技术支持

如遇到问题：
1. 检查CMake输出日志
2. 确认所有依赖已正确安装
3. 尝试清理构建目录重新编译
4. 查看编译器错误信息

---

**祝编译顺利！🎉**

完成编译后，您将拥有一个功能完整的专业点云配准软件！
