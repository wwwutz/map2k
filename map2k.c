

/*
            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004
 
 Copyright (C) 2004 Sam Hocevar
  14 rue de Plaisance, 75014 Paris, France
 Everyone is permitted to copy and distribute verbatim or modified
 copies of this license document, and changing it is allowed as long
 as the name is changed.
 
            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
 
  0. You just DO WHAT THE FUCK YOU WANT TO.
*/


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <byteswap.h>
#include <getopt.h>
#include <limits.h>
#include <time.h>
#include <math.h>


static int DEBUG = 0;
static int OPT_DEBUG = 0;

#define dprintf(...) if (DEBUG) printf(__VA_ARGS__)

//static int tDEBUG = 1;
static unsigned int X2SCORE[] = { 0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536 };
static unsigned int X1SCORE[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

unsigned int *XSCORE = X2SCORE;

uint64_t MAP_R[65536];
uint64_t MAP_L[65536];
uint32_t SCORE[65536];
uint32_t BASIS[65536];
uint32_t GRV_R[65536];          // gravity left side
uint32_t GRV_L[65536];          // gravity right side

uint64_t TOTALTRIES = 0;

int OPT_BASE = 0;
int OPT_PLAY = 0;

enum directions { DIR_UP = 0, DIR_RIGHT, DIR_DOWN, DIR_LEFT };
static char *DIRS[] = { "up", "right", "down", "left" };
static char *DIRC[] = { "^^", "=>", "vv", "<=" };

enum decisions { DEC_NONE = 0, DEC_SCORE, DEC_STRAT };
static char *DECISION[] = { "nothing", "score", "alt strategie" };

//static uint8_t RV[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
//static uint8_t LV[] = { 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 };
//static uint8_t DV[] = { 12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3 };
//static uint8_t UV[] = { 3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12 };

static uint64_t RVx = 0x0123456789ABCDEF;
static uint64_t LVx = 0xFEDCBA9876543210;
static uint64_t DVx = 0xC840D951EA62FB73;
static uint64_t UVx = 0x37BF26AE159D048C;

uint64_t *ROTMAP_[] = { &RVx, &LVx, &DVx, &UVx };

uint8_t ROTMAP[4][16] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0},
    {12, 8, 4, 0, 13, 9, 5, 1, 14, 10, 6, 2, 15, 11, 7, 3},
    {3, 7, 11, 15, 2, 6, 10, 14, 1, 5, 9, 13, 0, 4, 8, 12},
};

struct F_st {
    uint64_t f;                 // the bitfield
    int score;                  // the score
    int basis;                  // the 2**score
    unsigned int moves;         // the number of moves to get here
};

struct Stats_st {
    struct F_st f;              // the field
    unsigned int rseed;         // the seed for this field
};

void init_F(struct F_st *F)
{
    F->f = 0x0;
    F->score = 0;
    F->basis = 0;
    F->moves = 0;
}

void copy_F(struct F_st *F, struct F_st *T)
{
    T->f = F->f;
    T->score = F->score;
    T->basis = F->basis;
    T->moves = F->moves;
}

void setupMAP()
{
    uint16_t rc = 0;
    uint16_t rcx = 0;
    MAP_R[rc] = 0;
    MAP_L[rc] = 0;
    SCORE[rc] = 0;
    BASIS[rc] = 0;
    GRV_R[rc] = 0;
    GRV_L[rc] = 0;

    uint8_t A, B, C, D;
    // uint8_t Al,Bl,Cl,Dl;
    uint8_t a, b, c, d;

    for (d = 0; d <= 14; d++) { // man koennte bis 15, dann muss man aber die leeren felder initialisieren
        for (c = 0; c <= 14; c++) {
            for (b = 0; b <= 14; b++) {
                for (a = 0; a <= 14; a++) {
                    int skip = 0;
                    A = a;
                    B = b;
                    C = c;
                    D = d;
                    rc = d << 12 | c << 8 | b << 4 | a;
                    rcx = a << 12 | b << 8 | c << 4 | d;

                    MAP_R[rc] = rc;
                    SCORE[rc] = 0;
                    BASIS[rc] = 0;

                    // b->a 1

                    if (B != 0) {
                        if (B == A) {
                            B = 0;
                            A++;
                            SCORE[rc] += XSCORE[A];
                            BASIS[rc] += A;
                            skip = 1;
                        } else if (A == 0) {
                            A = B;
                            B = 0;
                        }
                    }
                    // c->a 2

                    if (!skip && C != 0 && B == 0) {
                        if (C == A) {
                            A++;
                            C = 0;
                            SCORE[rc] += XSCORE[A];
                            BASIS[rc] += A;
                            skip = 1;
                        } else if (A == 0) {
                            A = C;
                            C = 0;
                        }
                    }
                    // d->a 3

                    if (!skip && D != 0 && C == 0 && B == 0) {
                        if (D == A) {
                            A++;
                            D = 0;
                            SCORE[rc] += XSCORE[A];
                            BASIS[rc] += A;
                        } else if (A == 0) {
                            A = D;
                            D = 0;
                        }
                    }
                    // c->b 4

                    skip = 0;
                    if (C != 0) {
                        if (C == B) {
                            B++;
                            C = 0;
                            SCORE[rc] += XSCORE[B];
                            BASIS[rc] += B;
                            skip = 1;
                        } else if (C != 0 && B == 0) {
                            B = C;
                            C = 0;
                        }
                    }
                    // d->b 5

                    if (!skip && D != 0 && C == 0) {
                        if (D == B) {
                            B++;
                            D = 0;
                            SCORE[rc] += XSCORE[B];
                            BASIS[rc] += B;
                        } else if (B == 0) {
                            B = D;
                            D = 0;
                        }
                    }
                    // d->c 6

                    if (D != 0) {
                        if (D == C) {
                            C++;
                            D = 0;
                            SCORE[rc] += XSCORE[C];
                            BASIS[rc] += C;
                        } else if (C == 0) {
                            C = D;
                            D = 0;
                        }
                    }

                    MAP_R[rc] = D << 12 | C << 8 | B << 4 | A;
                    MAP_L[rcx] = A << 12 | B << 8 | C << 4 | D;
                    GRV_R[rc] = A + B;
                    GRV_L[rc] = C + D;
                    GRV_R[rcx] = C + D;
                    GRV_L[rcx] = A + B;

                    SCORE[rcx] = SCORE[rc];
                    BASIS[rcx] = BASIS[rc];

                }
            }
        }
    }
}

void pi2m(uint16_t x, char *tr)
{

    int y, t;

    for (y = 3; y >= 0; y--) {
        printf("%3d", x >> 4 * y & 0xf);
    }
    printf(" : ");
    for (y = 3; y >= 0; y--) {
        t = XSCORE[x >> 4 * y & 0xf];
        if (t) {
            printf("%6d", t);
        } else {
            printf("     _");
        }
    }
    printf("%s", tr);

}

void dumpMAP()
{
    unsigned int rc = 0;
    for (rc = 0; rc <= 65535; rc++) {
        if ((rc & 0xf) == 0xf)
            continue;
        if ((rc & 0xf0) == 0xf0) {
            rc += 0xf;
            continue;
        };
        if ((rc & 0xf00) == 0xf00) {
            rc += 0xff;
            continue;
        };
        if ((rc & 0xf000) == 0xf000) {
            rc += 0xfff;
            continue;
        };

        printf("score:%5d %016lx %04x %016lx GRV_L:%3d GRV_R:%3d ", SCORE[rc], MAP_L[rc], rc, MAP_R[rc], GRV_L[rc], GRV_R[rc]);
        pi2m(MAP_L[rc], " <- ");
        pi2m(rc, " -> ");
        pi2m(MAP_R[rc], "\n");
    }
}

void dumpF(uint64_t f)
{

    printf("dumpF()   f: %016lx\n", f);
    pi2m((f & 0xffff000000000000) >> 12 * 4, "\n");
    pi2m((f & 0x0000ffff00000000) >> 8 * 4, "\n");
    pi2m((f & 0x00000000ffff0000) >> 4 * 4, "\n");
    pi2m((f & 0x000000000000ffff), "\n");

}

void dumpFs(struct F_st *fs)
{
    dprintf("dumpFs()   f: %016lx\n", fs->f);
    pi2m((fs->f & 0xffff000000000000) >> 12 * 4, "\n");
    pi2m((fs->f & 0x0000ffff00000000) >> 8 * 4, "\n");
    pi2m((fs->f & 0x00000000ffff0000) >> 4 * 4, "\n");
    pi2m((fs->f & 0x000000000000ffff), "\n");   //(char *)sprintf("   =%d\n",fs->score));
    printf("Score: %d / %d\n", fs->score, fs->basis);
}

void dumpCFs(struct F_st *fs)
{
    dprintf("dumpFs()   f: %016lx\n", fs->f);
    printf("%6d ", fs->score);
    uint64_t f;
    int y;
    f = fs->f;
    for (y = 15; y >= 0; y--) {
        if (OPT_BASE) {
            printf("%3lu", (f >> 4 * y & 0xf));
        } else {
            printf("%6d", XSCORE[f >> 4 * y & 0xf]);
        }
    }
    printf("\n");
}

// 
// #          ######  
// #          #     # 
// #          #     # 
// #          ######  
// #          #   #   
// #          #    #  
// #######    #     # 
// 
uint64_t shift_LR(uint64_t f, uint64_t * MAP)
{
    uint64_t r;
    r = 0;
    r = MAP[(f & 0xffff000000000000) >> 12 * 4] << 12 * 4;
    r |= MAP[(f & 0x0000ffff00000000) >> 8 * 4] << 8 * 4;
    r |= MAP[(f & 0x00000000ffff0000) >> 4 * 4] << 4 * 4;
    r |= MAP[(f & 0x000000000000ffff)];
    if (DEBUG)
        printf("%016lx\n", r);
    return r;
}

uint64_t shift_L(uint64_t f)
{
    if (DEBUG)
        printf("shift_L(%016lx) = ", f);
    return shift_LR(f, MAP_L);
}

uint64_t shift_R(uint64_t f)
{
    if (DEBUG)
        printf("shift_R(%016lx) = ", f);
    return shift_LR(f, MAP_R);
}

//
// struct
//
uint64_t shift_LRs(struct F_st * fs, struct F_st * ts, uint64_t * MAP)
{
    uint64_t r;
    uint16_t R;

    TOTALTRIES++;

    r = 0;
    R = (fs->f & 0xffff000000000000) >> 12 * 4;
    r = MAP[R] << 12 * 4;
    ts->score = SCORE[R];
    ts->basis = BASIS[R];

    R = (fs->f & 0x0000ffff00000000) >> 8 * 4;
    r |= MAP[R] << 8 * 4;
    ts->score += SCORE[R];
    ts->basis += BASIS[R];

    R = (fs->f & 0x00000000ffff0000) >> 4 * 4;
    r |= MAP[R] << 4 * 4;
    ts->score += SCORE[R];
    ts->basis += BASIS[R];

    R = (fs->f & 0x000000000000ffff);
    r |= MAP[R];
    ts->score += SCORE[R];
    ts->basis += BASIS[R];

    if (DEBUG)
        printf("%016lx\n", r);
    ts->f = r;
    return r;
}

uint64_t shift_Ls(struct F_st * fs, struct F_st * ts)
{
    if (DEBUG)
        printf("shift_L(%016lx) = ", fs->f);
    return shift_LRs(fs, ts, MAP_L);
}

uint64_t shift_Rs(struct F_st * fs, struct F_st * ts)
{
    if (DEBUG)
        printf("shift_R(%016lx) = ", fs->f);
    return shift_LRs(fs, ts, MAP_R);
}


