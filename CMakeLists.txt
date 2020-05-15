project(mediagraph)
cmake_minimum_required(VERSION 3.14.0)

option(ENABLE_EASY_PROFILER "Enable profiling with easy_profiler" OFF)
if (NOT EASY_PROFILER_LIB)  # EASY_PROFILER_LIB is set if using conan
  find_package(easy_profiler REQUIRED)
  set(EASY_PROFILER_LIB "easy_profiler")
endif()

set (CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)
find_package(GTest)

if (GTEST_FOUND)
  enable_testing()
endif(GTEST_FOUND)

function(cxx_test name folder sources)
  if (GTEST_FOUND)
    add_executable(${name} ${sources})
    target_link_libraries(${name} ${ARGN} GTest::GTest GTest::Main)
    # Working directory: where the dlls are installed.
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> )
    set_property(TARGET ${name} PROPERTY FOLDER ${folder})
  endif(GTEST_FOUND)
endfunction()

add_library(thread_primitives
        thread_primitives.h
        thread_primitives.cpp
        )
    set_property(TARGET thread_primitives PROPERTY FOLDER "base")

cxx_test(thread_primitives_test "base" thread_primitives_test.cpp
         thread_primitives timestamp)

if (WIN32)
    set(TIMESTAMP_PLATFORM "win32")
else (WIN32)
    set(TIMESTAMP_PLATFORM "posix")
endif (WIN32)

add_library(timestamp
            timestamp-${TIMESTAMP_PLATFORM}.cpp
            timestamp.h
           )
    set_property(TARGET timestamp PROPERTY FOLDER "base")

cxx_test(timestamp_test "base" timestamp_test.cpp timestamp)

add_library(mediaGraph
            graph.cpp
            graph.h
            node.cpp
            node.h
            property.cpp
            property.h
            stream.cpp
            stream.h
            stream_reader.h
            )
    target_link_libraries(mediaGraph
                          mediaGraphTypes
                          timestamp
                          thread_primitives
                         )
    if (ENABLE_EASY_PROFILER)
      target_link_libraries(mediaGraph ${EASY_PROFILER_LIB})
      target_compile_definitions(mediaGraph PUBLIC MEDIAGRAPH_USE_EASY_PROFILER)
    endif(ENABLE_EASY_PROFILER)

    set_property(TARGET mediaGraph PROPERTY FOLDER "mediaGraph")

cxx_test(graph_test "mediaGraph" graph_test.cpp mediaGraph thread_primitives)
cxx_test(property_test "mediaGraph" property_test.cpp mediaGraph mediaGraphTypes)

add_library(GraphVisitor
            GraphVisitor.cpp
            GraphVisitor.h
            )
    target_link_libraries(GraphVisitor
                          mediaGraph
                         )
    set_property(TARGET GraphVisitor PROPERTY FOLDER "mediaGraph")

cxx_test(GraphVisitor_test "mediaGraph" GraphVisitor_test.cpp mediaGraph GraphVisitor)

	
	
add_subdirectory(graphHttpServer)
add_subdirectory(types)
