#-------------------------------------------------
#
# Project created by QtCreator 2011-09-21T11:33:35
#
#-------------------------------------------------
#include($$_PRO_FILE_PWD_/WASABIEngine/WASABIEngine.pri)
#include($$_PRO_FILE_PWD_/WASABI-qwt-clone/qwt-code/qwt/qwtconfig.pri)
#QMAKEFEATURES += $$_PRO_FILE_PWD_/WASABI-qwt-clone/qwt-code/qwt
#QMAKEFEATURES += $$_PRO_FILE_PWD_/WASABIEngine
QT       += core gui opengl network xml widgets
CONFIG += c++11

TARGET = WASABIGuiQt
TEMPLATE = app

DEPENDPATH += . $$_PRO_FILE_PWD_/WASABIEngine
INCLUDEPATH +=  $$_PRO_FILE_PWD_/WASABIEngine
CONFIG(release, debug|release) {
    win32-g++:LIBS += -L"$$_PRO_FILE_PWD_/build-WASABIEngine/release" -lWASABIEngine -lopengl32 -lglu32

    # On Mac, the debug and release folders are not automatically created
    macx:LIBS += -L"$$_PRO_FILE_PWD_/build-WASABIEngine" -lWASABIEngine -lopengl32 -lglu32
}
CONFIG(debug, debug|release) {
    win32-g++:LIBS += -L"$$_PRO_FILE_PWD_/build-WASABIEngine/debug" -lWASABIEngine -lopengl32 -lglu32

    # On Mac, the debug and release folders are not automatically created
    macx:LIBS += -L"$$_PRO_FILE_PWD_/build-WASABIEngine" -lWASABIEngine -lopengl32 -lglu32
}
# win32-g++:LIBS += -L"$$_PRO_FILE_PWD_/WASABI-qwt-clone/qwt-code/qwt/lib" -lqwtmathml
#win32-g++:LIBS += -lWASABIEngine
# win32-g++:LIBS += -L"WASABI-qwt-clone/qwt-code/qwt"
# win32-g++:LIBS += -lqwt
# unix:!symbian|win32: LIBS += -L$$PWD/WASABIEngine/ -lWASABIEngine


SOURCES += main.cpp\
        wasabiqtwindow.cpp \
    padwindow.cpp \
    glPADWidget.cpp \
#    wasabiqwtplotter.cpp

HEADERS  += \
    padwindow.h \
    glPADWidget.h \
    wasabiqtwindow.h #\
#    wasabiqwtplotter.h

FORMS    += wasabiqtwindow.ui

OTHER_FILES += \
    WASABI.ini \
    relief.se \
    init.emo_pad \
    init.emo_dyn \
    hope.se \
    fears-confirmed.se \
    Becker-Asano.emo_pad \
    Becker-Asano.emo_dyn

DISTFILES += \
    README.md

