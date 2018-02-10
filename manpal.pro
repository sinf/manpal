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
    vec3.h \
    dithered.h \
    imgfilter.h

FORMS    += mainwin.ui

# need to run 'make install'
#datathings.path = $$OUT_PWD
#datathings.files = img/*
#INSTALLS += datathings

copydata.commands = $(COPY_DIR) $$PWD/img $$OUT_PWD
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata

