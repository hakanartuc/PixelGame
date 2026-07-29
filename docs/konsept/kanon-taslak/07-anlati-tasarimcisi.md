Kodu okudum, dört adayı ona göre ölçtüm. Önce doğruladığım şeyler, sonra karar.

## Zemin: neyin gerçekten kodda olduğu

`/Users/hakanartuc/PixelGame/src/light.c` — nokta ışıkları dünya koordinatından ekrana taşıyan, yarıçaplı, titreşimli bir sistem. Havayla konuşuyor: yağmur meşale yarıçapını %20 kısıyor, fırtına pencereleri titretiyor. `light_append_point` **zaten bir hikaye bayrağıyla sürülüyor** (`src/main.c:10955`, `SF_K9_KANDIL`: 1 = mavi tapınak ateşi, 2 = sönmüş, ışık eklenmiyor). Yani "durumu olan kandil" uydurma değil, çalışan bir kalıp. Tavan: `R2D_MAX_LIGHTS 32` (`src/r2d.h:31`, MSL dizisiyle aynı kalmak zorunda) — ekran dışı ışıklar zaten atlanıyor ama uzaklaşmış kamerada uzun bir direk hattı bu bütçeyi yer, bilgi olarak not.

Kritik eksik: **dünyada "burası aydınlık mı" diye soran hiçbir fonksiyon yok.** Işık yalnız render tarafında yaşıyor. Ölünün halka dışında kalkması, gizlilikte ışığın sayılması, hepsi `light_level_at(x,y)` benzeri yeni bir sorguya bağlı — ama ışık verisi tabloda olduğu için bu küçük iş (yarım gün + lamba durum tablosu; `storyFlags[16]` dolmak üzere, `src/save.c` tamponlarına yeni dizi girecek).

Ölülerin gece kalkması ise **şaşırtıcı derecede hazır**. `src/worldevent.h` WE_GRAVE + `src/main.c:8517-8560`: gece çökerken bir mezar sahasında üç iskelet kalkıyor, **şafağa ikisi sağ kalırsa mezar kalıcı olarak yeniden yuva oluyor** (`src/ecology.h`, kolonizasyon). Temizleme fiili de var: NEST_GRAVE ritüeli — bütün mobları öldür, E'ye bas, saha temelli kapanır (`ecology_do_ritual`). Pyrewright yakma çukuru bu fonksiyonun kurgusudur, kod yazılmayacak. `mob.c:1172` nocturnal bayrağı gece-gündüz ritmi veriyor, gece algı ×1.5 (`main.c:9541`).

Ses tarafı ise iddia edildiği gibi değil. `src/ambience.c` üç global loop (gündüz/gece/zindan) + bir yağmur katmanı, hepsi salt gain crossfade. `src/audio.h`: `audio_play(SfxId)` — konum yok, çağrı başına ses seviyesi yok, panning yok. **Yön bilgisi taşıyan hiçbir ses altyapısı yok.** Mesafeye bağlı tek bir uğultu katmanı yağmur katmanının klonu olarak yarım günde yazılır; yön istiyorsan yeni iş, SDL3'ün gain-only API'siyle panning'in kanal haritası hilesi gerektirip gerektirmediğini **[DOĞRULANMADI]**.

Gürültü→şüphe kalıbı var ama sistem değil: maymuncuk kırılınca 260px yarıçapta moblara `suspicion += 0.65` (`main.c:1912`), elle yazılmış tek bir döngü.

## Adaylar

**A — kandil bir gürültü.** Fiiller: karanlığa gir ve uğultunun yükselişini duy; dur-dinle-yürü ile kaynağı üçgenle; kandil yakıp yerel olarak bastır; kendi gürültünü üret (çan, davul, kalabalık — bakım dışı bir çözüm, hoş); çağrıyı dinleyip ölüyü kendine çek (Yankıcı hattı, `SF_YANKICI` zaten kodda); direnen, adı olan ölüyü diyaloğa aç. Fiiller güzel, fatura ağır: mesafe ucuz, **yön yeni bir katman** ve 16px yukarıdan bakan kamerada oyuncu sesten yön okuyamaz — mecburen görsel bir vekil (asimetrik vinyet, kenar titremesi) yazacaksın, postfx vinyeti merkezli olduğu için o da yeni. Orta-büyük. Beş rol testinde de topallıyor: büyücü/Yankıcı bayram eder, suikastçi idare eder, tüccarla şövalye seyreder. İfşa kadranıyla çelişmez ama onu güçlendirmez — bir de kulak eksenini yönetmek zorunda kalırsın.

**B — kandil bir göz.** Fiiller: koru cebine koy ve gece yola çık (görünmeyi *seç*); koru kurşun kutuya kapat, kandil altından geç, halkadan halkaya değil gölgeden gölgeye yürü (görünmemeyi seç); ölüyü halkanın içine çek — mevcut aggro/leash ile bedava bir savaş taktiği; kandil dik ve mahallenin bakış altına girdiğini gör; ahit taşındaki çizikleri say (lore sayfası); ejderhayla bir vadi için pazarlık et. Fatura: en hafif. `light_append_point` + ışık sorgusu + nocturnal iskelet + WE_GRAVE; ejderha bakışı bölge durumu ve bayrak, yani `region.c`/`quest.c` işi. İki-üç gün. Ekran testi kusursuz: kural halkanın içi/dışı olarak *görünüyor*, yeni duyu kanalı yok.

