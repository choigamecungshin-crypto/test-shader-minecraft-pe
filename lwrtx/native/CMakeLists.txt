cmake_minimum_required(VERSION 3.16)
project(lwrtx_shader_transform LANGUAGES C CXX)

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
  set(LWRTX_SHADOWHOOK_ROOT "" CACHE PATH "Extracted ShadowHook AAR prefab module root")
  if(NOT EXISTS "${LWRTX_SHADOWHOOK_ROOT}/include/shadowhook.h")
    message(FATAL_ERROR "ShadowHook headers not found. Set LWRTX_SHADOWHOOK_ROOT to prefab/modules/shadowhook")
  endif()
  set(LWRTX_SHADOWHOOK_SO "${LWRTX_SHADOWHOOK_ROOT}/libs/android.${ANDROID_ABI}/libshadowhook.so")
  if(NOT EXISTS "${LWRTX_SHADOWHOOK_SO}")
    message(FATAL_ERROR "ShadowHook shared library not found: ${LWRTX_SHADOWHOOK_SO}")
  endif()

  add_library(shadowhook SHARED IMPORTED)
  set_target_properties(shadowhook PROPERTIES
    IMPORTED_LOCATION "${LWRTX_SHADOWHOOK_SO}"
    INTERFACE_INCLUDE_DIRECTORIES "${LWRTX_SHADOWHOOK_ROOT}/include")

  add_library(lwrtx_gles_hook SHARED shader_source_hook.cpp)
  target_compile_features(lwrtx_gles_hook PRIVATE cxx_std_17)
  target_link_libraries(lwrtx_gles_hook PRIVATE
    lwrtx_shader_transform shadowhook GLESv2 log)
  set_target_properties(lwrtx_gles_hook PROPERTIES
    BUILD_RPATH "$ORIGIN"
    INSTALL_RPATH "$ORIGIN")
endif()
