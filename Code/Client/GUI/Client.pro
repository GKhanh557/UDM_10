QT += core gui widgets network
CONFIG += c++17

TARGET = UDM10_Client
TEMPLATE = app

SOURCES += main.cpp MainWindow.cpp
HEADERS += MainWindow.h ../Shared/ProtocolCommon.h
INCLUDEPATH += ../Shared
