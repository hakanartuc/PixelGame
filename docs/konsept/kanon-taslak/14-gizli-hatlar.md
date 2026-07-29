Rol tanımını, üç kanon dosyasını ve adı geçen kaynakların hepsini okudum (`src/skill.h`, `sneak.c/h`, `lockpick.c`, `crime.h`, `assassin.c/h`, `skilltree.c` gizli ağaç blokları, `ability.c` suikast bloğu, `perk.h`, `dialog.h`, `player.h`). Aşağıdaki her sayı koddan çıktı.

---

# GİZLİ SKILL'LER VE HATLARI — "Sönen Kandiller"

## 1. ELDE NE VAR (grep'lendi)

**Gizlenme.** XP üç yerden akar: tehlike yanında süzülme (`SNEAK_XP_TICK 4/sn`), tanıksız suç (25), sinsi vuruş (15). Algı dört çarpanın ürünü — eğilme 0.45'ten skill'le 0.30'a, mobun arkası ×0.5, araya giren ev tabanı ×0.4, gizleyici tile'a eğilmek ×0.6 (`sneak.c:37-67`). Üstüne gece ×1.5, sis, Gölge Formu ×0.05, Duman ×0.05 (`main.c:9543`). Ağaç 18 düğüm, üç kol: algı / eğilme hız cezası / sinsi vuruş çarpanı; keystone GÖLGE (+%100 sinsi, −15 HP) ve HORTLAK (gece −%30 algı, gündüz +%10).

**Kilit açma.** Üç kademe, ping-pong imleç + yeşil bölge; bölge `taban × ağaç × (1 + lvl×0.004)`, tek basış (`lockpick.c`). Ağaç 8 düğüm: bölge, imleç hızı, maymuncuk kurtarma, **başarısızlık gürültüsü yarıya** (`lockNoiseMult`), keystone KİLİT FISILDAYAN (usta kilit bir kademe kolay, bedeli her açılışta bir maymuncuk).

**Yankesicilik.** Eğilerek yaklaş + P. Şans `0.35 + lvl×0.005 + eğilme 0.25 + NPC konuşuyorsa 0.15 − muhafız 0.30 − o gün o NPC'ye her deneme 0.15`, tavan %90 (`main.c:3495`). Başarı **kirli altın** verir — harcanamaz, aklanır (kervan %75, Kör Kuyu %50, şafakta teslim: `wellGold/wellDay`). Çalınan eşyada `stolen` damgası. Yakalanınca bounty 15 + itibar −3; muhafızın cebinde yakalanmak `catchOpen` hesabını açıyor. Başarı gizli sayaç `persKetum`'u artırıyor.

**Suikast.** Ayrı skill (`SKILL_ASSASSIN`), satırı zincir bitene kadar **gizli** (`skill_set_assassin_visible`). Beş aktif: Sırttan Bıçak ×3.0, Gölge Adımı, Duman Perdesi, İğne Batır (zehir DoT), Kiriş Kesme. Ağaç 16 düğüm; "Sessiz Son" eşiği %20 → düğümlerle %35 (eşik altı hedefi tek vuruşta bitirme). Sessiz El: 5 kurgulu kontrat, rütbe eşikleri 0/3/7/13/20 — Çırak · El · Bıçak · Gölge · Usta El. Loca gece + `launderedTotal >= 500` ile açılıyor (`main.c:5526`).

**Konuşma.** Diyalogda ZAR: eşik üstü %100, altı `(speech/eşik)×%70`, başarısızlık o oturumu kilitler (`burned`). Koşul değişkenleri arasında `DVAR_KABA/KIBAR/KETUM` — kişilik izleri diyalog koşulu olarak okunabiliyor. Muhafız rüşveti eşiği 25, başarılı rüşvet itibarı −5. Ağaç 18 düğüm: fiyat / zar / kontrat-rüşvet.

**Suç.** Tanık yarıçapı 350 px, eğilirken 160. Tanık yoksa iz yok — ne bounty ne itibar (`crime.c` yorumu, `crime_witnessed`).

**KRİTİK EKSİK — ışık terimi yok.** Algı hesabında hiçbir ışık değişkeni bulunmuyor: `sneak_detect_mul` ışığa bakmıyor, `detectMul` gece/sis/perk çarpanı, `crime_witnessed` sabit yarıçap alıyor. `ITEM_TORCH` envanterdeyken ışık **veriyor** (`main.c:10945` → `hasTorch` → `light_build`), ama bu ışık yalnız çizilir; ne mob görüşünü ne tanık yarıçapını değiştirir. Kanonun "kor korur ve ifşa eder" maddesi şu an **kodda hiç yok**. En ucuz ve en yüksek getirili iş bu.

---

