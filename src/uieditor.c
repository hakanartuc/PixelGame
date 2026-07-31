#include "uieditor.h"
#include "font.h"
#include <stdio.h>

#define HANDLE_R      5.0f    // tutamak işaretinin yarı-kenarı
#define GRAB_PAD     10.0f    // etiket kutusuna eklenen yakalama payı
#define NUDGE_FINE    1.0f
#define NUDGE_COARSE 10.0f
#define STATUS_TIME   2.5f
#define GUIDE_SNAP    4.0f    // hizalama kılavuzunun yakalama mesafesi (px)

// Izgara adımları: kapalı → 8 → 16 → 32 (G ile döner). 8 pikselden başlıyor
// çünkü HUD metin satır aralıkları 20-26px, yani 8'in katları doğal duruyor.
static const float GRID_STEPS[] = { 0.0f, 8.0f, 16.0f, 32.0f };
#define GRID_STEP_COUNT ((int)(sizeof(GRID_STEPS) / sizeof(GRID_STEPS[0])))

static bool  s_active   = false;
static int   s_sel      = -1;     // seçili öğe (-1 = yok)
static bool  s_dragging = false;
static bool  s_resizing = false;  // sağ-alt köşe tutamağı çekiliyor
static float s_grabDX   = 0.0f;   // fare ile öğe noktası arasındaki fark
static float s_grabDY   = 0.0f;
static bool  s_dirty    = false;  // kaydedilmemiş değişiklik var
static char  s_status[96] = { 0 };
static float s_statusT  = 0.0f;

// Render'da saklanan font. Olay işleyicinin fontu yok ama tıklama kutusunu
// ETİKETİN ölçüsünden çıkarmak zorunda: yoksa yakalama alanı görünen yazıdan
// çok daha küçük olur ve etikete tıklamak tutmaz. Render her kare çalıştığı
// için olaylar geldiğinde bu her zaman dolu.
static TTF_Font *s_font = NULL;

static int   s_gridIdx = 1;        // GRID_STEPS indeksi (1 = 8px, açık başlar)
// Sürüklerken yakalanan hizalama kılavuzları. Çizim için saklanıyor: kullanıcı
// NEDEN oraya yapıştığını görmeli, yoksa sürükleme "takılıyor" gibi gelir.
static bool  s_guideX = false, s_guideY = false;
static float s_guideXPos = 0.0f, s_guideYPos = 0.0f;

static bool  s_preview = true;   // ön izleme açık başlar: panel görülmeden dizilemez

static float grid_step(void) { return GRID_STEPS[s_gridIdx]; }

const char *uieditor_preview(void)
{
    if (!s_active || !s_preview) return NULL;
    if (s_sel < 0 || s_sel >= uilayout_count()) return NULL;
    return uilayout_name_at(s_sel);
}

static void status(const char *msg)
{
    SDL_strlcpy(s_status, msg, sizeof(s_status));
    s_statusT = STATUS_TIME;
}

bool uieditor_active(void) { return s_active; }

void uieditor_toggle(void)
{
    s_active = !s_active;
    s_dragging = false;
    if (s_active) {
        if (uilayout_count() == 0) {
            // Yerleşim yüklenmemişse düzenlenecek bir şey yok. Sessizce boş bir
            // ekran göstermek yerine sebebini söyle.
            status("Yerlesim BOS — layout.json yuklenemedi");
            SDL_Log("uieditor: layout.json yuklenmemis, duzenlenecek oge yok");
        } else {
            if (s_sel < 0) s_sel = 0;
            status("Duzenleyici ACIK");
        }
    } else if (s_dirty) {
        // Kapatırken kaydedilmemiş değişikliği hatırlat — sessizce kapanıp
        // sonra "kaydetmiş miydim?" sorusunu bırakmasın
        SDL_Log("uieditor: KAYDEDILMEMIS degisiklik var (Ctrl+U ile don, Ctrl+S)");
    }
}

// Öğenin ekrandaki tutamak noktası (hiza uygulanmadan: çapa + ofset)
static bool item_point(int i, float *x, float *y)
{
    return uilayout_point_at(i, x, y);
}

