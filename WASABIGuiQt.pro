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

win32:CONFIG(release, debug|release):LIBS += -L$$PWD/../../WASABIEngine-build-desktop/release/ $$PWD/../../WASABIEngine-build-desktop/release/libWASABIEngine.a
else:win32:CONFIG(debug, debug|release):LIBS += -L$$PWD/../../WASABIEngine-build-desktop/debug/ $$PWD/../../WASABIEngine-build-desktop/debug/libWASABIEngine.a

INCLUDEPATH += $$PWD/../WASABIEngine
DEPENDPATH += $$PWD/../WASABIEngine-build-desktop/debug

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






















