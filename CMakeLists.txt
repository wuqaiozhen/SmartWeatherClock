cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME SmartWeatherClock)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Hardware/Led.cpp
    ${CMAKE_SOURCE_DIR}/Hardware/Key.cpp
    ${CMAKE_SOURCE_DIR}/Hardware/uart.c
    ${CMAKE_SOURCE_DIR}/Hardware/esp32.c
    ${CMAKE_SOURCE_DIR}/Hardware/aht20.c
    ${CMAKE_SOURCE_DIR}/Hardware/bmp280.c
     ${CMAKE_SOURCE_DIR}/Hardware/st7789.c
     ${CMAKE_SOURCE_DIR}/Hardware/clock.c
      ${CMAKE_SOURCE_DIR}/GUI/font.c
     ${CMAKE_SOURCE_DIR}/GUI/gui.c
     ${CMAKE_SOURCE_DIR}/GUI/font_big.c 
     ${CMAKE_SOURCE_DIR}/GUI/font_cn.c
     ${CMAKE_SOURCE_DIR}/GUI/font_cn24.c
     ${CMAKE_SOURCE_DIR}/GUI/font_mid.c
     ${CMAKE_SOURCE_DIR}/GUI/ui.c
     ${CMAKE_SOURCE_DIR}/GUI/icon_wifi.c
     ${CMAKE_SOURCE_DIR}/GUI/icon_weather.c
     ${CMAKE_SOURCE_DIR}/GUI/icon_degc.c
     
      
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/Hardware
     ${CMAKE_SOURCE_DIR}/GUI
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
 target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -u _printf_float)