// Etiketin çizim kutusu. Hizaya göre noktanın soluna/ortasına/sağına oturur —
// böylece tutamağın kendisi hizayı GÖSTERİR (sağa hizalı öğenin etiketi sola
// doğru uzar). Hem geri bildirim hem tıklama alanı.
static SDL_FRect label_rect(TTF_Font *font, const char *name, UiAlign align,
                            float px, float py)
{
    int tw = 0, th = 0;
    if (font != NULL) TTF_GetStringSize(font, name, 0, &tw, &th);
    if (th <= 0) th = 16;
    float w = (float)tw, h = (float)th;
    float x = px;
    if (align == UI_ALIGN_CENTER)     x = px - w * 0.5f;
    else if (align == UI_ALIGN_RIGHT) x = px - w;
    return (SDL_FRect){ x, py, w, h };
}

// Boyutu veriden gelen öğenin gövde dikdörtgeni. Hiza noktanın öğenin neresi
// olduğunu söylediği için sol kenar buradan çıkıyor — çizim, tıklama ve boyut
// tutamağı üçü de bu tek fonksiyondan besleniyor.
// h == 0 (yükseklik koddan geliyor) durumunda tutamak kutusunun nominal
// yüksekliği. Kutu "genişlik denetlenebilir, yükseklik değil" demek için var.
#define WIDTH_ONLY_H 30.0f

static bool body_rect(int i, SDL_FRect *out)
{
    float w, h;
    if (!uilayout_size_at(i, &w, &h)) return false;
    if (h <= 0.0f) h = WIDTH_ONLY_H;
    float px, py;
    if (!item_point(i, &px, &py)) return false;
    UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
    if (!uilayout_raw_at(i, &an, &al, &va, &ox, &oy)) return false;
    float x = px, y = py;
    if (al == UI_ALIGN_CENTER)      x = px - w * 0.5f;
    else if (al == UI_ALIGN_RIGHT)  x = px - w;
    if (va == UI_VALIGN_MIDDLE)     y = py - h * 0.5f;
    else if (va == UI_VALIGN_BOTTOM) y = py - h;
    if (out) *out = (SDL_FRect){ x, y, w, h };
    return true;
}

// Sağ-alt köşedeki boyut tutamağı
static bool resize_handle_rect(int i, SDL_FRect *out)
{
    SDL_FRect b;
    if (!body_rect(i, &b)) return false;
    if (out) *out = (SDL_FRect){ b.x + b.w - 9.0f, b.y + b.h - 9.0f, 18.0f, 18.0f };
    return true;
}

static bool hit_test(TTF_Font *font, int i, float mx, float my)
{
    float px, py;
    if (!item_point(i, &px, &py)) return false;
    UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
    if (!uilayout_raw_at(i, &an, &al, &va, &ox, &oy)) return false;
    // Boyutu olan öğede yakalama alanı GÖVDE (panelin her yerinden tutulur);
    // boyutsuz öğede etiket kutusu + pay
    SDL_FRect r;
    if (!body_rect(i, &r)) {
        r = label_rect(font, uilayout_name_at(i), al, px, py);
        r.x -= GRAB_PAD; r.y -= GRAB_PAD;
        r.w += GRAB_PAD * 2.0f; r.h += GRAB_PAD * 2.0f;
    }
    return (mx >= r.x && mx < r.x + r.w && my >= r.y && my < r.y + r.h);
}

// ÜST ÜSTE BİNEN TUTAMAKLAR: aynı noktaya tekrar tıklamak ALTTAKİNİ seçer.
// Beş panel (dükkan, ilan panosu, beceri, craft, suikast panosu) ekranda aynı
// yuvayı paylaşıyor; tutamakları çakışıyor. Bu olmasa üsttekinden aşağı
// inmenin tek yolu Tab olurdu ve hangi öğede olduğunu kestirmek zorlaşırdı.
static int pick_at(TTF_Font *font, float mx, float my)
{
    int n = uilayout_count();
    // Seçili öğe bu noktada mı? Öyleyse ondan SONRAKİ isabeti ara (döngüsel).
    if (s_sel >= 0 && s_sel < n && hit_test(font, s_sel, mx, my)) {
        for (int k = 1; k <= n; k++) {
            int i = (s_sel + k) % n;
            if (hit_test(font, i, mx, my)) return i;
        }
        return s_sel;
    }
    // Sondan başa tara: üstte çizilen (sonraki) öğe önce yakalanır
    for (int i = n - 1; i >= 0; i--)
        if (hit_test(font, i, mx, my)) return i;
    return -1;
}

