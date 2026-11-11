cmake_minimum_required(VERSION 3.16)
project(lwrtx_shader_transform LANGUAGES CXX)

add_library(lwrtx_shader_transform STATIC shader_transform.cpp)
target_compile_features(lwrtx_shader_transform PUBLIC cxx_std_17)
target_include_directories(lwrtx_shader_transform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(lwrtx_shader_transform PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(ANDROID)
  set(LWRTX_TESTS_DEFAULT OFF)
else()
  set(LWRTX_TESTS_DEFAULT ON)
endif()

option(LWRTX_BUILD_TESTS "Build host-side shader transform tests" ${LWRTX_TESTS_DEFAULT})

if(LWRTX_BUILD_TESTS)
  enable_testing()
  add_executable(lwrtx_shader_transform_test shader_transform_test.cpp)
  target_link_libraries(lwrtx_shader_transform_test PRIVATE lwrtx_shader_transform)
  add_test(NAME lwrtx_shader_transform_test COMMAND lwrtx_shader_transform_test)
endif()

if(ANDROID)
  add_library(lwrtx_gles_hook SHARED shader_source_hook.cpp)
  target_compile_features(lwrtx_gles_hook PRIVATE cxx_std_17)
  target_link_libraries(lwrtx_gles_hook PRIVATE lwrtx_shader_transform GLESv2 log)
endif()
