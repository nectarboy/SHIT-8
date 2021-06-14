#include <stdio.h>
#include <SDL2/SDL.h>

#include "main.h"
#include "font.c"

// -------------------- // CHIP 8 EMU HERE
unsigned short pc = 0;
unsigned char v[16];
unsigned short stack[16];
unsigned int sp = 0;

unsigned char dt = 0;
unsigned char st = 0;

unsigned char ram[0x1000];

unsigned char keypad[16];

int cyclesPerFrame = CPF;

unsigned char vram[CHIP_WIDTH * CHIP_HEIGHT];
char shouldRefresh = 0;

// Emulation
void chipStepCycle()
{
    // Fetch
    unsigned short opcode = (ram[pc] << 8) | (ram[pc + 1]);
    pc += CHIP_OPCODE_SIZE;

    // Decode and execute
    switch (opcode)
    {

    }
}

void chipUpdateTimers()
{
    if (dt != 0) dt--;
    if (st != 0)
    {
        st--;
        // chipBeep();
    }
}

// Execution
void chipExecute()
{   
    // Emulate
    for (int i = 0; i < cyclesPerFrame; i++)
        chipStepCycle();
    chipUpdateTimers();

    if (shouldRefresh); // chipRender();
}

// Instructions
// ...

// -------------------- // MAIN PROGRAM HERE
int main(int argc, char *argv[])
{

    // SETUP SDL SHIT
    SDL_Window* window = SDL_CreateWindow
    (
        WINDOW_TITLE,
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_SCALE * CHIP_WIDTH, WINDOW_SCALE * CHIP_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    SDL_Renderer* windowRenderer = SDL_CreateRenderer
    (
        window,
        0,
        SDL_RENDERER_ACCELERATED
    );

    SDL_Texture* texture = SDL_CreateTexture
    (
        windowRenderer,
        SDL_PIXELFORMAT_RGB24,
        SDL_TEXTUREACCESS_STREAMING,
        WINDOW_SCALE * CHIP_WIDTH, WINDOW_SCALE * CHIP_HEIGHT
    );

    SDL_Event windowEvent;

    // Program loop ...
    char quit = 0;
    while (!quit)
    {   
        // -------------------- //
        // Wait / Check for events
        while(SDL_PollEvent(&windowEvent))
        {
            switch(windowEvent.type)
            {
                case SDL_QUIT:
                    quit = 1;
                    break;
            }
        }
        // -------------------- //

        // Update Chip
        chipExecute();

        // Render
        SDL_RenderCopy(windowRenderer, texture, NULL, NULL); 

        SDL_RenderClear(windowRenderer);
        SDL_RenderPresent(windowRenderer);
    }

    // When quit- destroy SDL
    SDL_DestroyRenderer(windowRenderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(texture);
    SDL_Quit();

    return 0;
}