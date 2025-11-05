#-------------------------------------------------
#
# Project created by QtCreator 2018-11-30T23:09:31-LMH
#
#-------------------------------------------------

QT       += core gui
QT       += opengl openglwidgets
LIBS += -lopengl32
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ShowLasFile
TEMPLATE = app

# 定义支持C++11
CONFIG += c++11

DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += \
        main.cpp \
        showwindow.cpp \
        widget.cpp \
        lasreader.cpp

HEADERS += \
        showwindow.h \
        widget.h \
        lasreader.h

FORMS += \
        showwindow.ui

INCLUDEPATH += /usr/local/include
INCLUDEPATH += /usr/include/pcl-1.7
INCLUDEPATH += /usr/include/eigen3
INCLUDEPATH += /usr/include/boost
DISTFILES +=
