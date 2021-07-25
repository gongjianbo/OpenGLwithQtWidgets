QT       += core gui widgets

CONFIG += c++11
CONFIG += utf8_source

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

include($$PWD/TheBasic/TheBasic.pri)
INCLUDEPATH += $$PWD/TheBasic
