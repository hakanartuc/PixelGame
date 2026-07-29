# HAKEM RAPORU — DÖRDÜNCÜ TUR

Kod doğrulaması önce, çünkü dört paket de aynı yerde şişiriyor: `src/shop.c` yalnızca **mevsim + itibar çarpanı** biliyor (`item_season_trade_mult`), `item.c:493` bölgesel fiyat farkını bir yorum satırı olarak bırakmış. Yani **"bölgeler arası fiyat dalgası" dördünde de mevcut mekanik değil, yeni sistem.** Kuşatma da öyle (C, D, I). Gerçekten kodda olanlar: `light.c`, `weather.c`, `gametime.c` (mevsim), `crime.c` (tanık), `sneak.c`, `lockpick.c`, `trap.c`, `ecology.c` (354 satır, yuva/göç), `contract.c` (3 kademeli bölge itibarı), `craft.c`, `worldevent.c`. Dört paket de bu listeye oturuyor — bu turda kimse mekanikten elenmiyor.

---

## 1) BEŞ OYUNCU TESTİ (eleme yetkili)

| | savaşçı | büyücü | suikastçi | tüccar | şövalye | ort |
|---|---|---|---|---|---|---|
| **C** Kandiller | 8 | 8 | **10** | **10** | 9 | **9.0** |
| **G** Diriköz | 9 | 8 | 9 | 9 | 8 | 8.6 |
| **I** İki Arada | 8 | 8 | 9 | 9 | 9 | 8.6 |
| **D** Geri Dönen | 9 | 9 | 8 | 8 | 8 | 8.4 |

**Eleme yok.** Dördü de üçüncü turun hatasını tekrarlamıyor: hiçbirinde merkeze bir alet konmamış, dördünde de madde ortada duruyor ve rol ona kendi elinden dokunuyor.

En keskin ayrım şurada: **C**, aynı maddeden beş ayrı *ekonomi* çıkarıyor ve her role bir **halka açık imza fiili** veriyor — hırsız için kandil direğinin fitil altındaki renkli bez (ölü posta kutusu), tüccar için on yedi direği kendi koruyla yakıp o yola geçiş parası koymak, şövalye için *Lampwake* töreni. Bunlar 16 pikselde görünen, tanıklı, itibar üreten fiiller. Dört pakette gördüğüm en iyi tüccar ve en iyi hırsız katmanı burada.

**G** en dürüst dengeye sahip: kor çeliğin "vurdukça ısınır, ısındıkça çağırır" kadranı savaşçıya bir taktik problemi veriyor (bir denge cezası değil, bir saat); "bir kırıntıyı düşmanın avlusuna at, muhafızlar mezarlığa koşar" cümlesi tek başına hırsız oyununu kuruyor. **D**'nin ayrıcalığı kuşamda: kor çeliği/kül çeliği ikilemi kimliği doğrudan §1.1'deki yere koyuyor — ne taşıdığın kim olduğunu söylüyor. **I** en somut ilk saate sahip (ölçü heyeti, pirinç kazık, koltuk altındaki mühürlü söküm listesi) ama finali bir **parley** olduğu için savaşçı masaya sahayı kazanarak geliyor; diğer dördü masaya oturuyor, o dışarıda kalıyor gibi kokuyor.

Ayrıcalık kusuru: C'de "ejderhaya kılıçla giden tek yol savaşçınındır / ejderha diliyle konuşan tek rol büyücüdür" gibi cümleler tek tek okununca kusur değil, **beşi birlikte** okununca simetri — her role bir "Kapısı:" satırı. §1.2b'ye aykırı; sadeleştirilmeli, biri iki paragraf almalı.

## 2) OYNANABİLİRLİK
**G 9** — fiiller: taşı kuyuya at, sandığın kapağını kapat, tuzla söndür, yem at, köyü yemin ettir, höyüğü yeniden tuzla. Kural bir *kadran* verdiği için oyuncu taktiği kendi icat ediyor.
**I 9** — şafta inmek = tarihe inmek; Graven'i geçmenin beş ayrı yolu; "her mektup bir hedef" (güvercin yok) kurye savaşını fiile çeviriyor.
**C 8** · **D 8** — D'nin "cesedi dereye sürükle" anı mükemmel ve kodda su karoları hazır (son commit: dünya suyu kıyı geçişleri).
**Metin duvarı riski:** dördünde de düşük; hepsi kuralı ilk gecede sahneyle öğretiyor.

