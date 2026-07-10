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
// yarıçap içinde aranır; kurban hayattaysa kendisi de tanıktır.
bool crime_witnessed(float x, float y, int victimIdx, bool victimAlive);

#endif
