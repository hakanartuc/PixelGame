cmake_minimum_required(VERSION 4.3.3)
project(PixelGame C)

set(C_MAKE_STANDARD 11)
set(C_MAKE_STANDARD_REQUIRED ON)

set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)

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
    src/equipment.c
    src/item_textures.c
    src/menu.c
    src/fx.c
    src/skill.c
    src/combat.c
    src/save.c
    src/ability.c
    src/hotbar.c
    src/region.c
    src/npc.c
    src/dialog.c
    src/shop.c
    src/quest.c
    src/gametime.c
    src/decor.c
    src/audio.c
    src/music.c
    src/minimap.c
    src/ui_theme.c
    src/skilltree.c
    src/light.c
    src/crime.c
    src/guard.c
    src/sneak.c
    src/resource.c
    src/craft.c
    src/contract.c
    src/worldmap.c
    src/lockpick.c
    src/trap.c
    src/r2d.c
) # 'game' adında çalıştırılabilir, kaynağı src/main.c
target_link_libraries(game PRIVATE SDL3::SDL3) # SDL3' linkle
target_link_libraries(game PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)   # ← SDL3_ttf

target_compile_options(game PRIVATE -Wall -Wextra)

# r2d duman testi — SDL_GPU yolunu oyundan bagimsiz dogrular (GPU gecisi araci)
add_executable(r2d_test src/r2d_test.c src/r2d.c)
target_link_libraries(r2d_test PRIVATE SDL3::SDL3)
target_compile_options(r2d_test PRIVATE -Wall -Wextra)