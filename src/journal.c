#include <stdio.h>
#include <string.h>
#include "journal.h"
#include "font.h"
#include "gametime.h"
#include "worldmap.h"

// --- Giriş havuzu -----------------------------------------------------------
static JournalEntry entries[JOURNAL_MAX];
static int entryCount = 0;

void journal_reset(void) { entryCount = 0; }
int  journal_count(void) { return entryCount; }

const JournalEntry *journal_get(int i)
{
    return (i >= 0 && i < entryCount) ? &entries[i] : NULL;
}

bool journal_add(int type, int param)
{
    if (entryCount >= JOURNAL_MAX) return false;
    entries[entryCount++] = (JournalEntry){
        (Sint16)type, (Sint16)param, (Sint16)gametime_day() };
    return true;
}

bool journal_add_unique(int type, int param)
{
    for (int i = 0; i < entryCount; i++)
        if (entries[i].type == type && entries[i].param == param)
            return false;
    return journal_add(type, param);
}

void journal_load_entry(int type, int param, int day)
{
    if (entryCount >= JOURNAL_MAX) return;
    entries[entryCount++] = (JournalEntry){
        (Sint16)type, (Sint16)param, (Sint16)day };
}

// --- Gün dönümü satırları: seyrek atmosfer (spam değil) ----------------------
static const struct { int day; const char *text; } DAY_NOTES[] = {
    { 3,  "Fısıltıyı ilk kez gündüz duydum. Kısa. Sabırlı. Sanki acelesi yok." },
    { 7,  "Yedinci gün. Rüyalarım hep aynı çukurla açılıyor. Kazan ellerin "
          "kime ait olduğunu hâlâ bilmiyorum." },
    { 14, "İki hafta oldu. Köylüler bana alıştı; mühür bana alışmadı — "
          "geceleri daha sıcak atıyor." },
    { 21, "Sessiz Gün'ü soruyorum, kimse konuşmak istemiyor. Susmaları "
          "cevaptan daha çok şey söylüyor." },
    { 30, "Bir ay. Buraya gelme sebebimi hatırlamıyorum ama gitme isteğim "
          "de yok. Sanki bir şey bekliyorum. Ya da bir şey beni." },
};

void journal_day_tick(int day)
{
    for (size_t i = 0; i < sizeof(DAY_NOTES) / sizeof(DAY_NOTES[0]); i++)
        if (DAY_NOTES[i].day == day)
            journal_add_unique(JE_DAY_NOTE, day);
}

