cmake_minimum_required(VERSION 4.3.3)
project(PixelGame C)

set(CMAKE_C_STANDARD 11)          # DİKKAT: değişken adı C_MAKE_* değil CMAKE_C_*
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SDLTTF_VENDORED ON CACHE BOOL "" FORCE)

#Gömülü SDL kaynağını bizim derlememizin parçası olarak inşa et.
#EXCLUDE_FROM_ALL: SDL'in kendi testlerini/örneklerini boşuna derleme
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)

# SDL_ttf — font render (SDL3 ile aynı desen: kaynaktan, vendored)
add_subdirectory(vendor/SDL_ttf EXCLUDE_FROM_ALL)


add_executable(game
    src/main.c
    src/skillfx.c
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
    src/journal.c
    src/ecology.c
    src/tooltip.c
    src/ambience.c
    src/title.c
    src/ally.c
    src/worldevent.c
    src/weather.c
    src/gametime.c
    src/decor.c
    src/audio.c
    src/music.c
    src/minimap.c
    src/ui_theme.c
    src/skilltree.c
    src/light.c
    src/lamp.c
    src/shadow.c
    src/decal.c
    src/jsonx.c
    src/uilayout.c
    src/uieditor.c
    vendor/cjson/cJSON.c
    src/crime.c
    src/guard.c
    src/sneak.c
    src/resource.c
    src/craft.c
    src/contract.c
    src/worldmap.c
    src/lockpick.c
    src/trap.c
    src/scene.c
    src/assassin.c
    src/r2d.c
    src/stb_vorbis.c
) # 'game' adında çalıştırılabilir, kaynağı src/main.c

# Vendored decoder: kendi uyarıları bizi ilgilendirmez (-Wall/-Wextra dışı)
set_source_files_properties(src/stb_vorbis.c PROPERTIES COMPILE_OPTIONS "-w")
# cJSON (MIT, vendor/cjson) — veri formatı ayrıştırıcısı. Aynı gerekçe: vendored
# kodun uyarıları bizim değil. src/jsonx.c bunun üstüne ince sarmalayıcı.
set_source_files_properties(vendor/cjson/cJSON.c PROPERTIES COMPILE_OPTIONS "-w")
target_include_directories(game PRIVATE vendor/cjson)
target_link_libraries(game PRIVATE SDL3::SDL3) # SDL3' linkle
target_link_libraries(game PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf)   # ← SDL3_ttf

target_compile_options(game PRIVATE -Wall -Wextra)

# ASan (opt-in): bellek bozulmasını yazıldığı ANDA yakalar — yalnız game hedefi,
# vendored SDL/SDL_ttf enstrümante edilmez. Kullanım:
#   cmake -B build-asan -DASAN=ON && cmake --build build-asan
option(ASAN "AddressSanitizer (yalniz game hedefi)" OFF)
if(ASAN)
    target_compile_options(game PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(game PRIVATE -fsanitize=address)
endif()

# ---------------------------------------------------------------------------
# SHADER DERLEME (offline) — HLSL kaynak → SPIR-V + MSL (+ DXIL varsa)
#
# Üretilen dosyalar assets/shaders/ altında REPODA duruyor, yani shader araç
# zinciri kurulu olmayan bir makinede de oyun derlenip çalışır. Araçlar varsa
# .hlsl dosyaları değiştiğinde çıktı otomatik yenilenir.
#
# Build makinesi gereksinimi (yalnız shader'a dokunulduğunda):
#   macOS/Linux : brew|apt install glslang spirv-cross      → SPIR-V + MSL
#   Windows     : + DXC (dxc.exe YANINDA dxil.dll şart)     → ayrıca DXIL
# Ayrıntı ve gerekçe: shaders/compile.sh başlığı.
# ---------------------------------------------------------------------------
find_program(GLSLANG_EXE glslangValidator)
find_program(SPIRV_CROSS_EXE spirv-cross)

file(GLOB SHADER_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.hlsl)

if(GLSLANG_EXE AND SPIRV_CROSS_EXE)
    # sprite.vert.spv'yi damga dosyası olarak kullan: script hepsini birlikte üretir
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/spirv/sprite.vert.spv
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/shaders/compile.sh
        DEPENDS ${SHADER_SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/shaders/compile.sh
        COMMENT "shader: HLSL -> SPIR-V + MSL (+DXIL)"
        VERBATIM)
    add_custom_target(shaders DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/spirv/sprite.vert.spv)
    add_dependencies(game shaders)
else()
    # Araç yok: repodaki hazır çıktılarla devam. Elle yenilemek için:
    #   sh shaders/compile.sh
    add_custom_target(shaders
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/shaders/compile.sh
        COMMENT "shader: HLSL -> SPIR-V + MSL (+DXIL)"
        VERBATIM)
    message(STATUS "shader araclari yok (glslangValidator/spirv-cross) — "
                   "assets/shaders/ altindaki hazir ciktilar kullanilacak")
endif()

# r2d duman testi — SDL_GPU yolunu oyundan bagimsiz dogrular (GPU gecisi araci)
add_executable(r2d_test src/r2d_test.c src/r2d.c)
target_link_libraries(r2d_test PRIVATE SDL3::SDL3)
target_compile_options(r2d_test PRIVATE -Wall -Wextra)