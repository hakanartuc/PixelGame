# KANON DENETİMİ — YENİTAÇ KADROSU (20 slot)

> Kanon editörü ajanının raporu, 2026-08-03.
> Denetlenen: `kadro-yenitac-slotlar.md`, `kadro-isim-havuzu.md`, `kadro-kartlari-salon.md`, `kadro-kartlari-sehir.md`
> Karşılaştırılan: `docs/LORE.md`, `docs/GDD.md`, `assets/scenes/intro_demirboyun.txt`, `src/npc.c`, `src/craft.c`, `src/main.c`
> Bu bir DENETİM raporu; düzeltmeler işlendikçe buraya "UYGULANDI / REDDEDİLDİ + gerekçe" damgası düşülecek.

---

# KIRICI

## K1 · Erteleme sayacı üç ayrı gerçek anlatıyor — ve o sayaç oyunun ilerleme çubuğu

**Nerede:** `kadro-kartlari-salon.md` slot 19 (Sekiz'in 3. barkı), slot 2 (Rüstem 3. bark), slot 5 (Vedia 2. bark) · `intro_demirboyun.txt` panel `[tarih]` · LORE §6 Şirin.

Dört kaynak, dört farklı sayım:

| Kaynak | Kaç erteleme oldu | Sebepler |
|---|---|---|
| LORE §6 | **üç** | "bir nöbet, bir yas günü, bir kırk" |
| Vinyet panel 3 görsel notu | **dördüncüsü şu an yazılıyor** | "üstü çizilmiş ÜÇ tarih ve mürekkebi ıslak dördüncüsü" · sebep: "beyimizin sağlığı" |
| Kart 19, Sekiz | **üç, dördüncü henüz yok** | "Bir: hasat sonrası. İki: kırktan sonra. Üç: bey şifa bulunca. Dört... dördü daha yazmadılar." |
| Kart 2 + Kart 5 | **dördüncü gelecekte** | Rüstem: *"bu gün de kayarsa dördüncü tarihi ben yazdırırım"* · Vedia: *"Kâtip, dördüncü tarihi yazarken yanına fiyatını da yaz."* |

Sekiz'in listesindeki üç sebepten yalnız biri ("kırk") LORE'unkiyle örtüşüyor; "hasat sonrası" ve "bey şifa bulunca" LORE'da yok, LORE'daki "nöbet" ve "yas" hiçbir kartta yok. Üstelik vinyet "şifa" sebebini **dördüncü** olarak gösterirken Sekiz onu **üçüncü** sayıyor.

**Neden kırıcı:** erteleme storyFlag'i slot 3, 4, 5, 19, 20'yi ve Şirin'in tüm hattını sürüyor (`slotlar.md` slot 19: *"Dünya durumunun sesli sayacı"*). Sayaç oyunun görünür ilerleme çubuğu ve başlangıç değeri belirsiz.

**Düzeltme:** LORE kazanır. Sebep listesi her yerde **nöbet / yas / kırk** olur. Vinyet oyuncuya **dördüncünün yazılışını izletir** (sebep: beyimizin sağlığı) — bu, açılışın en iyi anı olur. Sekiz'in barkı: *"Bir: nöbet. İki: yas. Üç: kırk. Dört: bey şifa bulunca — bunu bu sabah yazdılar."* Rüstem ve Vedia'nın "dördüncü tarih" replikleri **beşinci** olur ya da vinyet öncesine alınır.

---

## K2 · Kasada üç ay var mı, iki ay önce dip mi göründü?

**Nerede:** `kadro-kartlari-salon.md` slot 5 vs `kadro-kartlari-sehir.md` slot 18.

- Vedia (kart 5, LORE §7'nin kanonik repliği): *"Kasada üç ay var; sen o üç ayın içindeysen konuşuruz."* Aynı kart: *"Kadın yalan söylemiyor — yalnız sorulanı cevaplıyor."* · *"Yuvarlamaz… rakam ya vardır ya yoktur."*
- Hafize (kart 18): *"Kasa iki ay önce dibi gördü; işçi yevmiyesini o iki ay onun kesesi ödedi, vadeyle. Perçin'in elindeki para onun parasıydı, Mahir'in körüğü onun parasıydı, Bekmez'in kazanı onun parasıydı."*

Kasa iki ay önce boşaldıysa "kasada üç ay var" yanlıştır — ve bunu söyleyen, LORE §7'nin "odadaki tek gerçek konuşan"ı. `slotlar.md` slot 5 ayrıca Vedia'yı açıkça *"İskele ödemelerinin, işçi yevmiyesinin ve gazi maaşının kaynağı"* ilan ediyor; yani yevmiyeyi iki aydır bir tefecinin ödediğini bilmemesi imkânsız.

**Neden kırıcı:** 7. dakika vuruşu ("oyunun konusu bu cümleyle iniyor") bu tek cümleye asılı. Cümle yanlışsa açılış yanlış.

**Düzeltme:** Kart 18 değişsin, kart 5 değil. Hafize iskeleye **ödeme** yapmasın; **borcu satın almaya talip** olsun (zaten istediği bu) ve verdiği tek somut şey Mahir'in körüğü + Bekmez'in kazanı gibi **özel kalemler** olsun. Böylece "kasada üç ay var" doğru kalır, Vedia'nın karakteri sağlam durur ve Hafize'nin "Ve haklı" bölümü zayıflamaz.

---

## K3 · Salonun açık avluya çevrilmesi motor kısıtından türetilmiş — ve LORE §7'nin kapalı odasını yok ediyor

**Nerede:** `kadro-yenitac-slotlar.md` bölüm (A), madde 3.

> *"Salon ayrı harita OLMAYACAK — ve bu bir kadro kararı. `NpcDef.map` tek bir int; bir NPC'nin rutini harita sınırını geçemez… Salonu ayrı harita yaparsak on iki slotun rutini tek durağa çöker."*

Çelişen kanon, LORE §7:

> *"o cümleyi odada kuracak adam yok. Rüstem kuramaz, Sadık defterine yazamaz, Vedia kasa diliyle söyleyemez."* · *"Sarp gözünü kapatır. Gözü kapandığında oda durur."*

LORE'un tamamı **kapalı bir oda**nın fiziğine dayanıyor: kim duyduğu bilinen, sayısı belli bir dinleyici. "İşçi kırk adım ötede ip çekiyor, ikisi aynı ekranda" olduğu anda Şirin'in sessiz taktiği, Sadık'ın "unvansız" kaydı ve "söylenemeyen cümle" tabusu anlamını kaybeder — söylenmeyen şey zaten herkesin duyduğu yerde söylenmiyordur.

İkincisi: karar **kendi belgesiyle** çelişiyor. Kart 1 §3: *"Öğlen avluya çıkarılmayı istiyor — Sabri'den… Avlunun o köşesinden kuzey yolu görünüyor."* Salon zaten avluysa "avluya çıkarılmak" diye bir istek olamaz ve Sarp'ın kartta yazılı **tek arzusu** buharlaşır.

Üçüncüsü: "çatısı yok" ≠ "duvarı yok". Vinyet görsel notu duvarı sayıyor: *"sağ duvarda ALTI BOŞ ÇİVİ"*. GDD §1.4 *"salona her girişte kaç sancak asılı olduğunu sayarsın"* diyor — giriş var demektir.

Ve prosedürel olarak: bu, `kod en son gelir` kuralının ihlali. Kod faturası kanon kararının gerekçesi olamaz.

**Düzeltme:** Salon **ayrı harita** olsun (çatısı yok, duvarı ve kapısı var). Rutin sorunu için NPC harita geçişi mekaniği yazılsın ya da avlu/salon iki ayrı durak olarak modellensin. Karar gerekçesi belgede "kanon" başlığından çıkarılıp "mekanik borç" başlığına taşınsın.

---

## K4 · Rüstem'in gizli buzu artık resmî bir lojistik operasyonu

**Nerede:** Kart 2 §4 vs `slotlar.md` slot 6 + kart 14 + kart 12.

LORE §6:
> *"haftada iki kez, **kimseye haber vermeden**, sırtında buz çuvalıyla kışlanın mahzenine iniyor… Tek koluyla taşıyor, **yardım ettirmiyor**."*

Yeni kadronun kurduğu tablo:
- Slot 6 (Sabri): *"**19 mahzen** (hekim buz istedi)"* — hekim buz sipariş ediyor.
- Kart 6 §9: *"Seyisle **iki kıştır buz taşıyorlar** ve bugüne kadar hiç konuşmadılar."*
- Kart 14 (Kıraç): *"09 mahzen kapağı (kızak gelir)"* · bark: *"Buz iki çuval. Dün üçtü."* · *"Mahzen kapağını her sabah o açıyor."*
- Kart 12 (Ragıp): *"Seyis Kıraç'ın 'buz taşımaya **ikinci adam**' talebi dört aydır onun defterinde."*
- Vinyet panel 4 + pano: *"Mahzene buz taşı — 6 akçe."*

Yani buz: her sabah kızakla geliyor, hekim sipariş ediyor, defterde talebi var, panoda ücreti var, iki adam iki kıştır taşıyor. Bu tabloda Rüstem'in haftada iki gizli seferi ne gizli, ne gerekli, ne de dokunaklı.

**Düzeltme:** Tek cümle çözer ve LORE §8 zaten yarısını yazmış (*"para bittiği için buz azalıyor"*). Kart 2'ye ve kart 14'e şu eklensin: resmî tahsis **iki çuvala düştü, üç gerekiyor**; farkı Rüstem kendi kesesinden alıp kendi sırtında indiriyor ve defterde bu kalem yok. Böylece Kıraç'ın "dün üçtü" barkı Rüstem'in sırtındaki çuvalın sebebi olur; gizlilik ve yalnızlık geri gelir.

---

## K5 · Şirin'in borcunu kim kapattı — LORE "Şirin" diyor, iki kart "Vedia" diyor

**Nerede:** Kart 4 §9 ve kart 5 §9 vs LORE §6.

- LORE §6: *"**Şirin o borcu sarayın kasasından kapattı** ve Vedia bunu hâlâ hatırlıyor."*
- Kart 4: *"Vedia'ya borçlu: ilk kocasının kapanmayan hesabını sarayın kasasından **Vedia kapattı** ve hatırlıyor."*
- Kart 5: *"Şirin'in ilk kocasının borcunu kasadan **o kapattı**; kadını sevmiyor, ama sevmemesinin sebebi borç değil, borcun onu **suç ortağı** yapmış olması."*

Fail aynı değil, güç ilişkisi ters. LORE'da Şirin kasayı kendi çıkarına kullanan taraf, Vedia da bunu bir leke olarak hatırlayan tanık. Kartlarda Vedia iyilik yapan taraf, Şirin minnet borçlusu. Ayrıca zamanlama sorunu: koca **sekiz yıl önce** öldü; Şirin saray kasasını kullanabildiyse o tarihte zaten sarayda olmalı.

**Düzeltme:** LORE kazanır. Kartlar şuna dönsün: kalemi **Şirin yazdırdı**, Vedia **imzalamak zorunda kaldı**. Vedia'nın kartındaki "suç ortağı" cümlesi olduğu gibi kalır ve daha da sertleşir; Şirin'in kartındaki "minnet" yerine "Vedia'nın her sabah selamı ilk ondan alması" (zaten yazılı olan, çok daha iyi olan detay) tek başına taşır.

---

## K6 · **Kıraç ≈ Kırca** — havuzun kendi kuralını çiğniyor, üstelik ikisi de Karduvarlı

**Nerede:** `kadro-kartlari-sehir.md` slot 14 (Seyis Kıraç, *"Karduvarlı, 44"*) vs LORE §6 Kırca Ana, Karduvar'ın yöneticisi.

Havuzun kendi kuralı, `kadro-isim-havuzu.md` §6.6:
> *"Kanon adlarının ses ikizleri. Kural: yeni ad, mevcut bir kanon adından **en az iki sesbirim** uzak olacak."*

Havuz §5.6 aynı gerekçeyle **Karca**'yı elemiş: *"Karca ≈ **Kırca**"*. Kıraç, Karca'dan daha yakın: /kɯrdʒa/ ve /kɯratʃ/ **aynı beş sesbirimden** oluşuyor (k-ı-r-a-c/ç), tek fark sıralama ve ötümlülük. Bir sesbirimlik mesafe bile değil — sıfır.

Ağırlaştırıcı iki şey:
1. Havuz Kıraç'ı **Karduvar** listesine kendi koymuş (§3, *"Ek almayanlar da var, hepsi hava: Ayaz · Boran · **Kıraç** · Sepken"*) ve §5.6'da Karca'yı elerken bunu görmemiş. Mayını belge kendi döşemiş, kart üstüne basmış.
2. Havuz §7 Kıraç'ı ayrıca **ana kadro dışı** ilan etmiş: *"Dikkat gerektirenler: Argın, **Kıraç**, Sırrı… bunlar ikincil kadroda kalsın, **ana hat NPC'si olmasın**."* Seyis, GDD §1.2d'nin onaylı **9. dakika / ilk talep** vuruşunu taşıyan slot. Bu, tanımı gereği ana hat.

**Düzeltme:** Kıraç düşer. Havuzun Karduvar listesinden ve §7 temiz-ad listesinden ortak çözüm: **Ayaz** ya da **Boran** (ikisi de özel karaktersiz, "hava" kümesinden, Kırca'ya uzak). Kartın "Kıraç, Alaca oğlu" formu korunur → *"Ayaz, Alaca oğlu."* Ayrıca §3'ün Karduvar listesinden Kıraç silinsin ve §5.6'ya **Kıraç ≈ Kırca** satırı düşülsün, yoksa aynı ad ikinci turda geri gelir.

---

## K7 · İskele neden bitmiyor: "sekiz direk, iki kış, bir kese" — LORE "para" demiyor, "cümle" diyor

**Nerede:** Kart 18 Hafize'nin 2. barkı vs LORE §7.

- Kart 18: *"(15:00, iskele dibinde) 'Sekiz direk kalmış. Sekiz direk, iki kış, bir kese.'"* · slot 18: *"iskele dibi (ne kadar iş kaldığını kendi gözüyle sayar)"*.
- LORE §7: *"On bir yıllık iskele bu yüzden ayakta. **Bitmemiş bir çatı değil, söylenemeyen bir cümle.**"* · *"kimse töreni iptal etmedi, sadece hep haftaya kaldı."*
- Vinyet panel 1, kalfanın kendi ağzından: *"İndirme emri yok. On bir yıl havada durdu, bir kış daha durur."* · direkteki tebeşir: *"Çatı — sancaklar asılınca. Asılı sancak: yok."*

İskele **emirsiz** duruyor, parasız değil. "Sekiz direk, iki kış, bir kese" cümlesi oyunun amiral görüntüsünü finanse edilebilir bir inşaat kalemine indiriyor; oyuncu "para bulunca biter" diye okur ve LORE'un tüm siyasi kilidi çöker. Kart 5'in *"iskele yevmiyesi bir ay içinde durur, iskele durur"* cümlesi aynı yanlış okumayı besliyor.

**Düzeltme:** Hafize sayıyı saysın ama **bitişi** saymasın. Öneri: *"Sekiz direk çürümüş. Çürüyeni değiştirmek de bir kalem — bitirmeyen iş de para yiyor."* Böylece hem karakter (sayan kadın) hem kanon (bitmeyen değil, bitirilmesi yasak olan iş) ayakta kalır ve kesedarın iskele borcunu satın alma isteği daha da mantıklı olur: **bitmeyeceğini bildiği** bir işin borcunu alıyor.

---

## K8 · İki yeni belge birbirini yalanlıyor: gerçek tarihî/dinî adlar

**Nerede:** `kadro-isim-havuzu.md` §5.2-5 ve §6.1-2 vs `kadro-yenitac-slotlar.md` bölüm (C) satır 2, 3, 6, 7, 8, 9, 11.

- Havuz §5.2-5: *"Dinî/tarihî gerçek adlar MVP'de duruyor: Osman, Kemal, Selim, Fatma, Zehra, Nuh, Yusuf. Bunlar… **elden geçirilmeli**. Özellikle **Kemal** (Atatürk) ve **Osman** (Osmanlı kurucusu + güncel dizi) **oyuncuyu anında kurgudan çıkarır**."*
- Göç haritası: Kemal → `cayir_koy_ihtiyar` *"Yerinde kalır"*; Osman → `cayir_koy_ciftci` *"Yerinde kalır"*; Fatma, Zehra, Nuh, Selim, Nazife hepsi *"Yerinde kalır"*. Ad değişikliği yalnız tek satırda (#17 Kâtip Rüstem) not düşülmüş.

Havuz "Yenitaç kadrosuna taşınırken" diye sınırlıyor ama göç haritası köyü **Yenitaç'ın bir tarla ötesindeki tahıl köyü** yapıyor ve Rüstem'in tahıl yolu cümlesiyle ilk saate bağlıyor. Yani oyuncu ilk saatte Kemal, Osman ve Fatma ile karşılaşır. Ayrıca havuz §5.2-4 *"Nazife yeniden adlandırılmalı — Hediye ya da Kaymak öneririm"* diyor; göç haritası Nazife'yi adıyla koruyor ve gerekçe olarak GDD §1.2c-4'ün phasing örneğini gösteriyor — ama phasing örneği `.id`'ye bağlı, `.name`'e değil (GDD §8.1: *"görünen ad kimlik değildir"*).

**Düzeltme:** Göç tablosuna **üçüncü sütun** eklensin: "ad kararı". Kemal → §2.9 yaşlı havuzundan (Şemsi / Kalender), Osman → §2.5 esnaf-çiftçi hattından, Nazife → Hediye (havuzun kendi önerisi; Naz- kümesi Nazlı ile aynı sahnede geçecek). Fatma/Zehra/Nuh en azından "karar bekliyor" diye işaretlensin. `.id` değişmediği için maliyet sıfır — §8.1'in bombayı söktüğü nokta tam olarak burası.

---

# ÖNEMLİ

## Ö1 · Pano 19. dakikada değil, 1. dakikada iş veriyor (üç ayrı kanaldan)

GDD §1.2d [ONAYLI]:
> *"Panodan iş alma ilk 20 dakikada YOK… **19. dakikada** ilk kez görünür ve üstünde oyuncunun ZATEN yaptığı işin ilanı asılıdır."*

Ama:
1. Vinyet panel 4 (0-2. dk): *"Kışla panosunda, çiviyle çakılı bir kâğıt: 'Mahzene buz taşı — *6 akçe*. **Kışla kâtibine sorulur**.'"* — son üç kelime düpedüz bir görev yönlendirmesi.
2. Kart 11 (5. dk, Nazlı'nın barkı): *"(panonun önünde, heceleyerek) 'Mah-ze-ne buz ta-şı. Altı akçe. … Altı akçe bir hafta eder.'"* — ilan ikinci kez, yüksek sesle.
3. Pano dünyada duruyor ve 9. dakikada mahzen kapağı açılıyor.

Oyuncu 19. dakikada "almadan yaptım" değil, "1. dakikada okuduğum ilanı yaptım" diye okur. `slotlar.md` slot 12 bunu *"bu tanışma sırası sistemin kalan ömrünü belirliyor"* diye en yüksek öncelikli tasarım iddiası yapıyor; iddia kendi vinyetiyle çürüyor.

**Düzeltme:** (a) Vinyet panel 4'ten *"Kışla kâtibine sorulur"* silinsin; (b) Nazlı'nın hece barkı **buz ilanını değil başka bir ilanı** okusun (bir kervan ilanı — zaten "bir kervanda yer" istiyor, karakteri güçlenir); (c) buz ilanı vinyette **okunaksız/yarı yırtık** görünsün, tam metni 19. dakikada okunsun.

## Ö2 · Buz hiç taşınmıyor ama 6 akçe ödeniyor

`slotlar.md` bölüm (D) çizelgesi: 9. dk *"Kayışı tutar (günlüğe satır düşmez)"* → 15-18. dk *"İner, döğüşür ya da kaçar; çizmeyi okur"* → 19. dk *"Panoyu okur, parasını alır"*.

Çizelgenin hiçbir satırında oyuncu **buz taşımıyor**. Kart 12 ise ödeme cümlesini kesin yazıyor: *"Bu iş görülmüş. Altı akçe."* Kart 14 de *"Oyuncu buzu aşağı taşıdıysa"* diye bir dal varsayıyor — ama o fiil hiçbir yerde tarif edilmemiş.

**Düzeltme:** 9. dakikaya bir satır: kayış tutulduktan sonra **çuvallar kapağın ağzında kalır**; kimse taşımaz, kimse rica etmez (seyis: *"Kızak benim işim"*). Aşağı inen oyuncu çuvalı yanında götürür ya da götürmez. Götürmediyse 19. dakikada kâtip parayı **vermez** ve tek cümle söyler: *"Çuval kapıda duruyor."* Bu, panonun doğrulama olduğu dersini para ödemeden verir ve rol özgürlüğünü bozmaz.

## Ö3 · Akçe'nin ölçeği tanımsız — ve kadronun altı ağzı sayıyla konuşuyor

Aynı dünyada geçen rakamlar:

| Kaynak | Rakam |
|---|---|
| Kart 17 | bir kase çorba = **2 akçe** |
| Kart 12 / pano | mahzen işi = **6 akçe** |
| Kart 9 | 38 çivi = *"bir haftalık ekmek"* |
| Kart 16 | tüm çarşı veresiyesi = **340 akçe** |
| Kart 18 | Hafize'nin toplam kalemi = **900-980 akçe**, üstelik *"işçi yevmiyesini o iki ay onun kesesi ödedi"* |
| Kart 5 | kırk günlük erteleme = **1.600 akçe** |
| LORE §5 | Rüstem **12.000 adamı** kendi payından besliyor |

12.000 adamın günlük ekmeği bile bu tablodaki en büyük sayıyı katlar. Bir tefecinin toplam defteri (900) iki aylık iskele yevmiyesini ödeyemez. Vedia'nın "1.600 akçe" cümlesi devlet ölçeğinde yuvarlama hatası, ama kart onu *"yuvarlamaz"* diye tanımlıyor.

**Düzeltme:** Bir birim merdiveni yazılsın ve GDD'ye çakılsın. Öneri: **akçe = gündelik ölçek** (kase 2, yevmiye 6-8), **kese = 1.000 akçe**, **altın/sikke = devlet ölçeği**. Vedia devlet rakamlarını **kese** ile söylesin (*"Kırk gün, kırk kese."*), Hafize akçe ile — iki kadının aynı odada farklı birim kullanması bedava karakterizasyon olur. Kart 18'in "işçi yevmiyesini ödedi" iddiası K2 ile birlikte düşer.

## Ö4 · Sarp'ın rutininin taşıyıcısı yok

- Slot 1 (Sarp): *"07 tahtın arkasındaki sandalye · 14 aynı yer… · **21 `indoors`**"* · *"Hiç yürümez; durakları **Sabri'nin taşıdığı** yerlerdir."*
- Slot 6 (Sabri): *"06 sandalyenin yanı · **12 avlu** (bey güneşe çıkarılır) · **19 mahzen** · 23 sandalyenin yanı."*

Sarp 12'de avluya çıkıyor ama 14'te tahtın arkasında; onu kim geri getirdi? 21'de içeri giriyor ama Sabri 19'dan 23'e kadar mahzende. Kart 1 §9: *"Sabri: taşınmasına **izin verdiği tek adam**."*

**Düzeltme:** Sabri'nin mahzen durağı **19'dan 20:30'a** değil, **öğleden sonraya** (15) alınsın; ya da Sarp'ın `indoors` saati 20'ye çekilip Sabri'nin mahzen durağı 21 olsun. Dört durak tavanı (`NPC_MAX_STOPS`) buna izin veriyor.

## Ö5 · Rüstem'in kartında SEFER yok

LORE §5, Kılıç tarafının tamamı:
> *"Mareşal **Boz Rüstem**. 'Bir sefer, bir taç.' Güneye yürünsün, sürgünler ezilsin, Gökçe tacı sahada ordunun kalkanları üstünde giysin."*

Kart 2 §3 "NE İSTİYOR, KİMDEN":
> *"Vedia'dan bu ayın maaşını… Sadık'tan kaymayacak bir gün. Gökçe'den, kılıcı bir daha kaldırıp bırakmamasını."*

Seferin adı geçmiyor. Tek geçtiği yer bir bark ve orada bile **Sarp'ın** isteği olarak sunuluyor (*"Beyimiz sefer istiyor"*). Yani üç taraftan birinin tek somut talebi kadro belgesinde kayıp. Bu, LORE §1.5'in ("her tarafın haklı olduğu bir yeri var ve o yer somut") test edilebilir tarafını düşürüyor.

**Düzeltme:** §3'e dördüncü kalem: **Vedia'dan sefer bütçesinin defterde bir satır olması.** Reddedildiğini biliyor, her ay yine istiyor — Vedia'nın "bir ay kâğıt üstünde düşür" talebinin tam aynası olur ve iki kart birbirini kilitler.

## Ö6 · Gökçe'nin kanon becerisi kartta yok

LORE §6: *"**İyi yaptığı: şart yazmak.** Bir anlaşmanın nerede sızdığını dokunmadan görür."* · *"taç töreninin şartlarını üç yıl önce ikisi baş başa yazdılar."*

Kart 10 (`sehir.md` slot 10): istediği *"Depoda kaç kulaç ip kaldığını"*, asıl istediği *"kimsenin onun adına konuşamayacağı bir yer"*.

Kartın Gökçe'si sessizlik isteyen, çekilmiş bir kadın; LORE'un Gökçe'si babasından ve mareşalden habersiz kendi taç şartlarını yazan bir siyasi aktör. İkisi aynı insan olabilir ama kart bunu hiç ima etmiyor: "şart", "sızıntı", "kâğıt" kelimeleri kartta yok, mekanik kanca yok, LORE'un tek "iyi yaptığı" satırı düşmüş.

**Düzeltme:** Kartın §NE VERMİYOR bölümüne tek görünür iz: salon kenarında **dinlerken defterine bir şey çiziyor** (12:00 durağı zaten "konuşmaz, dinler"). Oyuncu sorarsa yalan söylemez — *"Nerede sızdığını yazıyorum. Kimseye vermem, kendime yazıyorum he."* Silme testine dayanıklı, ilk 20 dakikada hiçbir şey açmıyor, LORE'un kayıp becerisini geri koyuyor.

## Ö7 · "Yenitaç doğumlu" adlandırma kuralı üç karakterde üç farklı uygulanmış

Havuz §1: *"Şehirde doğanların adı inşaattan… **Sekiz** (sekizinci direk dikildiğinde doğdu), **Vinç**, **Kireç**… şimdi kimse gülmüyor."*

- **Sekiz, 24 yaşında** (kart 19). Ama iskele **on bir yıllık** (LORE §4, vinyet, kart 20). "Sekizinci direk" 24 yıl önce dikilmişse iskeleden değil, şehir surundan söz ediyoruz — belge bunu söylemiyor. **Vinç** adı ise imkânsız: taş tahtı bekleten vinç 11 yaşında, ama havuz o adı "ilk yıllar" kuşağına veriyor.
- **Uçkun, 19, Yenitaç doğumlu** (kart 13) — ama inşaat adı değil, §2.1 **gazi** havuzundan bir ad taşıyor. Kart yalnız sıfatsızlık kuralını uyguluyor, adlandırma kuralını hiç anmıyor.
- **Nazlı, 12, Yenitaç doğumlu** — istisnası **yazılı** ve iyi (*"o çocuk sokağa ait değil"*).

**Düzeltme:** Havuz §1'e tek cümle: inşaat adı geleneği **iskelelerle başladı, yani on bir yıllık**; ondan önce doğanlar (Sekiz dahil) sur ve palanka işlerinden ad aldı — o zaman "sekizinci direk" **sur direği** olur ve 24 yaş tutar. Uçkun'un kartına bir satır: adını annesi değil **kışla** verdi (yamaklığa yazıldığı gün), o yüzden inşaat adı yok — bu, adamın "lakap bekleyen çocuk" karakterini de besler.

## Ö8 · Perçin'in yılları kendi kartında tutmuyor

Kart 9, aynı sayfada:
- *"44 yaşında, Karaocaklı; **on dört yıl önce geldi**"* · *"Yenitaç ona bir iş adı taktı, defter onu yazdı, **on dört yıl geçti**."*
- Değişim dalı: *"Kâtip senin adını biliyormuş. Benimkini **on bir yıldır** yazıyor, hâlâ Perçin yazıyor."*

Ayrıca defteri tutan Ragıp **31 yaşında** ve *"Tuzkapı yazmanlığından geldi"*, salon arşivine nakil için **iki yıldır** bekliyor — 11 yıl önce bu defteri o yazmıyordu.

**Düzeltme:** İki sayıyı tek sayıya indirin. Öneri: geliş **on bir yıl önce** (iskelenin başladığı yıl — adamın Yenitaç'a gelme sebebi de böylece açıklanır) ve replik *"on bir yıldır"* kalsın. Repliğin öznesi "kâtip" değil "**defter**" olsun; Ragıp'ın yaşı kurtulur.

## Ö9 · Yedinci Ağız'ın kirlenme kuralı Sabri'yi dışarıda mı bırakıyor? Kimse söylemiyor

LORE §10: *"Sarp'ın tepsisini **altı ağzın hiçbirine sormadan** çözmek… **Tanık Deli Sabri'dir.**"* Ama LORE §7 tablosunda Sabri **altı ağızdan biri**.

Kart 6 §8 bu düğümü aynen tekrarlıyor: kapı *"Mahzende onunla birlikte iş yaptıysan"* açılıyor, ama *"Altı ağızdan birine beyi sorduysan: o cümleyi hiç duymazsın"*. Sabri altıdan biri olduğuna göre, ona beyi sormak kapıyı hem açıyor hem kapatıyor.

**Düzeltme:** LORE §10'a tek cümlelik istisna: *"Sabri sayılmaz — çünkü Sabri yorum yapmıyor, gördüğünü söylüyor. Kirleten şey soru değil, **başkasının okuması**."* Bu hem kuralı test edilebilir yapar (`beş ağız` bayrağı) hem Sabri'nin karakterini (kart 6 §10: *"Yorum yapmaz… kurmadığı için doğru olan tek ağız o"*) mekaniğin gerekçesine çevirir.

## Ö10 · Mahzendeki "duvarın ardındaki soluk"un kanonda kaynağı yok — sekizinci mühür riski

GDD §1.2d: *"Korku sesten gelir, mobtan değil — **duvarın ardındaki soluk**."* `slotlar.md` (E): *"mahzendeki duvar sesi (fantastiksiz: Akkemer su yolunun akıntısı, sıçan)"* — yani fantastik okuması olduğu ima ediliyor ama yazılmıyor.

LORE §9 **yedi mührü tek tek sayıyor** ve Yenitaç'ta olan tek mühür **çayırdaki dev fosil**; kışla mahzeni listede yok. LORE §9'daki *"Madenciler duvarın ardından nefes duyduklarını söylüyor; duyuyorlar, çünkü bağlı duvarın öbür yüzünde"* cümlesi **Karaocak'a** ait. Aynı sesi Yenitaç mahzenine koymak, oyuncunun gördüğü ilk zindanda sekizinci bir mühür ima eder ve Sarkaç'ın sabit yedisini bozar.

**Düzeltme:** Belgeye bağlayıcı bir satır: mahzenin altı LORE §12'nin **"Akkemer su yolu ve lağım"** katmanıdır, **mühür değildir**; ses akıntı ve havalandırmadır. Fantastik okuma "burada bir mühür var" değil, "**bu şehir Akkemer'in üstüne kurulmuş**" olsun — bu, Sabri'nin *"kendi çocukluğunun altında yürüyor"* satırıyla da birleşir ve bedava.

## Ö11 · Yirmi slotun sekizinin sırrı aynı: "adım nereden geldiğimi söylüyor"

- Perçin: Karaocak'ta ad alamadı, senette *"Kıymet oğlu"*.
- Yusuf: Yedisunak'ta ad almayı reddetti, eğitimi yarım.
- Bekmez: Yedisunak'tan aldığı ad *"artık bir itiraf"*.
- Ragıp: adı kısaltılıyor, üstüne bir de orduda lakap yapışmış.
- Vedia: adı kısaltılamaz, bilinen tek hikâyesi bu.
- Balaban: maaş defterinde ölü bir köyün adı.
- Sekiz: adını inşaat verdi, *"adı kısaltılamayan tek yetişkin ve bunu bir zafer sayıyor"*.
- Rasih: *"artık kimsenin çocuğuna koymadığı bir isim"*.

İsim havuzu kadroyu zenginleştirmek yerine **üretmiş**. Sekiz karakterin dramatik şekli aynı ve oyuncu üçüncüsünde kalıbı görür; görünce dokuzuncusunu da tahmin eder. LORE §15.4 "insan eli" maddesinin ruhu tam bunu yasaklıyor.

**Düzeltme:** En fazla **üç** slotta ad-yarası kalsın (öneri: Perçin — mekanik keşif zinciri var; Vedia — tek cümlede iş görüyor; Balaban — ölü köy en soğuğu). Yusuf'un sırrı **borç ve gemi yılları** olsun (zaten yazılı, daha iyi); Bekmez'inki **yirmi beşinci kaseden sonra suladığı çorba** olsun (zaten yazılı, çok daha iyi); Ragıp'ınki **iki renk mürekkep** olsun (zaten yazılı, mükemmel); Sekiz'inki **okuyamaması** olsun (zaten yazılı). Kesilecek malzeme yok — sadece hangisinin "asıl sır" olduğu değişecek.

## Ö12 · Sayı monokültürü: on bir, dokuz, kırk

**"On bir"**: iskele (LORE) · boş taş (LORE) · Sadık'ın Yedisunak yılları · Vedia'nın kese defteri yılları · Baran'ın tentesi · Mahir'in yeni iş dövmediği süre · Perçin'in defter yılı · Rasih'in bekleyişi · Balaban'ın "on bir ay" · havuz §4'teki "Kışlık" lakabı.
**"Dokuz"**: Tuğrul (LORE) · Gökçe'nin dönüşü (LORE) · Kırca mektupları (LORE) · Yusuf'un gemi yılları · Kıraç'ın gelişi · Rasih'in mezar taşı · Vedia'nın barkındaki kandil sayısı.
**"Kırk"**: şehir savaşları (LORE) · Kırk Kese (LORE) · kırk gümüş (LORE) · erteleme "kırk gün" · Bekmez'in kırk kasesi · Baran'ın kırk adamı · Vedia'nın "kırk gün, bin altı yüz".

LORE'da "on bir" tek bir şeyin sayısı: **söylenemeyen cümlenin yaşı**. Onu her karaktere dağıtmak sembolü tüketiyor; oyuncu üç NPC sonra sayıyı duymayı bırakır.

**Düzeltme:** "On bir" **yalnız** iskele ve boş taş için ayrılsın. Diğerleri dağıtılsın: Sadık on dört, Vedia yedi, Baran dokuz değil sekiz, Mahir altı, Perçin on bir (Ö8 gereği kalsın — çünkü onun yılı iskelenin yılıdır ve bu anlamlı). "Dokuz" yalnız Tuğrul hattına ait olsun; Yusuf'un gemi yılları **yedi** olsun.

## Ö13 · İsim havuzu kendi kurallarını kendi tablolarıyla çürütüyor

Kural ile tablo çakışmaları — havuz **denetim aracı** olarak kullanılamaz durumda:

| Kural | İhlal |
|---|---|
| §3 TUZKAPI: *"sonu ünlüyle ya da -et/-ret ile biter. Sert küme yok"* | **Yusuf** (kart 7: "Tuzkapılı") ve **Ragıp** (kart 12: "Tuzlu Ragıp") — ikisi de sessizle bitiyor. Ayrıca *"ad uzundur ve kısaltılmaz"* diyor; Yusuf iki hece. |
| §2.3 kâtip: *"üç hece… sonu ünlü ya da -i"* | Kendi listesinin çoğu: Rasim, Refik, Ragıp, Halim, Mükerrem — hiçbiri kuralı tutmuyor. |
| §3 KARAOCAK: *"sessizle biter"* | Kendi örnekleri: **Küskü, Tortu, Pota, Maşa** — dördü ünlüyle bitiyor. |
| §3 KUMKAPI: *"Adın sonunda ş/ç bulunmaz"* | §1'in kendi örneği **"Kumlu Turaç"** — ç ile bitiyor. |
| Tek ad, tek yer | **Muazzez** üç yerde: §1'de *"Sunaklı"*, §2.3'te kâtip, §3'te **Tuzkapı örneği**. Üstelik Yedisunak kuralı ("kadın adları ünlüyle") "Muazzez"i dışlıyor. **Turaç** §1'de Kumlu, §2.5'te Tuzkapılı. **Kunt** iki ayrı kişide (§2.2 kalfa, §2.5 Bileyci). **Örs** üç yerde. |

**Düzeltme:** Havuzun §3'ü ya kuralları gevşetsin ("eğilim, yasa değil") ya örnekleri temizlesin — ikisinin arasında kalmak en kötüsü, çünkü sonraki turlar bu kuralları test olarak kullanacak. Yusuf'un Tuzkapılılığı ya düşsün ya Tuzkapı kuralına *"kanon adları kural dışıdır"* istisnası yazılsın. Muazzez / Turaç / Kunt / Örs birer şehre sabitlensin.

## Ö14 · Nazlı'nın gizliliği kadronun kendi bilgi grafiğiyle çelişiyor

`sehir.md` KADRO NOTLARI: *"Kim olduğu Yenitaç kadrosunun **hiçbir ağzından çıkmaz**."*

Ama aynı belgede:
- Perçin: *"Çiviyi kimin çaldığını biliyor. On iki yaşında bir çocuk."*
- Baran: *"kızın kim olduğunu bilmiyor, çivilerin nereden geldiğini gayet iyi biliyor."*
- Bekmez: *"akşamları sessizce kase veriyor."*
- Uçkun: *"Nazlı'yı bir kez kovaladı."*
- Kart 4, Şirin: *"(Taşlıkta, hizmetçiye) 'Kızı gördünüz mü?'"* — hatun kızını açıkça arıyor.

Nüfusu bir avuç olan bir şehirde saray taşlığında aranan çocuğu beş yetişkin yüzünden tanıyor ve hiçbiri bağlantıyı kurmuyor. Gizlilik bir tasarım isteği; gerekçesi yazılmamış.

**Düzeltme:** Gizliliği "bilmiyorlar"dan "**söylemiyorlar**"a çevirin — daha güçlü ve zaten yarısı yazılı. Perçin söylemez (kışla defterine girmesin diye — yazılı), Bekmez söylemez (sormuyor — yazılı), Baran söylemez (alıcısı — yazılı), Uçkun söyleyemez (nöbet yerini bırakmıştı — yazılı). Not satırı şuna dönsün: *"Kim olduğu söylenmez; dördü de biliyor ve dördünün ayrı bir sebebi var."*

## Ö15 · Cesedi getiren adam ile babası aynı avluda ve aralarında hiçbir iplik yok

Kart 14: *"Dokuz yıl önce donmuş gölden bir çocuğun cesedini Yenitaç'a getiren kızak onun kızağıydı ve hâlâ onu kullanıyor."* · *"Mahzen kapağını her sabah o açıyor."*

- Kart 2 (Rüstem) ilişki ağında Kıraç/seyis **yok**.
- Kart 10 (Gökçe) ilişki ağı: *"Yenitaç'ta at sorusuna doğru cevap veren tek adam, ikisi de Karduvarlı, ikisi de bunu konu etmiyor"* — ama aynı kart Karduvar hattı için *"şehirde tek izi, saat on yedide burçta hep aynı yöne bakması"* diyor. Gökçe dokuz yıl önce o kızakla dönmüş olmalı; ahırda duran adam "tek iz" iddiasını çürütüyor.

LORE §8 çerçevesi ("üç tarafı da rahatsız ediyor, kimse katil değil") tam da bu adamı gerektiriyor ve kimse ona sahip çıkmamış.

**Düzeltme:** İki satır yeter. Rüstem'in kartına: *adamı tanıyor, dokuz yıldır tek kelime etmedi, ahıra hiç girmiyor* — mareşalin "gelinen adam" olmasının bedeli. Gökçe'nin kartına: *"tek iz" ikiye çıkar* — burçtaki bakış ve ahırdaki adam; oyuncu ikisini birleştirebilir.

## Ö16 · Üç kartta "bir şey olur" yazıp ne olduğunu yazmamışlar

- Kart 7: *"Sadık'a Yusuf'u sorarsan **tek kelimeyle** cevap alırsın ve o kelimenin acısını ancak sonradan anlarsın."* — kelime yok.
- Kart 20: *"Duvara dönük taşı çevirdiysen… Gelir, geri çevirir, **tek cümle söyler**."* — cümle yok.
- Kart 7: *"Revir duvarında bir niş var, boş, içinde tek bir çivi. Şehrin başka bir yerinde **altı boş çivi** daha var; oyuncu ikisini birleştirirse kimse ona yardım etmez."* — birleştirince ne çıkıyor, hiçbir yerde yazmıyor.

Sonuncusu en tehlikelisi: altı boş çivi LORE §1.4'ün **meşruiyet göstergesi** (asılacak altı şehir sancağı). Onu bir hekimin eksik diplomasıyla eşleştirmek kelime oyunu; oyuncu gerçek bir bağlantı arar, bulamaz.

**Düzeltme:** İki repliği yazın (Sadık'ın kelimesi: *"Ücretli."* — Yedisunak'ın adsız bıraktığı adam için en soğuğu). Yedinci çivi imasını **kesin**; Yusuf'un nişi boş kalsın ve boşluğun sebebi sorulunca *"Oraya asılacak kâğıt yazılmadı"* densin. Altı çivi tek anlam taşısın.

## Ö17 · Kavurgan başlangıç şehrine üçüncü bir işaretle giriyor

LORE §5 Kavurgan'ın görünürlüğünü **tam iki işaretle** sınırlıyor: *"**Ekranda ne görünür.** Kumkapı duvarının ötesindeki çadır pazarı… **İkinci işaret Tuzkapı'da:** o yılın Ocağı liman zincirinin kulesinin üst iki katını… kiralamış."*

- `slotlar.md` slot 18 ölçülü: *"terazisinde ara sıra güney sikkesi görünür (Kavurgan'ın ekonomik izi; kimse açıklamaz)"*.
- Kart 18 bunu büyütüyor: *"İskele borcunun bir payını güneye sattı — **bir Kavurgan Ocağının adına aracılık ediyor**… Oyuncu sorarsa **doğruyu söyler**."*

Yani başkentin iskelesinin bir payı, oyunun ilk saatinde, sorulunca açıklanan bir Kavurgan kalemi. Bu, Kumkapı Senedi'nin geç ve ağır açılışını (LORE §5: *"fatura henüz kesilmedi"*) baştan tüketiyor. Ayrıca LORE §17'nin açık soruları arasına yeni bir cevapsız kalem ekliyor: **hangi Ocak**, sırada kaçıncı sandalye?

**Düzeltme:** Slot belgesindeki hâli kalsın, kart geri çekilsin: Hafize'nin bildiği tek şey **sikkenin nereden geldiği değil, kimin getirdiği** olsun (*"Güneyden bir yazman geliyor, sikkesi ağır, adını vermiyor. Ben ağırlığa bakarım."*). Aracılık iddiası Tuzkapı açıldığında yazılsın.

## Ö18 · Üç taraftan ikisi başlangıç şehrinde temsilsiz

- **Kılıç**: Rüstem, Ragıp, Uçkun, Balaban, kışla, pano — beş ağız.
- **Taş**: yalnız Hafize — ve kartında **tek bir siyasi cümle yok**. LORE §5 Taş'ın talebini net yazıyor: *"Şehirler mahkemesini, sikkesini, surunu tutsun; **ordu dağıtılsın.**"* Hafize bunu hiç söylemiyor.
- **Gündoğdu**: yalnız kart 8'deki *"çölden gelen biri"* — adı, slotu, sahibi olmayan bir hayalet.

**Düzeltme:** Hafize'ye bir bark: *"Mareşalin on iki bini dağılırsa yevmiye ucuzlar. Ben bunu masada söylemem, burada söylerim."* — hem taraf, hem "ve haklı", hem sayı. Ve "çölden gelen adam" ya bir slot alsın (kervancı Selim'in rotası zaten Kumkapı — hazır ağız) ya da kart 8'deki teklif **bir kâğıda** dönsün (Balaban'ın yastığının altında, mühürsüz, güney eli).

## Ö19 · `simya_masa` GDD §1.2c-7'yi karşılamıyor

`slotlar.md` slot 7: *"`permQuest = simya_masa` yeniden yorumlanır: **üç ot getir, kazanı kullan**."*

GDD §1.2c-7 [ONAYLI] tam bu örneği çürütüyor:
> *"Üç ot hâlâ toplanır — ama toplama yeri mezarlık olduğu için toplamanın kendisi sahnedir ve otun **ikinci bir alıcısı** olduğu için üç ot bir bilet değil bir karardır."*

Belge yeniden yorumu yaparken sahneyi de, ikinci alıcıyı da atlamış — ve ikinci alıcı **elinin altında**: aynı belge Ela'yı *"`cayir_` köyünde ot toplayan bir ebe"* olarak rafa kaldırıyor.

**Düzeltme:** Ela rafa değil, `cayir_koy_otaci` olarak yerinde kalsın (havuz §2.7: otacı/ebe hattı hazır). Üç otu ya Yusuf'a ya Ela'ya verirsin; ikisi birbirinden nefret ediyor (havuz §2.7: *"birbirlerini sevmiyorlar"*), otun toplandığı yer mezarlık (LORE §12: kriptalar Akkemer kemerleri üstünde). Kural bedava karşılanır.

## Ö20 · Vinyet kadrosu eksik sayılmış

- `salon.md` başlığı: *"Vinyetteki **dört replik** bilerek bark olarak yerleştirildi"*. Gerçekte vinyette **on bir replik** var ve bunlardan **yedisi** kartlarda birebir bark/ilk cümle olarak geçiyor (mareşal, hekim, hatun, kâtip, tellal, taş ustası, gazi).
- `slotlar.md` (D) tablosu 0-2. dakika kadrosu: gazi (`revir_gazi`) **listede yok**, ama panel 4'te konuşuyor: *"Beşinci kez sayıyorum. Defter açılmadan sıra ilerlemiyor."* Kart 8 aynı repliği bark yapıyor.
- Panel 1'in ilk repliği **sahipsiz**: *"İskelenin üst katından bir işçi aşağıya bağırıyor: 'Örtüyü üstüne çekin! Taş su çekiyor, ipin yediği yerden akıyor!'"* — "ipin yediği yer" kart 9'da Perçin'in imza deyimi ilan edilmiş (*"'ipin yediği yer' der ve yerini gösterir"*), ama Perçin panel 1'de **aşağıda** ve cevap veren taraf. Üstelik iskelenin üst katındaki tek kanonik kişi Gökçe (slot 10: *"08 iskelenin üst katı, yirmi beş metre"*), ki slot doc onu vinyet kadrosuna **almamış**.

**Düzeltme:** Sayı "yedi replik" olarak düzeltilsin, `revir_gazi` 0-2 satırına eklensin. Panel 1'in üst kat sesi ya adsız ırgat olarak sabitlensin (o zaman "ipin yediği yer" deyimi Perçin'den alınsın, ikisi çakışmasın) ya da **Gökçe olsun** — o zaman slot 10'un "12-13. dakikada görülür" satırı düzeltilir ve varis oyuncunun ilk duyduğu ses olur. İkincisi daha iyi; ama seçilmeli.

---

# NOT

**N1 · Rüstem 58 · LORE "elli üçünde".** LORE §6: *"Elli üçünde sağ eliyle yazmayı yeniden öğrendi."* Kolunu Karaocak kuşatmasında (18-26 yıl önce) kaybetti; 58 yaşındaysa arada **on üç yıl** boyunca kendi maaş defterini imzalamamış. **Öner:** yaş **55** olsun — o zaman yeniden yazmayı öğrendiği yıl Sarp'ın felç yılına (2 kış önce) yaklaşır ve fiilin bir sebebi olur.

**N2 · Sabri'nin türküsü LORE'un vurgusunu çalıyor.** LORE §8/§6: *"adam bağırmadı, Sancar da elini titretmedi."* Kart 6: *"omuzları o tuttu, **adam bağırmasın diye** baştan sona bir türkü söyledi."* LORE'un beati Rüstem'in kendi dayanıklılığı; kart onu üçüncü bir kişinin marifetine bağlıyor. **Öner:** türkü kalsın, gerekçe düşsün — *"omuzlarını tuttu ve bir türkü söyledi; niye söylediğini kendi de bilmiyor."*

**N3 · "Babası da o terazinin başındaydı" — hangi terazi?** Kart 2 §1. Şirin'in ilk kocası *"Yenitaç'ın tahıl terazisini tutan adamdı"* (LORE §6). Antesedan belirsiz ve okuyucu iki adamı bağlıyor. **Öner:** *"babası köyün tahıl terazisindeydi"* diye ayrıştırın ya da bağlantıyı bilerek kurup yazın — ama açık bırakmayın.

**N4 · İki numaralı para kurumu.** LORE §2 Ocak için özellikle uyarıyor: *"daima sayıyla anılır ('Kırk İkinci Ocak') — Karaocak şehriyle karışmasın."* Kart 18 buna paralel bir form ekliyor: *"Yedinci Kese'den Hafize Kadın."* Doksan dokuz numaralı Ocak + kırk numaralı Kese + "Kırkbirinci Sandalye" aynı dünyada. **Öner:** Kese'ler numarayla değil **masayla/sandalyeyle** anılsın (*"Yedinci Sandalye'den Hafize Kadın"*) — LORE §10'un rütbe merdiveniyle de ("Kırkbirinci Sandalye") uyumlu olur.

**N5 · Perçin'in utancı fatura refleksine en yakın nokta.** LORE §15.1'in dört sorusuyla test edin: geçmiş kararı (dökümü beklemeden çıkmak) bugün hâlâ akıllı görünüyor mu? Kart bugün onu yalnız cezalandırıyor. **Öner:** Perçin'e bir savunma verin — *"Kalsaydım pota başında ölürdüm. Burada yirmi beş metrede ölürüm, aynı şey ama havası var."* Suç yerine mekanizma kalır.

**N6 · Mahir "adını hak etti" ama Mahir bir Karaocak iş adı değil.** Havuz §3: Karaocak *"ocaktan bir **iş adı** alır"* (Tav, Örs, Külçe, Çapak). Kart 15: *"ilk temiz dökümünü verdi, **adını hak etti**"* — ama adı Mahir. **Öner:** ocak adı **Tav** olsun, Yenitaç defteri onu "Demirci Mahir" yazsın; Perçin'in sırrının aynası olur (biri adını kazandı ama defter başka yazdı, öbürü hiç kazanamadı).

**N7 · Ölü referans.** `slotlar.md` slot 5: *"(§G 'Yenitaç düşerken' tetiği)"* — belgede **G bölümü yok** (A-E var). Kaynak `docs/konsept/reboot/22-demirboyun.md` satır 71.

**N8 · "Kaybolan içerik: sıfır" doğru değil.** Aynı bölümde iki satır önce: *"Ela'nın cadı-şifacı kimliği **rafta**"*. Rafa kalkan bir şey var; cümle düzeltilsin (LORE §15.8 rafı zaten meşru sayıyor, saklamaya gerek yok).

**N9 · "Çayırdaki goblinler artık **galeriden** çıkıyor."** Göç tablosu satır 2. Hangi galeri? LORE §9'da galeri **Karaocak derinlerinde** ve Karaocak dağın böğründe, doğuda — başlangıç çayırının komşusu değil. **Öner:** *"fosilin altındaki merdivenden çıkıyor"* (LORE §9-1 zaten orada bir açık mühür ve inen bir merdiven tarif ediyor).

**N10 · Taşdurak'ın mesafesi yazılmamış.** Göç tablosu satır 15: *"**Kumkapı yolundaki çöl durağı** olarak yeniden okunur. Harita, dekor, ekonomi aynı."* LORE §4: çöl **güneyin ucunda**. Başlangıç çayırının yanında bir çöl durağı ülkeyi bir avuca sıkıştırır. **Öner:** tabloya tek satır — "ayrı harita, kervan yolunun güney ucunda; oyuncu oraya yürüyerek değil kervanla varır" (kervancı Selim'in rotası zaten Yenitaç ↔ Kumkapı).

**N11 · Kod sayımı gevşek.** `slotlar.md` (A): *"`main.c:1409-1417` (dokuz satır, hepsi yan yana)"*. Gerçek: o aralıkta **yedi** `npc_index_by_id` çağrısı var (1409-1414, 1417) ve biri (`loca_salon_usta`) yeniden adlandırılmıyor. Ayrıca `main.c` 2329, 2331, 3320, 5736'da dört id karşılaştırması daha var — hepsi `loca_*`/`tasdurak_*` olduğu için bu turda etkilenmiyor, ama "hepsi yan yana" ifadesi yanıltıcı. Etkilenen çağrı yeri: **6 (main.c) + 2 (craft.c:18, craft.c:22) = 8**, "14" değil.

**N12 · Sabri'nin "Sen de bakıyorsun"u 16. dakikada tetiklenebiliyor.** Kart 6 §8 koşulu *"Mahzende onunla birlikte iş yaptıysan"*; mahzen vuruşu 15-18. dakikada. LORE §10 bunun bir sınıf kapısı olduğunu söylüyor. İlk yirmi dakikada oyuncuya *"sen de bakıyorsun"* denmesi, GDD §1.2d'nin "kimse oyuncuyu ayırmaz" ruhuna sürtünüyor. **Öner:** koşula ikinci bir şart: **bir salon oturumu** izlenmiş olsun. Mahzen tek başına yetmesin.

**N13 · Balaban ≈ Baran, üstelik aynı repliğin içinde.** Kart 8 barkı: *"Baran'a söyle bu ay gelmeyeceğim, maaş çıkmadı."* Konuşan **Balaban**. Havuz §6.6'nın "iki sesbirim" testini teknik olarak geçiyor ama aynı cümlede yan yana duruyorlar. Baran GDD §1.2c'de iki kez kanonik örnek olduğu için **Balaban** değişmeli. Havuz §2.1'den **Bürküt** ya da **Argın** (Argın "yorgun gazi" anlamıyla karaktere de oturuyor; havuz §7 ı-yığılması uyarısını verdiği için Bürküt daha güvenli).

**N14 · Vedia'nın 7. dakikası iki belgede farklı.** `slotlar.md` slot 5: *"oyuncu içeri girince **tek cümleyi** söyler: kasada üç ay var."* Kart 5 ilk cümlesi ise oyuncuya doğrudan bakıp *"Sana bakayım — yiyecek misin, yatacak mısın, para mı isteyeceksin?"* diye başlıyor — vinyetin *"Oyuncuya kimse bakmıyor, kimse ondan bir şey beklemiyor"* kuralına ters. **Öner:** 7. dakika **ambient** olsun (odaya söylenir), kartın ilk cümlesi E'ye basıldığında gelsin; kart bunu açıkça ayırsın.

**N15 · `STATION_ENCHANT` konumu.** `slotlar.md` (E) *"çayırın kenarına koyalım"* diyor; kodda zaten sahipsiz ve `craft.c:25` yorumu *"Karanlık Orman sınırı — keşifle bulunur"*. Öneri iyi ama "yeni konum" olarak yazılmalı, "zaten öyle" olarak değil.

---

# BU AÇILARDAN TEMİZ

- **KORKULUK (LORE §1.9).** Yirmi kartın hiçbirinde kıyamet, kehanet, seçilmişlik yok. Kart 1 §10 (*"kimseyi seçmez — oyuncuya rol, görev ya da miras işaret etmez"*) ve kart 10 (*"Oyuncuyu çağırmaz, aramaz, arkasından seslenmez"*) kuralı aktif olarak koruyor. Vinyet dosyasının başlığı da eski seçilmişlik dilini gerekçesiyle atmış.
- **"Kimse kötü adam değil" (LORE §15.6).** Yirmi slotta düz kötü yok. Hafize'nin kartında ayrı bir **"Ve haklı."** başlığı var; Yusuf'un yatağı bütçe argümanı olarak kullanması *"iki taraf da haklı"* diye kapatılıyor; Ragıp'ın yalanı *"mareşalin aleyhine"*; Bekmez'in suladığı çorbanın karşılığında iki haftadır kendi cebinden ödediği fark yazılı. Bu eksen sağlam.
- **İlk 20 dakikada görev yok (GDD §1.2d).** Kadro bunu titizlikle koruyor; seyisin talebi *"kabul edilmez, tamamlanmaz, ödenmez"* diye üç kez işaretlenmiş, on slotun "NE YAPMAZ" bölümünde *"görev vermez"* yazıyor. Tek gedik pano (Ö1) ve o da vinyetten geliyor, kartlardan değil.
- **Ulak kuralı ve perde kuralı (GDD §1.2e).** Hiçbir NPC oyuncuyu aramıyor, hiçbiri diyalog perdesini kendi açmıyor.
- **Silme testi (LORE §9).** Akkemer/mühür/damar taşı/ejderha silinse yirmi slottan **hiçbiri** düşmüyor — iddia doğru, denetledim. Tek şartlı yer mahzendeki ses (Ö10) ve `cayir_fosil_sayan`, ikisi de çift okumalı yazılmış.
- **LORE §7'nin altı ağzı eksiksiz ve rolleri doğru.** Rüstem/Sadık/Şirin/Yusuf/Vedia/Sabri altısı da slot almış; tablodaki "ne diyor" cümlelerinin altısı da bark olarak birebir taşınmış. Sabri'nin "gerçek işaretleri bilen tek kişi" konumu ve "Sen de bakıyorsun" tanıklığı kanonla harfi harfine örtüşüyor.
- **LORE §8 Tuğrul detayları.** Çizme, kazınmış kelime, gömülmeyen ceset, azalan buz, dokuz yıl, kutsanmış toprak şartı — hepsi doğru taşınmış. Rasih'in dokuz yıl önce oyduğu, yüzü duvara dönük mezar taşı LORE'a hiç dokunmadan ekleniyor ve iyi bir ekleme.
- **LORE §10 rütbeleri.** Uçkun "Yamak" (Kılıçartıkları'nın ilk rütbesi ✓), Hafize "Yazman" ve hedefi "Kese Sahibi" ✓, havuzun rütbe kelimelerini ad olarak reddetmesi ✓.
- **Kumkapı Senedi (LORE §5).** Kadro Senede hiç dokunmamış; Ö17 dışında Kavurgan hattına yeni bir okuma sokulmamış. Doğru karar.
- **Göç haritası ↔ kod.** `src/npc.c`'deki **18 kaydın 18'i** de tabloda, id'ler ve görünen adlar birebir doğru; `dunya_koy_sifaci` gerçekten son satır. `assets/dialogs/` de 18 dosya. `craft.c:18` ve `craft.c:22`'deki `ownerNpcId` tespitleri doğru. §8.1 uygulaması (id sabit, `.name` değişir) tutarlı.
- **Ölçek şişmesi yok.** Yeni tehdit, yeni örgüt, yeni felaket, yeni antik güç eklenmemiş. Yirmi kişi çivi, yevmiye, vade ve buz konuşuyor. Bu belgelerin en iyi tarafı bu ve korunmalı.
