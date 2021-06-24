#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

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

void chipPanic(char* msg[], int opcode)
{   
    printf("\n-- P A N I C ! ! -- %s\n", msg);
    if (opcode != NULL) printf("OPCODE %X\n", opcode);
    printf("PC: %X\n\n", pc);

    assert(0); // Crash
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
    if (sp == 0) chipPanic("stack underflow !!", NULL); // Handle stack underflow

    sp--;
    pc = stack[sp]; // (Will be masked after fetch)
}

// 1
static inline void JPnnn(int nnn)
{
    pc = nnn - CHIP_OPCODE_SIZE; // (Will be masked after fetch [important for JP v0 nnn ofc])
}

// 2
static inline void CALLnnn(int nnn)
{
    stack[sp] = pc;
    sp++;

    pc = nnn - 2; // (Will be masked after fetch)

    if (sp == CHIP_STACK_SIZE) chipPanic("stack overflow !!", NULL); // Handle stack overflow
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
static inline void SExy(int x, int y) // :|
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
    v[0xf] = v[x] & 1;
    v[SHR_REG] >>= 1; // smort B)
}
static inline void SUBNxy(int x, int y)
{
    v[0xf] = (v[y] > v[x]) ? 1 : 0;
    v[x] = v[y] - v[x];
}
static inline void SHLxy(int x, int y)
{
    v[0xf] = (v[x] & 0x80) >> 7;
    v[SHR_REG] <<= 1; // smort B) once more
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
// JP v0 nnn

// C
static inline void RNDxkk(int x, int kk)
{
    v[x] = rand() & kk;
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
            int result = chipPlotPx(
                (v[x] + ii) & (CHIP_WIDTH-1),
                (v[y] + i) & (CHIP_HEIGHT-1),
                bit
            );

            if (!shouldSetF) shouldSetF = result;
        }
    }

    v[0xf] = shouldSetF;
}

// E
static inline void SKPx(int x)
{
    if (keypad[v[x & 0xf]]) pc += CHIP_OPCODE_SIZE;
}
static inline void SKNPx(int x)
{
    if (!keypad[v[x & 0xf]]) pc += CHIP_OPCODE_SIZE;
}

// F
static inline void LDxDT(int x)
{
    v[x] = dt;
}
static inline void LDxK(int x)
{
    v[x] = 0;
}
static inline void LDDTx(int x)
{
    dt = v[x];
}
static inline void LDSTx(int x)
{
    st = v[x];
}
static inline void ADDix(int x)
{
    regI = (regI + v[x]) & 0xfff;
}
static inline void LDfx(int x)
{
    regI = (v[x] * 5) & 0xfff;
}
static inline void LDbx(int x)
{
    ram[regI]               = (unsigned char)(v[x] / 100);
    ram[(regI + 1) & 0xfff] = (unsigned char)(v[x] % 100 / 10);
    ram[(regI + 2) & 0xfff] = (unsigned char)(v[x] % 10);
}
static inline void LDix(int x)
{
    for (int i = 0; i < x+1; i++)
        ram[(regI + i) & 0xfff] = v[i];

}
static inline void LDxi(int x)
{
    for (int i = 0; i < x+1; i++)
        v[i] = ram[(regI + i) & 0xfff];

}
// -------------------- //

// Emulation
void chipStepCycle()
{
    // Fetch
    int opcode = (ram[pc] << 8) | (ram[pc + 1]);

    // printf("OPCODE %X\n", opcode);
    // printf("PC: %X\n\n", pc);

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

        case 0x2:
            CALLnnn(nnn);
            break;

        case 0x3:
            SExkk(x, kk);
            break;

        case 0x4:
            SNExkk(x, kk);
            break;

        case 0x5:
            SExy(x, y); // floshed !!
            break;

        case 0x6:
            LDxkk(x, kk);
            break;

        case 0x7:
            ADDxkk(x, kk);
            break;

        case 0x8:
            switch (n) {
                case 0x0: LDxy(x, y); break;
                case 0x1: ORxy(x, y); break;
                case 0x2: ANDxy(x, y); break;
                case 0x3: XORxy(x, y); break;
                case 0x4: ADDxy(x, y); break;
                case 0x5: SUBxy(x, y); break;
                case 0x6: SHRxy(x, y); break;
                case 0x7: SUBNxy(x, y); break;
                case 0xe: SHLxy(x, y); break;

                default: chipPanic("undefined opcode !!", opcode);
            }
            break;

        case 0x9:
            SNExy(x, y);
            break;

        case 0xa:
            LDinnn(nnn);
            break;

        case 0xb:
            JPnnn((nnn + v[0])); // Winky ;)
            break;

        case 0xc:
            RNDxkk(x, kk);
            break;

        case 0xd:
            DRAWxyn(x, y, n);
            break;

        case 0xe:
            if (kk == 0x9e) SKPx(x);
            else if (kk == 0xa1) SKNPx(x);

            else chipPanic("undefined opcode !!", opcode);
            break;

        case 0xf:
            switch (kk) {
                case 0x07: LDxDT(x); break;
                case 0x0a: LDxK(x); break;
                case 0x15: LDDTx(x); break;
                case 0x18: LDSTx(x); break;
                case 0x1e: ADDix(x); break;
                case 0x29: LDfx(x); break;
                case 0x33: LDbx(x); break;
                case 0x55: LDix(x); break;
                case 0x65: LDxi(x); break;

                default: chipPanic("undefined opcode !!", opcode);
            }
            break;

        default: chipPanic("THIS SHOULDNT BE HAPPENING.", 0x6969);
    }

    pc += CHIP_OPCODE_SIZE;
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
        printf("SHIT-8 :: ROM NOT FOUND !!\n");
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

    printf("SHIT-8 :: LOADED ROM !!\n");
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
    srand(time(NULL));

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

    printf("\nSHIT-8 :: SEE U NEXT TIME :3\n\n");
    return 0;
}