## 2. KANONUN GİZLİLİĞE GETİRDİĞİ ÜÇ ŞEY

**Karanlık bilgi verir.** Idris'in dersi mekanik olsun: **direğe kulak dayamak** (`E`, gece, kandil sönükse). Ekranda taş direğin yanında eğilmiş bir figür ve üç titreşen nota; oyuncu birini işaretler. Doğru nota şunları açar — o direğin kaç gecesi kaldığı, ve yerin altındaki damarın yönü (yüzeyde kısa bir turuncu ok izi olarak, 20 saniye). Kural tek cümle: **kandil yanarken hiçbir şey duyulmaz.** Yani hırsız için karanlık ödün değil çalışma koşulu; büyücü bir vadiyi aydınlatırsa aynı vadiyi sağır eder. Sneak XP kaynağı olarak "dinleme" ekle: gecede tanıksız üç direk dinlemek = 40 XP.

**Işık delildir.** Kor taşıyan görünür. Cebinde ham kor varken (veya `ITEM_TORCH` yanarken) tanık yarıçapı 350 → 560, mob algısı ×1.5. Cevap ekonomiye bağlı: **ıslak kül torbası** — koru saklar, ışığı keser, ama iki gecede korun bir kısmını kavlatır (satış değeri düşer). Hırsızın kararı artık kâr-görünürlük dengesi: dolu torbayla sessiz git, yarım torbayla parlak koş. Kor ağzı (ısıyı izleyen kazıcı) bu kuralın ikinci dişi — kaçmak için parayı yere bırakmak.

**Ölüler kalkandır.** Yürüyen kolonunun içinde canlı ayırt edilmiyor: kolonla aynı hızda, sekiz karo içinde, eğilmeden yürürsen kolcular ve kapı bekçileri seni tanık taramasına almaz. Ama iki şart: **el boş** (kor/fener ışığı seni ele verir) ve **konuşmamak**. Bu, ağır zırhlıya da açık olduğu için rol özgürlüğünün en temiz kanıtı; kolon zaten zırhlı yürüyor.

---

## 3. SKILL'LERİN BU DÜNYADAKİ ANLAMI

**Gizlenme görünmemek değil, duyulmamak.** Höyük iti izi sesle sürer, Sağır titreşimle avlanır, ölü adını duyar. O yüzden zırh sınıfı algıya girsin: ağır zırh gece ×1.3, orta ×1.15, hafif nötr — ve bu ceza **yalnız karanlıkta** işler, ışıkta zaten görülüyorsun. Zemin ikinci değişken: tahta döşeme ×1.4, kum/kül ×0.8. Sağır dövüşünün ("kumda dur, tahtada durma") mekaniği böylece dünyanın geneline yayılır, boss'a özel hile olmaktan çıkar.

**Kilit açmak bazen anahtar bulmaktır.** İmparatorluk kilidi = Usta kilit ve maymuncuk kırar. Ama karşılığı `Duran`ın kemerinde asılı: **bekçi anahtarı**. Duran kovalamıyor, üç dört karo hattı var, üstünde lejyon zırhı — yani anahtar dövüşle de, dolanıp yankesicilikle de, gündüz uykusunda sinsi vuruşla da alınır. Aynı kapı için üç ayrı skill. Anahtar tek kapıya değil bir **kademeye** açsın: taşıyan oyuncu tüm imparatorluk kilitlerinde bir kademe iner (KİLİT FISILDAYAN ile üst üste binmez).

**Yankesicilik istihbarat aletidir.** Bu dünyada en değerli cep içeriği altın değil: fitil altındaki **renkli bez** (Fitilci mesajı), **mumlu defter yaprağı** (Kuyu Gözü çetelesi), **kayıt pulu**, **damga kalıbı**, boş **ruhsat numarası**. Bez çaldığında haritada bir direk işaretlenir; mumlu yaprak çaldığında bir kervan teslim saati açılır. Kirli altın sistemi olduğu gibi kalsın — bu kalemler harcanmıyor, **bilgi** olarak düşüyor.

**Konuşma kayıt tutmaktır.** Akşam töreni halk önünde ad okumak; deftertutan olmak eski elyazmasını okumak; hane defterinde bir satırı tartışmak. Speech zarı bu üç işte de aynı zar. Ve Söndürücü sözleşmesinin **ruhsat numarası** buradan çıkıyor: numara almak bir Speech kontrolü + bir defter erişimi. Yani en sessiz meslek en çok konuşana bağımlı.

---

## 4. BEŞ GİZLİ HAT

