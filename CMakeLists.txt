cmake_minimum_required(VERSION 3.1)

if(${CMAKE_VERSION} VERSION_LESS 3.12)
	cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
endif()

project(ECTool
	VERSION 1.0.0
	DESCRIPTION "ChromeOS EC Tool"
	LANGUAGES CXX)

if(NOT WIN32)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(libusb REQUIRED libusb-1.0)
	pkg_check_modules(libftdi1 REQUIRED libftdi1)
else()
endif()

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)
add_subdirectory(extern)

if(WIN32)
	add_subdirectory(getopt)
endif()
