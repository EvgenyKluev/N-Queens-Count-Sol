cmake_minimum_required(VERSION 2.8.12)
set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)

if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
    add_compile_options(-Wconversion -Wold-style-cast -Wcast-align -Wcast-qual)
    add_compile_options(-Wunused -Woverloaded-virtual -Wno-noexcept-type)
    #add_compile_options(-march=haswell -mtune=haswell)
endif()

project(nqueens)
add_executable(${PROJECT_NAME} "main.cpp")

enable_testing()
set(GOOGLETEST_DIR "/usr/src/googletest")
if (EXISTS ${GOOGLETEST_DIR})
    set(GTestSrc ${GOOGLETEST_DIR}/googletest)
    set(GMockSrc ${GOOGLETEST_DIR}/googlemock)
endif ()

set(GTestFiles
    ${GTestSrc}/src/gtest-all.cc
    ${GTestSrc}/src/gtest_main.cc)
set(GTestIncludes ${GTestSrc} ${GTestSrc}/include)
if (NOT ${GMockSrc} STREQUAL "")
    list(APPEND GTestFiles ${GMockSrc}/src/gmock-all.cc)
    list(APPEND GTestIncludes ${GMockSrc} ${GMockSrc}/include)
endif ()

include_directories(${GTestIncludes})
set(TestSources
    util.t.cpp
    foreach2.t.cpp
    bitcombcolex.t.cpp
    board.t.cpp
    matchtr.t.cpp
    sieve.t.cpp
    pack.t.cpp
    qsymmetry.t.cpp
    subsquare.t.cpp
    quadrants1.t.cpp)
add_executable(tests
               ${TestSources}
               ${GTestFiles})

include(GoogleTest)
gtest_discover_tests(tests)