**FİTİL OKUYAN** — *bez*. Yankesicilik + Konuşma + Gizlenme + Hafif Zırh. Fitilciler alır (Nell Harrow). İlan anı: bir sabah kapının altında **delikli bir kuruş**; hiçbir iş yapmadın, ödeme geldi — ağ senin geçtiğin direkleri saymış. Gücü: dokuz menzilin bezini bir mevsim kendi rengine çevirirsen ağ alışkanlıkla senin olur. Zayıflığı: bez **yağmurda bozulur**, yağmurlu gecede taşınan mesaj yarı yolda okunmaz hale gelir ve gecikmeden Terazici Sevil senin çifte oynadığını anlar. Çifte üyelik riski: en yüksek olan hat bu — Fitilci mesajı yanan kandilde okunur, Kuyu Gözü çetelesi karanlıkta el değiştirir; ikisini birden taşıyan adam aynı direğin iki zıt halini istiyor.

**BOĞUCU** — *kutu*. Suikast + Simya + Gizlenme + Tek El. Söndürücüler alır (Corbin "Islak" Deel). İlan anı: ilk tanıksız ölümünden sonraki sabah eşikte bir tuz kesesi, içinde **soğuk köz**, üstünde tebeşirle bir numara. Yara yok, kan yok, adam üşür, sabah ölür — yani sivillerin öldürülemezliği kuralıyla çatışmayan bir cinayet: kurban revire düşer ve **üç gün sonra** kayıtta ölü görünür. İşin yarısı zaten adam değil: "Bir direk. Kimse ölmeyecek." Boğma kutusu (boss lootu) gürültüsüz söndürme veriyor. Zayıflığı: numara. Numarasız öldürürsen puan negatife düşer ve **seni öldürecek örgüt senin satırını defterden satın alır** — yani ölümün için gereken kâğıdı sen üretmiş olursun.

**KAVLATAN** — *tuz*. Kilit açma + Simya + Konuşma + Demircilik. Kızıl Kanca alır (Selde Mott). Tuzlu suda kalmış kor yanlış perdeden çıtırdar: ışık verir, ısıtır, ucuzdur, **ölüyü bastırmaz**. Hattın işi ucuz kor üretip yasal hatta sokmak: mühürlü depo kilidi, kavlatma kazanı, ve pazarda ağzın. İlan anı ekranda: sattığın üç direk yandıktan sonra bir NPC seni adınla değil **lakabınla** çağırır — `npc_bark` üstünden, "Kavlatan geldi." Beş dakika testini geçer, kimse anlatmaz. Zayıflığı: dördüncü gece o mahalle boşalır ve itibar bölgesel çöker. Çifte üyelik riski: Fitilciler kavlatanı fitil ayar taşıyla tespit eder — aynı anda Fitilci olan Kavlatan kendi malını ihbar etmek zorunda kalır.

**ÖLÜYLE YÜRÜYEN** — *kolon*. Gizlenme + Bitkicilik + Yankı + (istersen Ağır Zırh). Kalkan Nöbeti alır — höyük tarlasında kılavuz, kazı ruhsatı taşıyıcısı. Bitkicilik neden: kolonla yürümek için ölü kokusu merhemi kaynatılıyor (kurt yaklaşmıyor, höyük iti izi kaybediyor). Yankı neden: Fısıltı Patlaması kolonun ön safını böler, bir gecelik kapı açar. İlan anı: bir Yürüyen arkanda yürümeye başlar ve **sen durunca o da durur.** Zayıflığı: ışık. Elinde kor varken kolon kalkan değil hedef, kolcular tam olarak parlayan şeye bakıyor. İkincisi: kolonun içindeyken kılıç çekmek düzeni bozar, kol durur ve bocalar — kaçarken arkanda dokuz yüz şaşkın ölü bırakırsın.

**NUMARACI** — *satır*. Konuşma + Yankesicilik + Büyüleme + Kilit açma. Söndürücülerin kâtip kanadı ve Naip Vekili Wilmot Serle alır. İşi ruhsat numarası çıkarmak, damga kalıbını yeniden basmak (cüruf-mumlu mührü kor kıymığıyla eritmek — Büyüleme buradan giriyor), ve bir hane defterinde satır tartışmak. İlan anı: kapının altında **imzalı ama numara hanesi boş** bir sözleşme; birisi senden doldurmanı bekliyor. Zayıflığı: kâğıt üretiyorsun, kâğıt kalıyor. Numaracı öldükten sonra da suçlanabilen tek hat bu. Çifte üyelik riski en kârlısı: hem Söndürücülere numara satıp hem Sessiz El'e "bu numara kimin adına çıktı" bilgisini satmak — iki taraf da bir gün aynı satırı okuyacak.

**Hatlar birbirini besliyor:** Boğucu'nun numarasını Numaracı yazar. Kavlatan'ın malını Fitil Okuyan aklar (bez, sahte hattı yasal gösterir). Ölüyle Yürüyen'in getirdiği kurgan defteri sayfası Numaracı'nın hammaddesi. Silo yok.

