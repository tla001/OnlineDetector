#-------------------------------------------------
#
# Project created by QtCreator 2017-06-19T21:28:38
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ObjectDetector
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += /usr/local/opencv-2.4.11/include
INCLUDEPATH += /usr/local/opencv-2.4.11/include/opencv
INCLUDEPATH += /usr/local/opencv-2.4.11/include/opencv2
INCLUDEPATH += /usr/local/mediawork/ffmpeg/include
INCLUDEPATH += /usr/local/libcurl/include

LIBS +=-L /usr/local/opencv-2.4.11/lib \
    -lopencv_highgui  -lopencv_core -lopencv_imgproc \
    -L/usr/local/mediawork/ffmpeg/lib \
    -lavcodec -lavfilter -lpostproc -lavformat -lswresample -lavdevice -lavutil -lswscale -lm \
    -L/usr/local/libcurl/lib -lcurl

SOURCES += main.cpp\
        mainwindow.cpp \
    streamdealthread.cpp \
    StreamDeal.cpp \
    imagedeal.cpp \
    imagedealthread.cpp

HEADERS  += mainwindow.h \
    Include.h\
    streamdealthread.h \
    StreamDeal.h \
    imagedeal.h \
    imagedealthread.h

FORMS    += mainwindow.ui

RESOURCES += \
    resource.qrc
