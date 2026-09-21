QT += core gui widgets network
CONFIG += c++17

TARGET = UDM10_Client
TEMPLATE = app

SOURCES += main.cpp ClientWindow.cpp
HEADERS += ClientWindow.h ../Shared/ProtocolCommon.h
INCLUDEPATH += ../Shared
