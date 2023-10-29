cmake_minimum_required(VERSION 3.5)

project(RankingReader LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package( OpenCV REQUIRED )
# find_package( Tesseract REQUIRED )

set(PROJECT_SOURCES
        main.cpp
)

include_directories(./eigen ${OpenCV_INCLUDE_DIRS} ${Tesseract_INCLUDE_DIRS})

add_executable(RankingReader
    ${PROJECT_SOURCES}
)

target_link_libraries(RankingReader
    ${OpenCV_LIBS}
    ${Tesseract_LIBRARIES}
    libtesseract
)
