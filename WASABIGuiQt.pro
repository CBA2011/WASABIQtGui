#-------------------------------------------------
#
# Project created by QtCreator 2011-09-21T11:33:35
#
#-------------------------------------------------

QT       += core gui opengl network

TARGET = WASABIGuiQt
TEMPLATE = app
SOURCES += main.cpp\
        wasabiqtwindow.cpp \
    padwindow.cpp \
    glPADWidget.cpp

HEADERS  += \
    padwindow.h \
    glPADWidget.h \
    wasabiqtwindow.h

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


unix:!symbian|win32: LIBS += -L$$PWD/../WASABIEngine/ -lWASABIEngine

INCLUDEPATH += $$PWD/../WASABIEngine
DEPENDPATH += $$PWD/../WASABIEngine
