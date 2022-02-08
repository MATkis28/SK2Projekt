cmake_minimum_required(VERSION 3.5)

project(AchtungDieKurve VERSION 2.71828 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Network REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Network REQUIRED)

set(PROJECT_SOURCES
	client.h
	client.cpp
	connect.h
	connect.cpp
	connect.ui
	gamedata.h
	gamedraw.cpp
	gamelogic.h
	gamelogic.cpp
	gamenetworkingclient.cpp
	gamenetworkingserver.cpp
	gamewindow.h
	gamewindow.cpp
	global.h
	global.cpp
	host.h
	host.cpp
	host.ui
	ini.h
	ini.cpp
	INIReader.h
	INIReader.cpp
        main.cpp
	mainwindow.h
        mainwindow.cpp
        mainwindow.ui
	managedconnection.h
	managedconnection.cpp
	messages.h
	player.h
	player.cpp
	server.h
	server.cpp	
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AchtungDieKurve
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET AchtungDieKurve APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(AchtungDieKurve SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(AchtungDieKurve
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(AchtungDieKurve PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Network)

set_target_properties(AchtungDieKurve PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER wha.t.ever
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AchtungDieKurve)
endif()
