QT += core gui widgets network
CONFIG += c++17

TARGET = UDM10_Server
TEMPLATE = app

SOURCES += main.cpp ServerWindow.cpp
HEADERS += ServerWindow.h ../Shared/ProtocolCommon.h
INCLUDEPATH += ../Shared
