add_library(SplitString SplitString.h SplitString.cpp)


if (NOT CIVETWEB_LIB)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/civetweb/CMakeLists.txt")
    set(CIVETWEB_ENABLE_WEBSOCKETS  ON)
    set(CIVETWEB_ENABLE_ASAN OFF)
    set(CIVETWEB_ENABLE_CXX ON)
    set(CIVETWEB_BUILD_TESTING OFF)
    add_subdirectory(civetweb)
    set(CIVETWEB_LIB "civetweb-cpp")
  endif()
endif()

add_library(HttpServer
            http_server.cpp
            http_server.h
           )
target_link_libraries(HttpServer
                      SplitString
                      ${CIVETWEB_LIB})
if (CIVETWEB_INCLUDE)
  target_include_directories(HttpServer PRIVATE ${CIVETWEB_INCLUDE})
endif()

add_executable(HttpServerTest
               http_server_test.cpp
              )
target_link_libraries(HttpServerTest
                      HttpServer
                      timestamp)


add_library(GraphHttpServer
            GraphHttpServer.cpp
            GraphHttpServer.h
           )
target_link_libraries(GraphHttpServer
                      HttpServer
                      mediaGraph
                      )

add_executable(GraphHttpServerTest
               GraphHttpServer_test.cpp
              )
target_link_libraries(GraphHttpServerTest
                      GraphHttpServer
                      timestamp)

