cmake_minimum_required(VERSION 3.10)
project(lab2_var16 C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Строгая сборка: предупреждения как ошибки.
if(MSVC)
    add_compile_options(/W4 /WX)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Werror -pedantic)
endif()

add_library(win_utils STATIC src/win_utils.c)
target_include_directories(win_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/include)

# Родительский процесс
add_executable(parent src/parent.c)
target_link_libraries(parent PRIVATE win_utils)

# Дочерний процесс
add_executable(child src/child.c)
target_link_libraries(child PRIVATE win_utils)

# Оба исполняемых файла кладём в один каталог: родитель ищет child.exe
# рядом со своим образом.
set_target_properties(parent child PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
