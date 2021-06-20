#include <assert.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#include "main.h"
#include "font.c"

// Buffering
unsigned char frameBuffer[ WINDOW_SCALE*CHIP_WIDTH * WINDOW_SCALE*CHIP_HEIGHT * BPP];
int BUFFER_BYTES_PER_ROW = WINDOW_SCALE*CHIP_WIDTH * BPP;

int fgColor = 0xffff00;
int bgColor = 0x000077;
void drawBuffer(int x, int y, int bit)
{
    int color = bit ? fgColor : bgColor;

    x *= WINDOW_SCALE;
    y *= WINDOW_SCALE;

    for (int i = 0; i < WINDOW_SCALE; i++)
    {
        for (int ii = 0; ii < WINDOW_SCALE; ii++)
        {
            int index = BPP * ((y + i) * WINDOW_SCALE*CHIP_WIDTH + (x + ii));

            frameBuffer[index++] = 0xff;
            frameBuffer[index++] = (unsigned char)(color >> 4);
            frameBuffer[index++] = (unsigned char)(color >> 2);
            frameBuffer[index  ] = (unsigned char)(color >> 0);
        }
    }

}

// -------------------- // CHIP 8 EMU HERE
unsigned int pc = 0;
unsigned char v[16];
int stack[CHIP_STACK_SIZE];
int sp = 0;
int regI = 0;

int dt = 0;
int st = 0;

unsigned char ram[0x1000];
unsigned char rom[0x1000 - 0x200];

int keypad[16];

int cyclesPerFrame = CPF;

unsigned int vram[CHIP_WIDTH * CHIP_HEIGHT];
int shouldRefresh = 0;

// Chip functions
void chipUpdateTimers()
{
    if (dt != 0) dt--;
    if (st != 0)
    {
        st--;
        // chipBeep();
    }
}

int chipPlotPx(int x, int y, int bit)
{
    const int index = y * CHIP_WIDTH + x;

    int bitBefore = vram[index];
    vram[index] ^= bit;

    drawBuffer(x, y, vram[index]);

    return (bitBefore && !vram[index]) ? 1 : 0;
}

// -------------------- // INSTRUCTIONS
// 0
static inline void CLS()
{
    memset(vram, 0, sizeof(vram));

    // clear framebuffer
    for (int i = 0; i < CHIP_HEIGHT; i++)
        for (int ii = 0; ii < CHIP_WIDTH; ii++)
            drawBuffer(ii, i, 0);
}

static inline void RET()
{
    assert(sp != 0); // Will stack underflow occur ??

    sp--;
    pc = stack[sp];
}

// 1
static inline void JPnnn(int nnn)
{
    pc = nnn;
}

// 2
static inline void CALLnnn(int nnn)
{
    stack[sp] = pc;
    sp++;

    pc = nnn;

    assert(sp != CHIP_STACK_SIZE); // Did stack overflow occur ??
}

// 3
static inline void SExkk(int x, int kk)
{
    if (v[x] == kk) pc += CHIP_OPCODE_SIZE;
}

// 4
static inline void SNExkk(int x, int kk)
{
    if (v[x] != kk) pc += CHIP_OPCODE_SIZE;
}

// 5
static inline void SExy(int x, int y) // Dont u dare :|
{
    if (v[x] == v[y]) pc += CHIP_OPCODE_SIZE;
}

// 6
static inline void LDxkk(int x, int kk)
{
    v[x] = kk;
}

// 7
static inline void ADDxkk(int x, int kk)
{
    v[x] += kk;
}

// 8
static inline void LDxy(int x, int y)
{
    v[x] = v[y];
}
static inline void ORxy(int x, int y)
{
    v[x] |= v[y];
}
static inline void ANDxy(int x, int y)
{
    v[x] &= v[y];
}
static inline void XORxy(int x, int y)
{
    v[x] ^= v[y];
}
static inline void ADDxy(int x, int y)
{
    int sum = v[x] + v[y];
    v[x] = sum;

    v[0xf] = (sum > 0xff) ? 1 : 0;
}
static inline void SUBxy(int x, int y)
{
    v[0xf] = (v[x] > v[y]) ? 1 : 0;
    v[x] = v[x] - v[y];
}
static inline void SHRxy(int x, int y)
{
    v[SHR_REG] <<= 1; // smort B)
}
static inline void SUBNxy(int x, int y)
{
    v[0xf] = (v[y] > v[x]) ? 1 : 0;
    v[x] = v[y] - v[x];
}
static inline void SHLxy(int x, int y)
{
    v[SHR_REG] >>= 1; // smort B) once more
}

// 9
static inline void SNExy(int x, int y)
{
    if (v[x] != v[y]) pc += CHIP_OPCODE_SIZE;
}

// A
static inline void LDinnn(int nnn)
{
    regI = nnn;
}

