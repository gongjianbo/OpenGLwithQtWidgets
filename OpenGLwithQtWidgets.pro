QT       += core gui widgets

CONFIG += c++11
CONFIG += utf8_source

SOURCES += \
    MyCamera.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    MyCamera.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

include($$PWD/Image/Image.pri)
include($$PWD/UnitA/UnitA.pri)
include($$PWD/UnitB/UnitB.pri)
INCLUDEPATH += $$PWD/UnitA
INCLUDEPATH += $$PWD/UnitB