// Seçili öğeyi ekranda YERİNDE tutarak çapasını değiştir: yeni ofset =
// eski nokta - yeni çapa noktası. Çapa değiştirmenin öğeyi ekranın öbür ucuna
// fırlatması en can sıkıcı davranış olurdu.
static void cycle_anchor(int dir)
{
    UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
    if (!uilayout_raw_at(s_sel, &an, &al, &va, &ox, &oy)) return;
    float px, py;
    if (!item_point(s_sel, &px, &py)) return;
    UiAnchor next = (UiAnchor)(((int)an + dir + UI_ANCHOR_COUNT) % UI_ANCHOR_COUNT);
    float nax, nay;
    uilayout_anchor_point(next, &nax, &nay);
    uilayout_set_raw(s_sel, next, al, va, px - nax, py - nay);
    s_dirty = true;
    char buf[64];
    snprintf(buf, sizeof(buf), "capa: %s", uilayout_anchor_name(next));
    status(buf);
}

// EN UYGUN ÇAPA: öğe ekranda neredeyse oraya en yakın çapayı seçer ve ofseti
// ona göre yeniden hesaplar. Ekran konumu DEĞİŞMEZ, değişen tek şey konumun
// neye göre ölçüldüğü — yani pencere boyutlandığında nasıl davranacağı.
//
// Bu komut var, çünkü yanlış çapanın belirtisi görüntüde YOK: çapayı elle
// döndürdüğünde öğe yerinde kaldığı için hata anında görünmüyor, ancak pencere
// boyutlandırılınca öğe ekran dışına kaçıyor.
static void snap_best_anchor(void)
{
    UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
    if (!uilayout_raw_at(s_sel, &an, &al, &va, &ox, &oy)) return;
    float px, py;
    if (!item_point(s_sel, &px, &py)) return;
    UiAnchor best = uilayout_best_anchor(px, py);
    float bax, bay;
    uilayout_anchor_point(best, &bax, &bay);
    uilayout_set_raw(s_sel, best, al, va, px - bax, py - bay);
    s_dirty = true;
    char buf[80];
    snprintf(buf, sizeof(buf), "capa -> %s (konum ayni)",
             uilayout_anchor_name(best));
    status(buf);
}

static void cycle_align(void)
{
    UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
    if (!uilayout_raw_at(s_sel, &an, &al, &va, &ox, &oy)) return;
    UiAlign next = (UiAlign)(((int)al + 1) % UI_ALIGN_COUNT);
    uilayout_set_raw(s_sel, an, next, va, ox, oy);
    s_dirty = true;
    char buf[64];
    snprintf(buf, sizeof(buf), "hiza: %s", uilayout_align_name(next));
    status(buf);
}

static void nudge(float dx, float dy)
{
    UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
    if (!uilayout_raw_at(s_sel, &an, &al, &va, &ox, &oy)) return;
    uilayout_set_raw(s_sel, an, al, va, ox + dx, oy + dy);
    s_dirty = true;
}

// Boyutu OLAN öğelerde ok tuşları Ctrl ile boyutu değiştirir (konumu değil)
static void resize(float dw, float dh)
{
    float w, h;
    if (!uilayout_size_at(s_sel, &w, &h)) {
        status("bu ogenin boyutu koddan geliyor");
        return;
    }
    // h == 0 → yükseklik koddan geliyor, dikey boyutlandırma anlamsız.
    // Sessizce yok saymak yerine sebebini söyle.
    if (h <= 0.0f && dh != 0.0f && dw == 0.0f) {
        status("yukseklik koddan geliyor (satir sayisi)");
        return;
    }
    uilayout_set_size(s_sel, w + dw, h > 0.0f ? h + dh : 0.0f);
    s_dirty = true;
}

