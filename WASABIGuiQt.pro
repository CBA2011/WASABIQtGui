#-------------------------------------------------
#
# Project created by QtCreator 2011-09-21T11:33:35
#
#-------------------------------------------------

QT       += core gui opengl network

TARGET = WASABIGuiQt
TEMPLATE = app
DEPENDPATH += . ../WASABIEngine
INCLUDEPATH +=  ../WASABIEngine
win32-g++:LIBS += -L"../WASABIEngine/debug"
win32-g++:LIBS += -lWASABIEngine
unix:!symbian|win32: LIBS += -L$$PWD/../WASABIEngine/ -lWASABIEngine

SOURCES += main.cpp\
        wasabiqtwindow.cpp \
    padwindow.cpp \
    glPADWidget.cpp \
    wasabiqwtplotter.cpp

HEADERS  += \
    padwindow.h \
    glPADWidget.h \
    wasabiqtwindow.h \
    wasabiqwtplotter.h

FORMS    += wasabiqtwindow.ui

OTHER_FILES += \
    WASABI.ini \
    relief.se \
    init.emo_pad \
    init.emo_dyn \
    hope.se \
    fears-confirmed.se \
    Becker-Asano.emo_pad \
    Becker-Asano.emo_dyn \
    README.txt