// 
// #     #    ######  
// #     #    #     # 
// #     #    #     # 
// #     #    #     # 
// #     #    #     # 
// #     #    #     # 
//  #####     ######  
// 
uint64_t shift_UD(uint64_t f, uint64_t * MAP)
{
    uint64_t r;
    uint64_t t;
    uint16_t R;
    r = 0;

    R = 0;
    R |= (f & (uint64_t) 0x000000000000F000);   // C=0 L 0 1100
    R |= (f & (uint64_t) 0x00000000F0000000) >> 4 * 5;  // 8=1 L 0 0111
    R |= (f & (uint64_t) 0x0000F00000000000) >> 4 * 10; // 4=2 L 0 0010
    R |= (f & (uint64_t) 0xF000000000000000) >> 4 * 15; // 0=3 R 1 0011
    t = MAP[R];
    r |= (t & (uint64_t) 0x000000000000F000);
    r |= (t & (uint64_t) 0x0000000000000F00) << 4 * 5;
    r |= (t & (uint64_t) 0x00000000000000F0) << 4 * 10;
    r |= (t & (uint64_t) 0x000000000000000F) << 4 * 15;
//  printf("shift_D(%016lx) R=%04x rt=%016lx r=%016lx\n",f,R,t,r);

    R = 0;
    R |= (f & (uint64_t) 0x0000000000000F00) << 4 * 1;  // D=4 L 0 1001
    R |= (f & (uint64_t) 0x000000000F000000) >> 4 * 4;  // 9=5 L 0 0100
    R |= (f & (uint64_t) 0x00000F0000000000) >> 4 * 9;  // 5=6 R 1 0001
    R |= (f & (uint64_t) 0x0F00000000000000) >> 4 * 14; // 1=7 R 1 0110
    t = MAP[R];
    r |= (t & (uint64_t) 0x000000000000F000) >> 4 * 1;
    r |= (t & (uint64_t) 0x0000000000000F00) << 4 * 4;
    r |= (t & (uint64_t) 0x00000000000000F0) << 4 * 9;
    r |= (t & (uint64_t) 0x000000000000000F) << 4 * 14;
//  printf("shift_D(%016lx) R=%04x rt=%016lx r=%016lx\n",f,R,t,r);

    R = 0;
    R |= (f & (uint64_t) 0x00000000000000F0) << 4 * 2;  // E=8 L 0 0110
    R |= (f & (uint64_t) 0x0000000000F00000) >> 4 * 3;  // A=9 L 0 0001
    R |= (f & (uint64_t) 0x000000F000000000) >> 4 * 8;  // 6=A R 1 0100
    R |= (f & (uint64_t) 0x00F0000000000000) >> 4 * 13; // 2=B R 1 1001
    t = MAP[R];
    r |= (t & (uint64_t) 0x000000000000F000) >> 4 * 2;
    r |= (t & (uint64_t) 0x0000000000000F00) << 4 * 3;
    r |= (t & (uint64_t) 0x00000000000000F0) << 4 * 8;
    r |= (t & (uint64_t) 0x000000000000000F) << 4 * 13;
//  printf("shift_D(%016lx) R=%04x rt=%016lx r=%016lx\n",f,R,t,r);

    R = 0;
    R |= (f & (uint64_t) 0x000000000000000F) << 4 * 3;  // F=C L 0 0011
    R |= (f & (uint64_t) 0x00000000000F0000) >> 4 * 2;  // B=D R 1 0010
    R |= (f & (uint64_t) 0x0000000F00000000) >> 4 * 7;  // 7=E R 1 0111
    R |= (f & (uint64_t) 0x000F000000000000) >> 4 * 12; // 3=F R 1 1100
    t = MAP[R];
    r |= (t & (uint64_t) 0x000000000000F000) >> 4 * 3;
    r |= (t & (uint64_t) 0x0000000000000F00) << 4 * 2;
    r |= (t & (uint64_t) 0x00000000000000F0) << 4 * 7;
    r |= (t & (uint64_t) 0x000000000000000F) << 4 * 12;
//  printf("shift_D(%016lx) R=%04x rt=%016lx r=%016lx\n",f,R,t,r);
    if (DEBUG)
        printf("%016lx\n", r);
    return r;
}

//
// struct
//
uint64_t shift_UDs(struct F_st * fs, struct F_st * ts, uint64_t * MAP)
{
    uint64_t r;
    uint64_t t;
    uint16_t R;
    r = 0;

    ts->score = 0;
    ts->basis = 0;
    TOTALTRIES++;

    R = 0;
    R |= (fs->f & (uint64_t) 0x000000000000F000);       // C=0 L 0 1100
    R |= (fs->f & (uint64_t) 0x00000000F0000000) >> 4 * 5;      // 8=1 L 0 0111
    R |= (fs->f & (uint64_t) 0x0000F00000000000) >> 4 * 10;     // 4=2 L 0 0010
    R |= (fs->f & (uint64_t) 0xF000000000000000) >> 4 * 15;     // 0=3 R 1 0011
    t = MAP[R];
    ts->score += SCORE[R];
    ts->basis += BASIS[R];
    r |= (t & (uint64_t) 0x000000000000F000);
    r |= (t & (uint64_t) 0x0000000000000F00) << 4 * 5;
    r |= (t & (uint64_t) 0x00000000000000F0) << 4 * 10;
    r |= (t & (uint64_t) 0x000000000000000F) << 4 * 15;
//  printf("shift_D(%016lx) R=%04x rt=%016lx r=%016lx\n",f,R,t,r);

    R = 0;
    R |= (fs->f & (uint64_t) 0x0000000000000F00) << 4 * 1;      // D=4 L 0 1001
    R |= (fs->f & (uint64_t) 0x000000000F000000) >> 4 * 4;      // 9=5 L 0 0100
    R |= (fs->f & (uint64_t) 0x00000F0000000000) >> 4 * 9;      // 5=6 R 1 0001
    R |= (fs->f & (uint64_t) 0x0F00000000000000) >> 4 * 14;     // 1=7 R 1 0110
    t = MAP[R];
    ts->score += SCORE[R];
    ts->basis += BASIS[R];
    r |= (t & (uint64_t) 0x000000000000F000) >> 4 * 1;
    r |= (t & (uint64_t) 0x0000000000000F00) << 4 * 4;
    r |= (t & (uint64_t) 0x00000000000000F0) << 4 * 9;
    r |= (t & (uint64_t) 0x000000000000000F) << 4 * 14;
//  printf("shift_D(%016lx) R=%04x rt=%016lx r=%016lx\n",f,R,t,r);

    R = 0;
    R |= (fs->f & (uint64_t) 0x00000000000000F0) << 4 * 2;      // E=8 L 0 0110
    R |= (fs->f & (uint64_t) 0x0000000000F00000) >> 4 * 3;      // A=9 L 0 0001
    R |= (fs->f & (uint64_t) 0x000000F000000000) >> 4 * 8;      // 6=A R 1 0100
    R |= (fs->f & (uint64_t) 0x00F0000000000000) >> 4 * 13;     // 2=B R 1 1001
    t = MAP[R];
    ts->score += SCORE[R];
    ts->basis += BASIS[R];
    r |= (t & (uint64_t) 0x000000000000F000) >> 4 * 2;
    r |= (t & (uint64_t) 0x0000000000000F00) << 4 * 3;
    r |= (t & (uint64_t) 0x00000000000000F0) << 4 * 8;
    r |= (t & (uint64_t) 0x000000000000000F) << 4 * 13;
//  printf("shift_D(%016lx) R=%04x rt=%016lx r=%016lx\n",f,R,t,r);

    R = 0;
    R |= (fs->f & (uint64_t) 0x000000000000000F) << 4 * 3;      // F=C L 0 0011
    R |= (fs->f & (uint64_t) 0x00000000000F0000) >> 4 * 2;      // B=D R 1 0010
    R |= (fs->f & (uint64_t) 0x0000000F00000000) >> 4 * 7;      // 7=E R 1 0111
    R |= (fs->f & (uint64_t) 0x000F000000000000) >> 4 * 12;     // 3=F R 1 1100
    t = MAP[R];
    ts->score += SCORE[R];
    ts->basis += BASIS[R];
    r |= (t & (uint64_t) 0x000000000000F000) >> 4 * 3;
    r |= (t & (uint64_t) 0x0000000000000F00) << 4 * 2;
    r |= (t & (uint64_t) 0x00000000000000F0) << 4 * 7;
    r |= (t & (uint64_t) 0x000000000000000F) << 4 * 12;
//  printf("shift_D(%016lx) R=%04x rt=%016lx r=%016lx\n",f,R,t,r);
    if (DEBUG)
        printf("%016lx\n", r);
    ts->f = r;
    return r;
}

uint64_t shift_D(uint64_t f)
{
    if (DEBUG)
        printf("shift_D(%016lx) = ", f);
    return shift_UD(f, MAP_L);
}

uint64_t shift_U(uint64_t f)
{
    if (DEBUG)
        printf("shift_U(%016lx) = ", f);
    return shift_UD(f, MAP_R);
}

//
// struct
//
uint64_t shift_Ds(struct F_st * fs, struct F_st * ts)
{
    if (DEBUG)
        printf("shift_D(%016lx) = ", fs->f);
    return shift_UDs(fs, ts, MAP_L);
}

uint64_t shift_Us(struct F_st * fs, struct F_st * ts)
{
    if (DEBUG)
        printf("shift_U(%016lx) = ", fs->f);
    return shift_UDs(fs, ts, MAP_R);
}


//                                                                                                        
// #    # #####     #      ###### ###### #####    #####  #  ####  #    # #####   #####   ####  #    # #    # 
// #    # #    #    #      #      #        #      #    # # #    # #    #   #     #    # #    # #    # ##   # 
// #    # #    #    #      #####  #####    #      #    # # #      ######   #     #    # #    # #    # # #  # 
// #    # #####     #      #      #        #      #####  # #  ### #    #   #     #    # #    # # ## # #  # # 
// #    # #         #      #      #        #      #   #  # #    # #    #   #     #    # #    # ##  ## #   ## 
//  ####  #         ###### ###### #        #      #    # #  ####  #    #   #     #####   ####  #    # #    # 
//                                                                             


uint64_t up(struct F_st * fs, struct F_st * ts)
{
    return shift_Us(fs, ts);
}

uint64_t left(struct F_st * fs, struct F_st * ts)
{
    return shift_Ls(fs, ts);
}

uint64_t right(struct F_st * fs, struct F_st * ts)
{
    return shift_Rs(fs, ts);
}

uint64_t down(struct F_st * fs, struct F_st * ts)
{
    return shift_Ds(fs, ts);
}

uint64_t rot90(uint64_t f)
{
    uint64_t r;
//  printf("rot90(%016lx): \n",f);
//
// 0123456789ABCDEF
// 1111222233334444
// C840D951EA62FB73
//

    r = 0;

    //0123456789ABCDEF
    r |= (f & (uint64_t) 0x00000F0000000000) >> 4 * 1;  // 5=6 R 1 0001
    r |= (f & (uint64_t) 0x00000000000F0000) >> 4 * 2;  // B=D R 1 0010
    r |= (f & (uint64_t) 0xF000000000000000) >> 4 * 3;  // 0=3 R 1 0011
    r |= (f & (uint64_t) 0x000000F000000000) >> 4 * 4;  // 6=A R 1 0100
    r |= (f & (uint64_t) 0x0F00000000000000) >> 4 * 6;  // 1=7 R 1 0110
    r |= (f & (uint64_t) 0x0000000F00000000) >> 4 * 7;  // 7=E R 1 0111
    r |= (f & (uint64_t) 0x00F0000000000000) >> 4 * 9;  // 2=B R 1 1001
    r |= (f & (uint64_t) 0x000F000000000000) >> 4 * 12; // 3=F R 1 1100
    r |= (f & (uint64_t) 0x0000000000F00000) << 4 * 1;  // A=9 L 0 0001
    r |= (f & (uint64_t) 0x0000F00000000000) << 4 * 2;  // 4=2 L 0 0010
    r |= (f & (uint64_t) 0x000000000000000F) << 4 * 3;  // F=C L 0 0011
    r |= (f & (uint64_t) 0x000000000F000000) << 4 * 4;  // 9=5 L 0 0100
    r |= (f & (uint64_t) 0x00000000000000F0) << 4 * 6;  // E=8 L 0 0110
    r |= (f & (uint64_t) 0x00000000F0000000) << 4 * 7;  // 8=1 L 0 0111
    r |= (f & (uint64_t) 0x0000000000000F00) << 4 * 9;  // D=4 L 0 1001
    r |= (f & (uint64_t) 0x000000000000F000) << 4 * 12; // C=0 L 0 1100

    printf("rot90() r: %016lx\n", r);
    return r;
}

uint64_t placenew(uint64_t f)
{

    uint64_t g;
    int bfree[16];
    int i = 0;
    int j = 0;
    g = f;
    for (i = 0; i <= 15; i++) {
        bfree[i] = 0;
        if ((g & 0xf) == 0x0) {
            bfree[j++] = i + 1;
        } else {
        }
        g >>= 4;
    }
    if (j > 0) {
        int n = (rand() % 10) <= 1 ? 2 : 1;
        int r = rand() % j;
        int a = bfree[r] - 1;
        f = f | ((uint64_t) n << (4 * (a)));
    }
    dprintf("placenew() : %016lx\n", f);
    return f;
}

