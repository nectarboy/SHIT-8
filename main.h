#ifndef MAIN_H
#define MAIN_H

// ------------------------ //
#define CHIP_WIDTH 64
#define CHIP_HEIGHT 32

#define CHIP_OPCODE_SIZE 2
#define CHIP_STACK_SIZE 16

#define WINDOW_TITLE "SHIT-8"
#define WINDOW_SCALE 4

#define USLEEP_US (1000000 / 60)
#define CPF 10

#define SDL_INITS (SDL_INIT_VIDEO)
#define BPP 4 // ARGB-32

// CHIP-8 quirks
#define SHR_REG y

// ------------------------ //
// inline void CLS();
// inline void RET();
// inline void JPnnn(int nnn);
// inline void CALLnnn(int nnn);
// inline void SExkk(int x, int kk);
// inline void SNExkk(int x, int kk);
// inline void SExy(int x, int y);
// inline void LDxkk(int x, int kk);
// inline void ADDxkk(int x, int kk);
// inline void LDxy(int x, int y);
// inline void ORxy(int x, int y);
// inline void ANDxy(int x, int y);
// inline void XORxy(int x, int y);
// inline void ADDxy(int x, int y);
// inline void SUBxy(int x, int y);
// inline void SHRxy(int x, int y);
// inline void SUBNxy(int x, int y);
// inline void SHLxy(int x, int y);
// inline void SNExy(int x, int y);
// inline void LDinnn(int nnn);
// inline void JP0nnn(int nnn);
// inline void RNDxkk(int x, int kk);
// inline void DRAWxyn(int x, int y, int n);
// ------------------------ //

#endif