## 3) SOYUTLUK
Dördü de **TEMİZ**. İkinci turun metafiziği hiçbirinde merkezde değil. Tek riskli nokta D: *"o ejderhanın kim olduğu senin daha önce kimi yalnız bıraktığına bağlıdır."* Bu bir mekanik değil, bir edebi jest — finalin somut kapısı belirsizleşiyor. **D: RİSKLİ (tek cümle).**

## 4) FANTEZİ HİSSİ
**D 9** — isimli, konuşan, birbirine düşmüş ejderhalar (Orvath haraç istiyor çünkü korkuyor; kırk yaşında Sirna hiçbir şey bilmiyor), ejderhaya yemin edebilme, batmış ışıyan şehir. Hakan'ın istediği yüksek fantezi en çok burada.
**C 8** · **I 8** — I'nin Graven'i ve Emberborn'u gerçek; ejderhaları kenarda.
**G 7** — **en zayıf yeri burası.** G'de ejderha bir *madde*; oyuncu oyun boyunca uyuyan bir şeyin pulunu satıyor, hiç ejderhayla karşılaşmıyor. Hanedan siyaseti de ince (tek gerçek çekişme Ashfell–Ocakoğulları).

## 5) BASİTLİK
**C 9** — "İki bin yıl kandille aydınlattık; yirmi yıldır kandiller batıdan doğuya tek tek sönüyor." Skyrim netliğinde, tek nefeste söylenir.
**G 8** — birinci cümle harika; ikinci cümlenin kancası da harika ("zengin olan herkes geceyi kaybediyor") ama cümle uzun.
**D 7** — iki cümle diyor, sonra iki paragraf daha ekliyor. Kural ihlali.
**I 6** — *"Ambervale vadisinin altından geçen köz damarı dünyanın en büyüğü olduğu için…"* Bu bir tanıtım cümlesi değil, bir jeoloji notu. Yeniden yazılmalı.

## 6) MEKANİK UYUM + DERİNLİK
**C 9** — kandil = `light.c` + yeni post-fx karartma katmanı; karanlık mahalle seviyeden bağımsız tehlikeli, yani **bölge-sabit zorluk bedava**. Fitilci direği istihbarat boşluğunu tek görünür nesneyle dolduruyor.
**G 9** — parlayan yük = **görünen suç**: `crime.c` tanık sistemi + `sneak.c` tek hamlede evleniyor. Işık gizliliğin para birimi oluyor.
**D 9** · **I 9** — I'nin "şaft derinliği = çağ" fikri, dört paketteki en iyi derin-zaman aracı: tarih bir kitapta değil, bir tünelde aşağı inerken.

## 7) KLİŞE
- **D — ÖLDÜRÜCÜ RİSK.** "Ejderhalar geri döndü" + kor/kül/sönen ateş: silüet aynı anda **Skyrim** ve **Dark Souls**. "Geri dönen kötülük değil, mülk sahibi" kırma girişimi zekice ama afişi kurtarmıyor.
- **C — YIPRANMIŞ, kırma kısmen başarılı.** Çöken imparatorluk = **Fallout**; sönen merkez ateş = **Dark Souls**. Kurtaran şey: imparatorluğun ölülerini yakmak yerine üstüne lamba dikmesi — iki bin yıllık ucuz çözümün faturası. Bu özgün.
- **G — YIPRANMIŞ → TEMİZ.** Altına hücum = **Deadwood**; ama kural tersine çevrilmiş: **A Quiet Place**'in sesi yerine ışık. Zenginliğin bedeli geceler. Dört pakette en özgün tek fikir bu.
- **I — TEMİZ/yıpranmış.** Kaynak savaşı = **Dune**, arada kalan vadi = **New Vegas**, kazı ölüyü uyandırır = **Moria**. Silüet en az tanıdık olan bu.

---

## SIRALAMA (beş oyuncu ×2 ağırlıklı)
**1) C — Sönen Kandiller 52.0** · **2) G — Diriköz 50.2** · **3) D — Geri Dönen 49.8** · **4) I — İki Arada 49.2**