**C — kor toprağa inmesin diye.** İçinde tek bir muhteşem fiil var: **koru bilerek toprağa dökmek.** Bir mahalleyi diriltebiliyorsun. Coldhearth'ın stratejisi, suikastçinin sabotajı, ve "ölüleri silah olarak kullanan oyuncu" hepsi bu tek fiilden çıkar. Geri kalanı zehir: kül tavası boşaltmak, birikim ölçmek, drenaj kazmak. Bölgesel kor birikimi sayacı ve toprak durumu yeni sistem (orta; zemin tonunun runtime değişmesi için emsal var — `weather_snow_cover`). Bakım tuzağının tam merkezi.

**D — bilinmiyor.** Fiiller var ve bedavaya geliyor: rakip açıklamaları toplamak, bir teoriyi NPC'ye savunmak, teoriyi *test etmek* (bir kandili söndür, bak), teorisine göre örgüt seçmek, yanlış teoriyi bir kasabaya satmak. Ama tek başına omurga olamaz: sebep boşken kandil salt bir alet kalır, yak-söndür, yani müteahhit tuzağına **en açık** aday budur.

## Bakım tuzağı — kararın belkemiği

Hakemin haklı olduğu yer şu: fiil "lambayı besle" olduğu sürece oyun bir iş takvimidir. B bunu tek hamlede kırıyor, çünkü kandil yakmak artık **iki yönlü** bir eylem: yaktığın yerde ölü yatar *ve* yaktığın yer aşağıdan bakılan bir yer olur. Kandil bir bakım kalemi değil, bir karar. Millrun'un on yedi direğini kendi korunla yakan tüccar o yolun sahibi olur ve aynı gece kervanının nerede olduğunu Wyrmcrag'den, gümrükten ve Sessiz El'den herkes bilir. Bakım fiili yerinde duruyor ama artık bedeli var, bedeli olan fiil oynanış olur.

## İfşa kadranı

B ile kadran zaten aynı şeyin ikinci yüzü: kor bir gözün yakıtıysa, kor taşıyan görülür. Kodda hazır — meşale ve silah efekti oyuncunun kendisini ışık kaynağı yapıyor (`light.c:60-93`). Eksik olan tarafı: `sneak.c` ışığa bakmıyor (çömelme, örtü, bakış açısı, siper var — ışık yok) ve `crime.c` tanık yarıçapı sabit 350px. İkisine ışık terimi eklemek birer satır artı sorgu. B seçilirse ifşa kadranı bir ek sistem değil, aynı `light_level_at`'in ikinci müşterisi olur.

## Karar

**B — kandil bir göz.** Aşağıda uyanık bir şey var, kor onun ateşi, yanan kor onun baktığını belli eder ve ölü o bakışın altında doğrulmaya cesaret etmez. Ama bunu Perde 1'de kimse söylemez: oyuncu ilk yirmi saat **D içinde** yaşar — Pyrewright'lar "kandil ölüyü sadece bekletir" der, Fitilciler "hat besleme meselesi" der, bir kitapta "kor toprağa inerse" satırı geçer ve yanlıştır. Cevap Wyrmcrag'de bir ejderhanın ağzından, oyuncu kendi kararını çoktan vermişken gelir. C'den tek bir şey alınır: koru toprağa dökmek işler, ama gerekçesi C'nin fiziği değil — bakılan yeri gizlemenin bedeli olur. A rafa kalkar; yön taşıyan sesi olmayan bir motorda uğultu mekaniği görsel bir protez gerektirir.

**İlk on dakika nasıl değişir:** kandili sönmüş meydan, gece, bostanda kıpırtı — komşu kadın kor fenerini kaldırır ve toprağı yaran şey durur. Burada B'nin farkı bir kare: el durmakla kalmaz, **yukarı bakar.** Sonra fener iner. Oyuncu kandile yetecek koru bulur (satın al, vergi arabasından çal, Pyrewright'ı ikna et — hepsi mevcut ticaret/yankesicilik/ikna) ve gece o parçayı cebinde taşıyıp yola çıktığı an ekranda kendi halkası doğar. Sabah ihtiyar sorar: "Gece yolda ışık varmış. Kimdeydi?" Ders verilmez, kimse kural anlatmaz; oyuncu on dakikada iki şeyi birlikte öğrenir — halkanın içi güvenli, halkayı taşıyan görünür. Aynı avuç korun iki yüzü, ve bundan sonraki her kararı bu ikisi arasında verir.

**Kod faturası, toplam:** `light_level_at` + lamba durum tablosu (küçük, save sürümü artar — `src/save.c` paralel tamponları), `sneak.c` ve `crime.c`'ye ışık terimi (küçük), WE_GRAVE'i rastgele olaydan **sönmüş kandil alanına** bağlamak (`src/worldevent.c` + `main.c:8517` bloğu; orta değil, küçük-orta), "koru toprağa dök" fiili (küçük, ritüelin tersi). Yeni ses sistemi yok, yeni duyu kanalı yok, yeni tile katmanı yok.