// --- Metin türetme: kod+param → birinci ağız (dil tek noktadan) --------------
static void entry_text(const JournalEntry *e, char *out, size_t cap)
{
    const char *body = "";
    char buf[300];

    switch ((JournalEvent)e->type) {
    case JE_WAKE:
        body = "Yol kenarında uyandım. Buraya nasıl geldiğimi bilmiyorum. "
               "Göğsümde bir işaret var — sıcak, ve benimle birlikte atıyor.";
        break;
    case JE_QUEST_ACCEPT:
        body = (e->param >= 0 && e->param < QUEST_COUNT &&
                QUESTS[e->param].journal != NULL)
             ? QUESTS[e->param].journal : "Yeni bir iş üstlendim.";
        break;
    case JE_QUEST_DONE:
        if (e->param >= 0 && e->param < QUEST_COUNT) {
            snprintf(buf, sizeof(buf), "Sözümü tuttum — %s bitti. "
                     "İnsanın sözünün geçtiği bir yer, ev gibi kokuyor.",
                     QUESTS[e->param].name);
            body = buf;
        } else body = "Bir işi bitirdim.";
        break;
    case JE_ECHO_WITNESS:
        body = (e->param == 0)
             ? "Yol kenarında devrik bir araba buldum. Sahibinin son sabahını "
               "gördüm: yün eldivenler, bal küpleri, ocakta unutulmuş süt. "
               "Onu toprağa verdim. Adını bilmiyorum ama artık taşı var."
             : "Bir yankıya dokundum ve dinledim. Ölüler susmuyor; "
               "sadece dinleyeni bekliyorlar.";
        break;
    case JE_ECHO_CONSUME:
        body = (e->param == 0)
             ? "Devrik arabanın yanındaki soluğu kendime çektim. Daha güçlü "
               "hissediyorum. Adamın son sabahını da aldım — süt kokusunu "
               "artık ben hatırlıyorum, o değil. Düşünmemeye çalışıyorum."
             : "Bir yankıyı kendime çektim. Güç sıcak; arkasında kalan "
               "boşluk soğuk.";
        break;
    case JE_ECHO_YAZIT:
        body = "Köydeki dikilitaşı okudum. Dil eski, harfler yorgun — ama "
               "mühür sanki satırları benden önce tanıdı.";
        break;
    case JE_BOSS_KILL:
        body = "Eski Geçit'in dibinde, nöbetini ölümünden sonra da bırakmayan "
               "şeyi devirdim. O gece fısıltı ilk kez tam bir cümle kurdu. "
               "Keşke kurmasaydı.";
        break;
    case JE_DEATH:
        body = "Düştüm. Karanlık beni bir kez daha geri verdi — köyün ateşinin "
               "yanında açtım gözümü. Neden hep geri veriyor?";
        break;
    case JE_DUNGEON_FIRST:
        body = "Eski Geçit'e ilk kez adım attım. Taşlar soğuk değil; "
               "bekliyor gibiler. Mühür içeride hiç susmadı.";
        break;
    case JE_NEST_CLEANSE:
        body = (e->param == 0)
             ? "Goblin reisini devirdim; kamp gözümün önünde dağıldı. Reissiz "
               "goblin, çobansanız sürü gibi — arkalarına bakmadan kaçtılar."
             : (e->param == 1)
             ? "Orkların savaş totemini yıktım. Totem devrilince bağırışları "
               "öfkeden değil korkudandı. Bu vadiye bir daha yerleşemezler."
             : (e->param == 2)
             ? "Slime gölcüğünü meşaleyle kuruttum. Toprak günlerdir ilk kez "
               "nefes aldı sanki."
             : "Lanetli mezarı mühürledim. Elimi taşa koyduğumda göğsümdeki "
               "işaret aynı ritimde attı — sanki ikisi aynı dili konuşuyor.";
        break;
    case JE_DAY_NOTE: {
        body = "Günler geçiyor.";
        for (size_t i = 0; i < sizeof(DAY_NOTES) / sizeof(DAY_NOTES[0]); i++)
            if (DAY_NOTES[i].day == e->param) body = DAY_NOTES[i].text;
        break;
    }
    }
    snprintf(out, cap, "Gün %d — %s", (int)e->day, body);
}

// --- Kitap çizimi ------------------------------------------------------------
// Book_UI.png: parşömen kitap src (8,6,224,133); yer imi şeritleri
// kırmızı (752,31,22,18) / mavi (800,31,22,18). 4x ölçek → 896×532.
#define BOOK_SCALE 4.0f
#define PAGE_TEXT_W 340.0f
#define PAGE_TEXT_H 420.0f

// Akış yerleşimi: her blok bir sütuna (sayfaya) sığdırılır; taşarsa sonraki
// sütun. draw=false ile yalnız sayılır (sayfa sayısı için).
typedef struct {
    R2D *r; TTF_Font *font;
    float lx, rx, ty;      // sol/sağ sayfa metin x'i + üst y
    int col;               // 0-tabanlı sütun (spread = col/2)
    float cursorY;
    int page;              // çizilecek spread
    bool draw;
} Flow;

static void flow_block(Flow *f, const char *text, SDL_Color color)
{
    int tw = 0, th = 0;
    TTF_GetStringSizeWrapped(f->font, text, 0, (int)PAGE_TEXT_W, &tw, &th);
    float h = (float)th + 16.0f;   // blok arası nefes

    if (f->cursorY + h > f->ty + PAGE_TEXT_H && f->cursorY > f->ty) {
        f->col++;                  // sayfa doldu → sonraki sütun
        f->cursorY = f->ty;
    }
    if (f->draw && f->col / 2 == f->page) {
        float x = (f->col % 2 == 0) ? f->lx : f->rx;
        draw_text_wrapped(f->r, f->font, text, x, f->cursorY,
                          PAGE_TEXT_W, color);
    }
    f->cursorY += h;
}

