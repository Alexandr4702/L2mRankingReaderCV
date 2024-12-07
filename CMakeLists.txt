# for lib in $(ldd  ./ExpertiseRoller.exe | awk '{print $3}' | grep -v '^$' | grep mingw); do cp "$lib" ./; done
cmake_minimum_required(VERSION 3.5)

project(RankingReader LANGUAGES CXX)
project(ExpertiseRoller LANGUAGES CXX)
project(GetImage LANGUAGES CXX)
project(LifeStoneRoller LANGUAGES CXX)
project(MemAnalysisTest LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package( OpenCV REQUIRED )
# find_package( Tesseract REQUIRED )

include_directories(./eigen ${OpenCV_INCLUDE_DIRS} ${Tesseract_INCLUDE_DIRS} Tools/ ./Memory-Hacking-Class)

add_library(
    TestObject
    OBJECT 
    Tools/Tools.cpp
    Memory-Hacking-Class/Memory.cpp
)

add_executable(RankingReader
    RankinReader/main.cpp
)

add_executable(ExpertiseRoller
    ExpertiseRoller/main.cpp
)

add_executable(GetImage
    GetImage/main.cpp
)

add_executable(LifeStoneRoller
    LifeStoneRoller/main.cpp
)

add_executable(MemAnalysisTest
    MemReadTest/main.cpp
)

set(LIBS 
    ${OpenCV_LIBS}
    ${Tesseract_LIBRARIES}
    libtesseract
    TestObject
    -lgdi32
)

target_link_libraries(RankingReader
    ${LIBS}
)

target_link_libraries(ExpertiseRoller
    ${LIBS}
)

target_link_libraries(GetImage
    ${LIBS}
)

target_link_libraries(LifeStoneRoller
    ${LIBS}
)

target_link_libraries(MemAnalysisTest
    ${LIBS}
)

configure_file(./settings_ExpertiseRoller.json ./settings_ExpertiseRoller.json COPYONLY)
configure_file(./diamond.jpg ./diamond.jpg COPYONLY)