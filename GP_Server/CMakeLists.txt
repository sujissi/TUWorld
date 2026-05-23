cmake_minimum_required(VERSION 3.20)
project(GP_Server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE ALL_SOURCES
    "${CMAKE_SOURCE_DIR}/Source/*.cpp"
)

add_executable(GP_Server ${ALL_SOURCES})

target_include_directories(GP_Server PRIVATE
    ${CMAKE_SOURCE_DIR}/Includes
    ${CMAKE_SOURCE_DIR}/Includes/mysqlx/include
    ${CMAKE_SOURCE_DIR}/Source
    ${CMAKE_SOURCE_DIR}/Source/Common
    ${CMAKE_SOURCE_DIR}/Source/Core
    ${CMAKE_SOURCE_DIR}/Source/Core/IOCP
    ${CMAKE_SOURCE_DIR}/Source/Game
    ${CMAKE_SOURCE_DIR}/Source/Game/Character
    ${CMAKE_SOURCE_DIR}/Source/Game/Map
    ${CMAKE_SOURCE_DIR}/Source/Game/Item
    ${CMAKE_SOURCE_DIR}/Source/Game/DataTable
    ${CMAKE_SOURCE_DIR}/Source/DB
    ${CMAKE_SOURCE_DIR}/Source/Utils
)

target_precompile_headers(GP_Server PRIVATE Source/Utils/pch.h)

target_link_directories(GP_Server PRIVATE
    ${CMAKE_SOURCE_DIR}/Includes/mysqlx/lib64/vs14
)

target_link_libraries(GP_Server PRIVATE
    mysqlcppconnx
    WS2_32
    MSWSock
)

if(MSVC)
    target_compile_options(GP_Server PRIVATE /W3 /utf-8)
endif()

target_compile_definitions(GP_Server PRIVATE
    WIN32
    _CONSOLE
    NOMINMAX
    SERVER_BUILD
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Debug>:DB_LOCAL>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

# MySQL DLL들을 빌드 출력 디렉토리로 복사
set(MYSQL_LIB_DIR ${CMAKE_SOURCE_DIR}/Includes/mysqlx/lib64)
add_custom_command(TARGET GP_Server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${MYSQL_LIB_DIR}/libcrypto-3-x64.dll
        ${MYSQL_LIB_DIR}/libssl-3-x64.dll
        ${MYSQL_LIB_DIR}/mysqlcppconnx-2-vs14.dll
        $<TARGET_FILE_DIR:GP_Server>
)

# config.json을 빌드 출력 디렉토리로 복사
add_custom_command(TARGET GP_Server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config.json
        $<TARGET_FILE_DIR:GP_Server>
)

# Data 폴더(DataTable, MapJsonData)를 빌드 출력 디렉토리로 복사
add_custom_command(TARGET GP_Server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/Data
        $<TARGET_FILE_DIR:GP_Server>/Data
)