int journal_render(R2D *renderer, TTF_Font *font, R2DTexture *bookTex,
                   const QuestStatus *quests, int tab, int page,
                   int winW, int winH)
{
    float bw = 224.0f * BOOK_SCALE, bh = 133.0f * BOOK_SCALE;
    float bx = (winW - bw) / 2.0f, by = (winH - bh) / 2.0f + 10.0f;

    // Arka karartma + kitap
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);
    r2d_set_color(renderer, 8, 6, 12, 150);
    SDL_FRect dim = { 0, 0, (float)winW, (float)winH };
    r2d_fill_rect(renderer, &dim);
    if (bookTex != NULL) {
        SDL_FRect src = { 8, 6, 224, 133 };
        SDL_FRect dst = { bx, by, bw, bh };
        r2d_draw_texture(renderer, bookTex, &src, &dst);
    }

    // Sekme şeritleri: kitabın üstünden sarkar; aktif olan daha uzun iner
    SDL_Color ink    = { 62, 44, 30, 255 };    // parşömen üstü koyu mürekkep
    SDL_Color faded  = { 120, 100, 84, 255 };
    SDL_Color tabCol = { 255, 240, 210, 255 };
    for (int t = 0; t < 2; t++) {
        float rxp = bx + 60.0f + t * 150.0f;
        float drop = (t == tab) ? 26.0f : 8.0f;
        if (bookTex != NULL) {
            SDL_FRect rsrc = { (t == 0) ? 752.0f : 800.0f, 31, 22, 18 };
            SDL_FRect rdst = { rxp, by - 34.0f + drop, 66, 54 };
            r2d_draw_texture(renderer, bookTex, &rsrc, &rdst);
        }
        draw_text(renderer, font, (t == 0) ? "Görevler" : "Günlük",
                  rxp + 74.0f, by - 20.0f + drop,
                  (t == tab) ? tabCol : faded);
    }

    // Metin akışı
    Flow f = {
        .r = renderer, .font = font,
        .lx = bx + 60.0f, .rx = bx + bw / 2.0f + 36.0f,
        .ty = by + 44.0f, .col = 0, .cursorY = by + 44.0f,
        .page = page, .draw = true,
    };

    if (tab == 0) {
        // GÖREVLER: üstlenilmiş işler birinci ağız + durum satırı
        bool any = false;
        for (int i = 0; i < QUEST_COUNT; i++) {
            if (quests[i].state == QUEST_NOT_STARTED) continue;
            any = true;
            bool done = quests[i].state == QUEST_REWARDED;
            SDL_Color c = done ? faded : ink;
            char title[96];
            snprintf(title, sizeof(title), "~ %s ~", QUESTS[i].name);
            flow_block(&f, title, c);
            flow_block(&f, QUESTS[i].journal != NULL
                           ? QUESTS[i].journal : "...", c);
            char st[64];
            if (quests[i].state == QUEST_ACTIVE)
                snprintf(st, sizeof(st), "   ( %d / %d )",
                         quests[i].progress, QUESTS[i].targetCount);
            else if (quests[i].state == QUEST_DONE)
                snprintf(st, sizeof(st), "   ( Sözü yerine getirdim — "
                         "haber vermeliyim )");
            else
                snprintf(st, sizeof(st), "   ( Tamamlandı )");
            flow_block(&f, st, c);
        }
        if (!any)
            flow_block(&f, "Henüz üstlenilmiş bir iş yok. Köy küçük; "
                           "iş kendini buldurur.", faded);
    } else {
        // GÜNLÜK: kronolojik yaşanmışlıklar
        if (entryCount == 0)
            flow_block(&f, "Henüz yazacak bir şey yaşamadım.", faded);
        for (int i = 0; i < entryCount; i++) {
            char line[360];
            entry_text(&entries[i], line, sizeof(line));
            flow_block(&f, line, ink);
        }
    }

    int pageCount = f.col / 2 + 1;

    // Alt bilgi: sayfa + tuşlar
    char foot[80];
    snprintf(foot, sizeof(foot), "%d / %d   [Tab] Sekme  [<-/->]  Sayfa  [J] Kapat",
             page + 1, pageCount);
    int tw = 0, th = 0;
    TTF_GetStringSize(font, foot, 0, &tw, &th);
    SDL_Color footCol = { 200, 195, 215, 255 };
    draw_text(renderer, font, foot, (winW - tw) / 2.0f, by + bh + 10.0f, footCol);

    return pageCount;
}
