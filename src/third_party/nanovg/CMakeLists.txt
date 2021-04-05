project(nanovg)

add_library (${PROJECT_NAME} SHARED 
    nanovg.c
)

target_include_directories (${PROJECT_NAME} PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
)