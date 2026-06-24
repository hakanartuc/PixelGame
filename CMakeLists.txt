cmake_minimum_required(VERSION 4.3.3)
project(PixelGame C)

set(C_MAKE_STANDARD 11)
set(C_MAKE_STANDARD_REQUIRED ON)

#Gömülü SDL kaynağını bizim derlememizin parçası olarak inşa et.
#EXCLUDE_FROM_ALL: SDL'in kendi testlerini/örneklerini boşuna derleme
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

# SDL_ttf — font render (SDL3 ile aynı desen: kaynaktan, vendored)
add_subdirectory(vendor/SDL_ttf EXCLUDE_FROM_ALL)


add_executable(game
    src/main.c
    src/tilemap.c
    src/player.c
    src/mob.c
    src/render.c
    src/loot.c
    src/font.c 
    src/floattext.c
    src/item.c
    src/inventory.c 
) # 'game' adında çalıştırılabilir, kaynağı src/main.c
target_link_libraries(game PRIVATE SDL3::SDL3) # SDL3' linkle
target_link_libraries(game PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)   # ← SDL3_ttf

target_compile_options(game PRIVATE -Wall -Wextra)