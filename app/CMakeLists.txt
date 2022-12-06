cmake_minimum_required(VERSION 3.16)
project(xdeya VERSION 1.0 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Set up AUTOMOC and some sensible defaults for runtime execution
# When using Qt 6.3, you can replace the code block below with
# qt_standard_project_setup()
set(CMAKE_AUTOMOC ON)
include(GNUInstallDirs)

if(WIN32)
  set(CMAKE_WIN32_EXECUTABLE ON)
elseif(APPLE)
  set(CMAKE_MACOSX_BUNDLE ON)
endif()
set(target ${PROJECT_NAME})

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Bluetooth Gui Quick QuickControls2 WebView)

# icon Windows
    set(app_icon_resource_windows "${CMAKE_CURRENT_SOURCE_DIR}/xdeya.rc")
# icon MacOS
    #set(MACOSX_BUNDLE_ICON_FILE xdeya.icns)
    #set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/icons/xdeya.icns")
    #set_source_files_properties(${app_icon_macos} PROPERTIES
    #    MACOSX_PACKAGE_LOCATION "Resources")
#endif()

qt_add_executable(xdeya WIN32 MACOSX_BUNDLE
    BinProto/BinProto.cpp BinProto/BinProto.h
    BinProto/netsocket.h
    BinProto/nettcpsocket.cpp BinProto/nettcpsocket.h
    apphnd.cpp apphnd.h
    devinfo.cpp devinfo.h
    jmpinfo.cpp jmpinfo.h
    main.cpp
    netprocess.cpp netprocess.h
    nettypes.h
    trackhnd.cpp trackhnd.h
    trkinfo.cpp trkinfo.h
    wifidevicediscovery.cpp wifidevicediscovery.h
    wifiinfo.cpp wifiinfo.h

    ${app_icon_resource_windows}
)

target_link_libraries(xdeya PUBLIC
    Qt::Bluetooth
    Qt::Core
    Qt::Gui
    Qt::Quick
    Qt::QuickControls2
    Qt::WebView
)


# Resources:
set_source_files_properties("res/navi.html"
    PROPERTIES QT_RESOURCE_ALIAS "navi.html"
)
set(xdeya_resource_files
    "icons/default/20x20/back.png"
    "icons/default/20x20/drawer.png"
    "icons/default/20x20/menu.png"
    "icons/default/20x20/reload.png"
    "icons/default/20x20@2/back.png"
    "icons/default/20x20@2/drawer.png"
    "icons/default/20x20@2/menu.png"
    "icons/default/20x20@2/reload.png"
    "icons/default/20x20@3/back.png"
    "icons/default/20x20@3/drawer.png"
    "icons/default/20x20@3/menu.png"
    "icons/default/20x20@3/reload.png"
    "icons/default/20x20@4/back.png"
    "icons/default/20x20@4/drawer.png"
    "icons/default/20x20@4/menu.png"
    "icons/default/20x20@4/reload.png"
    "icons/default/index.theme"
    "icons/track.png"
    "icons/warning.png"
    "icons/wifi.png"
    "page/auth.qml"
    "page/jmpinfo.qml"
    "page/jmplist.qml"
    "page/main.qml"
    "page/trklist.qml"
    "page/trkview.qml"
    "page/wifipass.qml"
    "res/navi.html"
)

qt_add_resources(xdeya "xdeya"
    PREFIX
        "/"
    FILES
        ${xdeya_resource_files}
)

# apple Info/icons
#if(IOS OR APPLE)
#    set(MACOSX_BUNDLE_ICON_FILE xdeya.icns)
#    #set_target_properties(xdeya
#    #    PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info/Info.qmake.ios.plist")
#    file(GLOB_RECURSE app_icons CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/icons/xdeya.icns")
#    if(app_icons)
#        target_sources(xdeya PRIVATE ${app_icons})
#        set_source_files_properties(
#            ${app_icons}
#            PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
#    endif()
#
#    execute_process(
#        COMMAND ${qt_root}/bin/macdeployqt
#        ${CMAKE_INSTALL_PREFIX}/xdeya.app
#        -qmldir=${CMAKE_CURRENT_SOURCE_DIR}/quick_ui
#        -qmlimport=${qt_root}/qml
#    )
#endif()

# android icon
if(ANDROID)
    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
endif()

install(TARGETS xdeya
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION lib
)

if(APPLE)
    set(CMAKE_MACOSX_BUNDLE ON)
    set(CMAKE_MACOSX_RPATH ON)
    install(TARGETS ${target} BUNDLE DESTINATION .
                              RUNTIME DESTINATION .)
    set_source_files_properties(icons/${target}.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(${target}
                          PROPERTIES
                          MACOSX_BUNDLE_BUNDLE_NAME "${PROJECT_NAME}"
                          MACOSX_BUNDLE_INFO_STRING "${PROJECT_NAME} Copyright (c) 2021 Cliff"
                          MACOSX_BUNDLE_ICON_FILE Icon.icns
                          MACOSX_BUNDLE_GUI_IDENTIFIER "${PROJECT_NAME}"
                          MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}"
                          MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
                          MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
                          MACOSX_BUNDLE_COPYRIGHT "Copyright (c) 2021 Cliff"
                          )
endif()

#set(desktop.path applications)
#set(desktop.files resources/example.desktop)
#set(icon.path icons/hicolor/64x64/apps)
#set(icon.files resources/icon64.png)
#set(iconsvg.path icons/hicolor/scalable/apps)
#set(iconsvg.files resources/icon.svg)
#foreach(items IN ITEMS desktop icon iconsvg)
#    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/${${items}.files}
#            DESTINATION share/${${items}.path}
#            PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ)
#endforeach()
# Components:
if(CMAKE_BUILD_TYPE_UPPER MATCHES "^(DEBUG|RELWITHDEBINFO)$")
    set(CPACK_STRIP_FILES FALSE)
else()
    set(CPACK_STRIP_FILES TRUE)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/Packaging.cmake)