// B
static inline void JP0nnn(int nnn)
{
    pc = (nnn + v[0]) & 0xfff;
}

// C
static inline void RNDxkk(int x, int kk)
{
    v[x] = 0xff /* RANDOM NUM HERE */ & kk;
}

// D
static inline void DRAWxyn(int x, int y, int n)
{
    int shouldSetF = 0;

    for (int i = 0; i < n; i++)
    {
        int byte = ram[regI + i];
        for (int ii = 0; ii < 8; ii++)
        {
            int bit = (0x80 & (byte << ii)) >> 7;
            if (!shouldSetF)
                shouldSetF = chipPlotPx(
                    (v[x] + ii) & (CHIP_WIDTH-1),
                    (v[y] + i) & (CHIP_HEIGHT-1),
                    bit
                );
        }
    }

    v[0xf] = shouldSetF;
}
// -------------------- //

// Emulation
void chipStepCycle()
{
    // Fetch
    int opcode = (ram[pc] << 8) | (ram[pc + 1]);

    // printf("OPCODE %X\n", opcode);
    // printf("PC: %X\n\n", pc);

    pc += CHIP_OPCODE_SIZE;

    const int hi = opcode >> 12;
    const int n = opcode & 0x000f;

    const int x = (opcode >> 8) & 0xf;
    const int y = (opcode >> 4) & 0xf;
    const int kk = opcode & 0x00ff;

    const int nnn = opcode & 0x0fff;

    // Decode and execute
    switch (hi)
    {
        case 0x0:
            if (kk == 0xe0)
                CLS();
            else if (kk == 0xee)
                RET();
            break;

        case 0x1:
            JPnnn(nnn);
            break;

        case 0x6:
            LDxkk(x, kk);
            break;

        case 0x7:
            ADDxkk(x, kk);
            break;

        case 0xa:
            LDinnn(nnn);
            break;

        case 0xd:
            DRAWxyn(x, y, n);
            break;
    }

    pc &= 0xfff;
}

// Execution
void chipExecute()
{
    // Emulate
    for (int i = 0; i < cyclesPerFrame; i++)
    {
        chipStepCycle();
    }
    chipUpdateTimers();
}

// loading and resetting
int chipLoadRom(char* fileName[])
{
    // ------------------------ //
    FILE *file = fopen(fileName, "r+b"); // r for read, b for binary
    if (!file)
    {
        printf("SHIT-8 :: ROM NOT FOUND !! %s\n", fileName);
        return 1;
    }

    // Get length ...
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    if (len > sizeof(rom))
        len = sizeof(rom);
    fclose(file);

    // Then load into rom buffer !
    file = fopen(fileName, "r+b"); // r for read, b for binary
    fread(rom, len, 1, file); // read 10 bytes to our buffer
    fclose(file);

    printf("SHIT-8 :: LOADED ROM !! %s\n", fileName);
    // ------------------------ //

    return 0; // Success !
}

void chipReset()
{   
    // Reset registers and memory and stuff
    pc = 0x200;
    sp = 0;
    regI = 0;
    memset(v, 0, sizeof(v));

    dt = 0;
    st = 0;

    memset(ram, 0, sizeof(ram));
    memset(stack, 0, sizeof(stack));
    memset(vram, 0, sizeof(vram));

    // Reset framebuffer
    for (int i = 0; i < CHIP_HEIGHT; i++)
        for (int ii = 0; ii < CHIP_WIDTH; ii++)
            drawBuffer(ii, i, 0);

    // Copy font data to ram
    memcpy(ram, FONT_DATA, sizeof(FONT_DATA));

    // Load rom buffer into ram !
    for(int i = 0; i < sizeof(rom); i++)
        ram[i + 0x200] = rom[i];
}

// -------------------- // MAIN PROGRAM HERE
int main(int argc, char* argv[])
{

    // Load rom from command parameter
    if (argc < 2)
    {
        printf("SHIT-8 :: SPECIFY A ROM STUPID !!\n");
        return 0;
    }
    if (chipLoadRom(argv[1])) // If error occured
        return 0;

    // Setup shit-8
    chipReset();

    // SETUP SDL SHIT
    if (SDL_Init(SDL_INITS) != 0) {
        printf("SDL ERROR :(");
        return 1;
    }

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
        SDL_PIXELFORMAT_ARGB32,
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

        SDL_UpdateTexture(texture, NULL, frameBuffer, BUFFER_BYTES_PER_ROW);

        // Render
        SDL_RenderClear(windowRenderer);
        SDL_RenderCopy(windowRenderer, texture, NULL, NULL); 
        SDL_RenderPresent(windowRenderer);

        usleep(USLEEP_US);
    }

    // When quit- destroy SDL
    SDL_DestroyRenderer(windowRenderer);
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(texture);
    SDL_Quit();

    return 0;
}