cmake_minimum_required(VERSION 3.10)

project(garuda-downloader LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check https://doc.qt.io/qt/deployment-android.html for more information.
# They need to be set before the find_package( ...) calls below.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        garudadownloader.cpp
        garudadownloader.h
        garudadownloader.ui
        resources.qrc
        QDarkStyleSheet/qdarkstyle/style.qrc
)

if (WIN32)
    list(APPEND PROJECT_SOURCES resources-windows.qrc)
elseif(UNIX AND NOT APPLE)
    list(APPEND PROJECT_SOURCES updaterwindow.cpp
        updaterwindow.h
        updaterwindow.ui
        resources-linux.qrc
    )

endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(garuda-downloader
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(garuda-downloader SHARED
            ${PROJECT_SOURCES}
        )
    elseif(WIN32)
        add_executable(garuda-downloader WIN32
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(garuda-downloader
            ${PROJECT_SOURCES}
        )
    endif()
endif()

if (NOT WIN32)
    add_subdirectory(zsync2 EXCLUDE_FROM_ALL)
    target_link_libraries(garuda-downloader PRIVATE Qt${QT_VERSION_MAJOR}::Widgets PRIVATE libzsync2)
else()
    target_link_libraries(garuda-downloader PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::QSvgPlugin PRIVATE -static)
endif()

install(TARGETS garuda-downloader
    RUNTIME DESTINATION bin)
