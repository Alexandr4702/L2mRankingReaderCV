cmake_minimum_required(VERSION 3.20)
project(L2mRankingReaderCV VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(OpenCV REQUIRED)
find_path(TESSERACT_INCLUDE_DIR tesseract/baseapi.h)
find_library(TESSERACT_LIBRARY NAMES tesseract libtesseract)
if(NOT TESSERACT_INCLUDE_DIR OR NOT TESSERACT_LIBRARY)
    message(FATAL_ERROR "Tesseract development files were not found")
endif()

add_library(l2m_common STATIC Tools/Tools.cpp)
target_include_directories(l2m_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Tools
    ${CMAKE_CURRENT_SOURCE_DIR}/Memory-Hacking-Class
    ${OpenCV_INCLUDE_DIRS}
    ${TESSERACT_INCLUDE_DIR}
)
target_link_libraries(l2m_common PUBLIC ${OpenCV_LIBS} ${TESSERACT_LIBRARY} gdi32 psapi)

function(add_l2m_tool target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE l2m_common)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND}
            -DEXECUTABLE=$<TARGET_FILE:${target}>
            -DDESTINATION=$<TARGET_FILE_DIR:${target}>
            -DCOMPILER_DIRECTORY=${CMAKE_CXX_COMPILER}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
endfunction()

add_l2m_tool(RankingReader RankinReader/main.cpp)
add_l2m_tool(ExpertiseRoller ExpertiseRoller/main.cpp)
add_l2m_tool(GetImage GetImage/main.cpp)
add_l2m_tool(LifeStoneRoller LifeStoneRoller/main.cpp)

add_executable(MemAnalysisTest
    MemReadTest/main.cpp
    Memory-Hacking-Class/Memory.cpp
)
target_link_libraries(MemAnalysisTest PRIVATE l2m_common)
add_custom_command(TARGET MemAnalysisTest POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DEXECUTABLE=$<TARGET_FILE:MemAnalysisTest>
        -DDESTINATION=$<TARGET_FILE_DIR:MemAnalysisTest>
        -DCOMPILER_DIRECTORY=${CMAKE_CXX_COMPILER}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyRuntimeDependencies.cmake
    VERBATIM
)

foreach(asset IN ITEMS settings_ExpertiseRoller.json diamond.jpg)
    configure_file(${asset} bin/${asset} COPYONLY)
endforeach()

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/settings_lifeStoneRoller.json)
    configure_file(settings_lifeStoneRoller.json bin/settings_lifeStoneRoller.json COPYONLY)
endif()
