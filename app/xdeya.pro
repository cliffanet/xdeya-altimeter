QT       += core quick quickcontrols2 bluetooth webview

#greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    BinProto/BinProto.cpp \
    BinProto/nettcpsocket.cpp \
    apphnd.cpp \
    devinfo.cpp \
    jmpinfo.cpp \
    main.cpp \
    netprocess.cpp \
    trackhnd.cpp \
    trkinfo.cpp \
    wifidevicediscovery.cpp \
    wifiinfo.cpp

HEADERS += \
    BinProto/BinProto.h \
    BinProto/netsocket.h \
    BinProto/nettcpsocket.h \
    apphnd.h \
    devinfo.h \
    jmpinfo.h \
    netprocess.h \
    nettypes.h \
    trackhnd.h \
    trkinfo.h \
    wifidevicediscovery.h \
    wifiinfo.h

ios: {
    QMAKE_INFO_PLIST = Info/Info.qmake.ios.plist
    ios_icon.files = $$files($$PWD/icons/xdeya.icns)
    QMAKE_BUNDLE_DATA += ios_icon
}
macos: QMAKE_INFO_PLIST = Info/Info.qmake.macos.plist

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    CMakeLists.txt

ICON = icons/xdeya.icns
RC_ICONS = icons/xdeya.ico
RC_FILE = xdeya.rc
RESOURCES += \
    xdeya.qrc
