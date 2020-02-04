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

include($$PWD/Image/Image.pri)
include($$PWD/UnitA/UnitA.pri)
include($$PWD/UnitB/UnitB.pri)
INCLUDEPATH += $$PWD/UnitA
INCLUDEPATH += $$PWD/UnitB
