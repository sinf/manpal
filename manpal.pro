#-------------------------------------------------
#
# Project created by QtCreator 2018-02-08T02:04:31
#
#-------------------------------------------------

QT       += core gui
QMAKE_CXXFLAGS += -std=c++14 -O3 -g -Wno-parentheses -ffast-math -march=native -ftree-vectorize
#-fdump-tree-vect=vect.txt -fopt-info-vec-optimized-missed=vect2.txt

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = manpal
TEMPLATE = app


SOURCES += main.cpp\
        mainwin.cpp \
    palettem.cpp

HEADERS  += mainwin.h \
    palettem.h \
    vec3.h

FORMS    += mainwin.ui
