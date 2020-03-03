add_library(SplitString SplitString.h SplitString.cpp)

add_library(HttpServer
            http_server.cpp
            http_server.h
           )
target_link_libraries(HttpServer
                      SplitString
                      civetweb-cpp)

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

set(CIVETWEB_ENABLE_CXX ON)
set(CIVETWEB_BUILD_TESTING OFF)
add_subdirectory(civetweb)
