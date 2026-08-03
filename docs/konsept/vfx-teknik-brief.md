# VFX / Shader Teknik Brief — katmanlama, hile, ve bizim motorda maliyeti

Bu belge iki soruya cevap veriyor: **(1)** "su olmadan su dalgalanıyormuş gibi, alevin üstünde
duman" nasıl yapılıyor — hangi hile hangi hilenin üstüne konuyor; **(2)** bunların hangisi bizim
motorda bugün **hazır**, hangisi **eksik**, eksik olanın faturası ne.

Her teknik maddesinde dört satır aranıyor: *fikir · gereken matematik/doku · kaç pass · pixel-art'ta
neyi bozar.* Kod referansları `dosya:satır` biçiminde ve **doğrulanmış** — tahmin yok. Bulunamayan
kaynak "bulunamadı" diye yazıldı.

---

## (A) MOTORUN BUGÜNKÜ YETENEĞİ — kod referanslarıyla

### A.1 Boru hattının şekli

| Parça | Durum | Kod |
|---|---|---|
| GPU API | SDL_GPU (SDL 3.4.8, vendored submodule) | `r2d.c:486`, `vendor/SDL` @ `3.4.8` |
| Shader dili | **yalnız MSL**, tek gömülü string, 4 giriş noktası | `r2d.c:46-333` |
| Shader giriş noktaları | `vs_main`, `fs_main`, `fs_light`, `fs_postfx` | `r2d.c:60/70/116/260` |
| Vertex düzeni | pos(2) + uv(2) + rgba(4) = 8 float / 32 bayt | `r2d.c:13-16` |
| Kare bütçesi | 16384 quad, 2048 draw kırılımı | `r2d.c:8-9` |
| Blend modu (genel çizim) | **2 tane**: ALPHA, ADD | `r2d.h:22-26` |
| Blend modu (dahili) | MULTIPLY — `R2D_BLEND_COUNT` işaretiyle, **yalnız ışık pass'i** | `r2d.c:447-452`, `r2d.c:1160-1168` |
| Sampler | **2 tane**: NEAREST ve LINEAR, ikisi de `CLAMP_TO_EDGE` | `r2d.c:545-555` |
| Doku formatı | her zaman `R8G8B8A8_UNORM` — HDR/float hedef yok | `r2d.c:655` |
| Render target | `r2d_target_create` + `target_begin/end`; pass **kendi komut tamponuyla anında gönderilir** | `r2d.c:1310-1340` |
| Işık pass'i | 32 ışık, 16 tıkayıcı, 4 düşüş modu, 3 dither modu, kuantizasyon, ışık başına şekil | `r2d.c:116-249` |
| Post-fx pass'i | offscreen `sceneTex` (pencere boyutu) → `fs_postfx` → swapchain | `r2d.c:1212-1242` |
| Sahne/HUD ayrımı | `r2d_postfx_flush()` — dünya filtrelenir, HUD filtrelenmez | `r2d.c:1244-1258`, çağrı `main.c:11598` |
| Parçacık | 512 slot, tek `glowTex`, tek blend (ALPHA), kuvvet yok | `fx.c:34-117` |
| Sheet-anim efekt | 48 slot, `animfx_spawn_lit` — efekt dünyaya **ışık yayıp söner** | `fx.c:126-183` |
| Yer izi (decal) | 64 slot, prosedürel düzensiz kenarlı leke; **blend'i hiç ayarlamıyor** (miras alıyor) | `decal.c:11-99` |
| Ekran sarsıntısı | var (`shakeAmp`/`shakeDur`, dünya px'e çevrilir) | `main.c:1728`, `main.c:10463-10467` |
| Hit-stop | var (`g_hitStop`, dünya dt'si kısılır) | `main.c:330-335`, `main.c:5592-5600` |
| Hit flash | var (mob sprite'ı 0.06 sn beyaz-sıcak, sonra kırmızı tint) | `mob.c:1074-1133` |

### A.2 Çizim sırası (dünya karesi) — yeni katman nereye girecek, bunu bilmek şart

```
10601  tilemap_render                     zemin (16px sanat, TILE_SIZE 64 → 4× ölçek)
  └─ wld_water_pass (tilemap.c:609)   su karoları — STATİK, animasyon karesi YOK
10614  decor_render_shadows / shadow_blob  gölgeler
10643  decal_render                       yer izleri (yanık/kırağı/zehir/kan)
10645  decor_render_all                   evler, ağaçlar
10674  lamp_render                        kandil direkleri
11036  player_render / mob_render / ally_render
11150+ mermiler, kutsal alan, telegraph halkaları, yankı tayfı
11267  fx_render                          PARÇACIKLAR (ALPHA blend, glowTex)
11268  animfx_render                      SHEET EFEKTLERİ
11347  kar örtüsü (tam ekran beyaz tül)
11354  floattext                          hasar yazıları
11491  r2d_light_pass                     ══ IŞIK (multiply) ══
11494  hava görselleri                    yağmur/kar/sis — IŞIKTAN SONRA
11567  şimşek parlaması, hasar vinyeti
11598  r2d_postfx_flush                   ══ SAHNE / HUD SINIRI ══
11973+ HUD (hotbar, minimap, paneller)
```

**Buradan çıkan üç somut gözlem:**

1. **Su karoları statik.** `wld_water_pass` (`tilemap.c:463-489`) zamana bağlı hiçbir şey
   yapmıyor — kare indeksi yok, ofset yok. Yani limanın suyu şu an hareketsiz bir doku.
   "Su gibi görünen su" için ilk hamle burada ve **shader gerektirmiyor** (bkz. D.1).
2. **Hava görselleri ışık pass'inden SONRA çiziliyor.** Yani gece sis hâlâ parlak gri, yağmur
   damlası hâlâ tam parlaklıkta. Bu bir hata değil bir tercih olabilir ama *bilinçli* olmalı:
   kandil kanonunda (LORE §1.5) gece karanlığı mekanik, ve gecenin üstüne oturan açık gri tül
   halkanın kenarını okumayı zorlaştırır.
3. **Sahne/HUD sınırı var** (`main.c:11598`), yani `bloomThreshold`'ı HUD yüzünden yüksek tutma
   zorunluluğu **kalktı**. `r2d.h:164-165` ve `main.c:1071-1073`'teki "HUD'un beyaz yazısı da
   taşar" yorumları artık **eski** — eşik düşürülebilir, bu bedava bir kazanç.

### A.3 SCREEN-SPACE REFRACTION ALTYAPISI: HAZIR — kodda doğrulandı

Bu, brief'in en önemli tek bulgusu. Sorulan şey şuydu: *"post-fx pass'i sahne target'ını
örnekliyor mu?"* **Örnekliyor:**

- `r2d.c:1238` — post-fx pass'i `sceneTex`'i fragment texture 0 olarak bağlıyor:
  `SDL_GPUTextureSamplerBinding tsb = { .texture = sceneTex->tex, .sampler = sampler };`
- `r2d.c:1212-1219` — `sceneTex` pencere boyutunda, çizilebilir bir RGBA8 target.
- `r2d.c:266-274` — ve **UV kaydırma zaten yazılmış**: `heat` alanı tam olarak
  screen-space UV bozması yapıyor:

```
"    float2 uv = in.uv;\n"
"    if (u.d.w > 0.0) {\n"
"        float tt = u.c.w * u.e.x * 0.06;\n"
"        float depth = uv.y;\n"                 // 0 üst, 1 alt
"        float w1 = sin(uv.y * 38.0 + tt * 3.1) * 0.6;\n"
"        float w2 = sin(uv.y * 91.0 - tt * 5.7) * 0.4;\n"
"        uv.x += (w1 + w2) * u.d.w * depth;\n"
"        uv.y += sin(uv.x * 55.0 + tt * 2.3) * u.d.w * 0.35 * depth;\n"
"    }\n"
```

Yani **"su olmadan su" hilesinin kalbi motorda çalışıyor durumda** — VOLKAN preset'inde
(`main.c:1120`, `heat=0.0038`, `heatSpeed=1.8`) görülebiliyor. Eksik olan tek şey: bu bozma
**global**. Tüm ekrana uygulanıyor, maskesi yok, yalnız `uv.y` ile ağırlıklandırılıyor. Bir
gölete, bir kazana, bir ocağa **yerel** olarak uygulanamıyor.

### A.4 EKSİK OLANLAR — her biri fatura kalemi

| Eksik | Neden gerekli | Fatura |
|---|---|---|
| **Sprite başına özel fragment shader yolu** | `fs_main` sabit: `tex.sample(smp,uv) * col` (`r2d.c:73`). Eşikleme, rampa, dither, akış — hiçbiri sprite çiziminde yapılamıyor | Yeni fs + yeni pipeline + **draw yönlendirmesi**. Bugün yönlendirme `blend` enum'una bakıyor (`r2d.c:820`); ışık pass'i bunu `R2D_BLEND_COUNT` işaretiyle deliyor (`r2d.c:1160`). Temiz çözüm: `R2DDraw`'a (`r2d.c:31-35`) `pipelineId` + uniform payload alanı. 2048 draw × 32 bayt = 64 KB statik. **~120 satır r2d.c** |
| **İkinci doku slotu** | Gürültü + maske + rampa aynı çizimde okunamıyor; `fs_main` 1 sampler ile kurulu (`r2d.c:499`) | Yeni fs'de `num_samplers=2..3`; `flush_to` (`r2d.c:1174-1178`) tek doku bağlıyor → çoklu bağlama gerekli. **~30 satır** |
| **REPEAT (wrap) sampler** | Kayan/döşenen gürültü dokusu şu an **saramıyor** — iki sampler de `CLAMP_TO_EDGE` (`r2d.c:548-549`). Kayan maske hilesinin şartı bu | SDL'de mod var (`SDL_gpu.h:1302`), bizde yaratılmamış. **~8 satır** |
| **MULTIPLY blend'in genel çizime açılması** | Gölge, zehir lekesi, is, kirli su için doğru mod. Pipeline **zaten kurulu**, sadece enum'da yok | `R2DBlend`'e `R2D_BLEND_MUL` ekle, `make_pipeline`'daki sentinel mantığını ayıkla. **~20 satır** — ama `R2D_BLEND_COUNT` sentinel'i tüm ışık yolunda kullanıldığı için dikkatli |
| **Parçacık kuvvetleri** | `Particle`'da yalnız `vx/vy` var (`fx.h:23-32`); `fx_update` sabit sürtünme uyguluyor (`fx.c:45-46`) — yerçekimi, rüzgâr, türbülans yok | `fx.h`/`fx.c` alanları + güncelleme. **~60 satır**, shader gerekmez |
| **Parçacıkta blend seçimi** | `fx_render` her şeyi ALPHA çiziyor (`fx.c:61`) — kıvılcım ve alev **additive olmalı**, duman alpha | Parçacığa `blend` alanı + render'da gruplama. **~25 satır** |
| **Parçacıkta renk rampası** | Renk geçişi `if/else` zinciriyle elle yazılmış (`fx.c:82-102`); her yeni efekt yeni `else if` | 1D rampa dizisi (CPU'da örneklenirse shader bile gerekmez). **~40 satır** |
| **HDR / RGBA16F hedef** | Gerçek overbright yok; ACES modu tavanı açmıyor — kodun kendi yorumu söylüyor (`r2d.c:210-213`) | Target formatını değiştirmek + tüm pipeline varyantları. **Büyük iş, şimdi gerekmiyor** |
| **Düşük çözünürlüklü sanat-pikseli hedefi** | `sceneTex` **pencere boyutunda** (`r2d.c:1217`). Sanat pikseli 4 pencere pikseli (16px karo × TILE_SIZE 64 → 4×; `common.h:11`, `tilemap.c:370`). Yani post-fx'in UV kaydırması sanat ızgarasının **altında** çalışıyor → titreme riski | GDD §6.2'de zaten borç olarak yazılı ("1 texel = 1 sanat pikseli"). Bkz. B.10 |

### A.5 Zaten hazır olan ve tekrar yazılmaması gerekenler

- **Dither iki çeşit**: Bayer 4×4 (`r2d.c:231-234`) ve interleaved gradient noise (`r2d.c:241-242`),
  ayrıca **dünya-çivili** IGN varyantı (kamera hareketinde desen "crawl" etmiyor, `r2d.c:239-240`).
  Bunlar ışık pass'inde. Yeni bir VFX shader'ı için kopyalanacak hazır kod.
- **Bloom** (`r2d.c:284-304`): 2 halka × 6 tap, luma eşiği, renk koruyan taşma.
- **Efekt ışığı**: `animfx_spawn_lit` + `animfx_append_lights` (`fx.c:126-183`) — patlama duvarı
  aydınlatıyor, ışık eğrisi %20 yükseliş + kare sönüş.
- **Prosedürel doku üretimi** deseni: `create_glow_texture` (`render.c:40-73`),
  `create_vignette_texture` (`render.c:77-108`), `decal_init` (`decal.c:11-38`). Gürültü ve rampa
  dokuları **asset beklemeden** aynı desenle üretilebilir.
- **Düzensiz kenar disiplini**: decal dokusu yarıçapı üç sinüsle modüle ediyor (`decal.c:24-27`),
  ışık silueti çift/üçlü açı özdeşlikleriyle dalgalanıyor (`r2d.c:157-167`). "Kusursuz daire yapay
  duruyor" dersi bu projede zaten öğrenilmiş — yeni efektler de bu kurala uymalı.
- **Sarsıntı + hit-stop + hit flash** üçlüsü: beceri efekti anatomisinin (B.8) zamanlama
  katmanları hazır.

---

## (A2) SHADER TAŞINABİLİRLİĞİ — VFX'ten ÖNCE çözülecek

### A2.0 Bugünkü durum: oyun Windows'ta hiç çizmiyor

Üç satır bunu belirliyor:
- `r2d.c:486` — `SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_MSL, false, NULL)`: uygulamanın
  sağlayabildiği tek format MSL. Windows'ta ne D3D12 ne Vulkan bu formatı tüketebilir.
- `r2d.c:420` — `load_shader()` içinde `.format = SDL_GPU_SHADERFORMAT_MSL` sabit.
- `r2d.c:46-333` — shader kaynağı **gömülü tek bir MSL string'i** (288 satır C string'i,
  ~250 satır gerçek shader kodu), içinde **dört** giriş noktası: `vs_main`, `fs_main`,
  `fs_light`, `fs_postfx`.

SDL_GPU'nun API'si çok platformlu (Metal / D3D12 / Vulkan) ama **shader bytecode'unu taşımıyor**;
her arka uç kendi formatını ister. `SDL_shadercross` `vendor/` içinde **yok** (`vendor/`: SDL,
SDL_ttf, cjson).

### A2.1 SDL3'ün resmî yolu: SDL_shadercross

**Repo:** `github.com/libsdl-org/SDL_shadercross` · **lisans: zlib** (bizim için sorun yok).
README'nin kendi ifadesi: *"It takes SPIRV or HLSL as the source and outputs DXBC, DXIL, SPIRV,
MSL, or HLSL."* Yani **HLSL kaynak dil olur, üç hedefin hepsi ondan üretilir.** GLSL girdi
desteklenmiyor.

| Parça | Ne için | Not |
|---|---|---|
| DirectXShaderCompiler (DXC) | HLSL → SPIR-V ve HLSL → DXIL | `dxcompiler.dll` + `dxil.dll` |
| SPIRV-Cross | SPIR-V → MSL / HLSL | `spirv-cross-c-shared.dll` |
| `d3dcompiler_47` | yalnız DXBC (bize gerekmez) | Windows'la gelir |

**Kütüphane + CLI ikisi de var** (`SDLSHADERCROSS_CLI` CMake seçeneği varsayılan ON).

**Olgunluk — dürüst tablo:** GitHub'da **0 release, 0 tag**; header sürümü 3.0.0 diyor; vcpkg
paketi `sdl3-shadercross` **3.0.0-preview2**. API son bir yıl içinde değişti (eski blog
yazılarındaki `SDL_ShaderCross_CompileGraphicsShaderFromHLSL()` artık header'da yok).
README'de literal "unstable API" ibaresi **bulunamadı**, ama pratikte pre-release. Hazır binary
**SDL'in resmî release arşivlerinde yok** (`release-3.4.12` asset listesinde geçmiyor) — CI
artifact'lerinden (`SDL3_shadercross-macos-arm64`, `SDL3_shadercross-VC-x64`) ya da vcpkg'den
alınıyor.

**Bizim için bu neden sorun değil:** offline yolda shadercross yalnız bir **derleme aracı**;
oyunun çalışma zamanında ona hiç dokunulmuyor. Aracın API'si değişse bile üretilmiş `.spv/.msl/
.dxil` dosyaları etkilenmez. Yani pre-release olması riski taşımıyor — **runtime kullanırsak
taşır.**

### A2.2 Offline (derleme zamanı) vs runtime — öneri: OFFLINE

| | Offline (CLI, üç formatı paketle) | Runtime (HLSL'i açılışta derle) |
|---|---|---|
| Ne shipping edilir | yalnız `.spv` + `.msl` + `.dxil` dosyaları | `SDL3_shadercross.dll` + `spirv-cross-c-shared.dll` + `dxcompiler.dll` + `dxil.dll` |
| Çalışma zamanı maliyeti | sıfır | açılışta derleme (bizim 4 shader için küçük ama sıfır değil) |
| Hata ne zaman görülür | **CI'da / derlemede** | müşterinin makinesinde |
| Bağımlılık riski | yok | pre-release bir kütüphaneye runtime bağımlılık |
| Eksisi | shader değişince derleme adımı gerekir (hot-reload yok) | — |

Bizim için offline **açık ara** doğru: 4 shader, hot-reload ihtiyacı yok, ve SDL_gpu'nun kendi
resmî örnek deposu (`TheSpydog/SDL_gpu_examples`) aynen bunu yapıyor. Onun `compile.sh`'ı birebir
şu:

```sh
# Requires shadercross CLI installed from SDL_shadercross
for filename in *.vert.hlsl; do
    shadercross "$filename" -o "../Compiled/SPIRV/${filename/.hlsl/.spv}"
    shadercross "$filename" -o "../Compiled/MSL/${filename/.hlsl/.msl}"
    shadercross "$filename" -o "../Compiled/DXIL/${filename/.hlsl/.dxil}"
done
```
Kaynak/hedef/stage **uzantıdan çıkarılıyor**, tek flag gerekmiyor. Bonus: `-o foo.frag.json`
**reflection metadata** üretiyor (`{ "samplers": N, "uniformBuffers": N, ... }`).

**İki uyarı, ikisi de sessiz-felaket türü:**
1. **`dxil.dll` = imza kütüphanesi.** DXC derleme sırasında onu `LoadLibrary` ile arıyor;
   bulamazsa DXIL'i **imzasız** üretiyor ve müşterinin makinesinde pipeline yaratma
   *"Shader is corrupt or in an unrecognized format"* ile patlıyor. Yani `dxil.dll` **CLI'ın
   yanında** olmalı — bu bir *build* meselesi, shipping meselesi değil.
2. **`-c` / `--cull` kullanma.** CLI'ın kendi uyarısı: kullanılmayan binding'leri atıyor ve
   **binding indeksleri kayıyor**. Bizim `num_samplers`/uniform indekslerimiz elle yazılı.

### A2.3 Çalışma anında format seçimi

```c
// 1) Cihaz: sağlayabildiğin TÜM formatları bildir, backend'i SDL seçsin
gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV |
                          SDL_GPU_SHADERFORMAT_DXIL  |
                          SDL_GPU_SHADERFORMAT_MSL, debug, NULL);

// 2) Shader yükleme: hangi formatı tüketebiliyor?
SDL_GPUShaderFormat have = SDL_GetGPUShaderFormats(gpu);
const char *path, *entry;
SDL_GPUShaderFormat fmt;
if (have & SDL_GPU_SHADERFORMAT_SPIRV) {
    path = "assets/shaders/spirv/%s.spv";  fmt = SDL_GPU_SHADERFORMAT_SPIRV; entry = "main";
} else if (have & SDL_GPU_SHADERFORMAT_MSL) {
    path = "assets/shaders/msl/%s.msl";    fmt = SDL_GPU_SHADERFORMAT_MSL;   entry = "main0";
} else if (have & SDL_GPU_SHADERFORMAT_DXIL) {
    path = "assets/shaders/dxil/%s.dxil";  fmt = SDL_GPU_SHADERFORMAT_DXIL;  entry = "main";
} else { /* hata */ }
```

**İki kritik ayrıntı:**
- **MSL'de entrypoint `main0`.** Metal `main` adına izin vermiyor; SPIRV-Cross `main` → `main0`
  çeviriyor. Elle yazdığımız MSL'de bu sorun yoktu (adlarımız `vs_main` vb.), üretilen MSL'de
  olacak.
- **`SDL_GPU_SHADERFORMAT_DXIL` flag'ini vermezsen Windows'ta D3D12 backend'i hiç denenmez.**
  SDL'in D3D12 hazırlığı en başta bu bayrağı kontrol edip yoksa `false` dönüyor — yani sessizce
  Vulkan'a düşersin (Vulkan yoksa hiç çizmezsin).
- `SDL_GetGPUDeviceDriver()` seçilen backend'in adını veriyor (`"metal"`, `"direct3d12"`,
  `"vulkan"`) — bizde zaten loglanıyor (`r2d.c:606`), taşımadan sonra bu satır teşhis aracı olur.

### A2.4 HLSL bağlama kuralları — SDL'in DOKÜMANTE ETTİĞİ eşleme

Bu tahmin değil; kaynağı bizim kendi `vendor/`umuzda: `vendor/SDL/include/SDL3/SDL_gpu.h`,
`SDL_CreateGPUShader` doküman bloğu (satır 2698-2737). Birebir:

```
 * For DXBC and DXIL shaders, use the following register order:
 *   For vertex shaders:
 *     - (t[n], space0): Sampled textures, followed by storage textures, ...
 *     - (s[n], space0): Samplers with indices corresponding to the sampled textures
 *     - (b[n], space1): Uniform buffers
 *   For pixel shaders:
 *     - (t[n], space2): Sampled textures, followed by storage textures, ...
 *     - (s[n], space2): Samplers with indices corresponding to the sampled textures
 *     - (b[n], space3): Uniform buffers
```

| | MSL (bugün) | HLSL (yarın) | SPIR-V karşılığı |
|---|---|---|---|
| Vertex uniform | `[[buffer(0)]]` | `register(b0, space1)` | descriptor set 1 |
| Vertex doku/sampler | (yok) | `register(t0/s0, space0)` | set 0 |
| Fragment doku | `[[texture(0)]]` | `register(t0, space2)` | set 2 |
| Fragment sampler | `[[sampler(0)]]` | `register(s0, space2)` | set 2 |
| Fragment uniform | `[[buffer(0)]]` | `register(b0, space3)` | set 3 |

**Vertex semantikleri `TEXCOORD0`'dan başlamak ZORUNDA.** Aynı doküman bloğu: *"the SDL
implementation assumes that non system-value semantics will all be TEXCOORD… your vertex
semantics should start at TEXCOORD0 and increment."* Yani `POSITION`/`COLOR` **kullanılmaz**.
Bizim üç attribute'umuz (`r2d.c:459-463`) sırayla `TEXCOORD0` (pos), `TEXCOORD1` (uv),
`TEXCOORD2` (col) olur.

Çalışan referans (`SDL_gpu_examples`, `TexturedQuad.frag.hlsl`) tam olarak şöyle:
```hlsl
Texture2D<float4> Texture : register(t0, space2);
SamplerState Sampler : register(s0, space2);
float4 main(float2 TexCoord : TEXCOORD0) : SV_Target0 {
    return Texture.Sample(Sampler, TexCoord);
}
```

Ayrıca `SDL_GPUShaderCreateInfo`'daki `num_samplers` / `num_uniform_buffers` **doğru
doldurulmak zorunda** — SDL'in kendi FAQ'si bunu en yaygın hata olarak sayıyor ve reflection
kullanmayı öneriyor. Bizde bu değerler `load_shader()`'a elle geçiliyor (`r2d.c:498-530`) ve
doğru; taşımadan sonra `-d JSON` çıktısıyla teyit etmek bedava.

**Stage başına uniform slot limiti 4** (`SDL_gpu.h`). Biz 1 kullanıyoruz — VFX shader'ları
eklenirken bu tavan hatırlanmalı, ama 4 slot bizim için bol.

### A2.5 Windows'ta D3D12 mi Vulkan mı?

**SDL kendisi seçiyor ve tercihi D3D12.** Backend deneme dizisi (SDL kaynağı,
`src/gpu/SDL_gpu.c`): `Private → Metal → D3D12 → Vulkan`; ilk hazırlanabilen kazanıyor.
Bu sıra **kasıtlı** — libsdl commit'i *"GPU: Prefer D3D12 over Vulkan when available"*,
**2025-12-01** (commit mesajında gerekçe yazılmamış, sadece dizi yeniden sıralanmış).

Öneri: **Windows'ta birincil hedef D3D12/DXIL**, Vulkan/SPIR-V yedek olarak yine derlenip
paketlensin. Üçüncü artifact'in maliyeti sıfır (aynı `compile.sh` döngüsü) ve Vulkan-only
makinelerde bedava kazanç.

`SDL_HINT_GPU_DRIVER` (`"SDL_GPU_DRIVER"`) ile zorlanabiliyor — ama **shipping build'de
sabitlemeyin**: hint set edildiyse ve o backend hazırlanamazsa SDL **fallback yapmaz**, hata
verip çıkar. Kullanıcıya ortam değişkeni olarak bırakmak doğru (teşhis için birebir bu lazım
olacak).

Sistem gereksinimleri (`SDL_gpu.h`): **D3D12** — Windows 10+, DX12 Feature Level 11_0 **ve
Resource Binding Tier 2+**. Tier 2 şartı eski Intel'leri (Haswell/Broadwell) dışarıda bırakıyor;
`SDL_PROP_GPU_DEVICE_CREATE_D3D12_ALLOW_FEWER_RESOURCE_SLOTS_BOOLEAN` ile gevşetilebilir.
**Vulkan** — 1.0 + `VK_KHR_swapchain`, `VK_KHR_maintenance1`, `independentBlend`,
`imageCubeArray`, `depthClamp`, `shaderClipDistance`, `drawIndirectFirstInstance`,
`sampleRateShading`.

### A2.6 BİZİM DÖRT SHADER'IN MSL→HLSL MALİYETİ — kodu okuyarak

**Bedava olan (sıfır iş): koordinat sistemi.** SDL bunu tekilleştirmiş; `SDL_gpu.h:269-287`
birebir: *"The GPU API uses a left-handed coordinate system… If the backend driver differs from
this convention (e.g. Vulkan, which has an NDC that assumes +Y is down), SDL will automatically
convert the coordinate system behind the scenes, so you don't need to perform any coordinate
flipping logic in your shaders."* Yani `vs_main`'in NDC dönüşümü (`r2d.c:63-65`) **aynen** kalır,
`in.uv` origin'i aynı kalır, derinlik aralığı sorun değil. Bu, taşımanın en korkulan kısmının
bize hiç dokunmadığı anlamına geliyor.

**Kalem kalem iş listesi:**

| # | Kalem | Bizde nerede | Zorluk |
|---|---|---|---|
| 1 | **`select()` argüman sırası TERS** — MSL `select(falseVal, trueVal, cond)`, HLSL 2021 `select(cond, trueVal, falseVal)` | `r2d.c:109`, `r2d.c:113` (sRGB↔lineer dönüşümü) | **EN TEHLİKELİ.** Derleme hatası vermez, **sessizce yanlış renk** verir. Ve tam olarak ışık gama yolunda — yani Windows'ta ışık farklı görünür ve sebebi bulunmaz |
| 2 | `fract` → `frac` | 3 yer (`r2d.c:241,242,322,325` civarı — dither ve gren) | mekanik |
| 3 | `mix` → `lerp` | 1 yer (`r2d.c:306`, doygunluk) | mekanik |
| 4 | Attribute → semantik | `r2d.c:49-58` `VSIn/VSOut` yapıları | `TEXCOORD0/1/2`, `SV_Position`, `SV_Target0` |
| 5 | Binding sözdizimi | 4 shader'ın imzaları | A2.4 tablosu |
| 6 | Yerel sabit dizi | `float B[16]` Bayer (`r2d.c:231`), `float3 pal[4]` (`r2d.c:316`), `float2 dirs[6]` (`r2d.c:288`) | `static const` yaz — DXC'de en güvenli |
| 7 | Uniform hizalama | `lightU` ve `postfxU` **yalnız `float4`** içeriyor (`r2d.c:365-374`, `r2d.c:350`) | **zaten güvenli.** std140 kuralı "vec3/vec4 16 bayt hizalı olsun" diyor; `float4` dizileri iki tarafta da 16 bayt stride |
| 8 | Vertex uniform'u `float viewport[2]` | `r2d.c:1148-1149` — 8 bayt push | Güvenli tarafa geç: `float4`'e doldur. 8 baytlık bir cbuffer sınırda |
| 9 | `int2(in.pos.xy) & 3` | `r2d.c:233` | HLSL'de birebir çalışır |
| 10 | Vektörde `?:` / `&&` / `\|\|` | Bizde vektör ternary **yok** (koşullar skaler: `lin`, `isLeft`) | HLSL 2021'de vektörde `?:` derleme hatası; bizi etkilemiyor ama yeni shader yazarken kural |
| 11 | **Tek string → dört dosya** | `r2d.c:46-333` | **Yapısal iş.** Offline derleme dosya başına bir shader ister: `sprite.vert.hlsl`, `sprite.frag.hlsl`, `light.frag.hlsl`, `postfx.frag.hlsl`. Entrypoint hepsinde `main` |
| 12 | Yükleme yolu | `load_shader()` (`r2d.c:413-426`) | Gömülü string yerine dosyadan oku + format seç (A2.3). ~50 satır |
| 13 | Derleme adımı | `CMakeLists.txt` | `add_custom_command` ile 4 shader × 3 format = 12 artifact. ~30 satır |
| 14 | Doğrulama | **`src/r2d_test.c` HAZIR** — device + shader + pipeline + doku + batch + present'i uçtan uca sınıyor, 90 kare çizip 0 ile çıkıyor | Windows'ta ilk koşacak şey bu olmalı, oyun değil |

**Tahmin: 4 HLSL dosyası ≈ 260 satır + `r2d.c`'de ~60 satır + CMake ~30 satır.** Yani **bir
günlük iş ölçeği**, ve %90'ı mekanik çeviri. Riskin tamamı 1 numaralı kalemde (`select`) ve
onu bulmanın yolu belli: taşımadan sonra macOS'ta ışık pass'inin ekran görüntüsünü alıp
Windows'takiyle karşılaştırmak.

**İki yan not, ikisi de bedava:**
- **Dosya yolu harf duyarlılığı.** `tilemap.c:259` ve `tilemap.c:260` `"assets/Tiles/..."` diyor,
  gerçek dizin `assets/tiles`. macOS ve Windows harf duyarsız olduğu için ikisinde de çalışır,
  **Linux'ta patlar.** Bugün sorun değil; Steam Deck / Proton gündeme gelirse ilk kırılacak yer.
  (Düzeltmedim — bilgi olarak.)
- **Prosedürel gürültü artık bir taşınabilirlik riski.** Hash tabanlı gürültü
  (`fract(sin(dot(p,k))*43758.5453)` — bizde film greninde var, `r2d.c:325`) float hassasiyetine
  bağlıdır ve **farklı GPU'larda farklı sonuç verir.** Üç backend'e çıkarken bu, B.2'nin
  "önceden üretilmiş doku kullan" önerisine dördüncü ve en sert gerekçeyi ekliyor:
  **CPU'da üretilmiş doku üç platformda birebir aynıdır.**

---

## (B) TEKNİK TEKNİK

Ortak notasyon: `uv` = 0..1 doku koordinatı · `t` = saniye · `ART` = bir sanat pikselinin kaç
pencere pikseli olduğu (**bizde 4**: 16px karo × `TILE_SIZE 64`) · örnekler **HLSL**.

SDL_GPU'nun HLSL bağlama düzeni (kaynak: `vendor/SDL/include/SDL3/SDL_gpu.h:2711-2728`,
`SDL_CreateGPUShader` doküman bloğu) her örnekte aynı:

```hlsl
// --- fragment (pixel) shader kaynak başlığı ---
Texture2D<float4>  Tex   : register(t0, space2);   // örneklenen dokular: space2
SamplerState       Smp   : register(s0, space2);   // sampler'lar: space2, doku indeksiyle eş
Texture2D<float4>  Noise : register(t1, space2);
SamplerState       NSmp  : register(s1, space2);
cbuffer FxU : register(b0, space3) {               // fragment uniform buffer: space3
    float4 P0;   // x=zaman y=hız z=ölçek w=genlik
    float4 P1;
};
struct PSIn { float4 pos : SV_Position; float2 uv : TEXCOORD0; float4 col : TEXCOORD1; };
```

---

### B.1 UV bozma / akış haritaları — "su olmadan su"nun kalbi

**Fikir.** Suyu çizmeyi bırak; **arkasındaki görüntüyü kaydırarak örnekle.** Gözün sudan anladığı
tek şey kırılmadır: suyun altındaki taş yerinden kaymış ve kayma **zamanla dalgalanıyor** görünür.
O kaymayı üretebiliyorsan su yüzeyine hiç ihtiyacın yok. Aynı matematik ısı dalgası (heat haze),
serap, büyü kalkanı, sarhoşluk, zehirlenme ve "gerçeklik çatlıyor" efektleri için de aynen çalışır
— tek fark bozma alanının şekli.

**Matematik.** `uv' = uv + D(x, y, t) · amp`. `D` bir 2D vektör alanı ve üç kalitede yapılır:

1. **Sinüs toplamı** (doku yok). En ucuz. **Tek** frekans "akordeon" gibi durur — bizim kodun
   yorumu bunu zaten yazmış (`r2d.c:269`). En az iki, tercihen üç asal-ish frekans:
   ```hlsl
   float2 wave(float2 uv, float t) {
       float2 d;
       d.x = sin(uv.y * 38.0 + t * 3.1) * 0.6 + sin(uv.y * 91.0 - t * 5.7) * 0.4;
       d.y = sin(uv.x * 55.0 + t * 2.3) * 0.35;
       return d;
   }
   ```
2. **DUDV / normal map** (1 doku). Dokunun RG'si -1..1'e açılır, iki farklı hız ve ölçekte **iki
   kez** örneklenip toplanır — bir katman tek başına tekrar deseni gösterir, iki katman kırar:
   ```hlsl
   float2 duv1 = Noise.Sample(NSmp, uv * 1.0 + float2( t * 0.03,  t * 0.02)).rg * 2.0 - 1.0;
   float2 duv2 = Noise.Sample(NSmp, uv * 1.7 + float2(-t * 0.02,  t * 0.045)).rg * 2.0 - 1.0;
   float2 D    = (duv1 + duv2) * 0.5;
   ```
3. **Flow map** (2 doku). Yüzeye bir yön haritası boyanır (RG = yerel akış vektörü): nehir sağa
   akar, çevrintide döner, kıyıda durur. Doku o yön boyunca **ilerletilir** (advection). Tek fazla
   ilerletirsen doku sonsuza kadar gerilir, o yüzden iki faz + çapraz geçiş:
   ```hlsl
   float2 flow  = Flow.Sample(FSmp, uv).rg * 2.0 - 1.0;
   float  ph    = frac(t * 0.5);            // 0..1 testere
   float  ph2   = frac(ph + 0.5);           // yarım faz kaydırılmış ikinci kopya
   float  w     = abs(1.0 - 2.0 * ph);      // çapraz geçiş ağırlığı (üçgen dalga)
   float2 s1    = Noise.Sample(NSmp, uv * 2.0 - flow * ph ).rg;
   float2 s2    = Noise.Sample(NSmp, uv * 2.0 - flow * ph2).rg;
   float2 D     = lerp(s1, s2, w) * 2.0 - 1.0;
   ```
   Bu, Valve'ın Portal 2 suyunda kullandığı yapı (Vlachos 2010, F.1). Slaytlarda yazılı olan ve
   **kolay atlanan üç ayrıntı**, tekniğin çalışmasının asıl sebebi:
   - **İki katman, yarım faz kaydırmalı**: "Blend the short animated segments in two layers. Each
     layer is offset half a phase so we can hide the resets/start of each layer." Tek katman
     her döngü başında **sıçrar**.
   - **Nabız atmayı (pulsing) gürültü öldürür**: her pikselin faz döngüsüne bir gürültü
     dokusundan okunan sabit bir kayma eklenir. Bu olmadan tüm yüzey aynı anda nefes alır.
   - **Akış hızı dalga gücünü de ölçekler**: `strength ∝ length(flowVector)`. Durgun suda dalga
     yok, hızlı akanda güçlü. Tek doku, iki bilgi.
   - Ve renk/döküntü (yaprak, köpük, is) akıtırken faz aralığının **zirvesi sıfır bozmaya**
     denk getirilmeli (normal map'te zirve yarım-bozmadadır ve kimse farketmez, ama akan bir
     yaprak yanlış yerde görünür).

**Kaç pass.** Sahnenin ayrı bir target'a çizilip bozularak basılması gerekir → **1 ek pass**.
Bizde o pass **zaten var** (post-fx, `r2d.c:1222-1242`). Yani *global* bozma sıfır pass. *Yerel*
bozma (yalnız gölette, yalnız kazanın üstünde) için iki yol:
- (a) post-fx shader'ına **maske** eklemek: ekran-uzayı bir maske dokusu ya da birkaç daire
  uniform'u (`maskCenter[N]`, `maskRadius[N]`) — ışık pass'inin `pos[32]` dizisiyle birebir aynı
  desen (`r2d.c:82`). +0 pass, +1 doku ya da +birkaç uniform.
- (b) sahneyi target'a çizip, o target'ı bozulacak bölgede **şeritler halinde** geri çizmek —
  aşağıda.

**Shader'sız yol (bizim için ilk hamle).** SDL_GPU'suz, MSL'siz, HLSL'siz: sahne target'ını
`r2d_target_end` ile bitir (aynı karede örneklenebilir hale gelir, `r2d.c:1328-1333`), sonra
bozulacak dikdörtgeni **yatay şeritler** halinde geri çiz; her şeridin `src` UV'sini `u` yönünde
sinüsle kaydır. 4 px'lik şeritlerle 360 px yükseklikte bir bölge = 90 quad. 16384 quad bütçesinde
hiç. Bu, 8/16-bit donanımının satır-kaydırma (line scroll / HDMA) hilesinin birebir aynısı ve
o dönemin su yansımaları böyle yapılıyordu.

**Pixel-art'ta neyi bozar — ve snapping neden ŞART.** Kaydırma miktarı sürekli bir sayıdır
(`0.37 px`, `1.62 px`). Sanat pikseli 4 pencere pikseli olduğu için bu, sanat ızgarasının
**altında** bir hareket demek. İki ayrı hasar:

- **Titreme (shimmer).** NEAREST örneklemede 1.4 px'lik bir ofset ya 1 ya 2 pencele pikseline
  yuvarlanır; ofset kare kare 1.4→1.5→1.6 giderken çıktı 1→2→2 diye **zıplar**. Su yüzeyi
  dalgalanmıyor, **titriyor** görünür.
- **Sanat ızgarasının kırılması.** Kaydırılmış örnekleme, komşu iki sanat pikselinin parçalarını
  aynı ekran sanat-pikselinin içine düşürebilir. Sonuç: pixel-art'ın en temel sözleşmesi —
  "ekranda gördüğün her 4×4 blok tek renktir" — bozulur.

Çözüm iki satır: ofseti sanat pikseline yuvarla, **ve** örnekleme noktasını sanat pikselinin
merkezine çivile.
```hlsl
// ART = 4.0 (bir sanat pikseli kaç pencere pikseli), Res = hedefin px boyutu
float2 art = ART / Res;                       // bir sanat pikseli, uv biriminde
float2 D   = wave(uv, t) * amp;               // ham bozma (uv birimi)
uv += round(D / art) * art;                   // 1) ofset sanat pikselinin tam katı
uv  = floor(uv / art) * art + art * 0.5;      // 2) örnek noktası sanat pikselinin merkezi
float4 c = Tex.Sample(Smp, uv);               // Smp NEAREST
```
İkinci satır bedava bir kazanç daha veriyor: kamera kesirli konumdayken bile örnekleme ızgarası
sabit kalır, yani B.10.4'teki titreme de yumuşar.

**Daha da temiz bir formülasyon** (HaxeFlixel pixel-perfect su örneğinden, F.3): bozmayı **uv
biriminde değil piksel biriminde** yaz, genliği doğrudan "kaç sanat pikseli" olarak ver:
```hlsl
float2 pix = ART / Res;                        // bir sanat pikseli, uv birimi
uv.x += round(sin(uv.y * 26.0 + t * 2.0) * AMP_PX) * pix.x;   // AMP_PX = 1.0 → tam 1 sanat px
```
Genlik `1.0` ise dalga tam bir sanat pikseli sağa-sola oynar; `2.0` ise iki. Bu, sanat yönüne
konuşulabilir bir birim veriyor ("dalga iki piksel") ve snapping'i formülün içine gömüyor.

**Mimari not (Kyle Halladay, F.1).** Birden fazla bozma kaynağı olacaksa (su + ocak + büyü kalkanı
aynı karede) her biri için ayrı bir screen-read yapmak yerine **ayrı bir "bozma tamponu"** (RG =
ofset, nötr değer 0.5) çizilir; post-fx o tamponu bir kez okuyup tek seferde bozar. Bizim
mimarimize birebir uyuyor: `r2d_target_create` ile küçük (örn. yarım çözünürlüklü) bir RG hedefi,
bozucu efektler ona additive çizilir, `fs_postfx` onu ikinci doku olarak okur.

**Uyarı — üst üste binen screen-read.** Godot'un kendi dokümanı bu tuzağı açıkça yazıyor: ekranı
okuyan iki shader üst üste binerse ikincisi birincinin sonucunu **görmez**. Bizde de aynı sınır
var, hatta daha sert: post-fx **kare başına tek pass** (`r2d.c:1249`, `postfxSplit` bayrağı).
Yani "suyun içinden geçen ısı dalgası" gibi zincirlemeler tek pass'in içinde, ofsetlerin
**toplanmasıyla** çözülmek zorunda — pass zincirleyerek değil. Bozma tamponu mimarisi bunu
kendiliğinden yapıyor (ofsetler tamponda toplanır).

---

### B.2 Gürültü — value / Perlin / Simplex, FBM, domain warping

**Fikir.** Alev, duman, sis, çürüme, erime, bulut — hepsi "düzensiz ama sürekli" alanlardır. O
alanı üreten şey gürültüdür. Gürültü olmayan bir alev efekti ya el çizimi bir sheet'tir (pahalı,
sonlu) ya da sinüsten çıkmış bir şeydir (tekrarı gözle yakalanır).

**Aile.**
- **Value noise**: ızgara köşelerine hash'ten rastgele **skaler**, aralar smoothstep ile karışır.
  En ucuz, en yumuşak, en çok "ızgara hizası" artefaktı verir.
- **Perlin**: köşelere rastgele **gradyan vektörü**; değer gradyan·mesafe iç çarpımlarının
  interpolasyonu. Izgara artefaktı belirgin biçimde daha az.
- **Simplex**: kare ızgara yerine simpleks (2D'de üçgen) ızgara; 2D'de 4 köşe yerine 3, yönsel
  artefakt daha az, yüksek boyutta belirgin daha hızlı.
- **FBM (fractal Brownian motion)**: aynı gürültünün ölçek katları toplanır. Silüetin "hem büyük
  hem küçük detayı var" hissi buradan gelir.
  ```hlsl
  float fbm(float2 p) {
      float a = 0.5, s = 0.0;
      [unroll] for (int i = 0; i < 5; i++) { s += a * noise(p); p *= 2.0; a *= 0.5; }
      return s;   // lacunarity 2.0, gain 0.5 — standart
  }
  ```
- **Domain warping**: gürültünün girdisini gürültüyle bozmak. Duman ve sıvının o "kıvrılan"
  karakteri neredeyse tamamen bu. `q = fbm(p); r = fbm(p + 4.0 * q);` — iki kat sarma daha da
  kıvrımlı: `fbm(p + fbm(p + fbm(p)))`.

**Prosedürel mü, önceden üretilmiş tiled doku mu — bizim için cevap DOKU.** Üç gerekçe, üçüncüsü
en ağır:

1. **Perf** (en zayıf gerekçe): 5 oktav FBM = ~5 hash + interpolasyon seti / piksel; doku = 1 tap.
   Modern GPU'da ikisi de ucuz, tam ekran olsa bile. Bu tek başına karar vermez.
2. **Sanat kontrolü**: dokuyu Aseprite'ta elle rötuşlayabilirsin, prosedürel gürültüyü ikna
   etmen gerekir.
3. **PIXEL-ART (asıl gerekçe)**: prosedürel gürültü **fragment başına**, yani *pencere pikseli*
   başına değer üretir. Sanat pikseli 4 pencere pikseli olduğu için üretilen gürültü sanat
   ızgarasının altına iner ve pixel-art'ı bozar (B.10.1). Bunu shader'da düzeltmenin yolu
   girdiyi `floor(p / art) * art` ile kuantalamaktır — yani gürültüyü zaten dokuya *benzetmek*.
   Buna karşılık **128×128 bir gürültü dokusu 128 sanat pikselidir**; 4× basıldığında taneleri
   sanat ızgarasına kendiliğinden oturur.

Ek olarak bizim motorda dördüncü bir gerekçe var: gürültü dokusu **bugün sıfır shader** ile
üretilebiliyor (`create_glow_texture` deseni, `render.c:40-73`) ve mevcut `fs_main` ile
çizilebiliyor. Prosedürel gürültü ise **yeni shader** demek.

**Öneri: tek 128×128 tiled gürültü dokusu, dört kanala dört iş paketlenmiş.**
`R` = düşük frekans FBM (büyük siluet) · `G` = orta frekans · `B` = yüksek frekans (tanecik) ·
`A` = ikinci bağımsız tohum (maske/akış için). Tek doku, tek sampler, dört bilgi. CPU'da value
noise + FBM ile ~60 satırda üretilir.

**Tuzak:** doku **döşenebilir (tileable)** olmak zorunda, yoksa dikiş görünür. Value noise'da bu
kolay: hash fonksiyonuna giren ızgara indeksini doku boyutunda modülo al.

**Ve bizde REPEAT sampler YOK** (`r2d.c:548-549`, ikisi de `CLAMP_TO_EDGE`). Kayan gürültü hilesi
sarmaya muhtaç. SDL'de mod mevcut (`SDL_gpu.h:1302`), sadece yaratmıyoruz — ~8 satır.

---

### B.3 Kayan maske + eşikleme — "ateşin canlı görünmesini sağlayan asıl hile bu mu?"

**Kısa cevap: evet, büyük ölçüde.** Alevin dili çizilmiş bir şekil değil, **gezinen bir eşiktir.**
Alevin dilinin yukarı doğru uzayıp kopması, bir dilin doğup diğerinin sönmesi — hiçbiri
animasyonla değil, sabit bir gradyanın üstünde kayan gürültünün bir eşiği geçmesiyle olur.

**Tarif (dört satır, sırayla).**
```hlsl
float4 flame(float2 uv, float t) {
    // 1) YAKIT alanı: alt sıcak (1), üst soğuk (0). İstenirse yanlarda da daralt.
    float fuel = saturate(1.0 - uv.y) * (1.0 - abs(uv.x - 0.5) * 1.6);

    // 2) YUKARI KAYAN gürültü. Kayma miktarı sanat pikseline yuvarlanır (B.1).
    float2 nuv = uv * float2(1.0, 0.7) + float2(0.0, -t * 0.9);
    float  n   = Noise.Sample(NSmp, nuv).r * 0.65
               + Noise.Sample(NSmp, nuv * 2.3 + 0.37).g * 0.35;   // iki oktav

    // 3) EŞİK: yakıt ile gürültünün çarpımı bir "ısı" alanı verir.
    float heat = fuel * n * 2.0;
    if (heat < CUT) discard;                       // silüet burada doğuyor

    // 4) BURN EDGE: iki eşiğin FARKI = eşiğin hemen üstünde ince parlak bant.
    float core = step(CUT + BAND, heat);           // iç gövde
    float edge = 1.0 - core;                       // eşiğe yakın kabuk
    // renk: B.5 — heat'i rampadan geçir, edge'i rampanın en parlak ucuna it
    float  rt  = core ? saturate(heat * 0.5) : 0.98;
    float3 col = Ramp.Sample(RSmp, float2(rt, 0.5)).rgb;
    return float4(col, 1.0);
}
```

**Kanonik hali üç eşik katmanı kullanıyor** (Febucci 2019, F.2 — bu tekniğin referans
uygulaması). Tek eşik yerine aynı gradyanı üç kez, kaydırılmış eşiklerle örneklemek alevin
gövde/orta/uç katmanlarını **bir çizimde** veriyor:
```hlsl
float s1 = step(n, g);          // en dış katman → ALPHA maskesi
float s2 = step(n, g - 0.2);    // orta
float s3 = step(n, g - 0.4);    // iç çekirdek
// s1-s2 = dış kabuk bandı, s2-s3 = orta bant, s3 = çekirdek
float3 col = lerp(brightColor, darkColor, s1 - s2);
col        = lerp(col, midColor, s2 - s3);
```
`0.2` ve `0.4` sabitleri katman kalınlığı; pixel-art'ta bunları rampanın kademe sayısına göre
seçmek gerekiyor (5 renkli rampa → 5 eşik ya da rampa örneklemesi).

**Tarihsel ve kanıtlanmış pixel-art referansı: DOOM (PSX/64) ateşi** (F.2). Mimarisi tam olarak
"tek kanal ısı + 1D palet": framebuffer'da her piksel **0-36 arası tek bir ısı değeri** tutuyor,
palet **37 renk** (beyaz → sarı → turuncu → kırmızı → siyah), alt satır sabit 36 (kaynak),
yayılma `pixel[to] = pixel[from] - (rand & 1)` ve yatay kayma `to = from - W - rand + 1`.
Kırk yıllık ve hâlâ en iyi görünen pixel-art ateşlerden biri; **37 rengin üstüne çıkmak
gerekmediğinin kanıtı.**

**Neden bu kadar iyi çalışıyor.** Üç sebep:
- Eşik **doğrusal olmayan** bir operatör: gürültüdeki yumuşak bir tepe, eşiğin altındayken yok,
  üstündeyken tam var. Yani yumuşak bir alan **keskin ve hareketli bir silüete** dönüşüyor.
  Alevin "dil" karakteri tam olarak bu.
- Kopma (bir dilin ana gövdeden ayrılıp yukarıda sönmesi) bedava geliyor: gürültü tepesi yukarı
  kayarken yakıt alanı düşer, çarpım eşiğin altına iner, dil **sönerek kopar**. Kimse bunu
  animasyonlamıyor.
- **Burn edge** alevin sıcak ucunu veriyor. Bu bandın matematiği "iki smoothstep'in farkı" ve aynı
  matematik **dissolve** (erime/çürüme/ışınlanma) efektinin de tamamı: `CUT`'ı ömür ilerlemesiyle
  sür (`CUT = progress`), gövde silinirken kenarda yanma çizgisi yürür.

**Kaç pass.** Tek pass, tek quad, 2-3 doku örneklemesi. Yeni fragment shader gerekir.

**Pixel-art'ta neyi bozar.** `smoothstep` yumuşak kenar = gradyan = pixel-art'a aykırı. Pixel-art'ta
alev **eşiklerin merdivenidir**: `step` kullan (yukarıdaki gibi), ya da `smoothstep`in çıktısını
rampanın 5-8 kademesine kuantala. İkinci tuzak: gürültünün kayma hızı sanat pikselinden küçük
adımlarla ilerliyorsa alev "akmıyor, kaynıyor" görünür — kaymayı da sanat pikseline yuvarla,
ve *daha yavaş* kaydır (adım başına 1 sanat pikseli, 12-15 Hz).

---

### B.4 Blend modu katmanlama — alevin üstündeki duman neden İKİ blend

**Dört mod, dört fizik.**

| Mod | Formül | Fizik | Ne için |
|---|---|---|---|
| **ADD** (additive) | `dst + src·a` | **emisyon** — ışık ekler, asla karartmaz | alev, kıvılcım, büyü çekirdeği, ışıma, şimşek, damar taşı parıltısı |
| **ALPHA** | `src·a + dst·(1-a)` | **absorpsiyon + saçılma** — arkayı gizler | duman, buhar, sis, toz, sepken, ağ |
| **MULTIPLY** | `src · dst` | **soğurma** — karartır, renk filtreler | gölge, is, zehir lekesi, kirli su, kan |
| **SCREEN** | `1-(1-src)(1-dst)` | additive'in doymayan hali | parlak zeminde ışıma (ADD gibi patlamaz) |

**"Alevin üstüne dumanı koymak neden İKİ AYRI blend?"** Çünkü alev ile duman **zıt fiziksel
olaylardır**:
- Alev bir **ışık kaynağıdır**. Arkasındaki duvarı karartmaz, **aydınlatır**. Additive tam bunu
  yapar: hedefe ekler, hedefin ne olduğuna bakmaz.
- Duman bir **engeldir**. Işık yaymaz, ışığı **keser**. Dumanı additive çizerseniz duman
  *parlar* — gri bir sis yerine ışıyan bir bulut olur, ve gecenin karanlığında beyaza patlar.
  Ters yönde: alevi alpha ile çizerseniz alev arkasındaki her şeyi **siler** ve sahnenin üstüne
  yapıştırılmış ölü bir çıkartma olur (alevin içinden duvarın hafifçe görünmesi, canlılığın
  yarısı).

**"Neden sıra önemli?"** Çünkü iki modun hedefle ilişkisi farklı:
- **ADD toplamadır, komütatiftir.** Üç additive katmanın sırası sonucu değiştirmez.
- **ALPHA hedefi okur.** Yani alpha bir katman, kendinden **önce** çizilmiş her şeyi kısmen
  gizler; kendinden **sonra** çizilecek olanı gizlemez.

Buradan pratik kural: **dumanın alevi kısmen gizlemesini istiyorsan duman SONRA çizilir.** Gerçek
bir ocakta iki iş birden oluyor — kökte alev, tepede duman — ve doğru katmanlama üç çizimdir:
```
1) alev gövdesi        ADD    (kökte, kısa, doygun)
2) alev sıcak ucu      ADD    (daha küçük, daha beyaz, burn edge)
3) duman               ALPHA  (alevin BİTTİĞİ yerden başlar, yukarı doğru genişler ve solar)
```
Dumanın alevle **çakışan** ince bir bandı olması şart: keskin bir sınırla ayrılırsa iki ayrı efekt
gibi okunur; çakışırsa "alev dumana dönüşüyor" okunur. Çakışma bandında duman alpha'sı 0'dan
başlayıp yükselir.

**Bizde durum.** ADD ve ALPHA var (`r2d.h:22-26`). MULTIPLY pipeline'ı **kurulu ama enum'a
kapalı** — yalnız ışık pass'i sentinel ile erişiyor (`r2d.c:447-452`, `r2d.c:1160`); genel çizime
açmak ~20 satır. SCREEN yok. Blend değişimi draw kırılması demek (`r2d.c:820`), yani yukarıdaki
üç katman ≥3 draw call — 2048 bütçede önemsiz.

**Bir not: bizim ADD "alfa ölçekli additive".** `src_color_blendfactor = SRC_ALPHA` her iki modda
da (`r2d.c:438`). Yani ADD'de alpha bir **yoğunluk topuzu** gibi çalışıyor (`dst + src·a`), saf
`dst + src` değil. Sönen parçacıklar için doğru olan tam budur — `alpha_mod` düşürmek katkıyı
kısar. Bunu bilerek kullanmak lazım; "additive'de alpha işe yaramaz" varsayımı bizde YANLIŞ.

**Premultiplied alpha hilesi.** Tek blend state (`ONE, ONE_MINUS_SRC_ALPHA`) ile dokunun alpha
kanalına göre hem additive hem alpha davranışı alınabilir: `rgb>0, a=0` olan pikseller saf
additive eklenir, `a>0` olanlar normal karışır. Tek atlas, tek draw call, alev ve dumanı aynı
sheet'te tutmak. Bizde src factor `SRC_ALPHA` olduğu için premultiplied **değiliz**; geçiş
`make_pipeline`'da tek satır ama **mevcut tüm çizimlerin** alpha davranışını değiştirir → ayrı bir
`R2D_BLEND_PREMUL` modu olarak eklenmeli, mevcut modlar değiştirilmemeli.

---

### B.5 Gradient / ramp doku ile renklendirme (palette LUT)

**Fikir.** Tek kanallı bir skaler (gürültü, ısı, ömür oranı, yükseklik, hasar) → renk. Rampa 1D
bir dokudur; skaler onun `u` koordinatıdır.
```hlsl
float3 col = Ramp.Sample(RSmp, float2(saturate(heat), 0.5)).rgb;   // RSmp = NEAREST
```

**"Pixel-art palet disiplinini korumanın standart yolu bu mu?" — Evet, ve sebebi keskin.** Gürültü
*sürekli* bir değer üretir; onu doğrudan renge çevirmek (örn. `float3(heat, heat*0.5, 0)`) sürekli
bir renk uzayı verir — binlerce ton, yani pixel-art'ın reddettiği şey. Rampa o sürekli değeri
**sizin elle seçtiğiniz N renge** hapseder. Kritik detay: rampa **NEAREST** örneklenirse çıkan
renk sayısı tam olarak rampanın piksel genişliğidir. LINEAR örneklerseniz araları interpolasyonla
doldurur ve disiplini kaybedersiniz. **Rampa dokusu NEAREST, istisnasız.**

**Kaç renk — iki ayrı standart var ve karıştırılmamalı.**
- **Genel VFX standardı 256×1 px** gradyan dokusu (Tobias Noller, UE4 gradient mapping, F.5).
  Yumuşak, sürekli, sinematik. **Pixel-art için yanlış.**
- **Pixel-art disiplini**: rampa genişliği = istenen renk sayısı, **Point/NEAREST filtre, Clamp
  wrap**. Cyanilux'un palet LUT tarifi: N renk için indeksler `255/(N-1)` katları olarak
  saklanır — 5 renk → `0, 63.8, 127.5, 191.3, 255`.
- Referans sayılar: klasik pixel-art paletleri **16** (DB16, PICO-8) ya da **32** renk; **tek
  gölgeleme rampası 5 renk** (highlight×2, highlight, base, shadow, shadow×2); GameBoy **4**;
  DOOM PSX ateşi **37**.
- **Hue-shifting kuralı** (rampa boyunca, pixel-art'ın en temel disiplini): koyu uçta renk
  mavi/mora **soğur** ve doygunluk düşer; parlak uçta sarıya **ısınır**. Sadece parlaklığı
  değiştiren rampa ölü görünür. Bizim ışık pass'inde bunun karşılığı zaten var (altın saat
  turuncusu, mevsim tonları — `main.c:11393-11417`).

Pixel-art için pratik aralık **5-8**; 16'nın üstü palet disiplinini fiilen bırakmak.
Somut rampalar (bizim kanonumuza göre, D bölümünde tekrar kullanılıyor):

| Rampa | Genişlik | Renkler (koyu → parlak) |
|---|---|---|
| Ocak alevi (demirci, kazan altı) | 6 | koyu kızıl · kırmızı · turuncu · amber · sarı · soluk sarı-beyaz |
| Karaocak bacası (is + kor) | 5 | siyah-kahve · koyu is · gri-kızıl · kızıl · turuncu |
| Duman / buhar | 5 | koyu gri-mavi · gri · açık gri · kirli beyaz · beyaz |
| Buz (mahzen teknesi) | 5 | lacivert · mavi · camgöbeği · soluk mavi · beyaz |
| Damar taşı (kanon: "içten ışıyan kristal") | 5 | mor-siyah · mor · menekşe · lila · beyaz-lila |
| Sepken (Karduvar karı) | 4 | gri-mavi · soluk mavi · kirli beyaz · beyaz |

**Kaç pass / maliyet.** Pass yok, tek ek doku örneklemesi. Doku bedavaya üretilir (CPU'da 8×1
piksel). **Ama** shader gerekir.

**Bizde shader'sız yolu var ve ilk yapılacak olan o.** `fx_render` içindeki `if/else` zinciri
(`fx.c:82-102`) zaten elle yazılmış bir rampadır — sadece kodda, dokuda değil. Onu bir diziye
çevirip `t` ile örneklemek ve sonucu `r2d_texture_set_color_mod` ile vermek (`fx.c:104`)
**shader'sız, ~40 satır** ve şunları kazandırır: (a) her yeni efekt için `else if` yazmak biter,
(b) rampa tek yerde durur ve palet disiplini denetlenebilir olur, (c) sanat yönü kod okumadan
rampayı değiştirebilir.

---

### B.6 Parçacık kuvvet alanları

**Fikir.** Parçacığın *nasıl hareket ettiği*, neye benzediğinden daha çok şey anlatır. Dumanın
yükselip savrulması, kıvılcımın düşüp zıplaması, buharın tembelliği, sepkenin yatay savrulması —
hepsi kuvvet alanı farkı.

| Kuvvet | Formül | Ne yapar |
|---|---|---|
| Yerçekimi | `vy += g·dt` | kıvılcım/kan düşer (`g>0`) |
| Kaldırma (buoyancy) | `vy -= b·dt` | duman/buhar/kıvılcım-korları yükselir |
| Sürüklenme (drag) | `v *= exp(-k·dt)` | havada yavaşlama; duman "ağır" olur |
| Türbülans | `v += T(x,y,t)·dt` | dumanın kıvrılması, alevin kararsızlığı |
| Rüzgâr | `v += W·dt` | sepken, yağmurun eğimi, dumanın yatması |
| Zemin sıçraması | `y > y0 → vy = -vy·e` | kıvılcım taşta zıplar (`e≈0.4`) |

**Türbülans nasıl.** En ucuzu iki sinüs: `vx += A·sin(y·f₁ + t·f₂)`. Doğrusu **curl noise**:
2D'de bir skaler potansiyel ψ'nin gradyanının 90° döndürülmüşü — `v = (∂ψ/∂y, −∂ψ/∂x)`. Bu alan
**diverjanssızdır** (divergence-free), yani parçacıklar yapay kaynak/çukurlara toplanmaz, akışkanın
o karakteristik görünümünü verir ve **katı sınırlara tam olarak saygı duyar**. Kaynağı Bridson,
Hourihan, Nordenstam, *"Curl-Noise for Procedural Fluid Flow"*, SIGGRAPH 2007 — PDF ücretsiz
(F.6). Bilinen tuzak: curl noise eklenen parçacıklar gözle görülür şekilde **titreyebiliyor** —
alan genliği konuma göre yumuşatılmalı (kenarda sıfıra inmeli).

**Dumanın yükselişi sabit değil, AZALAN bir eğri.** Unity'nin kendi dokümanının verdiği gerekçe
tam da bu: yükselen hava soğudukça kaldırma kuvveti azalır. Yani `vy -= b·dt` yerine
`vy -= b·(1 - u)·dt` (u = ömür oranı). Sabit kaldırma kuvveti dumanı "füze" gibi gösteriyor.

**Kıvılcımın taşta zıplaması için üç parametre gerekiyor, ikisi değil**: `bounce` (geri dönen hız
oranı — pratikte **0.4 civarı**, daha yükseği parçacıkları "jöle fasulyesi" gibi zıplatıyor),
`dampen` (çarpışmada kaybedilen hız), ve **`minKillSpeed`** (bunun altına düşen parçacık silinir).
Üçüncüsü olmadan kıvılcımlar zeminde sonsuza kadar seğirir.

**"Ne kadarı CPU'da kalmalı?"** — **512-2000 parçacık için tartışma bile yok: hepsi CPU'da.**
- Aritmetik: 2000 parçacık × ~20 float işlemi = 40k işlem/kare. Mikrosaniye ölçeği.
- GPU parçacık simülasyonu (compute shader + ping-pong buffer) yüz binlerde anlam kazanır.
  SDL_GPU'da compute var ama bizim için bugün sıfır getirisi olan üç haftalık iş.
- **CPU'da tutmanın bedava kazancı**: parçacık dünya durumunu okuyabilir. Kıvılcım taşa çarpsın,
  duman duvarda yatsın, buhar tavana yayılsın, sepken evin arkasında birikmesin — bu veri GPU'da
  yok. Kanonda önemli: kıvılcımın **is bırakması** (`decal_*` çağrısı) ancak CPU'da mümkün.
- Godot'un ölçümlü topluluk çalışması aynı yere çıkıyor: GPU parçacıklarının **parçacık
  sayısından bağımsız sabit bir taban maliyeti** var; düşük sayılarda CPU daha iyi başlıyor.

**AMA — gerçek darboğaz simülasyon değil, ÖRTÜŞME (overdraw / fill-rate).** VFX optimizasyon
literatürünün ortak tespiti: parçacık *sayısı* büyük tabloda ihmal edilebilir; performansı yiyen
şey **büyük saydam quad'ların üst üste binmesi** (overdraw = katman sayısı × kaplanan piksel).
Bu bizim için doğrudan bir uyarı: `glowTex` **64×64 ve LINEAR** (`render.c:71`), parçacık boyutu
dünya ölçeğinde büyüyebiliyor, ve 512 parçacığın hepsi tam ekranı kaplayan bir alanda örtüşebilir.
Yani havuzu 2048'e çıkarmadan önce **parçacık boyutunu** ve örtüşmeyi denetlemek gerekir; sayıyı
artırmak bedava değil, *alanı* artırmak pahalı.

**Shader'a taşınacak tek şey görünüm**: rampa, dither, eşik, yumuşak kenar. Simülasyon CPU'da.

**Bizdeki durum ve iki küçük kusur.**
- `Particle` yalnız `vx/vy` taşıyor (`fx.h:23-32`); yerçekimi/rüzgâr/türbülans yok.
- `fx_update`'in sürüklenmesi `p->vx *= (1.0f - 2.0f*dt)` (`fx.c:45-46`) — bu **kare hızına
  bağımlı** (dt=1/30'da 0.933, iki kere dt=1/60'ta 0.935) ve `dt > 0.5` olsa işaret ters
  dönerdi. Doğrusu `SDL_expf(-k*dt)`. Küçük ama gerçek bir kusur.
- Tüm parçacıklar ALPHA çiziliyor (`fx.c:61`). **Kıvılcım ve alev additive olmalı** — bloom
  eşiğini geçen tek şey additive katmandır, yani "muhteşem" hissinin kaynağı burada kapalı
  duruyor.

---

### B.7 Dither / ordered dithering

**Fikir.** 8-bit hedefte yumuşak geçiş **bantlaşır** (görünür kademeler). Dither, kademe sınırını
noktalamaya çevirir. Pixel-art'ta ikinci bir işlevi var ve o daha önemli: dither bir **görsel
dildir** — 8/16-bit dönemi yarı-saydamlığı ve gradyanı böyle yapıyordu, oyuncu bu deseni "o
döneme ait" olarak okuyor.

**Bayer 4×4** (bizde zaten var, `r2d.c:231-234`):
```
 0  8  2 10
12  4 14  6      / 16
 3 11  1  9
15  7 13  5
```

**İki kullanım, farklı işler:**

(a) **Bant kırma** (görünmez): `c += (bayer - 0.5) / 255.0`. Sadece kuantizasyon hatasını dağıtır.
Bizim ışık pass'i bunu yapıyor (`r2d.c:244`).

(b) **Dither'lı saydamlık / kesme** (görünür, istenen): hiç yarı-saydam piksel üretme, saydamlığı
**desenle** ver:
```hlsl
static const float BAYER[16] = { 0, 8, 2,10, 12, 4,14, 6,  3,11, 1, 9, 15, 7,13, 5 };
float bayer4(float2 p) { int2 i = int2(p) & 3; return BAYER[i.y*4 + i.x] / 16.0; }

// duman kenarı ve ışık sönmesi için:
float a = smoothstep(1.0, 0.0, dist);            // sürekli sönüş
if (a < bayer4(pixCoord / ART)) discard;         // ← desene çevir. ART ile böl: desen
                                                  //   SANAT pikselinde, pencere pikselinde değil
clip(a - bayer4(pixCoord / ART));                 // discard'ın HLSL kısayolu
```
`/ ART` kritik: desen pencere pikselinde örneklenirse 4× ölçekte desen görünmez küçük olur ve
sadece bulanıklık üretir. Sanat pikselinde örneklenirse **görünür, kasıtlı, 16-bit** durur.

**Duman ve ışık sönmesi için doğru olan bu.** Yarı-saydam duman "airbrush" gibi durur ve pixel-art
sözleşmesini bozar; dither'lı duman sözleşmeyi korur.

**Kritik tuzak — desenin "crawl" etmesi.** Dither eşiği **ekran uzayında** örneklenirse kamera
hareket ettiğinde desen sabit kalır ve dünya onun altından kayar; sonuç "kirli cam" hissi. Bu
dersi kod zaten öğrenmiş: `dmode==2` dünya-çivili IGN varyantı tam bunun için var
(`r2d.c:239-240`). VFX dither'ı da efektin **yerel** uzayında (sprite uv'si) ya da dünya
koordinatında örneklenmeli.

**Prodüksiyonda kullanılan tam dizi** (Unity Shader Graph'ın Dither node'u, F.7) `(M4 + 1) / 17`:
```hlsl
static const float DITHER[16] = {
     1.0/17.0,  9.0/17.0,  3.0/17.0, 11.0/17.0,
    13.0/17.0,  5.0/17.0, 15.0/17.0,  7.0/17.0,
     4.0/17.0, 12.0/17.0,  2.0/17.0, 10.0/17.0,
    16.0/17.0,  8.0/17.0, 14.0/17.0,  6.0/17.0
};
```
`/16` yerine **`/17`** olmasının sebebi önemli: hiçbir eşik tam 0 ya da tam 1 olmuyor, böylece
alfa 0 tam kapalı, alfa 1 tam açık kalıyor (16'ya bölersen alfa=0 olan piksel bile bir yerde
görünür).

**4×4 mi 8×8 mi.** 4×4 = 16 kademe, 8×8 = **64 kademe**. Kısa geçişlerde (duman kenarı, efekt
sönmesi) 4×4 yeter ve deseni okunur kalır. **Uzun gradyanlarda (ışık düşüşü) 8×8** — bantlaşmayı
belirgin biçimde azaltıyor. Bizim ışık pass'i şu an 4×4 kullanıyor (`r2d.c:231-234`); halka
kenarında bant görülüyorsa doğru hamle 8×8'e çıkmak.

**Ve bir uyarı: "sinek teli" (screen door) etkisi.** Dither hücresi ekranda çok küçük kalırsa
desen bir doku olmaktan çıkıp bulanıklık/kirlilik olur. Pixel-art'ta kural net: **bir dither
hücresi = bir SANAT pikseli** (bizde 4 pencere pikseli). Bunun kodu yukarıdaki `/ ART`.

**IGN vs Bayer.** IGN (`r2d.c:241-242`) daha az desenli, daha "modern"; TAA'lı 3D boru
hatlarında tercih edilir. Pixel-art'ta **desen istenir** ve IGN ekran uzayında tam periyodik
olmadığı için "kirli" okunur — o yüzden VFX için **Bayer 4×4/8×8**, IGN ışık pass'inin bant
kırmasında kalsın.

---

### B.8 Beceri (skill) efekti anatomisi

Bir yetenek efekti tek bir şey değil, **zamanda üç faz × uzayda 6-7 katman**. Gerçek oyunların
yaptığı budur ve tek tek katmanların hiçbiri tek başına etkileyici değildir.

| Faz | Süre | İşİ | Katmanlar |
|---|---|---|---|
| **Telegraph** (hazırlık / anticipation) | 0.15-0.5 sn | *Ne olacağını* ve *nerede olacağını* söylemek. Rakip için kaçış penceresi; oyuncu için "yükleniyor" hissi | yer halkası (zeminde, ışığın ALTINDA) · parçacıkların **merkeze doğru** akması (ters akış — enerji toplanıyor) · sprite'ın hafif geri çekilmesi · efekt ışığının **yükselmesi** · sesin ön-vuruşu |
| **Çıkış** (impact) | 0.05-0.15 sn | Vuruşun *ağırlığını* satmak. Bu faz **kısa olmak zorunda** — uzarsa efekt sulanır | sheet-anim patlaması · **hit-stop** 0.06-0.12 sn · 1-2 kare beyaz flash · kısa+sert sarsıntı · additive çekirdek + **bloom darbesi** · yüksek hızlı kıvılcım fışkırması · efekt ışığı tepe değeri |
| **Kalıntı** (residue) | 0.3-45 sn | Dünyanın *hatırlaması*. Bu katman yoksa efekt "sahnenin üstüne yapıştırılmış çıkartma" olur — `decal.h`'nin açılış yorumu tam bunu söylüyor | duman/buhar yükselmesi (yavaş, ALPHA) · **yer izi** (decal) · sönen efekt ışığı · yavaş düşen kor parçacıkları · post-fx'in nötrale geri gevşemesi |

**Hangi katman hangi işi yapıyor** — bu tablo brief'in en kullanışlı yeri, çünkü bir efekt "düz"
duruyorsa hangi katmanın eksik olduğunu buradan bulursun:

| Katman | Sattığı şey | Yoksa ne olur |
|---|---|---|
| Sheet animasyon | olayın **kimliği** (alev mi, buz mu, kutsama mı) | efekt "genel parlama" olur, hangi beceri olduğu anlaşılmaz |
| Parçacık | olayın **enerjisi ve yönü** | efekt yerinde durur, "savurma" hissi yok |
| Işık | olayın **sahneye girmesi** — duvar aydınlanır, gölge kayar | efekt sahnenin üstünde yüzer (yapıştırılmış çıkartma) |
| Ekran sarsıntısı | olayın **kütlesi** | vuruş hafif kalır, "pat" olur |
| Hit-stop | olayın **okunabilirliği** — donma oyuncuya kareyi görme zamanı verir | vuruş "oldu mu?" belirsiz kalır. Dövüş oyunlarının merkez tekniği |
| Beyaz flash | **temas anı** — tam olarak hangi karede değdi | vuruşun anı bulanıklaşır |
| Post-fx darbesi | olayın **ekranın kendisine** dokunması | efekt "büyük" hissettirmez; ama abartılırsa ucuz durur |
| Yer izi (decal) | olayın **kalıcılığı** | dünya hiçbir şey hatırlamıyor gibi durur |
| Ses | zamanlamanın **omurgası** — görselden 1-2 kare ÖNCE başlar | her şey doğru olsa bile efekt "gecikmiş" gelir |

**Sektörde bu üç faz nasıl adlandırılıyor** (F.8): Stefan Jevremović (Nordeus) *Anticipation /
Fade-In → Climax / Impact → Dissipation / Fade-Out*; Jason Keyser (Riot, VFX Apprentice) beş
parçaya bölüyor: *windup → crescendo → impact → follow-through → dissipation*. GDC 2017'de
Hadidjah Chamberlain (Blizzard) ve Jason Keyser'in **"Visual Effects Bootcamp: Artistic
Principles of VFX"** oturumu League of Legends ve Hearthstone vaka analizleriyle bu ilkeleri
işliyor (slaytların tam içeriği doğrulanamadı — PDF indirilip okunmalı).

**Somut sayılar (kaynaklı, F.8):**
- **Telegraph süresi:** Jevremović'in Spellsouls örneğinde ayarlanabilir aralık **0.5-2 sn**;
  spell başına ortalama **3 animasyon** (en karmaşığı 5).
- **Hit-stop:** Street Fighter IV — hafif ≈ **9 kare**, orta ≈ **11**, ağır ≈ **13**, mermi
  sabit **8**. Street Fighter II'de hepsi ≈ **14**. Celeste'in dash'i **15 kare**, bunun
  **3'ü donma**. Sakurai'nin Smash notu: süre hasarla artar ama **bir üst sınır (cap) var**,
  mermilerde azaltılır, karakter bazında modifiye edilir (Ryu'nun saldırıları "Street Fighter
  hissi" için belirgin şekilde artırılmış). Bizde `hitstop(0.09f)` = **~5.4 kare @60fps** ve
  parry `0.12f` = **~7 kare** (`main.c:396`, `main.c:8997`) — yani biz SF ölçeğinin **altındayız**
  ve bu bilinçli olabilir (aksiyon RPG dövüş oyunu değil), ama ağır vuruşlarda 0.13-0.15'e
  çıkmak masasında.
- **Ekran sarsıntısı — matematiği var.** Squirrel Eiserloh, *"Math for Game Programmers: Juicing
  Your Cameras With Math"*, GDC 2016: **trauma** 0..1 birikimli değer, olay başına **+0.2 ya da
  +0.5**, zamanla lineer azalır, ve **sarsıntı = trauma² (ya da trauma³)**. Sayısal gerekçe:
  trauma 0.30 / 0.60 / 0.90 → sarsıntı ≈ **%3 / %22 / %73** — algısal olarak doğru tırmanma bunu
  gerektiriyor. 2D'de **ötelemeli + döndürmeli birlikte** (3D'de yalnız döndürmeli). Ve:
  *"Use Perlin noise for shakes"* — rastgele yerine gürültü, çünkü duraklatma/yavaşlatmayla
  uyumlu ve tekrarlanabilir. Yumuşatma: `x += (target - x) * 0.1`.
  **Bizde sarsıntı lineer sönüyor** (`k = shakeTime / shakeDur`, `main.c:10466`) — `k*k` yapmak
  tek karakter değişikliği ve Eiserloh'un tam olarak önerdiği şey.

**Bizde hazır olanlar:** sheet anim (`animfx_*`) · efekt ışığı (`animfx_spawn_lit`) · parçacık ·
sarsıntı (`main.c:10463`) · hit-stop (`main.c:330-335`) · hit flash (`mob.c:1124-1133`) ·
decal (`decal.c`) · ses. **Yani anatominin 8/9'u kodda.**

**Eksik olan tek katman: post-fx darbesi.** Ve maliyeti şaşırtıcı derecede düşük:
`r2d_postfx_set` her kare çağrılabilir (`r2d.c:1345`), yani filtre değerleri zamanla sürülebilir.
Bugün preset'ler statik (`main.c:1075-1122`). Vuruş anında `bloom` 0.30 → 0.75 sıçratıp 0.25 sn'de
geri indirmek, `aberration`'ı bir an açmak, `heat`i patlamanın yanında yükseltmek — **~15-25 satır
main.c**, sıfır shader, sıfır asset. Kâr/masraf oranı en yüksek tek iş.

---

### B.9 Rim light / iç ışıma (inner glow)

**Fikir.** "İçten ışıyan" malzeme (buz, damar taşı, kor, hayalet) iki işaretle satılır: (a) kenarın
gövdeden **daha parlak** olması, (b) ışımanın **nabız** atması (sabit ışıma boyalı durur).

Üç uygulama, artan maliyetle:

1. **Elle boyanmış emissive maske — asset, SHADER YOK.** Sprite'ın yanında ikinci bir PNG: yalnız
   ışıyan pikseller, kalanı şeffaf. Bu maske `R2D_BLEND_ADD` ile sprite'ın üstüne çizilir,
   `alpha_mod` nabızla modüle edilir. **Bugün, bu motorda, sıfır yeni kod ile çalışır** —
   `main.c:11311-11327`'deki yankı soluğu tam bu deseni kullanıyor.
2. **Kontur (outline) shader'ı.** Komşu 4 texel'in alpha'sına bak, kenarı bul, kenarı boya:
   `float n = max4(a(uv±texel)); float rim = n * (1 - a(uv));`. Yeni fs + texel boyutu uniform'u
   gerekir. Değişken kalınlıkta kontur, tüm sprite'lar için tek kod.
3. **Sprite'a bağlı nokta ışığı.** `light_append_point` (`light.h:83`) ile damar taşı **dünyaya
   ışık yayar**. Kanonun istediği asıl şey bu: "içten ışıyan kristal" karanlıkta *çevresini*
   aydınlatmalı, yoksa ışıyan değil sadece açık renkli olur. **Bugün mümkün.**

**Dördüncü ve bizim için en kullanışlı varyant: PALET EŞİĞİYLE ışıma.** Ekincan Taş'ın
"Glow Color Mask" tarifi (F.9): sprite'ın **belirli palet renkleri** ışır — "R/G/B değeri bir
eşiğin üstündeki pikseller custom renkte parlar". Pixel-art'ta bu çok güçlü, çünkü palet zaten
sonlu: damar taşının lila'sını ışıyan renk ilan edersen **her sprite'ta, ayrı maske PNG'si
üretmeden** o renk ışır. Kılıca damga basılınca damganın lila pikselleri kendiliğinden parlar.
Maliyeti: yeni fs + eşik uniform'u; asset maliyeti **sıfır**.

**Öneri: 1 + 3 birlikte; 2 gereksiz; 4 damar taşı için ciddi aday.** 2'nin (kontur shader'ı)
gereksiz olmasının sebebi: pixel-art'ta kontur zaten sanatçı tarafından çizilir; shader'ın
hesapladığı kontur sanatçının çizdiğiyle yarışır ve genelde kaybeder. Godot'un yaygın 2D rim
light shader'ı bunu alpha taramasıyla yapıyor ve `accuracy` parametresi **4-256 arası** — yani
piksel başına onlarca örnekleme; pixel-art ızgarasına da oturmuyor. Uzak dur.

**Üretimde doğrulanmış iki ayrı yol** (F.9) — ikisi de meşru, birini seçmek gerekiyor:
- **Dead Cells (Motion Twin):** normal map'ler **elle Photoshop'ta çiziliyor** ("probably one of
  the only videogame studios to draw our normal maps… by hand"). Her animasyon karesi kendi
  normal map'iyle export ediliyor, basit bir toon shader hacmi render ediyor. Palet stratejisi:
  **grayscale doku + gradient map** — Promenade bölgesini turuncu-ateş paletinden maviye sadece
  gradient map değiştirerek taşımışlar. Yani B.5'in rampa tekniği **tüm sanat pipeline'ının
  temeli** olabiliyor.
- **Children of Morta (Dead Mage):** pixel animasyonlar HD'ye ~4× büyütülüyor, **ışık
  pixelize EDİLMİYOR** — yüksek çözünürlüklü dinamik ışık. Geliştiricinin kendi benzetmesi:
  ***"It's like a puppet show. The puppets are pixelated but we shine them with real lights."***
- Ters kutup: **Blasphemous** — %100 elle çizilmiş, piksel piksel, shader yok.

**Bizim durumumuz bu eksende nerede:** ışık pass'imizde `gridPx` **var ama varsayılan 0**
(`r2d.c:951`), yani şu an "Children of Morta yolu" (ışık pixelize değil). Bu bir karar ve
GDD'ye yazılması gerekiyor — çünkü VFX'in tamamı bu karara uymak zorunda.

**Pixel-art'ta neyi bozar.** Additive ışıma katmanının kenarı yumuşaksa (glow dokusu gibi) sprite'ın
keskin kenarıyla çatışır. Çözüm: ışıma maskesi de **pixel-art olarak çizilmeli** ve NEAREST
örneklenmeli; yumuşak halo ancak *ayrı ve büyük* bir katman olarak (sprite'ın etrafındaki hava)
kullanılabilir.

---

### B.10 Pixel-art'a özgü tuzaklar

**1. "Shader'da yumuşak gradyan pixel-art'ı bozuyor mu?" — Evet, ama sebebi "yumuşaklık" değil
ÖLÇEK.** Sorun gradyanın var olması değil, gradyanın **sanat pikselinden küçük adımlarla**
değişmesi. Bizde 1 sanat pikseli = 4 pencere pikseli. Gradyan o 4 pencere pikselinde 4 farklı renk
gösteriyorsa oyuncu sanat ızgarasının altında detay görür ve görüntü "pixel-art + airbrush"
melezine düşer.

**Çözüm — her shader çıktısı sanat ızgarasına kuantalanır.** Bunun bu projede yazılmış örneği var:
ışık pass'inin `gridPx` parametresi ışığı sanat pikselinin **merkezinden** örnekliyor
(`r2d.h:98-99`, `r2d.c:135`):
```
"    if (grid > 0.5) sp = floor(sp / grid) * grid + grid * 0.5;\n"
```
Her yeni VFX shader'ı bu satırı taşımak zorunda. HLSL karşılığı:
```hlsl
float2 snapArt(float2 p, float art) { return floor(p / art) * art + art * 0.5; }
```

**2. Nearest sampling + render target ölçeği.** İdeal mimari: dünya **sanat çözünürlüğünde** bir
target'a çizilir, post-fx orada çalışır, sonra **tam sayı** ölçekle ekrana basılır. Bizde
`sceneTex` **pencere boyutunda** (`r2d.c:1217`) ve post-fx onu NEAREST örnekliyor (`r2d.c:1238`).
Bugünkü bedeli görünür değil (UV kaydıran tek filtre `heat` ve o da nadir kullanılıyor) **ama her
yeni UV bozması bu kusuru büyütür.** GDD §6.2 zaten borç olarak yazmış ("düşük çözünürlüklü
lightmap — 1 texel = 1 sanat pikseli").

Referans çözünürlükler (F.10) ve bizim yerimiz:

| Oyun | İç çözünürlük | Ölçek |
|---|---|---|
| Celeste | **320×180** | 6× → 1920×1080 (karo 8×8) |
| Owlboy | **640×360** | 2×/3×/4× |
| Hyper Light Drifter | ~**240p** | — |
| Dead Cells | karakterler oyun içinde ~50 px | — |
| **BİZ** | fiilen **winW/4 × winH/4** (1920×1080'de **480×270**) | 4× (16 px karo → `TILE_SIZE 64`) |

Yani bizim efektif sanat çözünürlüğümüz Celeste ile Owlboy arasında ve tam sayı ölçekte —
**mimari sağlam**, sadece post-fx'in o ızgarayı bilmesi gerekiyor.

**Dürüst bir maliyet notu** (F.10, Camarlinghi): UE4'te pixel-art üzerine çalışan bir geliştirici
tam bu adımı — "native çözünürlükte render et, sonra upscale et" — **denedi ve vazgeçti**, çünkü
post-processing efektleriyle sorun çıkardı ve motor seviyesinde değişiklik gerektirdi. Yerine
vertex shader'da ızgaraya snap + kamerayı `GridSnap` ile yuvarlama yolunu seçti. Unity'nin Pixel
Perfect Camera'sında ise benzer bir karşıtlık kurumsallaşmış: **"Upscale Render Texture" ile
"Pixel Snapping" birlikte kullanılamıyor** — biri seçilir. Yani bu, "doğrusu belli, sadece yapın"
denecek bir iş değil; iki meşru yol var ve **bizim için ucuz olanı ikincisi** (snap), çünkü
post-fx zincirini hiç bozmuyor.

**3. Kamera zoom'unda parçacık boyutu.** `camera.zoom` şu an sabit `1.0` (`main.c:1725`) ve
parçacık boyutu zoom ile çarpılıyor (`fx.c:74`). Zoom **tam sayı** kalmalı (1, 2, 3). Kesirli zoom
pixel-art'ta yalnız parçacığı değil **her sprite'ı** bozar, o yüzden bu kural VFX'ten önce genel
bir karardır.

**4. Sub-pixel titreme (jitter) — mevcut, VFX'ten bağımsız bir kusur.** Dünya→ekran dönüşümü
hiçbir yerde yuvarlanmıyor: `(p->x - camera->x) * zoom` (`fx.c:77`, aynı desen `animfx_render`,
`decal_render`, `mob_render`, `player_render`'da). Kamera kesirli konumdaysa her sprite kesirli
piksele oturur; NEAREST örnekleme onu bir tarafa yuvarlar ve nesneler **birbirine göre titrer**.
Klasik çözüm: kamera konumunu sanat pikseline yuvarla —
`camera.x = floorf(camera.x / ART) * ART;` — `camera_update` (`render.c:17-36`) sonunda tek satır.
Bu, VFX eklendikçe **daha görünür** olacak bir borç.

**5. VFX kendi sanat yoğunluğunda üretilmeli.** `assets/fx` sheet'leri 32-100 px kare
(`main.c:1298-1340`), zemin karosu 16 px. Yani mevcut efektler zeminden 2-6× **daha ince** taneli.
Bu bilinçli bir tercih olabilir ("efekt başka bir düzlemden gelir") ama tesadüf olmamalı. Tek
kural yazılmalı: *yeni efekt sanatı 16 px ızgarada çizilir ve 4× basılır.* Prosedürel dokular
(gürültü, rampa, glow) için aynı kural: 64-128 px doku, NEAREST, 4× basılır.

Camarlinghi'nin (F.10) üç kuralı bunun disiplinli hali ve üçüncüsü tam bu madde:
*"sprite'ları asla ölçekleme, hepsini aynı referans ızgarasında üret."* İkinci kuralı da bizi
ilgilendiriyor: **sprite döndürme kısıtlanmalı** — NEAREST filtrelenmiş bir dokuyu döndürmek
aliasing üretir; çözüm elle çizilmiş yön varyantları (bizde slash yayları zaten dört yön için
ayrı sheet, `main.c:1309-1313` — doğru yapılmış).

Ve dikdörtgen sprite'larda pikselleştirme ölçeğini tutturmanın formülü (Real Time VFX, F.10):
**UV tiling X = sprite genişliği ÷ sprite yüksekliği, tiling Y = 1.0, pikselleştirme ölçeği =
sprite yüksekliği.** Yoksa 64×80 bir sheet'te pikseller dikeyde oturur yatayda oturmaz.

**Bantlaşma (banding) kuralı.** Büyük bir gradyanda piksel sayısı palet renk sayısını aştıkça
bantlaşma kötüleşir; buna karşılık keskin bir kavis üstündeki küçük bir parlama en fazla palet
kadar piksel kaplar, yani güvenlidir. Pratik sonuç: **ışık/efekt gradyanları ya dikkatle
dither'lanır (B.7) ya da paletin taşıyabileceği aralıkta tutulur.** Yumuşak bir ışık gradyanı,
altındaki keskin pikselleri yıkar.

---

## (C) İKİ ÖRNEK VAKA, ADIM ADIM

### C.1 "SU OLMADAN SU" — Tuzkapı limanı, köy göleti, vaha

**Katman 0 — kabul: su bir yüzey değil, BEŞ AYRI İŞARETİN toplamı.** Oyuncu suyu şunlardan
tanıyor ve her biri ayrı bir hile:
1. altındaki/yanındaki şey **kaymış** görünür (kırılma),
2. kayma **zamanla dalgalanır** (bu olmadan kırılma bir doku hatası gibi durur),
3. yüzey ışığı **yansıtır** (yansıma + parıltı noktaları),
4. kıyıda bir **köpük/dalga çizgisi** vardır,
5. yüzeyde bir şey **akar** (yaprak, köpük, kütük).

Beşinin de birlikte olması gerekmiyor; **üçü yeter.** Ama hangi üçü olduğu önemli: 1+2 olmadan
diğerleri "hareketli doku" gibi durur.

**Top-down'a özgü kritik fark.** 3D'de suyun *altında* bir şey vardır ve kırılma onu büker.
Bizde su bir **zemin karosudur**; altında bir şey yok. Yani bizim bozacağımız şey (a) suyun kendi
dokusu ve (b) suya düşen **yansımalar**. Bu haber iyi: **sahne target'ını okumaya gerek yok**,
yani C.1'in tamamı bugünkü motorla, shader'sız yapılabilir.

---

**Katman 1 — HAZIR ASSET'İ KULLAN (0 shader, ~15 satır).** Asset paketi animasyonlu su karolarını
**zaten getirmiş ve biz kullanmıyoruz**:
```
assets/tiles/Water/Water_Middle.png          ← KULLANILAN (statik)
assets/tiles/Water/Water_Middle_Anim_1.png   ← KULLANILMIYOR
assets/tiles/Water/Water_Middle_Anim_2.png   ← KULLANILMIYOR
assets/tiles/Water/Water_Tile_1..4_Anim.png  ← KULLANILMIYOR
assets/tiles/Water/Water_Foam_Animation.png  ← KULLANILMIYOR
assets/tiles/Waterfall/Waterfall_1..8.png    ← KULLANILMIYOR
```
`tilemap.c:12` statik `Water_Middle.png`'yi, `tilemap.c:260` statik `Water_Tile_1.png`'yi
bağlıyor. `wld_water_pass` (`tilemap.c:463-489`) zamana bağlı hiçbir şey yapmıyor.
**Zaman tabanlı kare indeksi eklemek ~15 satır** ve "su olmadan su"nun **ilk %40'ı**.
*Bu maddenin faturası neredeyse sıfır ve en yüksek getirili tek iş.*

**Katman 2 — ŞERİT UV KAYDIRMA (0 shader, ~40 satır).** Kırılma + dalgalanma. Su bölgesi
**4 pencere pikseli (= 1 sanat pikseli) yükseklikte yatay şeritler** halinde çizilir; her şeridin
kaynak `u`'su sinüsle kaydırılır ve kaydırma sanat pikseline yuvarlanır:
```c
// tilemap.c — su karosu çizimi yerine, karo başına ART yükseklikte şeritler
const float ART = 4.0f;                      // 1 sanat pikseli = 4 pencere px
for (float sy = 0; sy < TILE_SIZE; sy += ART) {
    float wy   = tileWorldY + sy;
    float ph   = wy * 0.16f + t * 2.1f;                     // dalga fazı
    float offs = SDL_roundf(SDL_sinf(ph) * AMP_PX) * ART;    // AMP_PX = 1..2 sanat px
    SDL_FRect src = { srcX + offs / ART, srcY + sy / ART * 1.0f, 16.0f, 1.0f };
    SDL_FRect dst = { dstX,              dstY + sy,             TILE_SIZE, ART };
    r2d_draw_texture(renderer, waterTex, &src, &dst);
}
```
Bu, 8/16-bit donanımının **satır kaydırma (line scroll / HDMA)** hilesinin birebir aynısı ve o
dönemin su/sıcak hava efektleri böyle yapılıyordu. Bütçe: 64 px karo ÷ 4 px = **16 quad/karo**;
ekranda 30×17 karo görünürse ve bunların diyelim %15'i su ise ≈ **1200 quad** — 16384 bütçesinin
%7'si. Rahat.

**Katman 3 — YANSIMA (0 shader, ~50 satır).** İskele direği, tekne, meşale, ev: sprite dikey
**flip** edilir, alpha 60-90'a düşürülür, ve Katman 2'nin **aynı ofsetiyle** kaydırılır.
Motorumuzda bunun tam aleti var: `r2d_draw_texture_quad` (`r2d.h:199`) serbest 4 köşeli quad
alıyor, yani flip + shear bedava (gölge izdüşümü için yazılmış). Kırpma sorunu (stencil yok):
yansıma **yalnız su karosu olan satırlara** çizilir — kıyı zaten karo hizalı olduğu için karo
çözünürlüğünde kırpma göze yeterli.

**Katman 4 — PARILTI (0 shader, ~30 satır).** Eşiklenmiş gürültü yerine **parçacık**: su
bölgesinde rastgele konumda, **1-2 sanat pikseli** boyutunda, 0.25-0.4 sn ömürlü, ADD blend beyaz
nokta. Kanon buraya bir kural koyuyor: Tuzkapı'nın **feneri krallıktaki en yüksek yapı**
(LORE §4) ve iskelede **on dokuz meşale** yanıyor — yani parıltı rastgele dağılmamalı, **ışık
kaynaklarının yakınında yoğunlaşmalı.** Bu bedava: ışık listesi CPU'da hazır (`light_collect`,
`light.h:74`), parçacık doğurma olasılığı en yakın ışığın `light_atten`'i ile ölçeklenir.
Gece iskelede su parıldar, ıssız gölette parıldamaz — ve kimse bunu oyuncuya söylemez.

**Katman 5 — KIYI KÖPÜĞÜ (0 shader, ~25 satır).** `Water_Foam_Animation.png` + dual-grid maskesi
**zaten hesaplanıyor** (`WLD_WATER_LUT`, `tilemap.c:455-461`): hangi karonun hangi kenarının kıyı
olduğu biliniyor. Köpük o kenarlara, animasyonlu, çizilir.

**Katman 6 — AKAN DÖKÜNTÜ (0 shader, ~30 satır).** Yaprak/köpük/kütük sprite'ları akış yönünde
ilerler. Vlachos'un uyarısı burada geçerli (B.1): **akan RENK için faz aralığının zirvesi sıfır
bozmada olmalı**, yoksa yaprak yanlış yerde görünür. Bizde parçacık CPU'da olduğu için bu sorun
kendiliğinden yok — yaprağın konumu gerçek konumu.

**Katman 7 — OPSİYONEL SHADER.** Yukarıdaki yedisi bittikten sonra hâlâ eksik olan tek şey
**yumuşak, sürekli akış** (flow map). Tek fs ile: su karosu dokusu + gürültü dokusu, iki oktav
DUDV, sanat pikseline snap, rampadan renk. Bu, Katman 2'yi *değiştirir*, ona *eklenmez*.

**Fatura tablosu:**

| Katman | Getirisi | Shader | Yeni doku | Satır | Bugün mümkün? |
|---|---|---|---|---|---|
| 1 Animasyonlu karo | %40 | — | — (asset **var**) | ~15 | **EVET** |
| 2 Şerit UV kaydırma | %25 | — | — | ~40 | **EVET** |
| 3 Yansıma | %15 | — | — | ~50 | **EVET** |
| 4 Parıltı | %10 | — | — | ~30 | **EVET** |
| 5 Kıyı köpüğü | %5 | — | — (asset **var**) | ~25 | **EVET** |
| 6 Akan döküntü | %5 | — | küçük sprite | ~30 | **EVET** |
| 7 Flow map shader | cila | 1 fs | gürültü + rampa | ~150 | taşımadan sonra |

**Isı dalgası (heat haze) aynı ailenin ikinci üyesi.** Katman 2'nin **dikey** ve **maskeli** hali:
ocağın/kazanın/bacanın hemen üstündeki 2-6 sanat pikseli yükseklikte bir bant, sahne target'ından
kaydırılmış UV ile örneklenir. **Burada sahne target'ı GEREKİYOR** ve o hazır (A.3): post-fx'in
`sceneTex`'i (`r2d.c:1238`). İki yol: (a) `fs_postfx`'e maske eklemek, (b) `sceneTex`'i şerit
quad'larla geri çizmek — (b) bugünkü mimariyle çalışır, (a) daha temiz.

---

### C.2 "ALEVİN ÜSTÜNDE DUMAN" — demircinin ocağı, aşçının kazanı, kamp ateşi

Bu vakanın cevabı tek bir cümlede: **alev ile duman iki farklı fiziksel olay olduğu için iki
farklı blend modu, iki farklı hareket eğrisi ve iki farklı boyut eğrisi ister; ve dumanın alevi
kısmen gizlemesi gerektiği için duman SONRA çizilir.**

**Katman 0 — IŞIK, ÖNCE.** Ocak bir ışık kaynağıdır ve bu bizim oyunda **mekaniktir**
(LORE §1.5 "eşik = ışığın bittiği yer"). `light_append_point(..., LK_HEARTH)` (`light.h:83`) —
hazır. Kanon karakteri de belirliyor: LORE §1.9 *"seni koruyan şey durur, korumayan gezinir"* →
ocak **kor değil ateş**, yani kaotik ve kırmızıya doygun titreşim (`LK_HEARTH`, `light.h:19`).
Bu katman olmadan alev sahnenin üstünde yüzer.

**Katman 1 — ALEV GÖVDESİ.** `ADD` blend. İki seçenek:
- **Bugün:** `assets/fx/flame_small.png` (64×72, kullanılmıyor) sheet olarak `animfx_spawn_lit`
  ile — 0 shader.
- **Sonra:** B.3'ün eşikli gürültüsü, 6 renkli ocak rampasıyla. Avantajı: her ocak farklı görünür,
  sheet tekrarı yok, boyut serbest.

Kanon rampası (D.2'de tekrar): koyu kızıl · kırmızı · turuncu · amber · sarı · soluk sarı-beyaz.

**Katman 2 — SICAK UÇ / BURN EDGE.** `ADD`, Katman 1'in **içinde ve daha küçük**, daha beyaz.
B.3'ün ikinci eşiği (`step(n, g-0.4)`). Alevin "sıcak" okunması bu bandın işi; onsuz alev
turuncu bir leke.

**Katman 3 — KIVILCIM.** `ADD` (bugün **ALPHA** — `fx.c:61`, düzeltilecek). Kuvvetler: doğuşta
kaldırma (sıcak hava fışkırtır), sonra yerçekimi baskın; `bounce ≈ 0.4`, `dampen`, ve
**`minKillSpeed`** (yoksa zeminde sonsuza kadar seğirirler). Ömür 0.4-1.2 sn, boyut 1-2 sanat
pikseli. Rampa: kor (beyaz-sarı → turuncu → koyu kızıl → siyah).

**Katman 4 — DUMAN.** `ALPHA`, **Katman 1'den SONRA çizilir.** Beş ayrıntı, hepsi alevden farklı
olmak zorunda:
1. **Nerede başlar:** alevin **bittiği** yerde. Ama keskin bir sınırla değil — 2-4 sanat pikseli
   **çakışma bandında** duman alpha'sı 0'dan yükselir. Keskin sınır iki ayrı efekt gibi okunur;
   çakışma "alev dumana dönüşüyor" okunur.
2. **Boyut eğrisi TERS:** alev ömür boyunca **küçülür**, duman **büyür**. Bizim `fx_render`
   herkesi küçültüyor (`fx.c:74`, `0.3 + t*0.7`) — dumana ayrı eğri gerekiyor. Bu tek ayrım
   okunabilirliğin yarısı.
3. **Kaldırma kuvveti AZALAN eğri** (B.6): yükselen hava soğur. Sabit kaldırma dumanı füze yapar.
4. **Türbülans + rüzgâr:** kıvrılma curl noise (ya da iki sinüs); rüzgâr `weather_*`'dan gelir.
5. **Kenar dither'lı** (B.7, Bayer 4×4, `/ ART`): yarı-saydam duman airbrush gibi durur,
   dither'lı duman 16-bit durur.

Rampa: koyu gri-mavi · gri · açık gri · kirli beyaz (4-5 renk).

**Katman 5 — ISI BÜKÜLMESİ.** Alevin **hemen üstünde**, 2-6 sanat pikseli, dikey. C.1'in ısı
dalgası paragrafı. Bu katman az ve seyrek kullanılmalı — her ocakta olursa göz yorulur; demirci
ocağında ve Karaocak bacasında olsun, mumda olmasın.

**Katman 6 — İS LEKESİ.** `decal_scorch` var (`decal.h:44`). İs için doğru mod **MULTIPLY**:
is taşı boyamaz, **karartır**. Karaocak kanonu tam bunu istiyor.

**Ve burada kodda bir kırılganlık buldum:** `decal_render` (`decal.c:74-99`) ve `shadow.c`
**hiç `r2d_set_blend` çağırmıyor** — o anda hangi mod aktifse onu miras alıyorlar. Pratikte ALPHA
çıkıyor (kare `r2d_begin_frame`'de ALPHA'ya sıfırlanıyor, `r2d.c:1193`, ve tilemap modu
değiştirmiyor), yani bugün *doğru çalışıyor ama tesadüfen*. Çizim sırasına bir additive katman
eklendiği anda yer izleri sessizce additive olur — yani ateş efekti eklemek **yanık lekesini
bozar** ve sebebi bulunmaz. Düzeltmesi tek satır: her iki render fonksiyonu kendi blend'ini
açıkça set etsin.

**Katman 7 — BLOOM.** Zaten var; eşik (0.70-0.76) alev çekirdeğinin altında kalıyor, yani
kendiliğinden çalışıyor. Sahne/HUD ayrımı yapıldığı için eşik **düşürülebilir** (A.2.3).

**Çizim sırası ve blend — özet tablo (sıra bağlayıcı):**

| # | Katman | Blend | Neden bu sırada |
|---|---|---|---|
| 0 | ışık pass'i katkısı | MULTIPLY (kare sonunda) | ışık ayrı pass, ocak listeye eklenir |
| 1 | alev gövdesi | **ADD** | emisyon; hedefi karartmaz, sıradan bağımsız |
| 2 | sıcak uç | **ADD** | aynı sebep |
| 3 | kıvılcım | **ADD** | aynı sebep |
| 4 | **duman** | **ALPHA** | **hedefi OKUR** → alevi kısmen gizlemesi için alevden SONRA |
| 5 | ısı bükülmesi | sahne örnekleme | dumanın da bükülmesi istenirse en son |
| 6 | is lekesi | **MULTIPLY** | zeminde, en altta (kare başında decal geçişinde) |

Sıranın önemi sayısal olarak kanıtlanabilir: aynı iki yarı-saydam katmanı ters sırada karıştırmak
farklı sonuç veriyor (Microsoft'un klasik örneği: %50 gri + %50 mavi → mavi kanalı **0.875** ya da
**0.75**, hangisinin önce çizildiğine göre — F.4). Additive'de böyle bir fark yok çünkü toplama
komütatif ve hedef faktörü `ONE`.

**Kanon bağı — Karaocak bacası, tek satırda iki efekt.** LORE §4: *"bacaları **gündüz göğü
kirletir, gece kırmızı boyar**."* Bu cümle bize iki farklı katman dizilimi söylüyor:
- **Gündüz:** duman baskın, ALPHA gri, geniş, yavaş; ambient'i düşürür (bölgeye özel
  `weather_darkness01` benzeri bir katkı ya da post-fx doygunluk düşüşü).
- **Gece:** duman **altından aydınlanır**. Bunun bedava yolu: duman parçacığının rengi **doğuş
  anında** en yakın ışığın rengiyle karıştırılır (ışık listesi CPU'da hazır). Ocağa yakın doğan
  duman kızıl, yükseldikçe griye döner. Yani tek bir renk-lerp satırı, kanonun cümlesini
  ekranda gerçekleştiriyor.

---

## (D) BİZİM OYUNA ÖZEL ÖNERİ LİSTESİ

Kural (GDD §15 refleksiyle aynı): **efekt süs değil, kanondan çıkar.** Aşağıdaki her satırın bir
kanon dayanağı var ve efekt o cümlenin ekranda görünür halidir. "Hazır" kolonu bugünkü motoru
gösteriyor: **●** tamamen bugün yapılabilir · **◐** kısmen (bir altyapı kalemi eksik) · **○**
shader gerekiyor.

### D.1 Yenitaç — iskele, kazan, ocak

| Efekt | Kanon dayanağı | Trick yığını (alttan üste) | Fatura | Hazır |
|---|---|---|---|---|
| **Aşçının kazanı — buhar** | `kadro-yenitac-slotlar.md:228`: *"gece kandiller onun kazanının çevresinde en yoğun"* · akşam kazan kurulur (`:347`) | ① `LK_CANDLE` küçük ışık (altındaki ateş) ② buhar parçacığı: ALPHA, **azalan** kaldırma, drag, boyut ömürle **büyür**, dither'lı kenar, 4 renkli soluk rampa ③ 2 sanat px ısı bükülmesi (opsiyonel) ④ kazan ağzında dönen yavaş salınım | Parçacık kuvvetleri + boyut eğrisi + blend seçimi (~90 satır `fx.*`) | ◐ |
| **Demircinin ocağı — kıvılcım + is** | `kadro-kanon-raporu.md:38`: *"Mahir'in körüğü"* · Karaocak iş adı **Tav** (`kadro-kartlari-sehir.md:256`) | ① `LK_HEARTH` ışık ② alev gövdesi ADD (`flame_small.png` var) ③ sıcak uç ADD ④ kıvılcım ADD + yerçekimi + bounce 0.4 + minKillSpeed ⑤ is lekesi MULTIPLY (`decal_scorch`) ⑥ **körük vuruşunda**: kıvılcım patlaması + ışık sıçraması + kısa sarsıntı | C.2'nin 1-6 katmanı; ① ⑤ hariç hepsi parçacık işi | ◐ |
| **İskelede on dokuz meşale** | `kadro-kartlari-sehir.md:30`: *"On dokuz meşale yaktım. **Yirmincisi burçtan sonrası, oraya kimse para vermiyor.**"* | ① `LK_TORCH` ışık ×19, `lamp_*` deseniyle ② her meşalede küçük alev sheet'i ③ **yirminci meşalenin OLMAMASI** — kandil hattı orada biter, karanlık başlar ④ su varsa yansıma + parıltı (C.1 K3/K4) | ①② bugün hazır · ③ **kod değil TASARIM**: ışık dizisinin bittiği yer kanonun cümlesi | ● |
| **Yenitaç gecesi — takımyıldız** | LORE §4: *"Geceleri işçi meşaleleri surda **takımyıldız gibi** diziliyor."* | ① sur boyunca ışık dizisi ② her ışıkta bağımsız faz (hepsi aynı anda titremesin — `light_collect` bunu yapıyor) ③ uzak meşaleler için **yalnız parlak nokta** (ışık yarıçapı küçük, yoğunluk yüksek) — "takımyıldız" tam olarak bu ④ bloom eşiğinin üstünde kalsınlar | Işık pass'i hazır; iş **yerleştirme** ve yarıçap/yoğunluk ayarı | ● |
| **Beş damga VFX** | GDD §6.3 Karar 4: FLAME=Ocak · ICE=Buz (donmuş göl) · POISON=Ağı · VENOM=Kısıklar (**ışıksız**) · PINK=Diri Damga (**sonlu**) | Her biri C.2'nin şablonu, farklı rampa + farklı kuvvet: FLAME yükselir/ADD · ICE **ağır düşer**/ADD+rim · POISON kabarcık yükselir/ALPHA · VENOM **aşağı damlar**/ALPHA, **ışık YAYMAZ** (kanon: Kısıklar ışıksız) · PINK ADD + bloom darbesi, çok kısa ömür | Rampa sistemi (~40 satır) + kuvvet alanları (~60) + blend seçimi (~25) | ◐ |

**VENOM'un ışık yaymaması bir kanon kuralıdır, bir eksiklik değil.** `light_collect` silah efekti
ışığını `WeaponFxType`'a göre veriyor (`light.h:75`) — VENOM'un orada **sıfır** olması Kısıklar'ın
ışıksızlığının mekanik karşılığı. Bu, "efekt kanondan çıkar" ilkesinin en temiz örneği ve
kaybedilmemesi gerekiyor.

### D.2 Karaocak — döküm şehri

| Efekt | Kanon dayanağı | Trick yığını | Fatura | Hazır |
|---|---|---|---|---|
| **Bacalar** | LORE §4: *"bacaları **gündüz göğü kirletir, gece kırmızı boyar**"* | **Gündüz:** ① geniş ALPHA duman sütunu, yavaş, azalan kaldırma ② bölgeye özel doygunluk düşüşü + hafif sarı-gri warmth (post-fx `saturation`/`warmth`, **zaten var**) ③ zeminde ince is örtüsü. **Gece:** ④ duman parçacığı doğuş anında **en yakın ışığın rengiyle karışır** → kökte kızıl, tepede gri ⑤ baca ağzında `LK_HEARTH` ışık ⑥ bloom | ④ tek `lerp` satırı; ② post-fx bölgeye göre sürülmeli (bugün preset global) | ◐ |
| **Ocak / döküm istasyonu** | *"Krallığın çeliği buradan çıkar"* · Sancar **kendi damgasını basıyor** | C.2'nin tamamı + döküm anında: beyaz-sıcak akıntı (ADD, çok doygun), ısı bükülmesi, bloom darbesi, hit-stop'suz ama **sarsıntı** | C.2 + post-fx darbesi (~25 satır) | ◐ |
| **Galeriler / damar kazısı** | LORE §9: damar taşı *"içten ışıyan kristal"* · kazdığın **gece mezarlık kalkar** | ① damar taşı emissive maske (ADD + nabız) ② `light_append_point` mor ışık ③ kazma vuruşunda: toz parçacığı (ALPHA, düşer) + kıvılcım (ADD) + **damar taşı kırılınca ışığın SÖNMESİ** ④ kazı bittiğinde o gece mezarlık — efektin *bedeli* mekanikte | ① ② bugün hazır (`main.c:11311-11327` deseni) · ③ parçacık | ● |

### D.3 Tuzkapı, Karduvar, Kumkapı — bölge imzaları

| Efekt | Kanon dayanağı | Trick yığını | Fatura | Hazır |
|---|---|---|---|---|
| **Tuzkapı limanı — su** | LORE §4: *"Limanı zincirle kapanır; **feneri krallıktaki en yüksek yapı**."* | C.1'in yedi katmanı. Fener buraya iki şey ekliyor: ① dönen ışık huzmesi (dar aniso + dönen faz — `R2DLight.aniso/wobC/wobS` **zaten var**, `r2d.h:42-49`) ② huzme suya değdiğinde parıltı yoğunluğu artar (C.1 K4'ün ışık bağı) | C.1 K1-K6 hepsi shader'sız (~190 satır); fener huzmesi ışık pass'i parametresi | ● |
| **Karduvar — sepken** | `kadro-isim-havuzu.md:185`: *"**Sepken** *(sepken = savrulan kar)*"* · LORE §4: *"kışın kendini savunuyor"* | ① mevcut kar taneleri (`main.c:11523-11548`) → **yatay rüzgâr kuvveti** + kesme (shear) ② rüzgâr **darbeli** olmalı: sabit rüzgâr sepken değil, "savrulma" ani ve düzensizdir (gürültüyle sürülen esinti) ③ zemin birikmesi **zaten var** (`weather_snow_cover`) ④ nesnelerin rüzgâr altı tarafında birikme (kar gölgesi) ⑤ ağızdan çıkan soluk: küçük ALPHA buhar, oyuncu ve NPC'lerde, **yalnız soğukta** | ①② mevcut kar döngüsüne kuvvet eklemek (~40 satır) · ④ dekor sorgusu · ⑤ parçacık | ◐ |
| **Mahzenin buz teknesi** | `kadro-yenitac-slotlar.md:397`: *"mahzene buz taşımak 6 [akçe] — tam bir günlük yevmiye"* | ① soğuk buharı: ALPHA, **AŞAĞI akan** (soğuk hava iner — sıcak buharın tersi, bu ayrım tek başına "soğuk" okutur) ② tekne kenarında kırağı decal (`decal_frost` **var**) ③ ağızdan soluk ④ buz bloklarında rim light (emissive maske + `LK_MAGIC`'e yakın soğuk ışık) ⑤ ortam sesi | ① kuvvet yönü tersi (bedava) · ④ maske asset'i | ◐ |
| **Kumkapı — kum** | LORE §4: *"Kervan şehri; vaha, klif mezarları, çakal başlı taş muhafızlar"* | ① serap: **yatay** ısı bükülmesi, ufuk hizasında güçlü, yakında sıfır (C.1 ısı paragrafı, `heat`in `depth` ağırlığının **tersi**) ② kum savrulması: yer hizasında yatay ALPHA parçacık ② ③ rüzgârla gelen kum perdesi (hava durumu ailesine yeni üye) ④ vaha suyu: C.1 ama **dingin** (dalga genliği yarı) — çöl suyunun durgun olması anlatı taşıyor | ① `fs_postfx`'e ikinci bir heat modu (~15 satır MSL/HLSL) · ②③ parçacık | ◐ |

### D.4 Hava sisteminin görsel borcu (mevcut kodun düzeltmesi)

`main.c:11494-11566`'daki hava görselleri **ışık pass'inden sonra** çiziliyor ve hepsi düz renk
dikdörtgen. Üç somut iyileştirme, hiçbiri shader gerektirmiyor:

| Bugün | Sorun | Öneri |
|---|---|---|
| Sis = iki düz gri tam-ekran quad (`main.c:11552-11564`) | Gece hâlâ **parlak gri**; kandil halkasının kenarını okumayı zorlaştırıyor (LORE §1.5 mekanik) | ① sisi ışık pass'inden **önce** çiz, ya da alpha'yı `1 - dark01` ile ölçekle ② düz renk yerine **kayan gürültü dokusu** iki katman, farklı hız (B.2 dokusu aynı doku) — sis "çöker" ve "sürüklenir" |
| Kar tanesi = düz kare (`main.c:11546`) | Sepken savrulmuyor, düşüyor | D.3'teki rüzgâr kuvveti + kesme |
| Yağmur = iki dikey çubuk (`main.c:11518-11521`) | Zemine **çarpma** yok | Zemine değince küçük ALPHA sıçrama parçacığı + ıslak parlaklık (kısa ömürlü decal) — "yağmur yağıyor"dan "yağmur *bir yere* yağıyor"a geçiş |

---

## (E) UYGULAMA SIRASI VE GEREKÇESİ

### ADIM 0 — SHADER TAŞINABİLİRLİĞİ (her şeyden önce)

**Ne:** 4 MSL shader'ı HLSL'e taşı, `shadercross` ile offline üç format üret,
`SDL_GetGPUShaderFormats()` ile seç (A2 bölümünün tamamı). Doğrulama: `src/r2d_test.c`'yi
Windows'ta koştur.

**Neden sıfırıncı adım — aritmetik:**
- Bugün **4** shader var, ~250 satır, ve %90'ı mekanik çeviri: **~1 günlük iş.**
- Bu brief'in önerdiği VFX shader'ları: eşikli alev, yerel bozma/maske, palet ışıma, dither'lı
  duman, flow map suyu, gürültü örnekleyici → **6-10 shader daha.**
- Taşımayı sonraya bırakmak iki fatura çıkarıyor: (a) çevrilecek shader sayısı **3.5×**,
  (b) o shader'ların hepsi **iki kez yazılmak zorunda** kalır (MSL'de yazılıp sonra HLSL'e
  çevrilir) ya da **Windows'ta hiç çalışmayan** bir VFX kütüphanesi birikir.
- Ve daha sinsi bir üçüncü fatura: bu brief'in **B.2 kararı** (prosedürel gürültü değil, önceden
  üretilmiş doku) gerekçelerinden birini taşınabilirlikten alıyor — hash tabanlı gürültü farklı
  GPU'larda farklı sonuç verir. Yani hangi mimariyi seçeceğimiz taşımanın *sonucuna* bağlı.
  Sırayı ters çevirmek yanlış mimari seçmek demek.

**Ayrıca:** taşıma bittiğinde `r2d.c`'nin shader yükleme yolu dosya tabanlı olacak — yani yeni bir
VFX shader'ı eklemek "dosya yaz + `compile.sh` koştur" olacak, bugünkü "C string'ine kaçış
karakteriyle satır ekle" değil. **Taşıma, VFX işini de kolaylaştırıyor.**

### ADIM 1 — SHADER'SIZ BEDAVA KAZANÇLAR (taşımayla paralel yürüyebilir)

Sırası getiri/maliyet oranına göre. Hiçbiri shader'a dokunmuyor, yani Adım 0 ile çakışmaz:

1. **Animasyonlu su + kıyı köpüğü karolarını bağla** (~40 satır) — asset **zaten paketin içinde**,
   kullanılmıyor. Tek kalemde en yüksek görsel getiri.
2. **Parçacıkta blend seçimi: kıvılcım ve alev ADD** (~25 satır) — bloom'un görünmesinin şartı;
   bugün her şey ALPHA (`fx.c:61`).
3. **Parçacık rampası** (~40 satır) — `fx.c:82-102`'deki `if/else` zincirini rampa dizisine çevir.
   Palet disiplini tek yerde durur.
4. **Parçacık kuvvetleri**: yerçekimi, azalan kaldırma, doğru drag (`exp(-k·dt)`), rüzgâr,
   basit türbülans, bounce + minKillSpeed, **boyut eğrisi ömre göre (duman büyür)** (~90 satır).
5. **Post-fx darbesi** (~25 satır) — vuruşta `bloom` 0.30→0.75, 0.25 sn'de geri. Anatominin
   eksik tek katmanı (B.8) ve en yüksek "muhteşemlik/satır" oranı.
6. **Sarsıntı `k*k`** (1 satır) ve **hit-stop'un ağır vuruşta 0.13'e çıkması** (birkaç satır) —
   Eiserloh ve SF ölçeği (B.8).
7. **`bloomThreshold`'ı düşür** (preset başına 1 sayı) — HUD gerekçesi artık geçersiz (A.2.3).
8. **Kamerayı sanat pikseline yuvarla** (1 satır, `render.c:36` sonrası) — sub-piksel titremesi
   (B.10.4). VFX eklendikçe daha görünür olacak mevcut bir borç.
9. **MULTIPLY blend'i genel çizime aç** (~20 satır) — is ve zehir lekesi için doğru mod; pipeline
   zaten kurulu.
10. **Yansıma + parıltı + akan döküntü** (C.1 K3/K4/K6, ~110 satır).
11. **Hava görsellerinin üç düzeltmesi** (D.4).

**Bu on bir kalemin toplamı ~400 satır ve tek satır shader içermiyor.** Brief'in tarif ettiği
görsel sıçramanın tahminen **yarısı** bu adımda geliyor.

### ADIM 2 — VFX SHADER ALTYAPISI (taşımadan sonra)

1. **REPEAT sampler** (~8 satır) — kayan gürültünün şartı.
2. **Gürültü + rampa dokularının CPU üreticisi** (~100 satır, `render.c` deseni): 128×128 tiled
   value-noise FBM, 4 kanala 4 oktav; rampa üreticisi (N renk, hue-shift'li).
3. **`R2DDraw`'a `pipelineId` + uniform payload; çoklu doku bağlama** (~150 satır `r2d.c`) —
   sprite başına özel shader yolunun tamamı bu. Bir kez yapılır, sonraki her efekt bedava.

### ADIM 3 — İLK VFX SHADER: EŞİKLİ ALEV

Neden ilk: (a) en yüksek görsel getiri, (b) B.3'ün kanonik tarifi hazır ve doğrulanmış
(Febucci + DOOM), (c) çıktısı beş damganın (FLAME/ICE/POISON/VENOM/PINK) **hepsinde** yeniden
kullanılıyor — sadece rampa ve kuvvetler değişiyor, (d) Adım 2'nin altyapısını tek başına sınıyor
(iki doku + uniform + eşik + rampa).

### ADIM 4 — YEREL BOZMA (bozma tamponu)

`fs_postfx`'e maske/tampon girdisi (B.1 mimari notu). Çıktısı: yerel ısı dalgası (ocak, baca,
kazan, serap), gerçek su kırılması, büyü kalkanı, "gerçeklik çatlıyor". Bir shader, dört efekt.

### ADIM 5 — CURL TÜRBÜLANS + DITHER'LI DUMAN

Adım 1.4'ün üstüne curl noise (B.6) ve duman kenarında Bayer dither (B.7). Duman bu adımdan
sonra "16-bit" görünür.

### ADIM 6 — PALET IŞIMA (damar taşı)

B.9'un dördüncü varyantı: belirli palet renkleri ışır. Asset maliyeti sıfır, kanonun büyü
maddesini ("içten ışıyan kristal") tüm oyunda tek kuralla görünür kılıyor.

**Bir uyarı, sıraya dair:** Adım 1 bittikten sonra **dur ve bak.** Bu brief 400 satırlık
shader'sız bir liste veriyor ve o listenin ne kadarını gördüğünüzü bilmeden shader yazmak,
bilmediğiniz bir problemi çözmek olur. GDD §6.1'in kalıcı görsel imza kararı da hâlâ açık —
imza seçilmeden 10 shader yazmak, seçilen imzaya uymayan 10 shader yazmak riski taşıyor.

---

## (F) KAYNAKLAR

Hepsi doğrulandı (URL açıldı ya da yerel kaynakta okundu). Bulunamayanlar en sonda ayrıca
listelendi.

### F.0 Yerel / birincil (bizim depomuzda)

- `vendor/SDL/include/SDL3/SDL_gpu.h` — `SDL_CreateGPUShader` doküman bloğu (satır 2698-2737):
  SPIR-V descriptor set / HLSL `spaceN` / MSL `[[...]]` eşlemesi ve TEXCOORD semantik kuralı.
  Satır 269-287: koordinat sistemi normalizasyonu. Satır 288-297: stage başına 4 uniform slotu.
  Satır 1302-1304: sampler adres modları (REPEAT dahil).
- `vendor/SDL` @ **3.4.8** (`d9d5536`, 2026-05-01).
- Bizim kod: `src/r2d.c`, `src/r2d.h`, `src/fx.c`, `src/fx.h`, `src/light.h`, `src/decal.c`,
  `src/render.c`, `src/tilemap.c`, `src/weather.c`, `src/main.c`.
- Kanon: `docs/LORE.md` §1.5/§1.9/§4/§9 · `docs/GDD.md` §6.1/§6.2/§6.3 ·
  `docs/konsept/kadro-kartlari-sehir.md` · `kadro-yenitac-slotlar.md` · `kadro-isim-havuzu.md` ·
  `kadro-kanon-raporu.md`.

### F.1 UV bozma, screen-space refraction, flow map

- **Alex Vlachos, "Water Flow in Portal 2", SIGGRAPH 2010** (kurs: *Advances in Real-Time
  Rendering in 3D Graphics and Games*) —
  `https://cdn.akamai.steamstatic.com/apps/valve/2010/siggraph2010_vlachos_waterflow.pdf` ·
  kurs sayfası `https://advances.realtimerendering.com/s2010/index.html`.
  **Slaytlarda HLSL kodu yok** (görsel sunum); algoritma mantığı yazılı: sanatçı-yazımı flow map,
  iki katman yarım faz kaydırmalı, pulsing'i gürültüyle kırma, akış hızının normal gücünü
  ölçeklemesi, renk/döküntü için faz aralığının sıfır-bozmada zirve yapması.
  İlham verdiği çalışma slaytta anılıyor: Nelson Max & Barry Becker, *"Flow visualization using
  moving textures"*.
- **Graphics Runner, "Animating Water Using Flow Maps" (2010)** —
  `http://graphicsrunner.blogspot.com/2010/08/water-using-flow-maps.html` — Vlachos'un
  matematiğinin çalışan HLSL hali (faz, cycleOffset, iki katman, `flowLerp`).
- **Catlike Coding, "Texture Distortion" (Flow serisi)** —
  `https://catlikecoding.com/unity/tutorials/flow/texture-distortion/` — en temiz türetme,
  `FlowUVW()` fonksiyonu, `flowOffset`/`jump`/`tiling` parametrelerinin gerekçeleri.
- **Kyle Halladay, "Screen Space Distortion and a Sci-fi Shield Effect" (2016)** —
  `https://kylehalladay.com/blog/tutorial/2016/01/15/Screen-Space-Distortion.html` — **bozma
  tamponu** mimarisi (nötr 0.5, N efekt tek okuma).
- **Harry Alisavakis, "My take on shaders: Grab pass distortion"** —
  `https://halisavakis.com/my-take-on-shaders-grab-pass-distortion/` — maske × slider × vertex
  alpha çarpan zinciri.
- **SFML Wiki, HeatHazeShader** — `https://github.com/SFML/SFML/wiki/Source:-HeatHazeShader` —
  tam GLSL; `(1 - t)` ile yükseklikle sönümleme.
- **Godot resmî: Screen-reading shaders** —
  `https://docs.godotengine.org/en/stable/tutorials/shaders/screen-reading_shaders.html` —
  **üst üste binen ekran-okuyan shader'ların birbirini görmemesi** kısıtı.
- **Cyanilux, "2D Water Shader Breakdown"** —
  `https://www.cyanilux.com/tutorials/2d-water-shader-breakdown/` — iki seamless gürültü,
  `-0.5` merkezleme, **bozmayı yalnız X eksenine sınırlama**, mesh UV yerine world position.
- **Polycount Wiki, DuDv map** — `http://wiki.polycount.com/wiki/DuDv_map`.
- Godot Shaders örnekleri: `2d-water-distortion-effect-godot-4`,
  `yoshis-island-shimmer-heat-haze-distortion`, `pixel-art-water`.

### F.2 Eşikli alev / dissolve / burn edge

- **Febucci, "Unity Fire Shader Tutorial: Procedural Flames" (Mayıs 2019)** —
  `https://blog.febucci.com/2019/05/fire-shader/` — **bu tekniğin referans uygulaması**:
  kaydırılan gürültü + statik dikey gradyan + üç `step` katmanı (`g`, `g-0.2`, `g-0.4`).
  Godot portu (tam kod): `https://godotshaders.com/shader/2d-fire/`.
- **Cyanilux, "Dissolve Shader Breakdown"** —
  `https://www.cyanilux.com/tutorials/dissolve-shader-breakdown/` — iki `Step`'in **çıkarılması**
  = kenar bandı; varsayılan `edgeWidth` **0.05**.
- **Godot Shaders, "2D dissolve with burn edge"** —
  `https://godotshaders.com/shader/2d-dissolve-with-burn-edge/` — iki `smoothstep`
  (`threshold` = alfa, `border` = kenar rengi) tam kod.
- **Kyle Halladay, "A Burning Paper Shader" (2015)** —
  `https://kylehalladay.com/blog/tutorial/2015/11/10/Dissolve-Shader-Redux.html` — kenarı **rampa
  dokusundan** renklendirme (F.5 ile köprü).
- **Daniel Ilett, "Dissolve Effect in Shader Graph and URP"** —
  `https://danielilett.com/2020-04-15-tut5-4-urp-dissolve/` — `smoothstep` yerine **sert `Step`**
  varyantı (pixel-art için doğru referans).
- **Fabien Sanglard, "How DOOM fire was done"** — `https://fabiensanglard.net/doom_fire_psx/` ·
  kod `https://github.com/fabiensanglard/DoomFirePSX` — tek kanal ısı 0-36, **37 renk** palet,
  yayılma + yatay kayma. Pixel-art ateşin tarihsel kanıtı.
- **Real Time VFX, "How are these fire & smoke effects made?"** —
  `https://realtimevfx.com/t/how-are-these-fire-smoke-effects-made/11922` — *"an erosion with
  varying smoothstep (hard edged for fire sphere, soft for smoke)"* → **ateşe sert eşik, dumana
  yumuşak**.

### F.3 Pixel-art snapping / kuantalama

- **Handmade Network, "Pixel art fragment shader"** —
  `https://handmade.network/forums/t/7883-pixel_art_fragment_shader` — türev-farkında texel-merkez
  snap (`uv_iq`), ve kuantalanmamış UV'nin ürettiği hatalar (bulanık kenar, komşu karolardan arka
  plan sızması).
- **ohsat, "Pixel-Perfect 2D Water Shader" (HaxeFlixel)** —
  `https://www.ohsat.com/tutorial/flixel/pixel-perfect-water-shader/` — bozmayı **piksel
  biriminde** ifade etme (genlik = tam 1 piksel) + shader'ı native çözünürlükte çalıştırma.
- Godot Shaders: `pixel-quantization` (zoom < 1'de otomatik ölçekleme),
  `texel-space-quantization` (`dFdx/dFdy` ile texel merkezine ekstrapolasyon).
- **Lea Hayes, "Pixel perfection in 2D game projects"** —
  `https://leahayes.co.uk/2015/03/02/pixel-perfection-in-2d-game-projects.html`.

### F.4 Blend modları ve sıra

- **Microsoft (arşiv), Etay Rien, "Alpha Blending, Part 2" (2006)** —
  `https://learn.microsoft.com/en-us/archive/blogs/etayrien/alpha-blending-part-2` — **sıranın
  önemli olduğunun sayısal ispatı** (mavi kanal 0.875 vs 0.75) ve *"clearly, for standard alpha
  blending, order matters even if depth write is turned off"*. Ayrıca 2.5D örneği: opak → arka
  plan transparanlar (**alevler**) → oyun alanı → ön plan (**duman**).
- **Khronos, OpenGL Wiki "Blending"** — `https://wikis.khronos.org/opengl/Blending` ·
  `glBlendFunc` referansı
  `https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glBlendFunc.xml` — `dst` faktörü
  `ONE` olduğu için additive **karartamaz**, toplama komütatif olduğu için **sırasızdır**.
- **Shawn Hargreaves, "Premultiplied alpha"** —
  `https://shawnhargreaves.com/blog/premultiplied-alpha.html` — *"a superset of both conventional
  and additive blending"*; kendi örneği tam bizim vakamız: *"particle systems that want to smoothly
  transition from additive glowing sparks to dark pieces of soot as the particles age"*.
- **Real Time VFX, "Trying to grasp textures + blend modes additive/alpha blended"** —
  `https://realtimevfx.com/t/trying-to-grasp-textures-blend-modes-additive-alpha-blended/20914` —
  Andreas Glad ve Torbach'ın açıklamaları; additive'de siyah zemin nötr, alpha-blend'de artefakt.
  Uyarı (bgolus, aynı forum): Unity'nin hazır premultiply shader'ı **bozuk** — `rgb *= a`
  yalnız rgb'ye uygulanmalı.
- GDC oturumları (başlık + yıl doğrulandı): *Shader Sauce: How to Use Shaders to Create Stylized
  VFX* (Veer Sharma, **GDC 2021**) · *VFX Tricks that Might Save Your Life* (Bruno Afonseca,
  **GDC 2025**) · *How to (Not) Create Textures for VFX* (Simon Trümpler, **GDC 2022**, ücretsiz;
  slayt + proje `gdc2022.simonschreibt.de`).
- Simon Trümpler'in ilgili yazıları: *Stylized VFX in RIME*, *The Divine Fire*,
  *Volumetrically lit Smoke* (Company of Heroes), *Flamethrower* —
  `https://simonschreibt.de/game-art-tricks/`.

### F.5 Rampa / palet LUT

- **Tobias Noller, "Gradient Mapping for VFX – Unreal Engine 4"** —
  `https://tobiasnoller.com/articles/ue4-gradient-mapping-vfx/` — genel VFX standardı **256×1 px**
  gradyan; grayscale maskelerin kanal-paketlemesi; UE 4.20+ Curve Atlas alternatifi.
- **Cyanilux, "Swapping Colours"** — `https://www.cyanilux.com/tutorials/color-swap/` — pixel-art
  palet LUT'u: **width × 1, Point filtre, Clamp**, N renk için indeks `255/(N-1)` katları
  (5 renk → 0, 63.8, 127.5, 191.3, 255).
- **Lospec Palette List** — `https://lospec.com/palette-list` — 4000+ palet; DB16/PICO-8 = 16,
  GameBoy = 4, tek gölgeleme rampası = 5 renk pratiği.
- **Yanrishatum, "Cheap pixel-art color swap based on LUTs"** —
  `https://gist.github.com/Yanrishatum/86794e9e663a7e343f9ef66e8b0f38ae` — tam RGB LUT'un maliyeti
  ve pixel-art'ta neden gereksiz olduğu.
- **NullTale, LutLight2D** — `https://github.com/NullTale/LutLight2D` — "her rengin kendi
  gölgeleme gradyanı olsun, tıpkı pixel-art paletlerinde elle yapıldığı gibi".

### F.6 Gürültü, FBM, domain warping, parçacık kuvvetleri

- **Inigo Quilez, "fBM"** — `https://iquilezles.org/articles/fbm/` — `pow()`suz verimli hal,
  Hurst üsteli H, `G = 2^(-H)`, doğal olaylar için **H = 1 / G = 0.5**.
- **Inigo Quilez, "Domain warping"** — `https://iquilezles.org/articles/warp/` — birinci ve
  ikinci derece sarma, tam kod.
- **The Book of Shaders** — Ch 11 Noise `https://thebookofshaders.com/11/` (value vs Perlin:
  *"Value Noise interpolates random values, while Gradient Noise interpolates random gradients"*;
  simplex'in 2^N → N+1 nokta kazancı) · Ch 12 Cellular/Worley
  `https://thebookofshaders.com/12/` · Ch 13 FBM `https://thebookofshaders.com/13/`
  (oktav/lacunarity/gain tanımları, turbulence kodu).
- **Stefan Gustavson, "Simplex noise demystified" (2005)** —
  `https://cgvr.cs.uni-bremen.de/teaching/cg_literatur/simplexnoise.pdf` — F2/G2 skew sabitleri.
- **GM Shaders (Xor), "Noise 3"** — `https://mini.gmshaders.com/p/noise3` — prosedürel vs doku
  karşılaştırma tablosu; kritik madde: prosedürel gürültü *"inconsistent across devices and
  hardware"*. Döşenebilir value noise kodu.
- **GPU Gems 2, Ch. 26, "Implementing Improved Perlin Noise"** —
  `https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-26-implementing-improved-perlin-noise`
  — prosedürelin maliyeti ve *"current practice… is to prerender materials generated with software
  shaders that use Perlin Noise"*.
- **Bridson, Hourihan, Nordenstam, "Curl-Noise for Procedural Fluid Flow", SIGGRAPH 2007**
  (ACM TOG 26(3), Art. 46, DOI 10.1145/1275808.1276435) — **ücretsiz PDF**
  `https://www.cs.ubc.ca/~rbridson/docs/bridson-siggraph2007-curlnoise.pdf` — diverjanssız
  türbülans alanı, katı sınırlara saygı, uzayda modüle edilebilir genlik.
- **Unity, Force over Lifetime / Particle physics forces** —
  `https://docs.unity3d.com/6000.3/Documentation/Manual/particle-physics-forces.html` — dumanın
  kaldırma kuvvetinin **azalan eğri** olması gerektiğinin gerekçesi.
- **Unity, Collision module** —
  `https://docs.unity3d.com/6000.3/Documentation/Manual/PartSysCollisionModule.html` —
  `Dampen` / `Bounce` / `Lifetime Loss` / **`Min Kill Speed`**.
- **Godot, Particle systems (2D)** —
  `https://docs.godotengine.org/en/stable/tutorials/2d/particle_systems_2d.html` — CPU vs GPU
  parçacık tercihi. Ölçümlü topluluk çalışması: GPU parçacıklarının **sabit taban maliyeti** —
  `https://forum.godotengine.org/t/cpu-vs-gpu-particles-2d-performance-study.../67850`.

### F.7 Dither

- **Wikipedia, "Ordered dithering"** — `https://en.wikipedia.org/wiki/Ordered_dithering` —
  M2/M4/M8 matrisleri, normalizasyon, özyinelemeli inşa, `c' = nearest(c + r·(M − 1/2))`.
- **Unity Shader Graph, Dither node** —
  `https://docs.unity3d.com/Packages/com.unity.shadergraph@6.9/manual/Dither-Node.html` —
  prodüksiyon dizisi `(M4 + 1) / 17` ve `In - threshold` → `clip()` kalıbı.
- **Daniel Ilett, "Transparency Dithering in Shader Graph and URP"** —
  `https://danielilett.com/2020-04-19-tut5-5-urp-dither-transparency/` — 4×4 (16 kademe) → 8×8
  (**64 kademe**) geçişinin **falloff bantlaşmasını azalttığı** açık ifadesi.
- **Aeris, "Dithered Shading Tutorial" (The BKPT)** —
  `https://medium.com/the-bkpt/dithered-shading-tutorial-29f57d06ac39` — merkezlenmiş matris ve
  **"screen door effect"** uyarısı (hücre çok küçükse sinek teli).
- **Meta Horizon, "Shader Snippets for Efficient 2D Dithering"** —
  `https://developers.meta.com/horizon/blog/tech-note-shader-snippets-for-efficient-2d-dithering/`
  — duman gibi parçacıklarda eşik altı fragment'leri **discard** etmenin fill-rate kazancı.
- **Jorge Jimenez, "Next Generation Post Processing in Call of Duty: Advanced Warfare",
  SIGGRAPH 2014** — `https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/`
  — IGN'in kaynağı; sabitler `(0.06711056, 0.00583715, 52.9829189)`, karakteri *"half way between
  dithered and random"*. (Bizde `r2d.c:241-242`.)
- **hughsk, glsl-dither** — `https://github.com/hughsk/glsl-dither` — 2×2/4×4/8×8 varyantları.
- **Bart Wronski, "Dithering part three – real world 2D quantization dithering"** —
  `https://bartwronski.com/2016/10/30/dithering-part-three-real-world-2d-quantization-dithering/`.

### F.8 Beceri efekti anatomisi, game feel, hit-stop, sarsıntı

- **GDC 2017, "Visual Effects Bootcamp: Artistic Principles of VFX"** — Hadidjah Chamberlain
  (Blizzard) & Jason Keyser (Riot) —
  `https://www.gdcvault.com/play/1023943/Visual-Effects-Bootcamp-Artistic-Principles` · slayt PDF
  `https://media.gdcvault.com/gdc2017/Presentations/Chamberlin_Keyser_ArtisticPrinciplesVFX.pdf`
  (LoL + Hearthstone vaka analizleri; **slaytların tam içeriği doğrulanamadı**, PDF büyük).
- **Squirrel Eiserloh, "Math for Game Programmers: Juicing Your Cameras With Math", GDC 2016** —
  `https://gdcvault.com/play/1023146/Math-for-Game-Programmers-Juicing` · video
  `https://www.youtube.com/watch?v=tu-Qe66AvtY` · slayt OCR
  `https://archive.org/stream/GDC2016Eiserloh/GDC2016-Eiserloh_djvu.txt` — **trauma sistemi**,
  `shake = trauma²`, 2D'de öteleme+döndürme, *"use Perlin noise for shakes"*,
  `x += (target-x)*0.1`.
- **Stefan Jevremović (Nordeus), "VFX Staples: Shape, Color, and Motion"** —
  `https://80.lv/articles/vfx-staples-shape-color-and-motion` — üç faz + somut sayılar
  (anticipation 0.5-2 sn, spell başına ortalama 3 animasyon).
- **Jason Keyser (VFX Apprentice), "The Soul of Effects: What is Timing in VFX"** —
  `https://www.vfxapprentice.com/blog/the-soul-of-effects-what-is-timing-in-vfx` —
  windup → crescendo → impact → follow-through → dissipation.
- **Atif, "Step by Step VFX Breakdown — Pulse Cannon Effect" (Real Time VFX)** —
  `https://realtimevfx.com/t/step-by-step-vfx-breakdown-pulse-cannon-effect/3161` — **18
  emitter**'ın üç faza dağılımı; tek efektin gerçek katman dökümü.
- **Maj, "Impact Freeze" (sonichurricane.com)** — `https://sonichurricane.com/?p=1043` —
  SF4: hafif **9** / orta **11** / ağır **13** kare, mermi **8**; SF2 ≈ **14**; UMK3'te yok.
- **Celia Wagar, "Hitstop/Hitfreeze/Hitlag/Hitpause" (CritPoints, 2017)** —
  `https://critpoints.net/2017/05/17/hitstophitfreezehitlaghitpausehitshit/` — üç işlev,
  cancel penceresinin genişlemesi, hitspark'ların rolü.
- **Masahiro Sakurai, Famitsu vol. 490-1** (çeviri: Source Gaming) —
  `https://sourcegaming.info/2015/11/11/thoughts-on-hitstop-sakurais-famitsu-column-vol-490-1/`
  — hasarla artan süre + **üst sınır**, mermilerde azaltma, karakter modifier'ları.
- **Celeste teknik wiki** — `https://celeste.ink/wiki/Tech` — dash **15 kare**, **3'ü donma**.
- **"Juice it or lose it" — Martin Jonasson & Petri Purho, GDC Europe 2012** —
  `https://www.gdcvault.com/play/1016487/Juice-It-or-Lose`.
- **"The Art of Screenshake" — Jan Willem Nijman (Vlambeer), 2013** —
  `https://www.youtube.com/watch?v=AJdEqssNZ-U` · arşiv
  `https://archive.org/details/the-art-of-screenshake` — ~31 kalemin ekleme sırası
  (sayısal değer vermiyor).
- **Steve Swink, *Game Feel* (2008)** — Bölüm 1 PDF
  `http://mycours.es/gamedesign2014/files/2014/10/Game-Feel-Steve-Swink-chapter-1.pdf` —
  gerçek zamanlı kontrol eşiği **< 100 ms**; altı ölçüm ekseni.

### F.9 Rim light / iç ışıma / emissive maske

- **Cyanilux, "Sprite Glow/Outline Shader Breakdown"** —
  `https://www.cyanilux.com/tutorials/sprite-outline-shader-breakdown/` — SDF'yi alfa kanalında
  saklama, outer/inner glow ayrı `smoothstep`'ler, **pixel-art için iki seçenek**.
- **Ekincan Taş, "Sprite Shaders URP"** — `https://ekincantas.wordpress.com/spriteshaders/` —
  Inner Glow, Glow Amplify, **Glow Color Mask** (belirli palet renkleri ışır — bizim damar taşı
  önerisinin kaynağı), Glow Texture Mask, Dissolve Glow.
- **Godot Shaders, "2D Rim Light"** — `https://godotshaders.com/shader/2d-rim-light-2/` — alfa
  taramalı yaklaşım, `accuracy` **4-256** (pahalı; pixel-art için uygun değil).
- **Gwenaël Massé, "Art Design Deep Dive: Giving back colors to cryptic worlds in Dead Cells",
  Game Developer, 3 Nisan 2018** —
  `https://www.gamedeveloper.com/production/art-design-deep-dive-giving-back-colors-to-cryptic-worlds-in-i-dead-cells-i-`
  — *"probably one of the only videogame studios to draw our normal maps… by hand"*; grayscale +
  **gradient map** ile bölge palet değişimi.
- **Thomas Vasseur, "Using a 3D pipeline for 2D animation in Dead Cells", 25 Ocak 2018** —
  `https://www.gamedeveloper.com/production/art-design-deep-dive-using-a-3d-pipeline-for-2d-animation-in-i-dead-cells-i-`
  — her animasyon karesi kendi normal map'iyle export ediliyor.
- **Children of Morta postmortem** —
  `https://www.gamedeveloper.com/design/postmortem-children-of-morta` — pixel animasyonlar ~4×
  büyütülüyor, ışık **pixelize edilmiyor**: *"It's like a puppet show. The puppets are pixelated
  but we shine them with real lights."*
- **Blasphemous — The Game Kitchen röportajı** —
  `https://darkrpgs.home.blog/2019/09/20/interview-with-the-game-kitchen.../` — %100 elle çizilmiş
  "purist" yaklaşım (ters kutup örneği).

### F.10 Pixel-art tuzakları, çözünürlük, ölçekleme

- **Francesco Camarlinghi, "3 Tricks to Improve Pixel Art Rendering in UE4" (2018)** —
  `https://minifloppy.it/posts/2018/3-tricks-to-improve-pixel-art-rendering-in-ue4/` — (1) vertex
  shader'da ızgara snap + kamerada `GridSnap`, (2) sprite rotasyonunu kısıtla (elle yön
  varyantları), (3) tutarlı piksel yoğunluğu — *"sprite'ları asla ölçekleme"*. Ve dürüst itiraf:
  native çözünürlükte render + upscale **denendi, post-processing ile sorun çıkardı, vazgeçildi.**
- **Unity, 2D Pixel Perfect Camera** —
  `https://docs.unity3d.com/6000.1/Documentation/Manual/urp/2d-pixelperfect-ref.html` —
  Assets PPU / Reference Resolution / Upscale Render Texture / Pixel Snapping ve **ikisinin
  birlikte kullanılamaması**.
- **notkey.studio, "Godot low-res pixel-perfect rendering and smooth camera"** —
  `https://notkey.studio/en/tutorials/godot-low-res-pixel-perfect-rendering-and-smooth-camera/` —
  320×180 + **6×** mimarisi, iki katmanlı kamera, `global_position.round()`.
- **tanalin, "Integer scaling"** — `https://tanalin.com/en/articles/integer-scaling/`.
- **Celeste motion smoothing mod dökümanı** —
  `https://github.com/FancyFurret/celeste-motion-smoothing` — Celeste'in **320×180 → 6×**
  boru hattı (ve Madeline'in subpixel offsetle çizilebilmesi için parçalı render).
- **Owlboy sanat analizi** — `https://hundstrasse.com/2017/02/25/owlboy-pixelart-elegance/` —
  **640×360** taban; Otus **39 piksel** boyunda. İlgili GDC 2017 oturumu: *Owlboy: The Evolving
  Art of a 10 Year Project* — `https://www.youtube.com/watch?v=rOkq7fUdgYQ`.
- **"The ultra-modern stylings of Hyper Light Drifter"** —
  `https://www.gamedeveloper.com/business/the-ultra-modern-stylings-of-hyper-light-drifter` —
  *"240p alanının kısıtları içinde çalışmak"*.
- **Real Time VFX, "How to correctly match sprite resolution to pixelation of a shader?"** —
  `https://realtimevfx.com/t/how-to-correctly-match-sprite-resolution-to-pixelation-of-a-shader/24849`
  — dikdörtgen sprite'larda aspect düzeltmesi: tiling X = genişlik ÷ yükseklik, ölçek = yükseklik.
- **Lospec Pixel Art Scaler / Rotator** — `https://lospec.com/pixel-art-scaler/` ·
  `https://lospec.com/pixel-art-rotator/` — non-integer ölçeklemenin eşit olmayan piksel boyutu
  ürettiği; RotSprite.
- **VFX Apprentice, "Pixel Effects — Pixel Art Meets VFX"** —
  `https://www.vfxapprentice.com/blog/pixel-art-game-effects-animated` — iki üretim yöntemi (elle
  kare kare vs 512→128 downscale + palet düzeltme); **3/4/6 kare** denemeleri.
- Araç: **Pixel FX Designer (CodeManu)** — `https://codemanu.itch.io/particle-fx-designer`.

### F.11 Shader taşınabilirliği (A2 bölümünün kaynakları)

- **libsdl-org/SDL_shadercross** — `https://github.com/libsdl-org/SDL_shadercross` — **zlib**
  lisans; `README.txt`: *"It takes SPIRV or HLSL as the source and outputs DXBC, DXIL, SPIRV, MSL,
  or HLSL."*; bağımlılıklar (SPIRV-Cross, `dxcompiler.dll` + `dxil.dll`); CLI kullanımı
  (`src/cli.c`); CI artifact isimleri (`.github/workflows/main.yml`).
- **vcpkg: sdl3-shadercross** — `https://vcpkg.io/en/package/sdl3-shadercross.html` — sürüm
  **3.0.0-preview2** (olgunluk göstergesi).
- **TheSpydog/SDL_gpu_examples** — `https://github.com/TheSpydog/SDL_gpu_examples` — *facto*
  referans: HLSL kaynak, `compile.sh` ile üç formata offline derleme,
  `Examples/Common.c` `LoadShader()` format seçimi, **MSL için `entrypoint = "main0"`**.
- **Moonside Games, "Introducing: SDL_shadercross"** —
  `https://moonside.games/posts/introducing-sdl-shadercross/` — HLSL'i kaynak dil önerisi,
  `-d JSON` reflection çıktısı. **Uyarı: yazıdaki runtime API örneği artık geçerli değil.**
- **Glusoft, "How to Display a triangle with SDL3_gpu"** —
  `https://glusoft.com/sdl3-tutorials/display-triangle-sdl3_gpu/` — aynı desenin adım adım hali.
- **SDL wiki:** `SDL_CreateGPUShader` · `SDL_GetGPUShaderFormats` · `SDL_CreateGPUDevice` ·
  `SDL_GPUShaderFormat` · `SDL_HINT_GPU_DRIVER` · `CategoryGPU` (`https://wiki.libsdl.org/SDL3/…`).
- **"GPU: Prefer D3D12 over Vulkan when available" (2025-12-01)** —
  `https://discourse.libsdl.org/t/sdl-gpu-prefer-d3d12-over-vulkan-when-available/65012` —
  backend sırasının kasıtlı değişimi (commit mesajında **gerekçe yazılmamış**).
- **HLSL 2021** — `https://devblogs.microsoft.com/directx/announcing-hlsl-2021/` · migration
  `https://devblogs.microsoft.com/directx/hlsl-2021-migration-guide/` — vektörlerde `?:`/`&&`/`||`
  yerine `select()`/`and()`/`or()`; **`select(cond, t, f)` argüman sırası MSL'in tersi**.
- **HLSL Matrix Ordering** —
  `https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math`
  — uniform matrisler varsayılan **column-major**; `#pragma pack_matrix` ile değişir.
- **DXC `docs/SPIR-V.rst`** —
  `https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst` — bellek düzeni
  kuralları; varsayılan **vector-relaxed std140** (SDL'in `std140` beklentisiyle uyumlu).
- **DXC "Running Shaders" wiki** —
  `https://github.com/Microsoft/DirectXShaderCompiler/wiki/Running-Shaders` +
  **Graham Wihlidal, "Signing DXIL Post-Compile"** —
  `https://www.wihlidal.com/blog/pipeline/2018-09-16-dxil-signing-post-compile/` — `dxil.dll`
  yoksa **imzasız DXIL** ve *"Shader is corrupt or in an unrecognized format"*.

### F.12 BULUNAMADI — dürüst liste

Araştırıldı, doğrulanabilir kaynak çıkmadı. Bunlar için brief'te iddia yok:

1. **"Pixel-art'ta bozma ofsetlerinin kuantalanması gerektiğini" doğrudan savunan kanonik tek
   makale** — yok. Gerekçe iki ayrı literatürden sentezlendi (alt-piksel örnekleme artefaktları +
   piksel creep), formüller implementasyonlardan (F.3).
2. **Celeste'in su/displacement shader'ının teknik açıklaması** — yok. Noel Berry'nin yalnız
   **aydınlatma** üzerine yazısı var.
3. **Sea of Stars / Blasphemous / Hyper Light Drifter'ın su shader'ı için birinci elden
   geliştirici kaynağı** — yok (forum gözlemleri kaynak sayılmadı).
4. **Hollow Knight için ölçülmüş hit-stop / sarsıntı sayıları** — yok. Doğrulanan tek şey:
   Team Cherry v1.5.x'te sarsıntı şiddeti ayarı eklemiş.
5. **Klemen Lozar'ın blend modu yazısı** — yok (katkısı motion-vector flipbook blending).
6. **Andreas Glad'ın blend moduna ayrılmış tek tutorial'ı** — yok (yalnız forum katkıları).
7. **"2D pixel-art bağlamında prosedürel FBM vs bake gürültü"yü özel olarak karşılaştıran
   makale** — yok. Genel (3D/PC) karşılaştırmalar kullanıldı.
8. **SDL_shadercross README'sinde literal "unstable API" ibaresi** — yok. Olgunluk yargısı
   dolaylı kanıtlara dayanıyor (0 release, 0 tag, vcpkg `preview2`, değişmiş API).
9. **SDL'in resmî release arşivlerinin shadercross içerdiği iddiası** — **yanlış**;
   `release-3.4.12` asset listesinde yok.
10. **SDL'in D3D12'yi Vulkan'a tercih etmesinin yazılı gerekçesi** — commit mesajında yok.
11. **`docs/README-migration.md`'de SDL_GPU shader göç rehberi** — yok (dosyada "gpu" hiç
    geçmiyor); resmî doküman `SDL_gpu.h` doc-comment'leri.
12. **Wicked Engine "Which blend state for me?"** — URL 404, içerik doğrulanamadı.
13. **"Parçacık boyutu ile kamera zoom ilişkisi" üzerine doğrudan alıntılanabilir kaynak** — yok;
    yalnız dolaylı olarak overdraw tarafı var.