---

## 5. ÇİFTE ÜYELİK VE İHANET

**Kendi Kutun** — Fitil Okuyan hattı bir menzilin bakımını devraldığı gece direği açar ve içindeki bez kendi ustasını hedef gösterir. Karar: bezi bırak (usta ölür, ağ sana kalır) ya da bezi değiştir (ağ üç gün sonra hem ustayı hem seni arar).

**Yanlış Kutu** — bez ve mumlu yaprak aynı slotta durur; bez yağmurda ıslanır, mum sıcakta erir. Yanlış kutuya doğru mesajı bırakan oyuncu üç gün sonra iki ağın da aynı kişiyi aradığını görür: kendini. Bunu **mevsim + hava** sistemine bağla, zaten var; yaz gecesi mum, yağmurlu gece bez kaybediyor.

**Bedel — kanon kararı:** rütbe sıfırlanmaz, **negatife düşer.** Negatif rütbe iki şey demek: (1) o örgütün kontrat panosunda artık senin adın bir satır — pano açılınca en üstte kendi hedefin görünür, ki bu ekran testini tek karede geçer; (2) rakip örgütün kapısı açılır ve ilk teklifi eski ustan olur. İhanet bir kayıp değil **taraf değişikliği** — ama üç kez taraf değiştiren adamı hiçbiri almaz, çünkü `persKetum` sayacı bir eşiği geçtiğinde diyalog koşulu olarak aleyhine çalışır (altyapı `DVAR_KETUM` ile hazır).

---

## 6. BEŞ ROL, TEK KAPI

Tam zırhlı şövalye bir direği söndürebilir: boğma kutusu gürültüsüz, Sneak gerekmiyor, gereken şey tanıksız an. Savaşçı bekçi anahtarını Duran'ı keserek alır. Büyücü kandili yakarak bir vadiyi **sağır** eder ve orada kimse kimseyi duymaz — bu bir gizlilik hamlesi. Tüccar tanığı satın alır (rüşvet eşiği 25, zaten kodda) ya da kolcuyu kendi hattına maaşa bağlar. Suikastçi zaten evinde. Hiçbir hat tek role kilitli değil: her hattın dört skill'inden en az biri savaş ya da meslek.

---

## 7. NE GEREKİYOR

| İş | Maliyet |
|---|---|
| `hasTorch`/kor → `detectMul` çarpanı | ~10 satır, `main.c:9543` bloğuna |
| Tanık yarıçapı ışıkla 350→560 | `crime_witnessed` zaten range parametreli — çağrı yerinde çarpan |
| Zırh sınıfı + zemin → algı | `ArmorClass` var; zemin için `TileType`'a `noisy` bayrağı [YENİ MEKANİK, küçük] |
| Islak kül torbası, soğuk köz, boğma kutusu, bekçi anahtarı | item; paralel dizi tavanlarına dikkat |
| Direğe kulak dayama (üç nota) | mevcut sahne/diyalog sistemiyle, yeni sistem değil |
| Bez rengi okuma/değiştirme | dekor durumu + küçük UI [YENİ MEKANİK, orta] |
| "Tanıksız eylem" sayacı | `Player`'a bir int + save sürümü |
| Kolona karışma | tanık taramasında istisna + kolon mobu [YENİ MEKANİK, orta — kolon zaten `[AÇIK]` teknik karar] |
| Negatif rütbe | `assassinPoints` negatife açık; pano render'ında bir satır |

**Kendi işaretlediğim çelişkiler.** (1) Mevcut Sessiz El kontratları goblin/iskelet öldürmek — kanonun kayıt-temelli hedefleriyle uyuşmuyor; ama sivillerin öldürülemezliği yüzünden bu iyi bir kaza: kontratların yarısı **nesne hedefli** olsun (bir direk, bir mühür, bir defter), yarısı soğuk közle "üç gün sonra ölen" hedefli. (2) Kanon "ışık kalkmayı engeller, kalkmış olanı durdurmaz" diyor; ben kolona karışmayı **yalnız karanlıkta** çalıştırdım — kolon ışıkta yürümeye devam eder ama kolcular içindeki canlıyı görür. Bu benim kararım, kanonda yazılı değil. (3) Sessiz El "apolitik", Söndürücüler "numaralı": iki kapının kırmızı çizgisi farklı olsun — Sessiz El'i **tanık** bırakınca atarlar, Söndürücüler **numarasız** öldürünce. Aynı skill, iki ayrı ahlak. **[AÇIK]**: kavlatılmış korun oyuncuya nasıl fark ettirileceği (perde sesi mi, çıtırtı animasyonu mu) hâlâ bestiyerden devreden açık madde; ben lakap bark'ıyla geçici bir cevap verdim.