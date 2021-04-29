#include <stdio.h>
#include <SDL2/SDL.h>

int main(int argc, char* args[]) {
    //The window we'll be rendering to
    SDL_Window* window = SDL_CreateWindow(
        "chip-8",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        256, 128,
        SDL_WINDOW_SHOWN
    );
    SDL_Renderer* renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
    SDL_Event event;

    char active = 1;
    while(active) {
        while(SDL_PollEvent(&event)) {
            // ... nothing for now
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    while (1);

    return 0;
}