# GitHub上传文件清单

## 需要上传的文件和文件夹

### 核心代码文件
- `icp_registration.cpp` - ICP配准主程序（已添加3-sigma离群点剔除）
- `test_icp.cpp` - ICP算法测试程序

### 依赖库
- `Eigen/` - Eigen线性代数库（整个文件夹）

### 文档文件
- `README.md` - 项目说明文档
- `.gitignore` - Git忽略配置

## 不需要上传的文件（已在.gitignore中配置）

### 编译产物
- `*.exe` - 可执行文件
- `build/` - CMake构建目录
- `*.o`, `*.obj` - 目标文件

### 点云数据文件
- `*.las` - 点云文件（体积大，建议不上传）
- `Scannew_096.las`
- `Scannew_099.las`
- `registered_*.las`
- `sampled_*.las`
- `test_source_transformed.las`

### 其他不需要的文件
- `icp/` - 原ICP文件夹（Eigen已复制到主目录）
- `CMakeLists.txt` - 如果不需要CMake构建可以删除
- `icp_transformation.txt` - 输出结果文件

## Git操作步骤

```bash
# 1. 初始化Git仓库（如果还没有）
git init

# 2. 添加文件
git add icp_registration.cpp
git add test_icp.cpp
git add README.md
git add .gitignore
git add Eigen/

# 3. 提交
git commit -m "Initial commit: ICP点云配准算法实现（含3-sigma离群点剔除）"

# 4. 关联远程仓库
git remote add origin https://github.com/你的用户名/你的仓库名.git

# 5. 推送到GitHub
git branch -M main
git push -u origin main
```

## 项目特色说明

本项目的主要改进点：
1. ✅ 使用八叉树加速最近邻搜索
2. ✅ SVD求解最优刚体变换
3. ✅ **3-sigma原则剔除离群点**（新增）
4. ✅ 支持低重合度点云配准（30%-50%）
5. ✅ 完整的LAS文件读写支持

适合作为点云处理、计算机视觉课程的参考项目。
