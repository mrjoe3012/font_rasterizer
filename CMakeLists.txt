
cmake_minimum_required(VERSION 3.16)

project(FontRasterizer
    VERSION 0.0.1
    DESCRIPTION "Font Rasterizer"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DEBUG "Enable glyph visualisation" ON)

add_executable(${PROJECT_NAME}
    src/main.cpp
)

if(DEBUG)
    find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs)

    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=1)
    target_include_directories(${PROJECT_NAME} PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG=0)
endif()

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/thirdparty
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()
