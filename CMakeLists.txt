cmake_minimum_required(VERSION 2.8)
project (PhotoSort)

add_subdirectory(src)

if(UNIX)
    set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Wextra -pedantic ${CMAKE_CXX_FLAGS}")
endif()

set(Srcs ${SRC_LIST})
set(Hdrs "")
set(QRCs ${RC_LIST} res/res.qrc)
set(MOC_Hdrs ${MOC_LIST})

set(CMAKE_AUTOMOC ON)
find_package(Qt5Widgets REQUIRED)
qt5_add_resources(QRC_Srcs ${QRCs} )
add_executable(${PROJECT_NAME} #MACOSX_BUNDLE
	src/main.cpp ${Srcs} ${Hdrs} ${MOC_Hdrs} ${QRC_Srcs})
qt5_use_modules(${PROJECT_NAME} Core Gui Widgets)

target_link_libraries(${PROJECT_NAME} pHash exif)
