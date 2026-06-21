cmake_minimum_required(VERSION 4.3.3)
project(PixelGame C)

set(C_MAKE_STANDARD 11)
set(C_MAKE_STANDARD_REQUIRED ON)

#Gömülü SDL kaynağını bizim derlememizin parçası olarak inşa et.
#EXCLUDE_FROM_ALL: SDL'in kendi testlerini/örneklerini boşuna derleme
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

add_executable(game
    src/main.c
    src/tilemap.c
) # 'game' adında çalıştırılabilir, kaynağı src/main.c
target_link_libraries(game PRIVATE SDL3::SDL3) # SDL3' linkle

target_compile_options(game PRIVATE -Wall -Wextra)