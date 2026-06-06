APP_NAME = BBCord

CONFIG += qt warn_on cascades10
LIBS += -lbb
LIBS += -lbbdata
LIBS += -lbbmultimedia
LIBS += -lbbsystem
QT += network

include(config.pri)