void test_shifting()
{

    static uint64_t F_TEST[] = {
        0x1111222233334444,
        0x1122334413312244,
        0x0123456789abcde1,
        0x1002044004403004,
        0x1002056007803004,
        0
    };

    struct F_st F;
    struct F_st FR;             // Spielfeld

    int i = 0;

    while ((F.f = F_TEST[i++])) {

        printf("##### %d\n", i);
        dumpFs(&F);
        shift_Ds(&F, &FR);
        dumpFs(&FR);
        shift_Us(&F, &FR);
        dumpFs(&FR);
        shift_Ls(&F, &FR);
        dumpFs(&FR);
        shift_Rs(&F, &FR);
        dumpFs(&FR);
    }
};

void strat_hiscorer(struct F_st *F)
{
    int DEBUG = OPT_DEBUG;
//    struct F_st F;              // from
    struct F_st *T;             // to
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4];           // alle results


    int mv = 0;
    int i = 0;
    int hsc = 0;
    int moved = 0;
    int score = 0;
    int dir = 0;
    int fdir = 0;

    int try = 0;

    //  init_F(&F);

    for (i = 0; i <= 3; i++) {
        init_F(&R[i]);
    }
//    F.f = placenew(0);
    B = F;
//    RESULT->f = F.f;
//    RESULT->score = 0;
    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }

    while (1) {
        moved = 0;
        mv = 0;
        hsc = 0;
        

        dir = DIR_UP;
        fdir = dir;
        
        T = &R[dir];
        T->f = F->f;
        if (DEBUG) {
            printf("--- go for ---\n");
            dumpFs(T);
        }
        up(F, T);
        if (F->f != T->f) {     // hat sich bewegt
            hsc = T->score;
            B = T;
            moved++;
            mv++;
            fdir = dir;
        }

        dir = DIR_RIGHT;
        T = &R[dir];
        right(F, T);
        if (F->f != T->f) {
            if (T->score >= hsc) {
                hsc = T->score;
                B = T;
                moved++;
            }
            fdir = dir;
            mv++;
        }

        dir = DIR_LEFT;
        T = &R[dir];
        left(F, T);
        if (F->f != T->f) {
            if (T->score >= hsc) {
                hsc = T->score;
                B = T;
                moved++;
            }
            fdir = dir;
            mv++;
        }

        dir = DIR_DOWN;
        T = &R[dir];
        down(F, T);
        if (F->f != T->f) {
            if (T->score >= hsc) {
                hsc = T->score;
                B = T;
                moved++;
            }
            fdir = dir;
            mv++;
        }

        if (mv) {
            score += hsc;
            if (DEBUG)
                printf("shifted %s\n", DIRS[fdir]);
            if (DEBUG)
                dumpFs(B);
            F->f = placenew(B->f);
            F->score = score;
            F->moves++;
            moved++;
        } else {
            if (DEBUG)
                printf("NOT shifted\n");
        }

        if (moved == 0) {
            if (try == 1) {
                if (DEBUG) {
                    printf("full\n");
                    dumpFs(F);
                    printf("Score: %d\n", score);
                }
                // copy_F(&F,RESULT);
//              RESULT->score = F.score;
//                RESULT->f = F.f;

                return;
            }
            try++;
        } else {
            try = 0;
        }
    }
    return;
}

void strat_dlhiscorer(struct F_st *F)
{
    int DEBUG = OPT_DEBUG;
    //   struct F_st F;              // from
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4];           // alle results


    int mv = 0;
    int i = 0;
    int hsc = 0;
    int moved = 0;
    int SCORE = F->score;
    int fdir = 0;
    int try = 0;

//    init_F(&F);

    for (i = 0; i <= 3; i++) {
        init_F(&R[i]);
    }
    B = F;
    //   RESULT->f = F.f;
    //   RESULT->score = 0;
    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }

    while (1) {
        moved = 0;
        mv = 0;
        hsc = 0;

        up(F, &R[DIR_UP]);
        down(F, &R[DIR_DOWN]);
        left(F, &R[DIR_LEFT]);
        right(F, &R[DIR_RIGHT]);

        if (DEBUG) {
            printf("--- go for ---\n");
            dumpFs(F);
        }
        //     if (DEBUG) printf("\n");
        int j = DIR_DOWN;
        B = &R[j];
        for (i = 0; i <= 3; i++) {
            if (DEBUG)
                printf(" %5s=%d; ", DIRS[i], R[i].score);
            if (F->f != R[i].f) {       // hat sich bewegt
                if (R[i].score >= hsc) {
                    B = &R[i];
                    hsc = B->score;
                    j = i;
                    moved++;
                }
                fdir = i;
                mv++;
            }

        }
        if (DEBUG)
            printf("\n decided %5s would be cool\n", DIRS[j]);

        if (mv) {
            SCORE += B->score;
            if (DEBUG)
                printf("shifted %s\n", DIRS[fdir]);
            if (DEBUG)
                dumpFs(B);
            F->f = placenew(B->f);
            F->score = SCORE;
            F->moves++;
            moved++;
        } else {
            if (DEBUG)
                printf("NOT shifted\n");
        }

        if (moved == 0) {
            if (try == 1) {
                if (DEBUG) {
                    printf("full\n");
                    dumpFs(F);
                    printf("Score: %d\n", SCORE);
                }
                // copy_F(F,RESULT);

                return;
            }
            try++;
        } else {
            try = 0;
        }
    }
    return;
}

void strat_dl2hiscorer(struct F_st *F)
{
    int DEBUG = OPT_DEBUG;

//    struct F_st F;              // from
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4];           // alle results
    unsigned int STRAT[] = { DIR_DOWN, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_DOWN, DIR_UP };
    unsigned int SCORE_ORDER[] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };
    int mv = 0;
    int i = 0;
    int ii = 0;
    int hsc = 0;
    int moved = 0;
    int SCORE = 0;
    int stratidx = 0;
    int try = 0;
    int fdir = 0;

//    init_F(&F);
    for (i = 0; i <= 3; i++) {
        init_F(&R[i]);
    }
    B = F;

//    RESULT->f = F.f;
//    RESULT->score = 0;
    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }
    while (1) {
        moved = 0;
        mv = 0;
        hsc = 0;
        up(F, &R[DIR_UP]);
        down(F, &R[DIR_DOWN]);
        left(F, &R[DIR_LEFT]);
        right(F, &R[DIR_RIGHT]);
        if (DEBUG) {
            printf("--- go for ---\n");
            dumpFs(F);
        }
        //     if (DEBUG) printf("\n");
        int j = DIR_DOWN;
        B = &R[j];
        int decision = DEC_NONE;
        for (ii = 0; ii <= 3; ii++) {
            i = SCORE_ORDER[ii];
            dprintf(" %5s=%d; ", DIRS[i], R[i].score);
            if (F->f != R[i].f) {       // hat sich bewegt
                if (R[i].score > hsc) {
                    B = &R[i];
                    hsc = B->score;
                    j = i;
                    moved++;
                    decision = DEC_SCORE;
                    stratidx = 0;       // stratidx resetten
                }
                fdir = ii;
                mv++;
            }
        }
        if (hsc == 0) {         // no move scored
            if (R[STRAT[stratidx]].f != F->f) { // beweglich
                B = &R[STRAT[stratidx]];
                j = STRAT[stratidx];
                moved++;
                decision = DEC_STRAT;
            }
            stratidx++;
            if (stratidx >= sizeof(STRAT) / sizeof(STRAT[0])) {
                stratidx = 0;
            }
        }
        dprintf("\n %s decided \t\t\t%5s ", DECISION[decision], DIRS[j]);
        fdir = j;
        if (decision == DEC_STRAT) {
            dprintf(" next STRAT[%d]=%s", stratidx, DIRS[STRAT[stratidx]]);
        }
        dprintf("\n");
        if (mv) {
            SCORE += B->score;
            dprintf("shifted %s\n", DIRS[fdir]);
            if (DEBUG)
                dumpFs(B);
            F->f = placenew(B->f);
            F->score = SCORE;
            F->moves++;
            moved++;
        } else {
            dprintf("NOT shifted\n");
        }
        if (moved == 0) {
            if (try == 1) {
                if (DEBUG) {
                    printf("full\n");
                    dumpFs(F);
                    printf("Score: %d\n", SCORE);
                }
                // copy_F(&F,RESULT);
                return;
            }
            try++;
        } else {
            try = 0;
        }
    }
    return;
}

void strat_backtracker(struct F_st *F)  // ignore new drops
{
    int DEBUG = OPT_DEBUG;

    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][4];        // alle results
    //unsigned int STRAT[] = { DIR_DOWN, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_DOWN, DIR_UP };
    //unsigned int SCORE_ORDER[] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
