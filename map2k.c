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
    int basis;                  // the score
    unsigned char dir;          // the direction which made me
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
    F->dir = 0;
    F->moves = 0;
}

void copy_F(struct F_st *F, struct F_st *T)
{
    T->f = F->f;
    T->score = F->score;
    T->basis = F->basis;
    T->dir = F->dir;
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

                    SCORE[rcx] = SCORE[rc];
                    BASIS[rcx] = BASIS[rc];

                }
            }
        }
    }
}

void pi2m(uint16_t x, char *tr)
{

    int y,t;

    for (y = 3; y >= 0; y--) {
        printf("%3d", x >> 4 * y & 0xf);
    }
    printf(" : ");
    for (y = 3; y >= 0; y--) {
       t = XSCORE[x >> 4 * y & 0xf];
       if (t) {
           printf("%6d", t);
       }
       else {
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

        printf("%5d: %16lx %16x %16lx ", SCORE[rc], MAP_L[rc], rc, MAP_R[rc]);
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
    ts->dir = DIR_LEFT;
    return shift_LRs(fs, ts, MAP_L);
}

uint64_t shift_Rs(struct F_st * fs, struct F_st * ts)
{
    if (DEBUG)
        printf("shift_R(%016lx) = ", fs->f);
    ts->dir = DIR_RIGHT;
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
    ts->dir = DIR_DOWN;
    return shift_UDs(fs, ts, MAP_L);
}

uint64_t shift_Us(struct F_st * fs, struct F_st * ts)
{
    if (DEBUG)
        printf("shift_U(%016lx) = ", fs->f);
    ts->dir = DIR_UP;
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

        T = &R[DIR_UP];
        T->f = F->f;
        T->dir = 0;
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
        }

        T = &R[DIR_RIGHT];
        right(F, T);
        if (F->f != T->f) {
            if (T->score >= hsc) {
                hsc = T->score;
                B = T;
                moved++;
            }
            mv++;
        }

        T = &R[DIR_LEFT];
        left(F, T);
        if (F->f != T->f) {
            if (T->score >= hsc) {
                hsc = T->score;
                B = T;
                moved++;
            }
            mv++;
        }

        T = &R[DIR_DOWN];
        down(F, T);
        if (F->f != T->f) {
            if (T->score >= hsc) {
                hsc = T->score;
                B = T;
                moved++;
            }
            mv++;
        }

        if (mv) {
            score += hsc;
            if (DEBUG)
                printf("shifted %s\n", DIRS[B->dir]);
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
                mv++;
            }

        }
        if (DEBUG)
            printf("\n decided %5s would be cool\n", DIRS[j]);

        if (mv) {
            SCORE += B->score;
            if (DEBUG)
                printf("shifted %s\n", DIRS[B->dir]);
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
        if (decision == DEC_STRAT) {
            dprintf(" next STRAT[%d]=%s", stratidx, DIRS[STRAT[stratidx]]);
        }
        dprintf("\n");
        if (mv) {
            SCORE += B->score;
            dprintf("shifted %s\n", DIRS[B->dir]);
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
    struct F_st R[4][BT4DEPTH];        // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];         // partieller score
    int T = BT4DEPTH - 1;      // maximale Tiefe
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

void strat_backtracker4v2(struct F_st *F) // 3bo2 mit tiefe 4 prioscore nur bei score
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
    struct F_st R[4][BT4DEPTH];        // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = {
    &up, &right, &down, &left};
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];         // partieller score
    int T = BT4DEPTH - 1;      // maximale Tiefe
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

void strat_backtracker4v3(struct F_st *F) // 3bo2 mit tiefe 4 prioscore nur bei score
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
    struct F_st R[4][BT4DEPTH];        // alle results
    uint64_t(*DIRFUNC[])(struct F_st * f, struct F_st * t) = { &up, &right, &down, &left};
    int c = 0; // multipurpose counter
    int i = 0;
    int j = 0;
    int k = 0;
    int l = 0;
    int m = 0;
    int hsc = 0;
    int sc = 0;
    int psc[BT4DEPTH];         // partieller score
    int T = BT4DEPTH - 1;      // maximale Tiefe
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
                dprintf("%55s %5s (%d) score:%d hsc:%d\n","L1", DIRS[i], psc[0], sc , hsc);
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
//                        psc[2] = R[i][1].basis ? R[i][1].basis * 10 + (T - 2) : 0;
                        sc = psc[0] + psc[1];
//                        dprintf("R[%d][0].basis*F0 + R[%d][1].basis*F1 (%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], sc, hsc);
//                        dprintf("R[%d][0].basis*F0 + R[%d][1].basis*F1 (%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], sc, hsc);
                        dprintf("%55s %5s => %5s (%d+%d) score:%d hsc:%d\n", "L2", DIRS[i], DIRS[j], psc[0],psc[1],sc,hsc);
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
                                // dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis (%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], sc, hsc);
                                dprintf("%55s %5s => %5s => %5s (%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], psc[0],psc[1],psc[2],sc,hsc);
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
//                                    dprintf("%55s %s %5s %s %5s %s %5s %s\n", DIRS[i], DIRC[i], DIRS[j], DIRC[j], DIRS[k], DIRC[k], DIRS[l], DIRC[l]);
                                    DIRFUNC[l] (&R[i][2], &R[i][3]);
                                    if (R[i][2].f != R[i][3].f) {       // actually moved
                                        if (DEBUG)
                                            dumpFs(&R[i][3]);
                                        psc[3] = R[i][3].basis ? R[i][3].basis * 10 + (T - 3) : 0;
                                        sc = psc[0] + psc[1] + psc[2] + psc[3];
//                                        dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], psc[4], sc, hsc);
                                        dprintf("%55s %5s => %5s => %5s => %5s (%d+%d+%d+%d) score:%d hsc:%d\n", "L3", DIRS[i], DIRS[j], DIRS[k], DIRS[l], psc[0],psc[1],psc[2],psc[3],sc,hsc);
                                        if (sc > BESTscore[i]) {
						BESTscore[i] = sc;
					}
                                        if (sc > hsc) {
                                            hsc = sc;
                                            BEST = i;   // der is aber noch viel besser
                                        }
                                        
                                        // L5
                                        for (m = 0; m <= 3; m++) {
                                            //dprintf("%55s %s %5s %s %5s %s %5s %s %5s %s\n", DIRS[i], DIRC[i], DIRS[j], DIRC[j], DIRS[k], DIRC[k], DIRS[l], DIRC[l], DIRS[m], DIRC[m]);
				            dprintf("%55s %5s => %5s => %5s => %5s => %5s\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m]);
                                            DIRFUNC[m] (&R[i][3], &R[i][4]);
                                            if (R[i][3].f != R[i][4].f) {       // actually moved
                                                if (DEBUG)
                                                    dumpFs(&R[i][4]);
                                                psc[4] = R[i][4].basis ? R[i][4].basis * 10 + (T - 4) : 0;
                                                sc = psc[0] + psc[1] + psc[2] + psc[3] + psc[4];
                                                //dprintf("R[%d][0].basis + R[%d][1].basis + R[i][2].basis + R[i][3].basis (%d+%d+%d+%d+%d+%d) = %d > %d (hsc)\n", i, i, psc[0], psc[1], psc[2], psc[3], psc[4], psc[5], sc, hsc);
                                                dprintf("%55s %5s => %5s => %5s => %5s => %5s (%d+%d+%d+%d+%d) score:%d hsc:%d\n", "L5", DIRS[i], DIRS[j], DIRS[k], DIRS[l], DIRS[m], psc[0],psc[1],psc[2],psc[3],psc[4],sc,hsc);
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
	    
	    for (c=0;c<=3;c++) {
	        if (BEST !=c && hsc == BESTscore[c]) {
                    dprintf ("XXX found [%s] as good as [%s] XXX\n",DIRS[BEST],DIRS[c]);
                    	    
	        }
	    }
	    dprintf("\n");
	    

            if (DEBUG || OPT_PLAY) {
                printf("### best move is %s %s scoring %d ### ", DIRC[BEST], DIRS[BEST], hsc);
		for (c=0;c<=3;c++) {
		  printf(" %s=%d ",DIRS[c],BESTscore[c]);
		}
		printf("\n");
		
		
                dumpFs(F);
                dumpFs(B);
            }

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
    &strat_backtracker, &strat_backtracker2,
    &strat_backtracker3, &strat_backtracker3b, &strat_backtracker3bo, &strat_backtracker3bo2,
    &strat_backtracker4, &strat_backtracker4v2,  &strat_backtracker4v3
    
    };

    static char *STRATEGYNAME[] = {
        "hiscorer", "dlhiscorer", "dl2hiscorer",
        "bt depth 1", "bt depth 2",
        "bt depth 3", "base bt 3", "base bt3 ordered", "base bt3 ord v2",
        "base bt4", "base bt4 v2", "base bt4 v3"
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
            fprintf(stderr, " -S          statistics\n");
            fprintf(stderr, " -R <num>    startvalue iterating random seed\n");
            fprintf(stderr, " -Q          quiet\n");
            fprintf(stderr, " -b          output b instead of 2^b\n");
            fprintf(stderr, " -r <num>    seed to feed\n");
            fprintf(stderr, " -u          seed from /dev/urandom\n");
            fprintf(stderr, " -i <num>    num iterations\n");
            fprintf(stderr, " -s <idx>    strategy to follow\n");
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
// dumpMAP();  


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
        dumpFs(&LOWstats.f);
        dumpFs(&TOPstats.f);
    }

    exit(0);
}

/*

# -r 84577 :  score 520/67
# 
# ./map2k -s 5 -i 1 -S -Q -r 14246984 : 208/42

# https://chessprogramming.wikispaces.com/Flipping+Mirroring+and+Rotating

# cp /home/wwwutz/2048/map2k /scratch/cluster/wwwutz/; for i in `seq -w 1 1500`; do mxq_submit --stdout=/scratch/cluster/wwwutz/job.stdout.$i  --threads=1 --memory=20 --time=5 /scratch/cluster/wwwutz/map2k; done
# sort -n  job.stdout* > /home/wwwutz/2048/stats.highscorer

*/
