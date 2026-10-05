cmake_minimum_required(VERSION 3.16)
project(PrecisionClockLinux C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
pkg_check_modules(APPINDICATOR appindicator3-0.1)

add_executable(precision-clock main.c font_data.c icon_data.c)

target_include_directories(precision-clock PRIVATE
    ${GTK3_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
)

target_link_libraries(precision-clock PRIVATE
    ${GTK3_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    pthread m
)

target_compile_options(precision-clock PRIVATE
    ${GTK3_CFLAGS_OTHER}
    ${FONTCONFIG_CFLAGS_OTHER}
    -Wall -O2
)

if (APPINDICATOR_FOUND)
    message(STATUS "libappindicator found - tray enabled")
    target_compile_definitions(precision-clock PRIVATE HAVE_APPINDICATOR)
    target_include_directories(precision-clock PRIVATE ${APPINDICATOR_INCLUDE_DIRS})
    target_link_libraries(precision-clock PRIVATE ${APPINDICATOR_LIBRARIES})
    target_compile_options(precision-clock PRIVATE ${APPINDICATOR_CFLAGS_OTHER})
else()
    message(STATUS "libappindicator NOT found - tray disabled")
endif()

install(TARGETS precision-clock RUNTIME DESTINATION bin)