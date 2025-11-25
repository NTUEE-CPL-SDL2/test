#include <SDL.h>
#include <cmath>
#include <stdio.h>

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window* window = SDL_CreateWindow("Rainbow Sine Texture",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);

    int sw, sh, w, h;
    SDL_GetWindowSize(window, &sw, &sh);
    w = sw - 10;
    h = sh - 10;

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STATIC,
        w, h
    );

    Uint32 pixels[w * h];

    for (int i = 0; i < w * h; i++) {
        pixels[i] = 0xFFFFFFFF;
    }
    
    SDL_UpdateTexture(texture, NULL, pixels, w * sizeof(Uint32));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

    bool quit = false;
    SDL_Event e;

    int t = 0;

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_w &&
                    (e.key.keysym.mod & KMOD_CTRL))
                {
                    quit = true;
                }
            }
        }

        Uint8 r = static_cast<Uint8>((std::sin(t * 0.05) + 1.0) * 127.5);
        Uint8 g = static_cast<Uint8>((std::sin(t * 0.05 + 2.0) + 1.0) * 127.5);
        Uint8 b = static_cast<Uint8>((std::sin(t * 0.05 + 4.0) + 1.0) * 127.5);

        SDL_SetTextureColorMod(texture, r, g, b);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        SDL_Rect dst = { sw / 2 - w / 2, sh / 2 - h / 2,
                         w, h };
        SDL_RenderCopy(renderer, texture, NULL, &dst);

        SDL_RenderPresent(renderer);

        t++;
        SDL_Delay(16);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

