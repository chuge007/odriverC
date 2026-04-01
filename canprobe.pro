QT += core serialbus

CONFIG += console c++11
CONFIG -= app_bundle

TARGET = canprobe
TEMPLATE = app

win32-msvc*:QMAKE_CXXFLAGS += /utf-8

SOURCES += canprobe.cpp