bool uieditor_handle_event(const SDL_Event *ev)
{
    if (ev == NULL) return false;

    // Ctrl+U her durumda çalışır (kapalıyken açar, açıkken kapatır)
    if (ev->type == SDL_EVENT_KEY_DOWN && ev->key.key == SDLK_U &&
        (ev->key.mod & SDL_KMOD_CTRL)) {
        uieditor_toggle();
        return true;
    }
    if (!s_active) return false;

    switch (ev->type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
        if (ev->button.button != SDL_BUTTON_LEFT) return true;
        // SEÇİLİ öğenin boyut tutamağı her şeyden önce denenir: panel gövdesi
        // etiketten büyük, tutamak onun içinde kalıyor — sıra ters olsa köşeye
        // tıklamak taşımayı başlatırdı
        SDL_FRect rh;
        if (s_sel >= 0 && resize_handle_rect(s_sel, &rh) &&
            ev->button.x >= rh.x && ev->button.x < rh.x + rh.w &&
            ev->button.y >= rh.y && ev->button.y < rh.y + rh.h)
        {
            s_resizing = true;
            return true;
        }
        int hit = pick_at(s_font, ev->button.x, ev->button.y);
        if (hit >= 0) {
            s_sel = hit;
            float px, py;
            if (item_point(s_sel, &px, &py)) {
                s_grabDX = ev->button.x - px;
                s_grabDY = ev->button.y - py;
                s_dragging = true;
            }
        }
        return true;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP:
        s_dragging = false;
        s_resizing = false;
        return true;

    case SDL_EVENT_MOUSE_MOTION: {
        if (s_resizing && s_sel >= 0) {
            SDL_FRect b;
            float curW = 0.0f, curH = 0.0f;
            uilayout_size_at(s_sel, &curW, &curH);
            if (body_rect(s_sel, &b)) {
                float nw = ev->motion.x - b.x, nh = ev->motion.y - b.y;
                float g = grid_step();
                if (g > 0.0f && !(SDL_GetModState() & SDL_KMOD_ALT)) {
                    nw = SDL_roundf(nw / g) * g;
                    nh = SDL_roundf(nh / g) * g;
                }
                // Yüksekliği koddan gelen öğede h'yi 0 BIRAK: köşeyi çekmek
                // onu kazara veri-güdümlü hale getirmesin (satır sayısıyla
                // büyümesi gereken panel sabit yüksekliğe kilitlenirdi)
                uilayout_set_size(s_sel, nw, curH > 0.0f ? nh : 0.0f);
                s_dirty = true;
            }
            return true;
        }
        if (!s_dragging || s_sel < 0) return true;
        UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
        if (!uilayout_raw_at(s_sel, &an, &al, &va, &ox, &oy)) return true;

        // Hedef EKRAN noktası. Yapıştırma ekran uzayında yapılıyor: kılavuz
        // çizgileri ekranda görünüyor, öğelerin çapaları farklı olabilir.
        float tx = ev->motion.x - s_grabDX;
        float ty = ev->motion.y - s_grabDY;

        // Fare olayında modifier alanı yok (SDL_MouseMotionEvent'te 'mod'
        // bulunmuyor) — klavye durumunu doğrudan soruyoruz.
        // Alt = tüm yapıştırmaları baypas et (piksel hassas yerleştirme).
        bool freeMove = (SDL_GetModState() & SDL_KMOD_ALT) != 0;

        s_guideX = s_guideY = false;
        if (!freeMove) {
            // 1) HİZALAMA KILAVUZU önce: başka bir öğenin x ya da y'siyle
            //    aynı hizaya geliyorsa oraya yapış. Izgaradan önceliği var
            //    çünkü "şu öğeyle aynı hizada" kasıtlı bir tasarım kararı,
            //    ızgara ise sadece düzenli görünsün diye.
            float bestDX = GUIDE_SNAP + 1.0f, bestDY = GUIDE_SNAP + 1.0f;
            for (int i = 0; i < uilayout_count(); i++) {
                if (i == s_sel) continue;
                float ox2, oy2;
                if (!item_point(i, &ox2, &oy2)) continue;
                float dx = SDL_fabsf(tx - ox2), dy = SDL_fabsf(ty - oy2);
                if (dx <= GUIDE_SNAP && dx < bestDX) {
                    bestDX = dx; s_guideX = true; s_guideXPos = ox2;
                }
                if (dy <= GUIDE_SNAP && dy < bestDY) {
                    bestDY = dy; s_guideY = true; s_guideYPos = oy2;
                }
            }
            if (s_guideX) tx = s_guideXPos;
            if (s_guideY) ty = s_guideYPos;

            // 2) Kılavuz yakalamadıysa ızgaraya yapış (ızgara açıksa)
            float g = grid_step();
            if (g > 0.0f) {
                if (!s_guideX) tx = SDL_roundf(tx / g) * g;
                if (!s_guideY) ty = SDL_roundf(ty / g) * g;
            }
        }

        // Ofset = ekran noktası - çapa noktası. Çapa noktası sabit olduğu için
        // ekran deltası doğrudan ofset deltası oluyor.
        float ax, ay;
        uilayout_anchor_point(an, &ax, &ay);
        uilayout_set_raw(s_sel, an, al, va, tx - ax, ty - ay);
        s_dirty = true;
        return true;
    }

    case SDL_EVENT_KEY_DOWN: {
        bool shift = (ev->key.mod & SDL_KMOD_SHIFT) != 0;
        bool ctrl  = (ev->key.mod & SDL_KMOD_CTRL) != 0;
        float step = shift ? NUDGE_COARSE : NUDGE_FINE;
        switch (ev->key.key) {
        case SDLK_ESCAPE:
            s_active = false; s_dragging = false;
            return true;
        case SDLK_TAB:
            if (uilayout_count() > 0) {
                int n = uilayout_count();
                s_sel = shift ? (s_sel - 1 + n) % n : (s_sel + 1) % n;
                status(uilayout_name_at(s_sel));
            }
            return true;
        // Ctrl+oklar BOYUT değiştirir (yalnız boyutu veriden gelen öğelerde),
        // düz oklar konumu iter
        case SDLK_LEFT:
            if (s_sel >= 0) { if (ctrl) resize(-step, 0); else nudge(-step, 0); }
            return true;
        case SDLK_RIGHT:
            if (s_sel >= 0) { if (ctrl) resize( step, 0); else nudge( step, 0); }
            return true;
        case SDLK_UP:
            if (s_sel >= 0) { if (ctrl) resize(0, -step); else nudge(0, -step); }
            return true;
        case SDLK_DOWN:
            if (s_sel >= 0) { if (ctrl) resize(0,  step); else nudge(0,  step); }
            return true;
        case SDLK_A:     if (s_sel >= 0) cycle_anchor(shift ? -1 : 1); return true;
        case SDLK_H:     if (s_sel >= 0) cycle_align(); return true;
        case SDLK_E:     if (s_sel >= 0) snap_best_anchor(); return true;
        case SDLK_P:
            s_preview = !s_preview;
            status(s_preview ? "on izleme ACIK (secili panel cizilir)"
                             : "on izleme KAPALI");
            return true;
        case SDLK_G: {
            s_gridIdx = (s_gridIdx + 1) % GRID_STEP_COUNT;
            char buf[48];
            if (grid_step() > 0.0f)
                snprintf(buf, sizeof(buf), "izgara: %.0f px", grid_step());
            else
                SDL_strlcpy(buf, "izgara: KAPALI", sizeof(buf));
            status(buf);
            return true;
        }
        case SDLK_S:
            if (ctrl) {
                if (uilayout_save(UI_LAYOUT_PATH)) {
                    s_dirty = false;
                    status("KAYDEDILDI: " UI_LAYOUT_PATH);
                } else {
                    status("KAYDEDILEMEDI — loga bak");
                }
            }
            return true;
        case SDLK_R:
            // Diskten geri yükle: kaydedilmemiş değişiklikleri ATAR.
            if (uilayout_load(UI_LAYOUT_PATH)) {
                s_dirty = false;
                if (s_sel >= uilayout_count()) s_sel = uilayout_count() - 1;
                status("Diskten geri yuklendi");
            } else {
                status("Geri yuklenemedi — loga bak");
            }
            return true;
        default:
            // Düzenleyici AÇIKKEN diğer tuşları da yut: oyunun kısayolları
            // (hotbar rakamları, envanter, post-fx) araya girmesin.
            return true;
        }
    }
    default:
        break;
    }
    return false;
}

