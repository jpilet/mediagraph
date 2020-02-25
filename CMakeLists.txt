project(mediagraph)
cmake_minimum_required(VERSION 3.14.0)

find_package(Threads REQUIRED)
find_package(GTest)

if (GTEST_FOUND)
  enable_testing()
endif(GTEST_FOUND)

function(cxx_test name sources)
  if (GTEST_FOUND)
    add_executable(${name} ${sources})
    target_link_libraries(${name} ${ARGN} GTest::GTest GTest::Main)
    # Working directory: where the dlls are installed.
    add_test(NAME ${name} COMMAND $<TARGET_FILE:${name}> )
  endif(GTEST_FOUND)
endfunction()

if (CMAKE_USE_WIN32_THREADS_INIT)
    set(PLATFORM_SUFFIX "windows")
elseif (CMAKE_USE_PTHREADS_INIT)
    set(PLATFORM_SUFFIX "pthread")
else (CMAKE_USE_WIN32_THREADS_INIT)
    message(FATAL_ERROR "Can't find a pthread or windows threads.")
endif (CMAKE_USE_WIN32_THREADS_INIT)

add_library(thread_primitives
        thread_primitives.h
        thread_primitives_${PLATFORM_SUFFIX}.cpp
        )
    set_property(TARGET thread_primitives PROPERTY FOLDER "base")

cxx_test(thread_primitives_test thread_primitives_test.cpp
         thread_primitives timestamp)
    set_property(TARGET thread_primitives_test PROPERTY FOLDER "base")

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

cxx_test(timestamp_test timestamp_test.cpp timestamp)
    set_property(TARGET timestamp_test PROPERTY FOLDER "base")

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
                         )
    set_property(TARGET mediaGraph PROPERTY FOLDER "mediaGraph")

cxx_test(graph_test graph_test.cpp mediaGraph thread_primitives)
    set_property(TARGET graph_test PROPERTY FOLDER "mediaGraph")

cxx_test(property_test property_test.cpp mediaGraph mediaGraphTypes)
    set_property(TARGET property_test PROPERTY FOLDER "mediaGraph")

add_library(GraphVisitor
            GraphVisitor.cpp
            GraphVisitor.h
            )
    target_link_libraries(GraphVisitor
                          mediaGraph
                         )
    set_property(TARGET GraphVisitor PROPERTY FOLDER "mediaGraph")

cxx_test(GraphVisitor_test GraphVisitor_test.cpp mediaGraph GraphVisitor)
    set_property(TARGET GraphVisitor_test PROPERTY FOLDER "mediaGraph")

	
	
#add_subdirectory(graphHttpServer)
add_subdirectory(types)
