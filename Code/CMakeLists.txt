cmake_minimum_required(VERSION 3.16)

project(WidgetQuadtree VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets WebSockets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets WebSockets)

set(PROJECT_SOURCES
    main.cpp
    app/appwindow.cpp
    app/appwindow.h
    services/trajectoryservice.cpp
    services/trajectoryservice.h
    services/quadtreerepository.cpp
    services/quadtreerepository.h
    services/incidentservice.cpp
    services/incidentservice.h
    services/settingsservice.cpp
    services/settingsservice.h
    services/statisticservice.cpp
    services/statisticservice.h
    services/reportservice.cpp
    services/reportservice.h
    ui/pages/dashboardpage.cpp
    ui/pages/dashboardpage.h
    ui/pages/queryworkbenchpage.cpp
    ui/pages/queryworkbenchpage.h
    ui/pages/visualizationpage.cpp
    ui/pages/visualizationpage.h
    ui/pages/playbackpage.cpp
    ui/pages/playbackpage.h
    ui/pages/incidentpage.cpp
    ui/pages/incidentpage.h
    ui/pages/reportpage.cpp
    ui/pages/reportpage.h
    ui/pages/datamanagerpage.cpp
    ui/pages/datamanagerpage.h
    ui/pages/settingspage.cpp
    ui/pages/settingspage.h
    ui/components/infocard.cpp
    ui/components/infocard.h
    ui/components/queryform.cpp
    ui/components/queryform.h
    common.cpp
    common.h
    common/queryparameters.h
    common/querytypes.h
    quadtree.cpp
    quadtree.h
    querymanager.cpp
    querymanager.h
    drawwidget.cpp
    drawwidget.h
)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../ElaWidgetTools/ElaWidgetTools
                 ${CMAKE_CURRENT_BINARY_DIR}/ElaWidgetToolsBuild)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(WidgetQuadtree
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        thepic.qrc
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET WidgetQuadtree APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(WidgetQuadtree SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(WidgetQuadtree
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(WidgetQuadtree PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../ElaWidgetTools/ElaWidgetTools
)

target_link_libraries(WidgetQuadtree PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::WebSockets
    ElaWidgetTools
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.WidgetQuadtree)
endif()
set_target_properties(WidgetQuadtree PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS WidgetQuadtree
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(WidgetQuadtree)
endif()
