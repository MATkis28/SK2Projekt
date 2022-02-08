QT       += core gui widgets network

QMAKE_CXXFLAGS += -Wall

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    INIReader.cpp \
    client.cpp \
    connect.cpp \
    gamedraw.cpp \
    gamelogic.cpp \
    gamenetworkingclient.cpp \
    gamenetworkingserver.cpp \
    gamewindow.cpp \
    global.cpp \
    host.cpp \
    ini.cpp \
    main.cpp \
    mainwindow.cpp \
    managedconnection.cpp \
    player.cpp \
    server.cpp

HEADERS += \
    INIReader.h \
    client.h \
    connect.h \
    gamedata.h \
    gamelogic.h \
    gamewindow.h \
    global.h \
    host.h \
    ini.h \
    mainwindow.h \
    managedconnection.h \
    player.h \
    server.h

FORMS += \
    connect.ui \
    host.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
