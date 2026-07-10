#ifndef CRIME_H
#define CRIME_H
#include <stdbool.h>

// Suç sistemi çekirdeği: tanık tespiti + bounty sabitleri.
// Kural: suç yalnız TANIK varsa iz bırakır (bounty). Hayatta kalan kurban
// kendisi tanıktır; tanıksız cinayet iz bırakmaz (ileride Sneak sinerjisi).

#define CRIME_BOUNTY_HIT        25    // NPC'ye vuruş (tanıklı, vuruş başına değil olay başına — bkz. crime uygulaması main'de)
#define CRIME_BOUNTY_MURDER     150   // cinayet
#define CRIME_BOUNTY_RESIST     50    // muhafıza karşı koyma (Faz 2)
#define CRIME_BOUNTY_GUARD_KILL 300   // muhafız öldürme (Faz 2)

#define CRIME_WITNESS_RANGE 350.0f    // tanığın suçu görebildiği yarıçap (dünya px)

// (x,y)'deki suçun tanığı var mı? Kurban dışındaki görünür canlı NPC'ler
// range içinde aranır; kurban hayattaysa kendisi de tanıktır.
// range: normalde CRIME_WITNESS_RANGE, eğilirken SNEAK_WITNESS_RANGE (sneak.h).
bool crime_witnessed(float x, float y, int victimIdx, bool victimAlive, float range);

// --- İTİBAR (bölge başına, -100..+100) ---
// Bounty yasal borçtur (ödenince silinir); itibar toplumsal hafızadır
// (ödeme DÜZELTMEZ — karar defteri). Tanıksız suç itibarı da etkilemez.
#define REP_MIN (-100.0f)
#define REP_MAX 100.0f

#define REP_HIT        (-2.0f)    // tanıklı vuruş
#define REP_MURDER     (-15.0f)   // tanıklı cinayet
#define REP_GUARD_KILL (-25.0f)   // muhafız öldürme (kanun bilir)
#define REP_QUEST      10.0f      // bölge görevi tamamlama
#define REP_SHOPPING   0.5f       // vendor'dan alım (ekonomik bağ)
#define REP_DAILY      1.0f       // suçsuz geçen gün: 0'a doğru toparlanma

#define REP_TALK_REFUSE (-50.0f)  // bu eşiğin altında NPC konuşmaz (bounty ödense bile)
#define REP_NO_QUESTS   (-25.0f)  // bu eşiğin altında yeni görev verilmez

// Clamp'li itibar ekleme
void rep_add(float *rep, float delta);

// Vendor fiyat çarpanları: itibar +50↑ alışta ×0.9, -50↓ ×1.5 (satış tersi)
float rep_buy_mult(float rep);
float rep_sell_mult(float rep);

#endif