static void frame(R2D *r, SDL_FRect box, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 ca)
{
    r2d_set_color(r, cr, cg, cb, ca);
    r2d_rect(r, &box);
}

void uieditor_render(R2D *renderer, TTF_Font *font, int winW, int winH)
{
    s_font = font;   // tıklama kutusu bunu kullanıyor (yukarıdaki nota bak)
    if (!s_active) return;
    r2d_set_blend(renderer, R2D_BLEND_ALPHA);

    // Ekranı hafifçe karart: tutamaklar HUD'un üstünde okunur olsun ama
    // altındaki gerçek öğeler hâlâ görünsün (asıl referans onlar).
    // Ön izleme açıkken daha da hafif: bakılan şey artık gerçek panel içeriği.
    r2d_set_color(renderer, 10, 12, 20, s_preview ? 55 : 90);
    r2d_fill_rect(renderer, &(SDL_FRect){ 0, 0, (float)winW, (float)winH });

    // PİKSEL IZGARASI (G ile açılır). Her 4. çizgi belirgin: sayarken
    // kaybolmamak için — 8px ızgarada belirgin çizgiler 32px'te düşüyor.
    float g = grid_step();
    if (g > 0.0f) {
        int n = 0;
        for (float x = 0.0f; x <= (float)winW; x += g, n++) {
            bool major = (n % 4) == 0;
            r2d_set_color(renderer, 120, 140, 175, major ? 60 : 26);
            r2d_fill_rect(renderer, &(SDL_FRect){ x, 0, 1.0f, (float)winH });
        }
        n = 0;
        for (float y = 0.0f; y <= (float)winH; y += g, n++) {
            bool major = (n % 4) == 0;
            r2d_set_color(renderer, 120, 140, 175, major ? 60 : 26);
            r2d_fill_rect(renderer, &(SDL_FRect){ 0, y, (float)winW, 1.0f });
        }
    }

    // Çapa ızgarası: üçe-üç bölen çizgiler — hangi çapanın nereye düştüğü
    // görünsün (çapa soyut bir kavram, çizgi onu somutlaştırıyor).
    // Piksel ızgarasından SONRA çiziliyor ki onun üstünde kalsın.
    r2d_set_color(renderer, 90, 130, 190, 110);
    for (int k = 0; k <= 2; k++) {
        float fx = (float)winW * (float)k * 0.5f;
        float fy = (float)winH * (float)k * 0.5f;
        r2d_fill_rect(renderer, &(SDL_FRect){ fx - 0.5f, 0, 1.0f, (float)winH });
        r2d_fill_rect(renderer, &(SDL_FRect){ 0, fy - 0.5f, (float)winW, 1.0f });
    }

    // HİZALAMA KILAVUZU: sürüklerken başka bir öğeyle aynı hizaya gelindiğinde
    // parlak çizgi. Yapışmanın SEBEBİNİ gösteriyor — çizgi olmasa sürükleme
    // "takılıyor" gibi gelirdi.
    if (s_dragging) {
        r2d_set_color(renderer, 120, 255, 190, 200);
        if (s_guideX)
            r2d_fill_rect(renderer,
                          &(SDL_FRect){ s_guideXPos - 0.5f, 0, 1.0f, (float)winH });
        if (s_guideY)
            r2d_fill_rect(renderer,
                          &(SDL_FRect){ 0, s_guideYPos - 0.5f, (float)winW, 1.0f });
    }

    for (int i = 0; i < uilayout_count(); i++) {
        float px, py;
        if (!item_point(i, &px, &py)) continue;
        UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
        if (!uilayout_raw_at(i, &an, &al, &va, &ox, &oy)) continue;
        bool sel = (i == s_sel);
        const char *nm = uilayout_name_at(i);
        SDL_FRect lb = label_rect(font, nm, al, px, py);

        // Seçili öğeyi çapasına bağlayan çizgi: ofsetin NEYE göre ölçüldüğünü
        // gösterir. Çapa kavramını anlatan tek en iyi görsel bu.
        if (sel) {
            float ax, ay;
            uilayout_anchor_point(an, &ax, &ay);
            r2d_set_color(renderer, 255, 210, 60, 120);
            float x0 = (ax < px) ? ax : px, x1 = (ax < px) ? px : ax;
            float y0 = (ay < py) ? ay : py, y1 = (ay < py) ? py : ay;
            r2d_fill_rect(renderer, &(SDL_FRect){ x0, ay - 0.5f, x1 - x0, 1.0f });
            r2d_fill_rect(renderer, &(SDL_FRect){ px - 0.5f, y0, 1.0f, y1 - y0 });
            r2d_set_color(renderer, 255, 210, 60, 200);   // çapa noktası
            r2d_fill_rect(renderer, &(SDL_FRect){ ax - 3.0f, ay - 3.0f, 6.0f, 6.0f });
        }

        // Boyutu veriden gelen öğe: gövdeyi çiz (panel sınırı görünsün) +
        // seçiliyse sağ-alt köşeye boyut tutamağı
        SDL_FRect body;
        if (body_rect(i, &body)) {
            r2d_set_color(renderer, sel ? 255 : 130, sel ? 210 : 150,
                          sel ? 60 : 190, sel ? 60 : 35);
            r2d_fill_rect(renderer, &body);
            frame(renderer, body, sel ? 255 : 130, sel ? 210 : 150,
                  sel ? 60 : 190, sel ? 210 : 120);
            SDL_FRect rhr;
            if (sel && resize_handle_rect(i, &rhr)) {
                r2d_set_color(renderer, 255, 210, 60, 230);
                r2d_fill_rect(renderer, &rhr);
            }
        }

        // Etiket zemini + çerçevesi
        r2d_set_color(renderer, sel ? 40 : 20, sel ? 34 : 22, sel ? 12 : 30,
                      sel ? 220 : 170);
        r2d_fill_rect(renderer, &(SDL_FRect){ lb.x - 4.0f, lb.y - 2.0f,
                                             lb.w + 8.0f, lb.h + 4.0f });
        if (sel) frame(renderer, (SDL_FRect){ lb.x - 4.0f, lb.y - 2.0f,
                                             lb.w + 8.0f, lb.h + 4.0f },
                       255, 210, 60, 255);
        else     frame(renderer, (SDL_FRect){ lb.x - 4.0f, lb.y - 2.0f,
                                             lb.w + 8.0f, lb.h + 4.0f },
                       120, 140, 180, 160);

        // Nokta işareti: ÇÖZÜLEN konum tam olarak burası
        r2d_set_color(renderer, sel ? 255 : 150, sel ? 90 : 200, sel ? 90 : 255, 255);
        r2d_fill_rect(renderer, &(SDL_FRect){ px - HANDLE_R * 0.5f,
                                             py - HANDLE_R * 0.5f,
                                             HANDLE_R, HANDLE_R });
        if (font != NULL) {
            SDL_Color c = sel ? (SDL_Color){ 255, 235, 180, 255 }
                              : (SDL_Color){ 200, 210, 230, 220 };
            draw_text(renderer, font, nm, lb.x, lb.y, c);
        }
    }

    if (font == NULL) return;

    // Bilgi paneli (sol alt): seçili öğe + kısayollar
    float panelH = 146.0f;   // uyarı satırı da sığsın
    float py0 = (float)winH - panelH - 8.0f;
    r2d_set_color(renderer, 8, 10, 18, 225);
    r2d_fill_rect(renderer, &(SDL_FRect){ 8.0f, py0, 580.0f, panelH });
    frame(renderer, (SDL_FRect){ 8.0f, py0, 580.0f, panelH }, 255, 210, 60, 200);

    SDL_Color hd = { 255, 210, 60, 255 };
    SDL_Color tx = { 215, 220, 235, 255 };
    SDL_Color dim = { 150, 158, 178, 255 };
    char line[160];

    snprintf(line, sizeof(line), "YERLESIM DUZENLEYICI%s", s_dirty ? "  *degisti" : "");
    draw_text(renderer, font, line, 18.0f, py0 + 8.0f, hd);

    if (s_sel >= 0 && s_sel < uilayout_count()) {
        UiAnchor an; UiAlign al; UiVAlign va; float ox, oy;
        uilayout_raw_at(s_sel, &an, &al, &va, &ox, &oy);
        float bw = 0.0f, bh = 0.0f;
        bool hasSize = uilayout_size_at(s_sel, &bw, &bh);
        if (hasSize)
            snprintf(line, sizeof(line), "%s  capa=%s hiza=%s x=%.0f y=%.0f %.0fx%.0f",
                     uilayout_name_at(s_sel), uilayout_anchor_name(an),
                     uilayout_align_name(al), ox, oy, bw, bh);
        else
            snprintf(line, sizeof(line), "%s   capa=%s  hiza=%s  x=%.0f y=%.0f",
                     uilayout_name_at(s_sel), uilayout_anchor_name(an),
                     uilayout_align_name(al), ox, oy);
        draw_text(renderer, font, line, 18.0f, py0 + 30.0f, tx);

        // ŞÜPHELİ OFSET UYARISI: yanlış çapanın tek belirtisi bu. Görüntüde
        // hiçbir şey ters görünmez, hata ancak pencere boyutlandırılınca çıkar.
        if (uilayout_offset_suspect(s_sel)) {
            SDL_Color warn = { 255, 130, 100, 255 };
            draw_text(renderer, font,
                      "! ofset cok buyuk — capa yanlis olabilir (E: duzelt)",
                      18.0f, py0 + 110.0f, warn);
        }
    } else {
        draw_text(renderer, font, "(hicbir oge secili degil — Tab)",
                  18.0f, py0 + 30.0f, dim);
    }

    char gl[64];
    if (grid_step() > 0.0f)
        snprintf(gl, sizeof(gl), "G: izgara %.0fpx   Alt: yapistirmayi kapat",
                 grid_step());
    else
        SDL_strlcpy(gl, "G: izgara KAPALI   Alt: yapistirmayi kapat", sizeof(gl));
    draw_text(renderer, font, gl, 18.0f, py0 + 52.0f, dim);
    draw_text(renderer, font, "Tab: sonraki  A: capa  E: en uygun capa  H: hiza",
              18.0f, py0 + 72.0f, dim);
    {
        SDL_Color pc = s_preview ? (SDL_Color){ 140, 235, 150, 255 }
                                 : (SDL_Color){ 150, 158, 178, 255 };
        draw_text(renderer, font,
                  s_preview ? "P: on izleme ACIK — secili panel cizilir"
                            : "P: on izleme kapali",
                  18.0f + 300.0f, py0 + 72.0f, pc);
    }
    draw_text(renderer, font, "oklar: 1px (Shift 10)  Ctrl+oklar: boyut  Ctrl+S: kaydet",
              18.0f, py0 + 92.0f, dim);

    if (s_statusT > 0.0f) {
        SDL_Color sc = { 140, 235, 150, 255 };
        draw_text(renderer, font, s_status, 18.0f + 300.0f, py0 + 8.0f, sc);
        // Zamanlayıcı burada azalıyor: düzenleyicinin dt'si yok ve olmasına da
        // gerek yok — kare başına sabit düşüş yeterli (60fps'te ~2.5 sn).
        s_statusT -= 1.0f / 60.0f;
    }
}
