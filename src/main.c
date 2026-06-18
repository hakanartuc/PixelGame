#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h> # platformlar arası 'main' köprülemesi (mobil/web için de gerekli)
#include <stdbool.h>

int main(int argc, char *argv[]){
    // 1) SDL'in video alt sistemini ayağa kaldır. SDL3 başarılıysa 'true' döner.
    if(!SDL_Init(SDL_INIT_VIDEO)){
        SDL_Log("SDL_Init basarısız: %s", SDL_GetError());
        return 1; // Başarısızsa çık
    }

    // 2) Pencere oluştur.
    SDL_Window *window = SDL_CreateWindow("Pixe Game", 1280, 720, 0);
    if(window == NULL){
        SDL_Log("SDL_CreateWindow basarısız: %s", SDL_GetError());
        SDL_Quit(); // SDL'i temizle
        return 1; // Başarısızsa çık
    }

    // 3) Pencereye bağlı bir renderer oluştur.
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if(renderer == NULL){
        SDL_Log("SDL_CreateRenderer basarısız: %s", SDL_GetError());
        SDL_DestroyWindow(window); // Pencereyi temizle
        SDL_Quit(); // SDL'i temizle
        return 1; // Başarısızsa çık
    }    

    // 4) Oyun döngüsü
    bool running = true;
    while(running){
        // 4a) INPUT: OS'in bu kare için kuyruğa attığı tüm olayları boşalt.
        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_EVENT_QUIT){ // Kullanıcı pencereyi kapatmak istedi mi?
                running = false; // Oyun döngüsünden çık
            } else if(event.type == SDL_EVENT_KEY_DOWN){ // Bir tuşa basıldı mı?
                if(event.key.key == SDLK_ESCAPE){ // ESC tuşuna basıldı mı?
                    running = false; // Oyun döngüsünden çık
                }
            }
        }

        // 4b) UPDATE: şimdilik boş, ileride oyun durumunu güncellemek için kullanacağız.

        // 4c) RENDER: tüm pencereyi bir renge boya, sonra ekrana bas.
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // RGBA
        SDL_RenderClear(renderer); // Renderer'ı temizle
        SDL_RenderPresent(renderer); // Ekrana bas.
    }

    // 5) Her şeyi oluşturmanın TERS sırasıyla temizle. C'de bu bizim sorumluluğumuzda,
    // hiçbir destructor bunu kendiliğinden yapmaz.
    SDL_DestroyRenderer(renderer); // Renderer'ı temizle
    SDL_DestroyWindow(window); // Pencereyi temizle
    SDL_Quit(); // SDL'i temizle
    return 0; // Başarılı çık
}