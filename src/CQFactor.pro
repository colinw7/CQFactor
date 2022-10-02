TEMPLATE = app

QT += widgets

TARGET = CQFactor

DEPENDPATH += .

QMAKE_CXXFLAGS += -std=c++17

#CONFIG += debug

# Input
SOURCES += \
CQFactor.cpp \
CPrime.cpp \

HEADERS += \
CQFactor.h \
CPrime.h \

DESTDIR     = ../bin
OBJECTS_DIR = ../obj
LIB_DIR     = ../lib

INCLUDEPATH += \
../include \
.

unix:LIBS += \
-L$$LIB_DIR \
