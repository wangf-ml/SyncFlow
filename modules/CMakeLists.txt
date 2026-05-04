add_library(syncflow_modules STATIC
    virtual_camera.cpp
    display.cpp
)
target_include_directories(syncflow_modules PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(syncflow_modules syncflow_core)