#define MAXDEPTH 1000           // sollte erst mal reichen
    // uint64_t SEEN[MAXDEPTH]; // hier kommen alle felder rein als abbruchbedingung 'hab ich schon'
    int i = 0;
    int j = 0;
    int hsc = 0;
    int sc = 0;
    int itercnt = 0;
    int BEST = -1;
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 3; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s ###############\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;
        for (i = 0; i <= 3; i++) {
            dprintf("%50s <- =\n", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                for (j = 0; j <= 3; j++) {
                    if (j == i)
                        continue;
                    dprintf("%55s => %5s\n", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (DEBUG)
                        dumpFs(&R[i][1]);
                    sc = R[i][0].score + R[i][1].score;
                    dprintf("R[%d][0].score + R[%d][1].score (%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].score, R[i][1].score, sc, hsc);
                    if (sc > hsc) {
                        hsc = sc;
                        BEST = i;       // der is aber besser
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);
            }
        }
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            dprintf("#### best move is %s scoring %d #####\n", DIRS[BEST], hsc);
            B = &R[BEST][0];
            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void strat_backtracker2(struct F_st *F) // ignore new drops
{
    int DEBUG = OPT_DEBUG;

    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][4];        // alle results
    //unsigned int STRAT[] = { DIR_DOWN, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_DOWN, DIR_UP };
    //unsigned int SCORE_ORDER[] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
#define MAXDEPTH 1000           // sollte erst mal reichen
    // uint64_t SEEN[MAXDEPTH]; // hier kommen alle felder rein als abbruchbedingung 'hab ich schon'
    int i = 0;
    int j = 0;
    int k = 0;
    int hsc = 0;
    int sc = 0;
    int itercnt = 0;
    int BEST = -1;
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 3; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s ###############\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;
        for (i = 0; i <= 3; i++) {
            dprintf("%50s <- =\n", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                for (j = 0; j <= 3; j++) {
                    if (j == i)
                        continue;
                    dprintf("%55s => %5s\n", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (DEBUG)
                        dumpFs(&R[i][1]);
                    sc = R[i][0].score + R[i][1].score;
                    dprintf("R[%d][0].score + R[%d][1].score (%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].score, R[i][1].score, sc, hsc);
                    if (sc > hsc) {
                        hsc = sc;
                        BEST = i;       // der is aber besser
                    }
                    for (k = 0; k <= 3; k++) {
                        if (k == j)
                            continue;
                        dprintf("%55s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k]);
                        DIRFUNC[k] (&R[i][1], &R[i][2]);
                        if (DEBUG)
                            dumpFs(&R[i][2]);
                        sc = R[i][0].score + R[i][1].score + R[i][2].score;
                        dprintf("R[%d][0].score + R[%d][1].score + R[i][2].score (%d+%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].score, R[i][1].score, R[i][2].score, sc, hsc);
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber noch besser
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);
            }
        }
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            dprintf("#### best move is %s scoring %d #####\n", DIRS[BEST], hsc);
            B = &R[BEST][0];
            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void strat_backtracker3(struct F_st *F) // ignore new drops
{
    int DEBUG = OPT_DEBUG;

    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][4];        // alle results
    //unsigned int STRAT[] = { DIR_DOWN, DIR_LEFT, DIR_DOWN, DIR_RIGHT, DIR_DOWN, DIR_UP };
    //unsigned int SCORE_ORDER[] = { DIR_DOWN, DIR_LEFT, DIR_RIGHT, DIR_UP };
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
#define MAXDEPTH 1000           // sollte erst mal reichen
    // uint64_t SEEN[MAXDEPTH]; // hier kommen alle felder rein als abbruchbedingung 'hab ich schon'
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int hsc = 0;
    int sc = 0;
    int itercnt = 0;
    int BEST = -1;
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 3; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s ###############\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;
        for (i = 0; i <= 3; i++) {
            dprintf("%50s\n", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                for (j = 0; j <= 3; j++) {
                    // if (j == i) continue;
                    dprintf("%55s => %5s\n", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (DEBUG)
                        dumpFs(&R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        sc = R[i][0].score + R[i][1].score;
                        dprintf("R[%d][0].score + R[%d][1].score (%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].score, R[i][1].score, sc, hsc);
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        for (k = 0; k <= 3; k++) {
                            // if (k==j) continue;
                            dprintf("%55s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (DEBUG)
                                dumpFs(&R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                sc = R[i][0].score + R[i][1].score + R[i][2].score;
                                dprintf("R[%d][0].score + R[%d][1].score + R[i][2].score (%d+%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].score, R[i][1].score, R[i][2].score, sc, hsc);
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                for (l = 0; l <= 3; l++) {
                                    // if (l==k) continue;
                                    dprintf("%55s => %5s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k], DIRS[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (DEBUG)
                                        dumpFs(&R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        sc = R[i][0].score + R[i][1].score + R[i][2].score + R[i][3].score;
                                        dprintf("R[%d][0].score + R[%d][1].score + R[i][2].score + R[i][3].score (%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].score, R[i][1].score, R[i][2].score, R[i][3].score, sc, hsc);
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            dprintf("#### best move is %s scoring %d #####\n", DIRS[BEST], hsc);
            if (DEBUG)
                dumpFs(F);

            B = &R[BEST][0];
            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void strat_backtracker3b(struct F_st *F)        // based on basis
{
    int DEBUG = OPT_DEBUG;

    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][4];        // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int hsc = 0;
    int sc = 0;
    int itercnt = 0;
    int BEST = -1;
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 3; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s ###############\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;
        for (i = 0; i <= 3; i++) {
            dprintf("%50s\n", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                for (j = 0; j <= 3; j++) {
                    // if (j == i) continue;
                    dprintf("%55s => %5s\n", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (DEBUG)
                        dumpFs(&R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        sc = R[i][0].basis + R[i][1].basis;
                        dprintf("R[%d][0].basis + R[%d][1].basis (%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].basis, R[i][1].basis, sc, hsc);
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        for (k = 0; k <= 3; k++) {
                            // if (k==j) continue;
                            dprintf("%55s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (DEBUG)
                                dumpFs(&R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                sc = R[i][0].basis + R[i][1].basis + R[i][2].basis;
                                dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis (%d+%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].basis, R[i][1].basis, R[i][2].basis, sc, hsc);
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                for (l = 0; l <= 3; l++) {
                                    // if (l==k) continue;
                                    dprintf("%55s => %5s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k], DIRS[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (DEBUG)
                                        dumpFs(&R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        sc = R[i][0].basis + R[i][1].basis + R[i][2].basis + R[i][3].basis;
                                        dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, R[i][0].basis, R[i][1].basis, R[i][2].basis, R[i][3].basis, sc, hsc);
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            dprintf("#### best move is %s scoring %d #####\n", DIRS[BEST], hsc);
            if (DEBUG)
                dumpFs(F);

            B = &R[BEST][0];
            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void strat_backtracker3bo(struct F_st *F)       // based on basis, frueher score ist besser
{
    /* 
     * 0+0+11+4 ist schlechter als 11+4+0+0
     * also packen wir noch einen wertogkeitfaktror rein
     * je tiefer der backtrack desto geringer deren wertigkeit
     *  score= 10*sc + (T-t)
     *  => 11.4 +  4.3 +  0  + 0
     *  vs   0  + 11.3 + 4.2 + 0
     */
    int DEBUG = OPT_DEBUG;

    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][4];        // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int hsc = 0;
    int sc = 0;
    int psc[4];                 // partieller score
    int T = 4;                  // maximale Tiefe
    int itercnt = 0;
    int BEST = -1;
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= 3; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s ###############\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;
        for (i = 0; i <= 3; i++) {
            dprintf("%50s\n", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                for (j = 0; j <= 3; j++) {
                    // if (j == i) continue;
                    dprintf("%55s => %5s\n", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (DEBUG)
                        dumpFs(&R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        psc[0] = R[i][0].basis * 10 + (T - 0);
                        psc[1] = R[i][1].basis * 10 + (T - 1);
                        sc = psc[0] + psc[1];
                        //sc = (R[i][0].basis*10+(T-0)) + (R[i][1].basis*10+(T-1));
                        dprintf("R[%d][0].basis*F0 + R[%d][1].basis*F1 (%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], sc, hsc);
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        for (k = 0; k <= 3; k++) {
                            // if (k==j) continue;
                            dprintf("%55s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (DEBUG)
                                dumpFs(&R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                psc[2] = R[i][2].basis * 10 + (T - 2);
                                sc = psc[0] + psc[1] + psc[2];
//                                sc = R[i][0].basis + R[i][1].basis + R[i][2].basis;
                                dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis (%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], sc, hsc);
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                for (l = 0; l <= 3; l++) {
                                    // if (l==k) continue;
                                    dprintf("%55s %s %5s %s %5s %s %5s %s\n", DIRS[i], DIRC[i], DIRS[j], DIRC[j], DIRS[k], DIRC[k], DIRS[l], DIRC[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (DEBUG)
                                        dumpFs(&R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        psc[3] = R[i][3].basis * 10 + (T - 3);
                                        sc = psc[0] + psc[1] + psc[2] + psc[3];
//                                        sc = R[i][0].basis + R[i][1].basis + R[i][2].basis + R[i][3].basis;
                                        dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }

            B = &R[BEST][0];
            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ####\n", DIRC[BEST], DIRS[BEST], hsc);
                dumpFs(F);
                dumpFs(B);
            }
            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void strat_backtracker3bo2(struct F_st *F)      // based on basis, inklusive erstem
{
    /* 
     * 0+0+11+4 ist schlechter als 11+4+0+0
     * also packen wir noch einen wertogkeitfaktror rein
     * je tiefer der backtrack desto geringer deren wertigkeit
     *  score= 10*sc + (T-t)
     *  => 11.4 +  4.3 +  0  + 0
     *  vs   0  + 11.3 + 4.2 + 0
     */
    int DEBUG = OPT_DEBUG;

#define CALCDEPTH 5
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][CALCDEPTH];        // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int hsc = 0;
    int sc = 0;
    int psc[CALCDEPTH];         // partieller score
    int T = CALCDEPTH - 1;      // maximale Tiefe
    int itercnt = 0;
    int BEST = -1;
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s ###############\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;
        for (i = 0; i <= 3; i++) {
            dprintf("%50s\n", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis * 10 + (T - 0);
                for (j = 0; j <= 3; j++) {
                    // if (j == i) continue;
                    dprintf("%55s => %5s\n", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (DEBUG)
                        dumpFs(&R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        psc[1] = R[i][0].basis * 10 + (T - 1);
                        psc[2] = R[i][1].basis * 10 + (T - 2);
                        sc = psc[0] + psc[1] + psc[2];
                        //sc = (R[i][0].basis*10+(T-0)) + (R[i][1].basis*10+(T-1));
                        dprintf("R[%d][0].basis*F0 + R[%d][1].basis*F1 (%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], sc, hsc);
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        for (k = 0; k <= 3; k++) {
                            // if (k==j) continue;
                            dprintf("%55s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (DEBUG)
                                dumpFs(&R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                psc[3] = R[i][3].basis * 10 + (T - 3);
                                sc = psc[0] + psc[1] + psc[2] + psc[3];
//                                sc = R[i][0].basis + R[i][1].basis + R[i][2].basis;
                                dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis (%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                for (l = 0; l <= 3; l++) {
                                    // if (l==k) continue;
                                    dprintf("%55s %s %5s %s %5s %s %5s %s\n", DIRS[i], DIRC[i], DIRS[j], DIRC[j], DIRS[k], DIRC[k], DIRS[l], DIRC[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (DEBUG)
                                        dumpFs(&R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        psc[4] = R[i][3].basis * 10 + (T - 4);
                                        sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
//                                        sc = R[i][0].basis + R[i][1].basis + R[i][2].basis + R[i][3].basis;
                                        dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }

            B = &R[BEST][0];
            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ####\n", DIRC[BEST], DIRS[BEST], hsc);
                dumpFs(F);
                dumpFs(B);
            }
            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void strat_backtracker4(struct F_st *F) // 3bo2 mit tiefe 4
{
    /* 
     * 0+0+11+4 ist schlechter als 11+4+0+0
     * also packen wir noch einen wertogkeitfaktror rein
     * je tiefer der backtrack desto geringer deren wertigkeit
     *  score= 10*sc + (T-t)
     *  => 11.4 +  4.3 +  0  + 0
     *  vs   0  + 11.3 + 4.2 + 0
     */
    int DEBUG = OPT_DEBUG;

#define BT4DEPTH 6
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][BT4DEPTH]; // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];          // partieller score
    int T = BT4DEPTH - 1;       // maximale Tiefe
    int itercnt = 0;
    int BEST = -1;
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s ###############\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;
        for (i = 0; i <= 3; i++) {
            dprintf("%50s\n", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis * 10 + (T - 0);
                for (j = 0; j <= 3; j++) {
                    // if (j == i) continue;
                    dprintf("%55s => %5s\n", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (DEBUG)
                        dumpFs(&R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        psc[1] = R[i][0].basis * 10 + (T - 1);
                        psc[2] = R[i][1].basis * 10 + (T - 2);
                        sc = psc[0] + psc[1] + psc[2];
                        dprintf("R[%d][0].basis*F0 + R[%d][1].basis*F1 (%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], sc, hsc);
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        for (k = 0; k <= 3; k++) {
                            // if (k==j) continue;
                            dprintf("%55s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (DEBUG)
                                dumpFs(&R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                psc[3] = R[i][3].basis * 10 + (T - 3);
                                sc = psc[0] + psc[1] + psc[2] + psc[3];
                                dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis (%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                for (l = 0; l <= 3; l++) {
                                    // if (l==k) continue;
                                    dprintf("%55s %s %5s %s %5s %s %5s %s\n", DIRS[i], DIRC[i], DIRS[j], DIRC[j], DIRS[k], DIRC[k], DIRS[l], DIRC[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (DEBUG)
                                        dumpFs(&R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        psc[4] = R[i][3].basis * 10 + (T - 4);
                                        sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                        dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        for (m = 0; m <= 3; m++) {
                                            // if (l==k) continue;
                                            dprintf("%55s %s %5s %s %5s %s %5s %s %5s %s\n", DIRS[i], DIRC[i], DIRS[j], DIRC[j], DIRS[k], DIRC[k], DIRS[l], DIRC[l], DIRS[m], DIRC[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (DEBUG)
                                                dumpFs(&R[i][3]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                psc[5] = R[i][4].basis * 10 + (T - 5);
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4] + psc[5];
                                                dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], psc[4], psc[5], sc, hsc);
                                                if (sc > hsc) {
                                                    hsc = sc;
                                                    BEST = i;   // der is aber noch viel viel besser
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }

            B = &R[BEST][0];
            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ####\n", DIRC[BEST], DIRS[BEST], hsc);
                dumpFs(F);
                dumpFs(B);
            }
            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void strat_backtracker4v2(struct F_st *F)       // 3bo2 mit tiefe 4 prioscore nur bei score
{
    /* 
     * 0+0+11+4 ist schlechter als 11+4+0+0
     * also packen wir noch einen wertogkeitfaktror rein
     * je tiefer der backtrack desto geringer deren wertigkeit
     *  score= 10*sc + (T-t)
     *  => 11.4 +  4.3 +  0  + 0
     *  vs   0  + 11.3 + 4.2 + 0
     */
    int DEBUG = OPT_DEBUG;

#define BT4DEPTH 6
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][BT4DEPTH]; // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];          // partieller score
    int T = BT4DEPTH - 1;       // maximale Tiefe
    int itercnt = 0;
    int BEST = -1;
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s ###############\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;
        for (i = 0; i <= 3; i++) {
            dprintf("%50s\n", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis ? R[i][0].basis * 10 + (T - 0) : 0;
                for (j = 0; j <= 3; j++) {
                    // if (j == i) continue;
                    dprintf("%55s => %5s\n", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (DEBUG)
                        dumpFs(&R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        psc[1] = R[i][0].basis ? R[i][0].basis * 10 + (T - 1) : 0;
                        psc[2] = R[i][1].basis ? R[i][1].basis * 10 + (T - 2) : 0;
                        sc = psc[0] + psc[1] + psc[2];
                        dprintf("R[%d][0].basis*F0 + R[%d][1].basis*F1 (%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], sc, hsc);
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        for (k = 0; k <= 3; k++) {
                            // if (k==j) continue;
                            dprintf("%55s => %5s => %5s\n", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (DEBUG)
                                dumpFs(&R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                psc[3] = R[i][3].basis ? R[i][3].basis * 10 + (T - 3) : 0;
                                sc = psc[0] + psc[1] + psc[2] + psc[3];
                                dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis (%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                for (l = 0; l <= 3; l++) {
                                    // if (l==k) continue;
                                    dprintf("%55s %s %5s %s %5s %s %5s %s\n", DIRS[i], DIRC[i], DIRS[j], DIRC[j], DIRS[k], DIRC[k], DIRS[l], DIRC[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (DEBUG)
                                        dumpFs(&R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        psc[4] = R[i][3].basis ? R[i][3].basis * 10 + (T - 4) : 0;
                                        sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                        dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        for (m = 0; m <= 3; m++) {
                                            // if (l==k) continue;
                                            dprintf("%55s %s %5s %s %5s %s %5s %s %5s %s\n", DIRS[i], DIRC[i], DIRS[j], DIRC[j], DIRS[k], DIRC[k], DIRS[l], DIRC[l], DIRS[m], DIRC[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (DEBUG)
                                                dumpFs(&R[i][3]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                psc[5] = R[i][4].basis ? R[i][4].basis * 10 + (T - 5) : 0;
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4] + psc[5];
                                                dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], psc[4], psc[5], sc, hsc);
                                                if (sc > hsc) {
                                                    hsc = sc;
                                                    BEST = i;   // der is aber noch viel viel besser
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }

            B = &R[BEST][0];
            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ####\n", DIRC[BEST], DIRS[BEST], hsc);
                dumpFs(F);
                dumpFs(B);
            }
            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void strat_backtracker4v3(struct F_st *F)       // 3bo2 mit tiefe 4 prioscore nur bei score
{
    /* 
     * 0+0+11+4 ist schlechter als 11+4+0+0
     * also packen wir noch einen wertogkeitfaktror rein
     * je tiefer der backtrack desto geringer deren wertigkeit
     *  score= 10*sc + (T-t)
     *  => 11.4 +  4.3 +  0  + 0
     *  vs   0  + 11.3 + 4.2 + 0
     */
    int DEBUG = OPT_DEBUG;

#define BT4DEPTH 6
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][BT4DEPTH]; // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int c = 0;                  // multipurpose counter
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];          // partieller score
    int T = BT4DEPTH - 1;       // maximale Tiefe
    int itercnt = 0;
    int BEST = -1;
    int BESTscore[4];
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s #\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;

        // L1 
        for (i = 0; i <= 3; i++) {
            BESTscore[i] = 0;
            dprintf("%55s %5s\n", "L1", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis ? R[i][0].basis * 10 + (T - 0) : 0;
                sc = psc[0];
                dprintf("%55s %5s (%d) score:%d hsc:%d\n", "L1", DIRS[i], psc[0], sc, hsc);
                if (sc > BESTscore[i]) {
                    BESTscore[i] = sc;
                }
                if (sc > hsc) {
                    hsc = sc;
                    BEST = i;   // der is aber besser
                }
                // L2
                for (j = 0; j <= 3; j++) {
                    dprintf("%55s %5s => %5s\n", "L2", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        if (DEBUG)
                            dumpFs(&R[i][1]);
                        psc[1] = R[i][1].basis ? R[i][1].basis * 10 + (T - 1) : 0;
                        sc = psc[0] + psc[1];
                        dprintf("%55s %5s => %5s (%d+%d) score:%d hsc:%d\n", "L2", DIRS[i], DIRS[j], psc[0], psc[1], sc, hsc);
                        if (sc > BESTscore[i]) {
                            BESTscore[i] = sc;
                        }
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        // L3
                        for (k = 0; k <= 3; k++) {
                            dprintf("%55s %5s => %5s => %5s\n", "L3", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                if (DEBUG)
                                    dumpFs(&R[i][2]);
                                psc[2] = R[i][2].basis ? R[i][2].basis * 10 + (T - 2) : 0;
                                sc = psc[0] + psc[1] + psc[2];
                                dprintf("%55s %5s => %5s => %5s (%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], psc[0], psc[1], psc[2], sc, hsc);
                                if (sc > BESTscore[i]) {
                                    BESTscore[i] = sc;
                                }
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                // L4
                                for (l = 0; l <= 3; l++) {
                                    dprintf("%55s %5s => %5s => %5s => %5s\n", "L4", DIRS[i], DIRS[j], DIRS[k], DIRS[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        if (DEBUG)
                                            dumpFs(&R[i][3]);
                                        psc[3] = R[i][3].basis ? R[i][3].basis * 10 + (T - 3) : 0;
                                        sc = psc[0] + psc[1] + psc[2] + psc[3];
                                        dprintf("%55s %5s => %5s => %5s => %5s (%d+%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], DIRS[l], psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                        if (sc > BESTscore[i]) {
                                            BESTscore[i] = sc;
                                        }
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        // L5
                                        for (m = 0; m <= 3; m++) {
                                            dprintf("%55s %5s => %5s => %5s => %5s => %5s\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                if (DEBUG)
                                                    dumpFs(&R[i][4]);
                                                psc[4] = R[i][4].basis ? R[i][4].basis * 10 + (T - 4) : 0;
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                                dprintf("%55s %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                                if (sc > BESTscore[i]) {
                                                    BESTscore[i] = sc;
                                                }
                                                if (sc > hsc) {
                                                    hsc = sc;
                                                    BEST = i;   // der is aber noch viel viel besser
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }


        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            
            for (c = 0; c <= 3; c++) {
                if (BEST != c && hsc == BESTscore[c]) {
                    dprintf("XXX found [%s] as good as [%s] XXX\n", DIRS[BEST], DIRS[c]);

                }
            }
            dprintf("\n");


            B = &R[BEST][0];

            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ### ", DIRC[BEST], DIRS[BEST], hsc);
                for (c = 0; c <= 3; c++) {
                    printf(" %s=%d ", DIRS[c], BESTscore[c]);
                }
                printf("\n");


                dumpFs(F);
                dumpFs(B);
            }


            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

// calculate weight, set weight-array
int weight(uint64_t F, uint8_t *G )
{
    uint8_t *UD[] = { &G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP] }; 
    uint8_t *LR[] = { &G[DIR_RIGHT],&G[DIR_RIGHT],&G[DIR_LEFT],&G[DIR_LEFT],&G[DIR_RIGHT],&G[DIR_RIGHT],&G[DIR_LEFT],&G[DIR_LEFT],&G[DIR_RIGHT],&G[DIR_RIGHT],&G[DIR_LEFT],&G[DIR_LEFT],&G[DIR_RIGHT],&G[DIR_RIGHT],&G[DIR_LEFT],&G[DIR_LEFT] }; 
    int DEBUG = OPT_DEBUG;
    uint64_t Fb = F;
    int c = 0;
    int x = 0;
    //dumpF(F);

    for (c=0;c<=3;c++) {
        G[c]=0;
    }
    for ( c=0; c<=15; c++ ) {
        x = F & 0xf;
        *UD[c]+=x;
        *LR[c]+=x;
        
        F = F >> 4;
    }
    dprintf("WWW %016lx: U:%d R:%d D:%d L:%d\n",Fb,G[0],G[1],G[2],G[3]);
  return 0;    
}
// count, set count-array
int count(uint64_t F, uint8_t *G )
{
    uint8_t *UD[] = { &G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_DOWN],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP],&G[DIR_UP] }; 
    uint8_t *LR[] = { &G[DIR_RIGHT],&G[DIR_RIGHT],&G[DIR_LEFT],&G[DIR_LEFT],&G[DIR_RIGHT],&G[DIR_RIGHT],&G[DIR_LEFT],&G[DIR_LEFT],&G[DIR_RIGHT],&G[DIR_RIGHT],&G[DIR_LEFT],&G[DIR_LEFT],&G[DIR_RIGHT],&G[DIR_RIGHT],&G[DIR_LEFT],&G[DIR_LEFT] }; 
    int DEBUG = OPT_DEBUG;

    int c = 0;
    int x = 0;
    //dumpF(F);

    for (c=0;c<=3;c++) {
        G[c]=0;
    }
    for ( c=0; c<=15; c++ ) {
        x = F & 0xf;
        if (x) {
            *UD[c]+=1;
            *LR[c]+=1;
        }
        F = F >> 4;
    }
    dprintf("%016lx: U:%d R:%d D:%d L:%d\n",F,G[0],G[1],G[2],G[3]);
  return 0;    
}

int weightcount(uint64_t F, uint8_t *W, uint8_t *C )
{
    int idx_ud[] = { 2,2,2,2,2,2,2,2,0,0,0,0,0,0,0,0 };
    int idx_lr[] = { 1,1,3,3,1,1,3,3,1,1,3,3,1,1,3,3 };
    int DEBUG = OPT_DEBUG;

    int c = 0;
    int x = 0;
    //dumpF(F);

    for (c=0;c<=3;c++) {
        W[c]=0;
        C[c]=0;
    }
    for ( c=0; c<=15; c++ ) {
        x = F & 0xf;
        if (x) {
            W[ idx_ud[ c ] ] += x;
            W[ idx_lr[ c ] ] += x;
            C[ idx_ud[ c ] ] += 1;
            C[ idx_lr[ c ] ] += 1;
        }
        F = F >> 4;
    }
    /// dprintf("%016lx: weight: U:%d R:%d D:%d L:%d | count:  U:%d R:%d D:%d L:%d \n",F,W[0],W[1],W[2],W[3],C[0],C[1],C[2],C[3]);
    //dprintf("XXX weight: U:%3d R:%3d D:%3d L:%3d\nXXX count:  U:%3d R:%3d D:%3d L:%3d \n",W[0],W[1],W[2],W[3],C[0],C[1],C[2],C[3]);
    dprintf("XXX weight:   %3d   %3d   %3d   %3d\nXXX count:    %3d   %3d   %3d   %3d\n",W[0],W[1],W[2],W[3],C[0],C[1],C[2],C[3]);
  return 0;    
}


// wenn richtungen den gleichen score ergeben, dann nimm die richtung, auf deren haelfte am meisten punkte stehen
// ein versuch nur die anzahl der belegten felder zu werten brachte nix (leicht schlechter als backtracker4v3

void strat_backtracker4v4(struct F_st *F)       // 3bo2 mit tiefe 4 prioscore nur bei score mit richtungwichtung bei gleichem score
{
    /* 
     * 0+0+11+4 ist schlechter als 11+4+0+0
     * also packen wir noch einen wertogkeitfaktror rein
     * je tiefer der backtrack desto geringer deren wertigkeit
     *  score= 10*sc + (T-t)
     *  => 11.4 +  4.3 +  0  + 0
     *  vs   0  + 11.3 + 4.2 + 0
     */
    int DEBUG = OPT_DEBUG;

#define BT4DEPTH 6
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][BT4DEPTH]; // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int c = 0;                  // multipurpose counter
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];          // partieller score
    int T = BT4DEPTH - 1;       // maximale Tiefe
    int itercnt = 0;
    int BEST = -1;
    int BESTscore[4];
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s #\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;

        // L1 
        for (i = 0; i <= 3; i++) {
            BESTscore[i] = 0;
            dprintf("%55s %5s ?\n", "L1", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis ? R[i][0].basis * 10 + (T - 0) : 0;
                sc = psc[0];
                dprintf("%55s %5s (%d) score:%d hsc:%d\n", "L1", DIRS[i], psc[0], sc, hsc);
                if (sc > BESTscore[i]) {
                    BESTscore[i] = sc;
                }
                if (sc > hsc) {
                    hsc = sc;
                    BEST = i;   // der is aber besser
                }
                // L2
                for (j = 0; j <= 3; j++) {
                    dprintf("%55s %5s => %5s ?\n", "L2", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        if (DEBUG)
                            dumpFs(&R[i][1]);
                        psc[1] = R[i][1].basis ? R[i][1].basis * 10 + (T - 1) : 0;
                        sc = psc[0] + psc[1];
                        dprintf("%55s %5s => %5s (%d+%d) score:%d hsc:%d\n", "L2", DIRS[i], DIRS[j], psc[0], psc[1], sc, hsc);
                        if (sc > BESTscore[i]) {
                            BESTscore[i] = sc;
                        }
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        // L3
                        for (k = 0; k <= 3; k++) {
                            dprintf("%55s %5s => %5s => %5s ?\n", "L3", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                if (DEBUG)
                                    dumpFs(&R[i][2]);
                                psc[2] = R[i][2].basis ? R[i][2].basis * 10 + (T - 2) : 0;
                                sc = psc[0] + psc[1] + psc[2];
                                dprintf("%55s %5s => %5s => %5s (%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], psc[0], psc[1], psc[2], sc, hsc);
                                if (sc > BESTscore[i]) {
                                    BESTscore[i] = sc;
                                }
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                // L4
                                for (l = 0; l <= 3; l++) {
                                    dprintf("%55s %5s => %5s => %5s => %5s ?\n", "L4", DIRS[i], DIRS[j], DIRS[k], DIRS[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        if (DEBUG)
                                            dumpFs(&R[i][3]);
                                        psc[3] = R[i][3].basis ? R[i][3].basis * 10 + (T - 3) : 0;
                                        sc = psc[0] + psc[1] + psc[2] + psc[3];
                                        dprintf("%55s %5s => %5s => %5s => %5s (%d+%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], DIRS[l], psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                        if (sc > BESTscore[i]) {
                                            BESTscore[i] = sc;
                                        }
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        // L5
                                        for (m = 0; m <= 3; m++) {
                                            dprintf("%55s %5s => %5s => %5s => %5s => %5s ?\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                if (DEBUG)
                                                    dumpFs(&R[i][4]);
                                                psc[4] = R[i][4].basis ? R[i][4].basis * 10 + (T - 4) : 0;
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                                dprintf("%55s %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                                if (sc > BESTscore[i]) {
                                                    BESTscore[i] = sc;
                                                }
                                                if (sc > hsc) {
                                                    hsc = sc;
                                                    BEST = i;   // der is aber noch viel viel besser
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }

  
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            
            uint8_t W[4];
            

            weight(F->f,W);
            for (c = 0; c <= 3; c++) {
                if (BEST != c && hsc == BESTscore[c]) {
                    dprintf("XXX found [%s] = %d = [%s] XXX\n", DIRS[BEST],hsc, DIRS[c]);
                    if (W[BEST] < W[c]) {
                        dprintf("XXX [%d < %d] so new BEST will be %s XXX\n", W[BEST], W[c],DIRS[c]);
                        BEST = c;
                    }
                }
            }
            dprintf("\n");

            B = &R[BEST][0];

            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ### ", DIRC[BEST], DIRS[BEST], hsc);
                for (c = 0; c <= 3; c++) {
                    printf(" %s=%d ", DIRS[c], BESTscore[c]);
                }
                printf("\n");


                dumpFs(F);
                dumpFs(B);
            }


            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}
// ZONK 9kl = 6290
void strat_backtracker4v5(struct F_st *F)       // 4v4 jetzt mal mit -L (aus bt5)
{
    /* 
     * 0+0+11+4 ist schlechter als 11+4+0+0
     * also packen wir noch einen wertogkeitfaktror rein
     * je tiefer der backtrack desto geringer deren wertigkeit
     *  score= 10*sc + (T-t)
     *  => 11.4 +  4.3 +  0  + 0
     *  vs   0  + 11.3 + 4.2 + 0
     */
    int DEBUG = OPT_DEBUG;

#define BT4DEPTH 6
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][BT4DEPTH]; // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int c = 0;                  // multipurpose counter
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];          // partieller score
    int T = BT4DEPTH - 1;       // maximale Tiefe
    int L = 0;
    int itercnt = 0;
    int BEST = -1;
    int BESTscore[4];
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s #\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;

        // L1 
        for (i = 0; i <= 3; i++) {
            L = 1;
            BESTscore[i] = 0;
            dprintf("%55s %5s ?\n", "L1", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis ? R[i][0].basis * 10 - L : 0;
                sc = psc[0];
                dprintf("%55s %5s (%d) score:%d hsc:%d\n", "L1", DIRS[i], psc[0], sc, hsc);
                if (sc > BESTscore[i]) {
                    BESTscore[i] = sc;
                }
                if (sc > hsc) {
                    hsc = sc;
                    BEST = i;   // der is aber besser
                }
                // L2
                for (j = 0; j <= 3; j++) {
                    L = 2;
                    dprintf("%55s %5s => %5s ?\n", "L2", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        if (DEBUG)
                            dumpFs(&R[i][1]);
                        psc[1] = R[i][1].basis ? R[i][1].basis * 10 - L : 0;
                        sc = psc[0] + psc[1];
                        dprintf("%55s %5s => %5s (%d+%d) score:%d hsc:%d\n", "L2", DIRS[i], DIRS[j], psc[0], psc[1], sc, hsc);
                        if (sc > BESTscore[i]) {
                            BESTscore[i] = sc;
                        }
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        // L3
                        for (k = 0; k <= 3; k++) {
                            L = 3;
                            dprintf("%55s %5s => %5s => %5s ?\n", "L3", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                if (DEBUG)
                                    dumpFs(&R[i][2]);
                                psc[2] = R[i][2].basis ? R[i][2].basis * 10 - L : 0;
                                sc = psc[0] + psc[1] + psc[2];
                                dprintf("%55s %5s => %5s => %5s (%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], psc[0], psc[1], psc[2], sc, hsc);
                                if (sc > BESTscore[i]) {
                                    BESTscore[i] = sc;
                                }
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                // L4
                                for (l = 0; l <= 3; l++) {
                                    L = 4;
                                    dprintf("%55s %5s => %5s => %5s => %5s ?\n", "L4", DIRS[i], DIRS[j], DIRS[k], DIRS[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        if (DEBUG)
                                            dumpFs(&R[i][3]);
                                        psc[3] = R[i][3].basis ? R[i][3].basis * 10 - L : 0;
                                        sc = psc[0] + psc[1] + psc[2] + psc[3];
                                        dprintf("%55s %5s => %5s => %5s => %5s (%d+%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], DIRS[l], psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                        if (sc > BESTscore[i]) {
                                            BESTscore[i] = sc;
                                        }
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        // L5
                                        for (m = 0; m <= 3; m++) {
                                            L = 5;
                                            dprintf("%55s %5s => %5s => %5s => %5s => %5s ?\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                if (DEBUG)
                                                    dumpFs(&R[i][4]);
                                                psc[4] = R[i][4].basis ? R[i][4].basis * 10 - L : 0;
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                                dprintf("%55s %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                                if (sc > BESTscore[i]) {
                                                    BESTscore[i] = sc;
                                                }
                                                if (sc > hsc) {
                                                    hsc = sc;
                                                    BEST = i;   // der is aber noch viel viel besser
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }

  
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            
            uint8_t W[4];
            

            weight(F->f,W);
            for (c = 0; c <= 3; c++) {
                if (BEST != c && hsc == BESTscore[c]) {
                    dprintf("XXX found [%s] = %d = [%s] XXX\n", DIRS[BEST],hsc, DIRS[c]);
                    if (W[BEST] < W[c]) {
                        dprintf("XXX [%d < %d] so new BEST will be %s XXX\n", W[BEST], W[c],DIRS[c]);
                        BEST = c;
                    }
                }
            }
            dprintf("\n");

            B = &R[BEST][0];

            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ### ", DIRC[BEST], DIRS[BEST], hsc);
                for (c = 0; c <= 3; c++) {
                    printf(" %s=%d ", DIRS[c], BESTscore[c]);
                }
                printf("\n");


                dumpFs(F);
                dumpFs(B);
            }


            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}



// wenn richtungen den gleichen score ergeben, dann nimm die richtung, auf deren haelfte am meisten punkte stehen
// v4 hat NUR bei score-gleichheit auf gravity gesetzt. das versaut z.B.
/*
    ### best move is vv down scoring 130 ###  up=128  right=129  down=130  left=109 
      2  3  5  1 :      4     8    32     2
      2  2  0  2 :      4     4     _     4
      0  0  1  0 :      _     _     2     _
      0  0  0  0 :      _     _     _     _
    Score: 132 / 42
      0  0  0  0 :      _     _     _     _
      0  0  0  0 :      _     _     _     _
      0  3  5  1 :      _     8    32     2
      3  2  1  2 :      8     4     2     4
*/
// also bauen wir eine +/- tolerance ein, die zur gravity-entscheidung fuehrt
// das wuerde im oberen fall mit +-1 down und right gleich bewerten
// mit tolerance 1 liegt er zwischen 4v4 und 4v3
// bei 15k runs: stats 7k => 10k == score(9000) (je kleiner je besser)
// tolerance   4v3    4v4   4v5
//     1      7860   7260  7660
//     2                   9350
//
// grundsaetzlich ist toleranz erst mal schlechter, 2 noch schlechter
//
// eventuell die toleranz nur bei niedrigeren scores anwenden 


// hier hat up 152, down waere aber klueger.
/*
WWW 0001000101230034: U:2 R:14 D:13 L:1

### best move is ^^ up scoring 152 ###  up=152  right=0  down=150  left=150 
  0  0  0  1 :      _     _     _     2
  0  0  0  1 :      _     _     _     2
  0  1  2  3 :      _     2     4     8
  0  0  3  4 :      _     _     8    16
Score: 72 / 28
  0  1  2  2 :      _     2     4     4
  0  0  3  3 :      _     _     8     8
  0  0  0  4 :      _     _     _    16
  0  0  0  0 :      _     _     _     _
Score: 4 / 2
*/
// hat alles nix gebracht. ich setz noch mal die depth einen tiefer
// ausgehend vom 4v4 source mit 9k-value = 7260

// bt5 geht noch einen level tiefer als bt4v4 und scheitert!
// der 9k-value liegt bei 8960, also nochmal schlechter als bt4v3
// erste analysen zeigen entscheidungen aufgrund von 
//   L6  left =>    up => right =>  down =>  left =>  left (0+0+0+0+22+31) score:53 hsc:53
// also 4 mal keine punkte, aber dann so viele, dass das die richtung beeinflusst
//
//// Test 1. wenn die ersten X (4) tiefen kein Score gemacht wurde, brich ab : (0+0+0+0) == 0
// ==> 8690 ( -= 270 )
//// Test 2. wenn die ersten X (5) tiefen kein Score gemacht wurde, brich ab : (0+0+0+0+0) == 0
// ==> 8800 ( -= 160 )
// eventuell ist das aber von der belegung abhaengig.
// also spaeter doch tiefer ?
//    -> solange die placenews nicht beachtet werden, ist das egal
//
// continue vor die L6 sollte v4v emulieren. tuts aber nicht. und das liegt an der 
// R[i][1].basis * 10 + (T - 1) : 0 - Mechanik. Der Score ist je nach tiefe anders.
//// Test 3. Score-Berechnung R[i][1].basis * 10 - L : 0 (L = leveltiefe)
// ==> ganz mies: 12900
//   kann aber auch an der tiegfe l;iegen. werd mal den 4v4 auf -L testen
//// Test 4. 4v4 mit -L = 4v5
// ==> zonk: 6290
//// Test 5. 4v5 mit L=6 = 5v2
// ==> grmbl: 7000

void strat_backtracker5(struct F_st *F)       // 4v4++
{

    int DEBUG = OPT_DEBUG;

#define BT5DEPTH 7
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][BT5DEPTH]; // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int c = 0;                  // multipurpose counter
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int n = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT5DEPTH];          // partieller score
    int T = BT5DEPTH - 1;       // maximale Tiefe
    int L = 0;
    int itercnt = 0;
    int BEST = -1;
    int BESTscore[4];
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }

    int dcnt = 0;
    while (1) {
        dprintf("-[%d]- go for %40s #\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;

        // L1 
        for (i = 0; i <= 3; i++) {
            L = 1;
            BESTscore[i] = 0;
            dprintf("%55s %5s ?\n", "L1", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                dcnt++;
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis ? R[i][0].basis * 10 - L : 0;
                dprintf("DDD L%d moving %s would add %d score, or %d basis, resulting in psc[0]=%d\n",L,DIRS[i],R[i][0].score,R[i][0].basis,psc[0]);
                sc = psc[0];
                dprintf("%55s %5s (%d) score:%d hsc:%d\n", "L1", DIRS[i], psc[0], sc, hsc);
                if (sc > BESTscore[i]) {
                    BESTscore[i] = sc;
                }
                if (sc > hsc) {
                    hsc = sc;
                    BEST = i;   // der is aber besser
                }
                // L2
                for (j = 0; j <= 3; j++) {
                    L = 2;
                    dprintf("%55s %5s => %5s ?\n", "L2", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        dcnt++;
                        if (DEBUG)
                            dumpFs(&R[i][1]);
                        psc[1] = R[i][1].basis ? R[i][1].basis * 10 - T : 0;
                        sc = psc[0] + psc[1];
                        dprintf("%55s %5s => %5s (%d+%d) score:%d hsc:%d\n", "L2", DIRS[i], DIRS[j], psc[0], psc[1], sc, hsc);
                        if (sc > BESTscore[i]) {
                            BESTscore[i] = sc;
                        }
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        // L3
                        for (k = 0; k <= 3; k++) {
                            L = 3;
                            dprintf("%55s %5s => %5s => %5s ?\n", "L3", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                dcnt++;
                                if (DEBUG)
                                    dumpFs(&R[i][2]);
                                psc[2] = R[i][2].basis ? R[i][2].basis * 10 - T : 0;
                                sc = psc[0] + psc[1] + psc[2];
                                dprintf("%55s %5s => %5s => %5s (%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], psc[0], psc[1], psc[2], sc, hsc);
                                if (sc > BESTscore[i]) {
                                    BESTscore[i] = sc;
                                }
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                // L4
                                for (l = 0; l <= 3; l++) {
                                    L = 4;
                                    dprintf("%55s %5s => %5s => %5s => %5s ?\n", "L4", DIRS[i], DIRS[j], DIRS[k], DIRS[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        dcnt++;
                                        if (DEBUG)
                                            dumpFs(&R[i][3]);
                                        psc[3] = R[i][3].basis ? R[i][3].basis * 10 - T : 0;
                                        sc = psc[0] + psc[1] + psc[2] + psc[3];
                                        dprintf("%55s %5s => %5s => %5s => %5s (%d+%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], DIRS[l], psc[0], psc[1], psc[2], psc[3], sc, hsc);
//                                        if (sc == 0) {
//                                          dprintf("ABRT 4 level kein score\n");
//                                          continue;   
//                                        }
                                        if (sc > BESTscore[i]) {
                                            BESTscore[i] = sc;
                                        }
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        // L5
                                        for (m = 0; m <= 3; m++) {
                                            L = 5;
                                            dprintf("%55s %5s => %5s => %5s => %5s => %5s ?\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                dcnt++;
                                                if (DEBUG)
                                                    dumpFs(&R[i][4]);
                                                psc[4] = R[i][4].basis ? R[i][4].basis * 10 - T : 0;
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                                dprintf("%55s %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
//                                                if (sc == 0) {
//                                                  dprintf("ABRT 5 level kein score\n");
//                                                  continue;   
//                                                }
                                                if (sc > BESTscore[i]) {
                                                    BESTscore[i] = sc;
                                                }
                                                if (sc > hsc) {
                                                    hsc = sc;
                                                    BEST = i;   // der is aber noch viel viel besser
                                                }

                                                // L6
                                                for (n = 0; n <= 3; n++) {
                                                    L = 6;
                                                    dprintf("%55s %5s => %5s => %5s => %5s => %5s => %5s ?\n", "L6", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], DIRS[n]);
                                                    DIRFUNC[n] (&R[i][4], &R[i][5]);
                                                    if (R[i][4].f != R[i][5].f) {       // actually moved
                                                        dcnt++;
                                                        if (DEBUG)
                                                            dumpFs(&R[i][5]);
                                                        psc[5] = R[i][5].basis ? R[i][5].basis * 10 - T : 0;
                                                        sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4] + psc[5];
                                                        dprintf("%55s %5s => %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L6", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], DIRS[n], psc[0], psc[1], psc[2], psc[3], psc[4], psc[5], sc, hsc);
                                                        if (sc > BESTscore[i]) {
                                                            BESTscore[i] = sc;
                                                        }
                                                        if (sc > hsc) {
                                                            hsc = sc;
                                                            BEST = i;   // der is aber noch viel viel viel besser
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }

  
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            
            uint8_t W[4];
            

            weight(F->f,W);
            for (c = 0; c <= 3; c++) {
                if (BEST != c && hsc == BESTscore[c]) {
                    dprintf("XXX found [%s] = %d = [%s] XXX\n", DIRS[BEST],hsc, DIRS[c]);
                    if (W[BEST] < W[c]) {
                        dprintf("XXX [%d < %d] so new BEST will be %s XXX\n", W[BEST], W[c],DIRS[c]);
                        BEST = c;
                    }
                }
            }
            dprintf("\n");

            B = &R[BEST][0];

            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ### ", DIRC[BEST], DIRS[BEST], hsc);
                for (c = 0; c <= 3; c++) {
                    printf(" %s=%d ", DIRS[c], BESTscore[c]);
                }
                printf("\n");


                dumpFs(F);
                dumpFs(B);
            }


            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}
// nur 7000
void strat_backtracker5v2(struct F_st *F)       // 4v4 jetzt mal mit -L und depth++
{
    int DEBUG = OPT_DEBUG;

#define BT6DEPTH 8
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][BT6DEPTH]; // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int c = 0;                  // multipurpose counter
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int n = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT6DEPTH];          // partieller score
    int T = BT6DEPTH - 1;       // maximale Tiefe
    int L = 0;
    int itercnt = 0;
    int BEST = -1;
    int BESTscore[4];
    int GOOD = -1;

    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s #\n", itercnt++, " ");
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;

        // L1 
        for (i = 0; i <= 3; i++) {
            L = 1;
            BESTscore[i] = 0;
            dprintf("%55s %5s ?\n", "L1", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis ? R[i][0].basis * 10 - L : 0;
                sc = psc[0];
                dprintf("%55s %5s (%d) score:%d hsc:%d\n", "L1", DIRS[i], psc[0], sc, hsc);
                if (sc > BESTscore[i]) {
                    BESTscore[i] = sc;
                }
                if (sc > hsc) {
                    hsc = sc;
                    BEST = i;   // der is aber besser
                }
                // L2
                for (j = 0; j <= 3; j++) {
                    L = 2;
                    dprintf("%55s %5s => %5s ?\n", "L2", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        if (DEBUG)
                            dumpFs(&R[i][1]);
                        psc[1] = R[i][1].basis ? R[i][1].basis * 10 - L : 0;
                        sc = psc[0] + psc[1];
                        dprintf("%55s %5s => %5s (%d+%d) score:%d hsc:%d\n", "L2", DIRS[i], DIRS[j], psc[0], psc[1], sc, hsc);
                        if (sc > BESTscore[i]) {
                            BESTscore[i] = sc;
                        }
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        // L3
                        for (k = 0; k <= 3; k++) {
                            L = 3;
                            dprintf("%55s %5s => %5s => %5s ?\n", "L3", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                if (DEBUG)
                                    dumpFs(&R[i][2]);
                                psc[2] = R[i][2].basis ? R[i][2].basis * 10 - L : 0;
                                sc = psc[0] + psc[1] + psc[2];
                                dprintf("%55s %5s => %5s => %5s (%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], psc[0], psc[1], psc[2], sc, hsc);
                                if (sc > BESTscore[i]) {
                                    BESTscore[i] = sc;
                                }
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                // L4
                                for (l = 0; l <= 3; l++) {
                                    L = 4;
                                    dprintf("%55s %5s => %5s => %5s => %5s ?\n", "L4", DIRS[i], DIRS[j], DIRS[k], DIRS[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        if (DEBUG)
                                            dumpFs(&R[i][3]);
                                        psc[3] = R[i][3].basis ? R[i][3].basis * 10 - L : 0;
                                        sc = psc[0] + psc[1] + psc[2] + psc[3];
                                        dprintf("%55s %5s => %5s => %5s => %5s (%d+%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], DIRS[l], psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                        if (sc > BESTscore[i]) {
                                            BESTscore[i] = sc;
                                        }
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        // L5
                                        for (m = 0; m <= 3; m++) {
                                            L = 5;
                                            dprintf("%55s %5s => %5s => %5s => %5s => %5s ?\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                if (DEBUG)
                                                    dumpFs(&R[i][4]);
                                                psc[4] = R[i][4].basis ? R[i][4].basis * 10 - L : 0;
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                                dprintf("%55s %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                                if (sc > BESTscore[i]) {
                                                    BESTscore[i] = sc;
                                                }
                                                if (sc > hsc) {
                                                    hsc = sc;
                                                    BEST = i;   // der is aber noch viel viel besser
                                                }

                                                // L6
                                                for (n = 0; n <= 3; n++) {
                                                    L = 6;
                                                    dprintf("%55s %5s => %5s => %5s => %5s => %5s => %5s ?\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], DIRS[n]);
                                                    DIRFUNC[m] (&R[i][4], &R[i][5]);
                                                    if (R[i][4].f != R[i][5].f) {       // actually moved
                                                        if (DEBUG)
                                                            dumpFs(&R[i][5]);
                                                        psc[5] = R[i][5].basis ? R[i][5].basis * 10 - L : 0;
                                                        sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4] + psc[5];
                                                        dprintf("%55s %5s => %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], DIRS[n], psc[0], psc[1], psc[2], psc[3], psc[4], psc[5], sc, hsc);
                                                        if (sc > BESTscore[i]) {
                                                            BESTscore[i] = sc;
                                                        }
                                                        if (sc > hsc) {
                                                            hsc = sc;
                                                            BEST = i;   // der is aber noch viel viel besser
                                                        }
                                                    }
                                                }


                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }

  
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            
            uint8_t W[4];
            

            weight(F->f,W);
            for (c = 0; c <= 3; c++) {
                if (BEST != c && hsc == BESTscore[c]) {
                    dprintf("XXX found [%s] = %d = [%s] XXX\n", DIRS[BEST],hsc, DIRS[c]);
                    if (W[BEST] < W[c]) {
                        dprintf("XXX [%d < %d] so new BEST will be %s XXX\n", W[BEST], W[c],DIRS[c]);
                        BEST = c;
                    }
                }
            }
            dprintf("\n");

            B = &R[BEST][0];

            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ### ", DIRC[BEST], DIRS[BEST], hsc);
                for (c = 0; c <= 3; c++) {
                    printf(" %s=%d ", DIRS[c], BESTscore[c]);
                }
                printf("\n");


                dumpFs(F);
                dumpFs(B);
            }


            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}


// Test 1: Freifelder zaehlen, ab
void strat_backtracker4v6(struct F_st *F)       // schumama
{
    /* 
     * 0+0+11+4 ist schlechter als 11+4+0+0
     * also packen wir noch einen wertogkeitfaktror rein
     * je tiefer der backtrack desto geringer deren wertigkeit
     *  score= 10*sc + (T-t)
     *  => 11.4 +  4.3 +  0  + 0
     *  vs   0  + 11.3 + 4.2 + 0
     */
    int DEBUG = OPT_DEBUG;

#define BT4DEPTH 6
    struct F_st *B;             // best pointer to R[...]
    struct F_st R[4][BT4DEPTH]; // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int c = 0;                  // multipurpose counter
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];          // partieller score
    int T = BT4DEPTH - 1;       // maximale Tiefe
    int L = 0;
    int itercnt = 0;
    int BEST = -1;
    int BESTscore[4];
    int GOOD = -1;
    
    for (i = 0; i <= 3; i++) {
        for (j = 0; j <= T; j++) {
            init_F(&R[i][j]);
        }
    }
    B = F;

    if (DEBUG) {
        printf("--- START ---\n");
        dumpFs(F);
    }


    while (1) {
        dprintf("-[%d]- go for %40s #\n", itercnt, " ");
        
        if (DEBUG)
            dumpFs(F);
        GOOD = -1;
        BEST = -1;

        // L1 
        for (i = 0; i <= 3; i++) {
            L = 1;
            BESTscore[i] = -1;
            dprintf("%55s %5s ?\n", "L1", DIRS[i]);
            DIRFUNC[i] (F, &R[i][0]);
            if (DEBUG)
                dumpFs(&R[i][0]);
            if (F->f != R[i][0].f) {    // actually moved
                GOOD = i;       // einen gibts immer
                psc[0] = R[i][0].basis ? R[i][0].basis * 10 - L : 0;
                sc = psc[0];
                dprintf("%55s %5s (%d) score:%d hsc:%d\n", "L1", DIRS[i], psc[0], sc, hsc);
                if (sc > BESTscore[i]) {
                    BESTscore[i] = sc;
                }
                if (sc > hsc) {
                    hsc = sc;
                    BEST = i;   // der is aber besser
                }
                // L2
                for (j = 0; j <= 3; j++) {
                    L = 2;
                    dprintf("%55s %5s => %5s ?\n", "L2", DIRS[i], DIRS[j]);
                    DIRFUNC[j] (&R[i][0], &R[i][1]);
                    if (R[i][0].f != R[i][1].f) {       // actually moved
                        if (DEBUG)
                            dumpFs(&R[i][1]);
                        psc[1] = R[i][1].basis ? R[i][1].basis * 10 - L : 0;
                        sc = psc[0] + psc[1];
                        dprintf("%55s %5s => %5s (%d+%d) score:%d hsc:%d\n", "L2", DIRS[i], DIRS[j], psc[0], psc[1], sc, hsc);
                        if (sc > BESTscore[i]) {
                            BESTscore[i] = sc;
                        }
                        if (sc > hsc) {
                            hsc = sc;
                            BEST = i;   // der is aber besser
                        }
                        // L3
                        for (k = 0; k <= 3; k++) {
                            L = 3;
                            dprintf("%55s %5s => %5s => %5s ?\n", "L3", DIRS[i], DIRS[j], DIRS[k]);
                            DIRFUNC[k] (&R[i][1], &R[i][2]);
                            if (R[i][1].f != R[i][2].f) {       // actually moved
                                if (DEBUG)
                                    dumpFs(&R[i][2]);
                                psc[2] = R[i][2].basis ? R[i][2].basis * 10 - L : 0;
                                sc = psc[0] + psc[1] + psc[2];
                                dprintf("%55s %5s => %5s => %5s (%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], psc[0], psc[1], psc[2], sc, hsc);
                                if (sc > BESTscore[i]) {
                                    BESTscore[i] = sc;
                                }
                                if (sc > hsc) {
                                    hsc = sc;
                                    BEST = i;   // der is aber noch besser
                                }
                                // L4
                                for (l = 0; l <= 3; l++) {
                                    L = 4;
                                    dprintf("%55s %5s => %5s => %5s => %5s ?\n", "L4", DIRS[i], DIRS[j], DIRS[k], DIRS[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        if (DEBUG)
                                            dumpFs(&R[i][3]);
                                        psc[3] = R[i][3].basis ? R[i][3].basis * 10 - L : 0;
                                        sc = psc[0] + psc[1] + psc[2] + psc[3];
                                        dprintf("%55s %5s => %5s => %5s => %5s (%d+%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], DIRS[l], psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                        if (sc > BESTscore[i]) {
                                            BESTscore[i] = sc;
                                        }
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        // L5
                                        for (m = 0; m <= 3; m++) {
                                            L = 5;
                                            dprintf("%55s %5s => %5s => %5s => %5s => %5s ?\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                if (DEBUG)
                                                    dumpFs(&R[i][4]);
                                                psc[4] = R[i][4].basis ? R[i][4].basis * 10 - L : 0;
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                                dprintf("%55s %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                                if (sc > BESTscore[i]) {
                                                    BESTscore[i] = sc;
                                                }
                                                if (sc > hsc) {
                                                    hsc = sc;
                                                    BEST = i;   // der is aber noch viel viel besser
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                dprintf("no need to check %s further\n", DIRS[i]);

            }
        }

  
        // dprintf("tracked. GOOD=%d BEST=%d\n",GOOD,BEST);
        if (GOOD >= 0) {
            if (BEST < 0) {     // ok ok kompromiss
                BEST = GOOD;
            }
            
            uint8_t W[4];
            uint8_t C[4];
            

            
            dprintf("XXX score: ");
            for (c = 0; c <= 3; c++) {
                if ( BESTscore[c] >= 0 ) {
                    dprintf("   %3d",BESTscore[c]);
                }
                else {
                    dprintf("     -");
                }
            }
            dprintf(" \n");

           weightcount(F->f,W,C);
            
            for (c = 0; c <= 3; c++) {
                if (BEST != c && hsc == BESTscore[c]) {
                    dprintf("XXX found [%s] = %d = [%s] XXX\n", DIRS[BEST],hsc, DIRS[c]);
                    if (W[BEST] < W[c]) {
                        dprintf("XXX [%d < %d] so new BEST will be %s XXX\n", W[BEST], W[c],DIRS[c]);
                        BEST = c;
                    }
                }
            }
            dprintf("\n");

            B = &R[BEST][0];
            if (DEBUG || OPT_PLAY) {
                printf("### %d # best move is %s %s scoring %d ### ", itercnt, DIRC[BEST], DIRS[BEST], hsc);
                for (c = 0; c <= 3; c++) {
                    printf(" %s=%d ", DIRS[c], BESTscore[c]);
                }
                printf("\n");


                dumpFs(F);
                dumpFs(B);
            }

            itercnt++;

            F->f = placenew(B->f);
            F->score += R[BEST][0].score;
            F->basis += R[BEST][0].basis;
            F->moves++;
            hsc = 0;            // reset hsc for next move
        } else {
//           printf("full\n");
//            dumpFs(F);
//            printf("Score: %d\n", F->score);
            return;
        }
    }
    return;
}

void mysrand()
{
    FILE *urandom;
    unsigned int seed;

    urandom = fopen("/dev/urandom", "r");
    if (urandom == NULL) {
        fprintf(stderr, "Cannot open /dev/urandom!\n");
        exit(1);
    }
    fread(&seed, sizeof(seed), 1, urandom);
    fclose(urandom);
    srand(seed);
}

int main(int argc, char **argv)
{

    int opt;
    int OPT_SEED;
    int OPT_URND;
    int OPT_RITER;
    int OPT_ITER;
    int OPT_STRT;
    int OPT_STAT;
    int OPT_QUIET;

    void (*STRATEGYFUNC[]) (struct F_st *) = {
    &strat_hiscorer, &strat_dlhiscorer, &strat_dl2hiscorer,
    &strat_backtracker, &strat_backtracker2, &strat_backtracker3,
    &strat_backtracker3b, &strat_backtracker3bo, &strat_backtracker3bo2,
    &strat_backtracker4, &strat_backtracker4v2, &strat_backtracker4v3, &strat_backtracker4v4, &strat_backtracker4v5,
    &strat_backtracker5, &strat_backtracker5v2, &strat_backtracker4v6 };

    static char *STRATEGYNAME[] = {
        "hiscorer", "dlhiscorer", "dl2hiscorer",
        "bt depth 1", "bt depth 2",
        "bt depth 3", "base bt 3", "base bt3 ordered", "base bt3 ord v2",
        "base bt4", "base bt4 v2", "base bt4 v3", "base bt4 v4", "base bt4 v5", "base bt5", "base bt5 v2", "base bt4 v6"
    };

    int i = 0;

    OPT_BASE = 0;
    OPT_SEED = 1;
    OPT_ITER = 1500;
    OPT_URND = 0;
    OPT_STRT = 0;
    OPT_STAT = 0;
    OPT_QUIET = 0;
    OPT_RITER = 0;
    OPT_DEBUG = 0;              // static !

    while ((opt = getopt(argc, argv, "dbur:s:R:SQpi:")) != -1) {
        switch (opt) {
        case 'b':
            OPT_BASE = 1;
            XSCORE = X1SCORE;
            break;
        case 'd':
            OPT_DEBUG = 1;
            break;
        case 'u':
            OPT_URND = 1;
            break;
        case 'r':
            OPT_SEED = atoi(optarg);
            break;
        case 'R':
            OPT_RITER = atoi(optarg);
            break;
        case 's':
            OPT_STRT = atoi(optarg);
            break;
        case 'S':
            OPT_STAT = 1;
            break;
        case 'p':
            OPT_PLAY = 1;
            break;
        case 'Q':
            OPT_QUIET = 1;
            break;
        case 'i':
            OPT_ITER = atoi(optarg);
            break;
        default:               /* '?' */
            fprintf(stderr, "Usage: %s [options]\n", argv[0]);
            fprintf(stderr, "Options:\n");
            fprintf(stderr, " -s <idx>    strategy to follow\n");
            fprintf(stderr, " -i <num>    num iterations\n");
            fprintf(stderr, " -S          statistics\n");
            fprintf(stderr, " -Q          quiet (for benchmarking use)\n");
            fprintf(stderr, " -u          seed from /dev/urandom\n");
            fprintf(stderr, " -r <num>    seed to feed (default 1)\n");
            fprintf(stderr, " -R <num>    startvalue incremental random seed\n");
            fprintf(stderr, " -b          output b instead of 2^b\n");
            fprintf(stderr, " -p          play single stepping all moves\n");
            
            for (i = 0; i < sizeof(STRATEGYFUNC) / sizeof(STRATEGYFUNC[0]); i++) {
                fprintf(stderr, "     %d: %-s\n", i, STRATEGYNAME[i]);
            }
            fprintf(stderr, "\n");
            exit(EXIT_FAILURE);
        }
        // fprintf(stderr,"optind=%d;\n",optind);
    }

    fprintf(stderr, "lets follow strategy #%d \"%s\"\n", OPT_STRT, STRATEGYNAME[OPT_STRT]);

//printf("name argument = %s\n", argv[optind]);

    if (!OPT_RITER) {
        if (OPT_URND) {
            mysrand();
        } else {
            srand(OPT_SEED);
        }
    }
    setupMAP();
//  dumpMAP();
//  exit(0);

    //struct F_st TOP;
    //struct F_st LOW;
    struct F_st TEMP;

    struct Stats_st TOPstats;
    struct Stats_st LOWstats;

    unsigned int AVGscore = 0;
    unsigned int AVGmoves = 0;
    unsigned int TOTmoves = 0;

    int riter = OPT_RITER;

    clock_t tstart, tend;

    //init_F(&TOP);
    //init_F(&LOW);

    init_F(&TOPstats.f);
    init_F(&LOWstats.f);

    //LOW.score = INT_MAX;

    LOWstats.f.score = INT_MAX;


    tstart = clock();

    for (i = OPT_ITER; i > 0; i--) {
        if (OPT_RITER) {
            srand(riter);
        }
        init_F(&TEMP);
        TEMP.f = placenew(0);
//        TEMP.f = 0x0000000000001212;

        //
//printf("XXX %d XXX\n",i);
        STRATEGYFUNC[OPT_STRT] (&TEMP);
//printf("XXX %d XXX\n",i);
        //
        //

        if (!OPT_QUIET) {
            dumpCFs(&TEMP);
        }
        //       if (TEMP.score > TOP.score) {
        //           copy_F(&TEMP, &TOP);
        //       }
        //       if (TEMP.score < LOW.score) {
        //           copy_F(&TEMP, &LOW);
        //       }

        if (TEMP.score > TOPstats.f.score) {
            copy_F(&TEMP, &TOPstats.f);
            TOPstats.rseed = riter;
        }
        if (TEMP.score < LOWstats.f.score) {
            copy_F(&TEMP, &LOWstats.f);
            LOWstats.rseed = riter;
        }
        AVGscore += TEMP.score;
        TOTmoves += TEMP.moves;
        riter++;
    }

    tend = clock();


    if (OPT_STAT) {
        AVGscore = AVGscore / OPT_ITER;
        AVGmoves = TOTmoves / OPT_ITER;
        float telapsed = (tend - tstart) / (float) CLOCKS_PER_SEC;
        fprintf(stderr, "Stats out of %d iterations in %4.2fs (%.0f/s). %d moves done (%5.0f/s)\n", OPT_ITER, telapsed, ((float) OPT_ITER / (float) telapsed), TOTmoves, (float) TOTmoves / (float) telapsed);
        fprintf(stderr, " %'d moves done (%5.0f/s)\n", TOTmoves, (float) TOTmoves / (float) telapsed);
        fprintf(stderr, " %'ld tries done (%5.0f/s)\n", TOTALTRIES, (float) TOTALTRIES / (float) telapsed);
        fprintf(stderr, "       %6s | %6s | %6s\n", "LOW", "AVG", "TOP");
        fprintf(stderr, "score: %6d | %6d | %6d \n", LOWstats.f.score, AVGscore, TOPstats.f.score);
        fprintf(stderr, "moves: %6d | %6d | %6d \n", LOWstats.f.moves, AVGmoves, TOPstats.f.moves);
        if (OPT_RITER)
            fprintf(stderr, "seed: %6d |        | %6d \n", LOWstats.rseed, TOPstats.rseed);
        dumpFs(&LOWstats.f); // spam on STDOUT
        dumpFs(&TOPstats.f); // dito
    }

    exit(0);
}

