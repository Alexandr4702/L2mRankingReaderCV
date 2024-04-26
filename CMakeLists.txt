cmake_minimum_required(VERSION 3.5)

project(RankingReader LANGUAGES CXX)
project(ExpertiseRoller LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package( OpenCV REQUIRED )
# find_package( Tesseract REQUIRED )

set(RankingReader_SOURCES
    RankinReader/main.cpp
)

set(ExpertiseRoller_SOURCES
    ExpertiseRoller/main.cpp
)

include_directories(./eigen ${OpenCV_INCLUDE_DIRS} ${Tesseract_INCLUDE_DIRS})

add_executable(RankingReader
    ${RankingReader_SOURCES}
)

add_executable(ExpertiseRoller
    ${ExpertiseRoller_SOURCES}
)

target_link_libraries(RankingReader
    ${OpenCV_LIBS}
    ${Tesseract_LIBRARIES}
    libtesseract
)

target_link_libraries(ExpertiseRoller
    ${OpenCV_LIBS}
    ${Tesseract_LIBRARIES}
    libtesseract
)