Aralık gürültü seviyesinde; sıralamayı puan değil, aşağıdaki kusurlar belirliyor.

**En güçlü tek sahne.** C: karanlık bostanda topraktan çıkan el, komşu kadının kor fenerini kaldırması, elin titreyip durması, fener inince yeniden hareket etmesi — on saniye, sıfır metin. G: Taşoluk'ta cebindeki taş parlarken köpeğin mezarlığa havlaması ve taşı kuyuya attığın an yürüyen şeyin adımının ortasında donması. D: öldürdüğün adamın kalkması, kalkması, kalkması ve Gedik'in arkadan "suya sürükle!" diye bağırması. I: bir tünelde aşağı inerken kemerlerin tuğladan oymaya, ölülerin daha eski konuşmaya başlaması.

**En öldürücü kusur.** C: dünyanın merkezî fiili **bakım** — lamba yak, fitil değiştir, hat besle. Belediye aydınlatma müteahhidi hissi gerçek bir risk, ve Pyrewright'ın "yak ve temelli bitir" yolu kâğıt üstünde pahalı bırakıldığı için oyuncu bakım döngüsüne mahkûm. G: ejderha oyunda yok, sadece uyuyor — Hakan'ın istediği fantezinin yarısı eksik. D: klişe silüeti; ayrıca Ashwell finali bulanık. I: dünya bir vadiye sıkışmış, oyunun yarısı Ambervale'de geçiyor ve "onlarca bölge" vaadi zayıflıyor.

**MELEZ (somut).** Merkez **C**: kandiller sönüyor, ölüler kalkıyor. Üstüne üç aşı:
1. **G'den ışığın ikinci yüzü:** kor yalnız koruma değil **ifşa** — üstünde kor taşıyan adam gece iki mil öteden görünür, gümrük görür, hırsız görür, ejderha koklar. Aynı kural ("kor ışığında ölü kalkmaz") bozulmadan bir kadran kazanır: güvenlik = görünürlük. `crime.c` tanık + `sneak.c` anında devreye girer, bakım hissi biter.
2. **I'den şaft = çağ:** Kilnmoor'un mühürlü galerileri ve Barrowfell höyükleri, inildikçe kemeri ve ölüsü yaşlanan katmanlara bölünür. Derin geçmiş beş çağlık liste olmaktan çıkar, mekân olur.
3. **D'den ejderhalar:** C'nin dört ejderhası "uyanıyor" durumundan çıkarılıp isimli, konuşan, birbirine düşmüş **oynanabilir taraflara** dönüşür (biri haraç, biri intikam, biri yavrusunu ister) — ve oyuncu bir tanesine yemin edebilir.
Ek not: üç paket güç kaynağına *kor/köz* demiş, Kızıl Kanca/Külhan/Ashfell üç pakette tekrar ediyor. Seçilen pakette isim komitesi baştan çalışacak.

**NİHAİ ÖNERİ.** **C — Sönen Kandiller** omurga olsun; premisi tek nefeste söylenen, koddaki ışık-gece-post-fx katmanına birebir oturan ve bölge-sabit zorluğu bedava veren tek paket bu. Ama iki düzeltme şart: (a) korun ikinci yüzü **ifşa** olarak eklenecek — G'nin "parlayan mal görünen suçtur" mekaniği; bu olmadan dünya bir bakım işine dönüşür, (b) ejderhalar D'nin isimli-konuşan-bölünmüş haline getirilecek, yoksa yüksek fantezi yerine harap Roma kalır. "Oyuncu kendini ifade edebiliyor mu?" sorusuna C en net cevabı veriyor: bir avuç kor beş ayrı hayata giriyor ve oyun hiçbirini tercih etmiyor — kılıcın sırtına gömersin, fitile koyup mahalleye adını duyurursun, öğütüp ciğere çekirtirsin, hattı satın alıp yola geçiş parası koyarsın, ya da külünü çalıp yasak büyü yaparsın. Dünya sana bir alet vermiyor, bir **madde** verip ne yaptığına bakıyor; kuşandığın silahı ve zırhı Kilnmoor'da kendin seçiyorsun. G ikinci sıradadır ve yalnız tek sebeple: en özgün fikir onda, en eksik fantezi de onda — C kabul görmezse G'ye dön, ejderhayı uykudan kaldır.