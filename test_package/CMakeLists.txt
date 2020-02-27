project(mediagraphConanTest)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

add_executable(test test.cpp)
target_link_libraries(test CONAN_PKG::mediagraph)

