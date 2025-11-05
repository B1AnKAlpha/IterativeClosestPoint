cmake_minimum_required(VERSION 3.16)

project(PointCloudRegistration VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加Eigen头文件路径
set(EIGEN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../icp)
include_directories(${EIGEN_INCLUDE_DIR})

# 查找OpenGL库
find_package(OpenGL REQUIRED)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Concurrent)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGL OpenGLWidgets Concurrent)

set(PROJECT_SOURCES
    main.cpp
    
    # Core - ICP engine and data structures
    core/pointcloud.h
    core/pointcloud.cpp
    core/icpengine.h
    core/icpengine.cpp
    core/octree.h
    core/octree.cpp
    core/lasio.h
    core/lasio.cpp
    
    # Services
    services/registrationservice.h
    services/registrationservice.cpp
    services/settingsservice.h
    services/settingsservice.cpp
    
    # Widgets
    widgets/pointcloudviewer.h
    widgets/pointcloudviewer.cpp
    
    # UI Pages
    ui/mainwindow.h
    ui/mainwindow.cpp
    ui/pages/settingspage.h
    ui/pages/settingspage.cpp
    ui/pages/datamanagerpage.h
    ui/pages/datamanagerpage.cpp
    ui/pages/registrationpage.h
    ui/pages/registrationpage.cpp
    ui/pages/visualizationpage.h
    ui/pages/visualizationpage.cpp
    ui/pages/dashboardpage.h
    ui/pages/dashboardpage.cpp
)

# 添加ElaWidgetTools子项目
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ElaWidgetTools/ElaWidgetTools
                 ${CMAKE_CURRENT_BINARY_DIR}/ElaWidgetToolsBuild)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PointCloudRegistration
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    add_executable(PointCloudRegistration
        ${PROJECT_SOURCES}
    )
endif()

target_include_directories(PointCloudRegistration PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ElaWidgetTools/ElaWidgetTools
    ${EIGEN_INCLUDE_DIR}
)

target_link_libraries(PointCloudRegistration PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::Concurrent
    ElaWidgetTools
    OpenGL::GL
)

set_target_properties(PointCloudRegistration PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS PointCloudRegistration
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(PointCloudRegistration)
endif()
