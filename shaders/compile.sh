#!/bin/sh
# shaders/compile.sh — HLSL kaynağı üç arka-uç formatına çevirir (OFFLINE).
#
# NEDEN OFFLINE: oyunun çalışma zamanında hiçbir shader derleyicisine bağımlılığı
# olmasın. Üretilen dosyalar assets/shaders/ altında repoya girer ve oyunla
# paketlenir; çalışma anında yalnız dosya okunur (r2d.c load_shader).
#
# ARAÇ ZİNCİRİ ve GEREKÇESİ
#   HLSL → SPIR-V : glslangValidator (Khronos glslang, HLSL ön-yüzü `-D`)
#   SPIR-V → MSL  : spirv-cross
#   HLSL → DXIL   : dxc (DirectXShaderCompiler) — YALNIZ varsa
#
# SDL_shadercross yerine bu ikili neden seçildi: shadercross'un kendisi de içeride
# tam olarak "DXC ile SPIR-V üret, SPIRV-Cross ile MSL'e çevir" yapıyor, ama
# GitHub'da 0 release/0 tag ile pre-release durumda ve hazır macOS binary'si
# resmî SDL release arşivlerinde yok. glslang + spirv-cross ikisi de Homebrew'da
# sürümlü paket olarak var (brew install glslang spirv-cross), yani build makinesi
# kurulumu tek satır. Aracın kimliği zaten önemsiz: çıktı bytecode repoda duruyor.
#
# BUILD MAKİNESİ GEREKSİNİMLERİ
#   macOS/Linux : brew (ya da apt) install glslang spirv-cross   → SPIR-V + MSL
#   Windows     : yukarıdakiler + DXC (dxc.exe YANINDA dxil.dll) → ayrıca DXIL
#
# DXIL UYARISI (sessiz-felaket türü): dxil.dll bir İMZA kütüphanesidir. DXC onu
# derleme sırasında LoadLibrary ile arar; bulamazsa DXIL'i İMZASIZ üretir ve
# müşterinin makinesinde pipeline yaratma "Shader is corrupt or in an unrecognized
# format" ile patlar. dxil.dll dxc.exe'nin yanında olmak ZORUNDA.
#
# Kullanım: proje kökünden ya da bu dizinden — sh shaders/compile.sh

set -e
cd "$(dirname "$0")"
OUT=../assets/shaders
mkdir -p "$OUT/spirv" "$OUT/msl" "$OUT/dxil"

# MSL 2.0: macOS 10.13+. SDL'in Metal arka ucu MSL KAYNAK metnini alıp Metal'e
# derletir (SDL_GPU_SHADERFORMAT_MSL bytecode değil kaynaktır) — bugünkü davranışın
# aynısı, elle yazılmış string yerine üretilmiş dosya.
MSL_VERSION=20000

have_dxc=0
if command -v dxc >/dev/null 2>&1; then have_dxc=1; fi

for src in *.vert.hlsl *.frag.hlsl; do
    base=$(basename "$src" .hlsl)          # örn. sprite.vert
    case "$src" in
        *.vert.hlsl) gstage=vert; dprofile=vs_6_0 ;;
        *)           gstage=frag; dprofile=ps_6_0 ;;
    esac

    # 1) HLSL → SPIR-V.  -D = girdi HLSL.  Register→(set,binding) eşlemesi
    #    SDL'in sözleşmesiyle birebir çıkıyor: (b0,space1)→set1, (t0/s0,space2)→set2,
    #    (b0,space3)→set3 (bkz. SDL_gpu.h SDL_CreateGPUShader doküman bloğu).
    glslangValidator -D -e main -S "$gstage" --target-env vulkan1.0 -V \
        -o "$OUT/spirv/$base.spv" "$src" >/dev/null

    # 2) SPIR-V → MSL.  --msl-decoration-binding: MSL'de descriptor set yok, bu
    #    yüzden SPIR-V binding numarası doğrudan [[buffer/texture/sampler(n)]]
    #    indeksi olarak kullanılır — SDL'in Metal düzeni tam olarak bu.
    #    Vertex girdileri [[stage_in]] ile geliyor (SDL_gpu.h'ın önerdiği yol).
    spirv-cross --msl --msl-version "$MSL_VERSION" --msl-decoration-binding \
        "$OUT/spirv/$base.spv" --output "$OUT/msl/$base.msl"

    # 3) HLSL → DXIL.  Windows/D3D12 için. Bu makinede dxc yoksa atlanır;
    #    o zaman assets/shaders/dxil/ repoda ne varsa öyle kalır (silinmez).
    if [ "$have_dxc" = 1 ]; then
        dxc -T "$dprofile" -E main -Fo "$OUT/dxil/$base.dxil" "$src"
        echo "  $base  → spv + msl + dxil"
    else
        echo "  $base  → spv + msl        (dxc yok: DXIL atlandi)"
    fi
done

if [ "$have_dxc" = 0 ]; then
    echo "UYARI: dxc bulunamadi — DXIL uretilmedi. Windows/D3D12 hedefi icin"
    echo "       DirectXShaderCompiler gerekli (dxc + dxil.dll yan yana)."
fi
