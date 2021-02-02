cmake_minimum_required(VERSION 2.8.12)

project(rdp LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)
set(SOURCE_FILES
    qrdpwidget.cpp
    qrdpwork.cpp
)

set(HEADER_FILES
    qrdpwidget.h
    qrdpwork.h
    qrdpdef.h
)

set(OTHER_LIBRARIES freerdp-client winpr freerdp)
add_definitions(-DQRDP_LIBRARY)
if(WIN32)
    set(CMAKE_DEBUG_POSTFIX "d")
    set(OTHER_LIBRARIES ${OTHER_LIBRARIES} msimg32)
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})
    add_executable(q${PROJECT_NAME} WIN32 main.cpp ${SOURCE_FILES} ${HEADER_FILES})
else()
    add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})
    add_executable(q${PROJECT_NAME} main.cpp ${SOURCE_FILES} ${HEADER_FILES})
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets ${OTHER_LIBRARIES})
target_compile_definitions(q${PROJECT_NAME} PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(q${PROJECT_NAME} Qt5::Core Qt5::Gui Qt5::Widgets ${OTHER_LIBRARIES})
