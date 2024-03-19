#if 0
gcc -s -O2 -Wno-unused-result -fwrapv main.c `sdl-config --cflags --libs`
exit
#endif

// This file is public domain.

#include "SDL.h"
#include <dirent.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static const Uint8 fontdata[]={ // font for built-in menus
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x30,0x78,0x78,0x30,0x30,0x00,0x30,0x00,
  0x6C,0x6C,0x6C,0x00,0x00,0x00,0x00,0x00,0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00,
  0x30,0x7C,0xC0,0x78,0x0C,0xF8,0x30,0x00,0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00,
  0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00,0x60,0x60,0xC0,0x00,0x00,0x00,0x00,0x00,
  0x18,0x30,0x60,0x60,0x60,0x30,0x18,0x00,0x60,0x30,0x18,0x18,0x18,0x30,0x60,0x00,
  0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00,0x00,0x30,0x30,0xFC,0x30,0x30,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x60,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x30,0x30,0x00,0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00,
  0x7C,0xC6,0xCE,0xDE,0xF6,0xE6,0x7C,0x00,0x30,0x70,0x30,0x30,0x30,0x30,0xFC,0x00,
  0x78,0xCC,0x0C,0x38,0x60,0xCC,0xFC,0x00,0x78,0xCC,0x0C,0x38,0x0C,0xCC,0x78,0x00,
  0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00,0xFC,0xC0,0xF8,0x0C,0x0C,0xCC,0x78,0x00,
  0x38,0x60,0xC0,0xF8,0xCC,0xCC,0x78,0x00,0xFC,0xCC,0x0C,0x18,0x30,0x30,0x30,0x00,
  0x78,0xCC,0xCC,0x78,0xCC,0xCC,0x78,0x00,0x78,0xCC,0xCC,0x7C,0x0C,0x18,0x70,0x00,
  0x00,0x30,0x30,0x00,0x00,0x30,0x30,0x00,0x00,0x30,0x30,0x00,0x00,0x30,0x30,0x60,
  0x18,0x30,0x60,0xC0,0x60,0x30,0x18,0x00,0x00,0x00,0xFC,0x00,0x00,0xFC,0x00,0x00,
  0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00,0x78,0xCC,0x0C,0x18,0x30,0x00,0x30,0x00,
  0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x78,0x00,0x30,0x78,0xCC,0xCC,0xFC,0xCC,0xCC,0x00,
  0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00,0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00,
  0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00,0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00,
  0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00,0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3E,0x00,
  0xCC,0xCC,0xCC,0xFC,0xCC,0xCC,0xCC,0x00,0x78,0x30,0x30,0x30,0x30,0x30,0x78,0x00,
  0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00,0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00,
  0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00,0xC6,0xEE,0xFE,0xFE,0xD6,0xC6,0xC6,0x00,
  0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00,0x38,0x6C,0xC6,0xC6,0xC6,0x6C,0x38,0x00,
  0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00,0x78,0xCC,0xCC,0xCC,0xDC,0x78,0x1C,0x00,
  0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00,0x78,0xCC,0xE0,0x70,0x1C,0xCC,0x78,0x00,
  0xFC,0xB4,0x30,0x30,0x30,0x30,0x78,0x00,0xCC,0xCC,0xCC,0xCC,0xCC,0xCC,0xFC,0x00,
  0xCC,0xCC,0xCC,0xCC,0xCC,0x78,0x30,0x00,0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00,
  0xC6,0xC6,0x6C,0x38,0x38,0x6C,0xC6,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x30,0x78,0x00,
  0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00,0x78,0x60,0x60,0x60,0x60,0x60,0x78,0x00,
  0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00,0x78,0x18,0x18,0x18,0x18,0x18,0x78,0x00,
  0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,
  0x30,0x30,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00,
  0xE0,0x60,0x60,0x7C,0x66,0x66,0xDC,0x00,0x00,0x00,0x78,0xCC,0xC0,0xCC,0x78,0x00,
  0x1C,0x0C,0x0C,0x7C,0xCC,0xCC,0x76,0x00,0x00,0x00,0x78,0xCC,0xFC,0xC0,0x78,0x00,
  0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00,0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8,
  0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00,0x30,0x00,0x70,0x30,0x30,0x30,0x78,0x00,
  0x0C,0x00,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00,
  0x70,0x30,0x30,0x30,0x30,0x30,0x78,0x00,0x00,0x00,0xCC,0xFE,0xFE,0xD6,0xC6,0x00,
  0x00,0x00,0xF8,0xCC,0xCC,0xCC,0xCC,0x00,0x00,0x00,0x78,0xCC,0xCC,0xCC,0x78,0x00,
  0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0,0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E,
  0x00,0x00,0xDC,0x76,0x66,0x60,0xF0,0x00,0x00,0x00,0x7C,0xC0,0x78,0x0C,0xF8,0x00,
  0x10,0x30,0x7C,0x30,0x30,0x34,0x18,0x00,0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00,
  0x00,0x00,0xCC,0xCC,0xCC,0x78,0x30,0x00,0x00,0x00,0xC6,0xD6,0xFE,0xFE,0x6C,0x00,
  0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00,0x00,0x00,0xCC,0xCC,0xCC,0x7C,0x0C,0xF8,
  0x00,0x00,0xFC,0x98,0x30,0x64,0xFC,0x00,0x1C,0x30,0x30,0xE0,0x30,0x30,0x1C,0x00,
  0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00,0xE0,0x30,0x30,0x1C,0x30,0x30,0xE0,0x00,
  0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,
};

#define EXPANDED_MEMORY 8
#define MEMSIZE ((EXPANDED_MEMORY+1)*0x10000)

#define GET16(p) (((p)[0]<<8)|(p)[1])
#define PUT16(p,q) ((p)[0]=(q)>>8,(p)[1]=(q))

typedef struct {
  Uint8 p;
  Uint8 d[256];
} Stack;

typedef struct Device {
  Uint8 d[16];
  Uint8(*in)(struct Device*,Uint8);
  void(*out)(struct Device*,Uint8);
  void(*exp)(struct Device*,Uint16);
  void*aux;
} Device;

typedef struct {
  FILE*file;
  DIR*dir;
  char name[4096];
  struct dirent*de;
  Uint8 mode;
} UxnFile;

typedef struct {
  Uint32 envtime,phase,freq;
  Uint32 length,end,pos;
  Uint32 env[4];
  Uint8 vol[2];
} UxnAudio;

typedef struct {
  Uint8*data;
  Uint32 addr;
  Uint16 len;
  Uint8 kind,stage;
} Script;

static SDL_Color colors[64]={
  // 0
  {0xFF,0xFF,0xFF,0},
  {0x00,0x00,0x00,0},
  {0x00,0xFF,0xFF,0},
  {0xFF,0x00,0x00,0},
  {0xAA,0xAA,0xAA,0},
  {0x43,0x43,0x33,0},
  {0x43,0xAA,0xAA,0},
  {0xAA,0x43,0x33,0},
  // 1
  {0x00,0x00,0x00,0},
  {0xFF,0xFF,0xFF,0},
  {0x00,0xFF,0xFF,0},
  {0xFF,0x00,0x00,0},
  {0x43,0x43,0x33,0},
  {0xAA,0xAA,0xAA,0},
  {0x43,0xAA,0xAA,0},
  {0xAA,0x43,0x33,0},
  // 2
  {0x10,0x30,0x10,0},
  {0x60,0xE0,0x60,0},
  {0x60,0x60,0xE0,0},
  {0xE0,0x60,0x60,0},
  {0x10,0x80,0x10,0},
  {0x38,0xFF,0xF8,0},
  {0xF8,0x38,0xFF,0},
  {0xFF,0xF8,0x38,0},
  // 3
  {0x55,0x55,0x55,0},
  {0xFF,0x55,0x00,0},
  {0x00,0x00,0x00,0},
  {0xFF,0xFF,0xFF,0},
  {0x55,0x55,0x55,0},
  {0xFF,0x55,0x00,0},
  {0x00,0x00,0x00,0},
  {0xFF,0xFF,0xFF,0},
  // 4
  {0x55,0x55,0x55,0},
  {0xFF,0x55,0x00,0},
  {0xFF,0xFF,0xFF,0},
  {0x00,0x00,0x00,0},
  {0x55,0x55,0x55,0},
  {0xFF,0x55,0x00,0},
  {0xFF,0xFF,0xFF,0},
  {0x00,0x00,0x00,0},
  // 5
  {0xFF,0xFF,0xFF,0},
  {0xAA,0xAA,0xAA,0},
  {0x55,0x55,0x55,0},
  {0x00,0x00,0x00,0},
  {0xFF,0x80,0x00,0},
  {0xC6,0x60,0x00,0},
  {0x94,0x40,0x00,0},
  {0x72,0x20,0x00,0},
  // 6
  {0xAA,0xAA,0xAA,0},
  {0x00,0x00,0x00,0},
  {0xAA,0xAA,0xAA,0},
  {0x00,0x00,0x00,0},
  {0x91,0xAA,0x9A,0},
  {0x0D,0x0E,0x0E,0},
  {0x91,0xAA,0x9A,0},
  {0x19,0x15,0x00,0},
  // 7
  {0xFF,0xFF,0xFF,0},
  {0x00,0x00,0x00,0},
  {0x77,0xCC,0xAA,0},
  {0x99,0x22,0x44,0},
  {0xFF,0xFF,0xFF,0},
  {0x00,0x00,0x00,0},
  {0x77,0xCC,0xAA,0},
  {0x99,0x22,0x44,0},
};

static Uint8 blend[4][16]={
  {4,0,0,0,1,4,1,1,2,2,4,2,3,3,3,4},
  {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3},
  {1,2,3,1,1,2,3,1,1,2,3,1,1,2,3,1},
  {2,3,1,2,2,3,1,2,2,3,1,2,2,3,1,2},
};

static const Uint32 notes[12]={
  0x20000000,0x21e71f26,0x23eb3588,0x260dfc14,0x285145f3,0x2ab70212,
  0x2d413ccd,0x2ff221af,0x32cbfd4a,0x35d13f33,0x39047c0f,0x3c686fce,
};

static Uint8 mem[MEMSIZE];
static Stack ds,rs;
static Device device[16];
static Uint16 scr_w,scr_h;
static Uint8*bglayer;
static Uint8*fglayer;
static UxnFile uxnfile0;
static UxnFile uxnfile1;
static FILE*inf;
static FILE*outf;

static const char*rom_name;
static Uint8 use_console=1;
static Uint8 use_screen=1;
static Uint8 use_mouse=1;
static Uint8 use_debug=0;
static Uint8 use_utc=0;
static Uint8 use_extension=0;
static Uint8 use_thread=0;
static Uint8 hide_cursor=0;
static Uint8 allow_write=0;
static Uint8 joypad_repeat=0;
static Uint16 scroll_size=1;
static Uint8 zoom=1;
static Uint8 disallow_size_change=0;
static Sint16 default_width=512;
static Sint16 default_height=320;
static Uint32 scrflags=SDL_SWSURFACE;
static Uint16 timer_rate=16;
static Uint8 paused=0;
static Uint8 picture_changed=0;
static Uint8 size_changed=0;
static volatile Uint8 timer_expired=0;
static SDL_Surface*screen;
static Uint8 layers;
static Uint8 palet=7;
static Uint8 touch_mode=0;
static SDL_cond*stdin_cond;
static Uint8 screencompat=0x03;

static Uint16 scriptc;
static Script*scriptv;
static FILE*scriptf;

static Sint32*audio_option;
static SDL_AudioSpec audiospec;

static Uint8 joy_mask;
static Uint8 joy_player[4];
static Uint8 joy_hat,joy_hat_mode;
static Uint8 joy_axis_x,joy_axis_y,joy_axis_mode;
static Uint8 joy_button_map[128];
static Uint16 joy_sensitivity=8192;

static Uint8 debugger=0;
static Uint16 breakpoint=0;
static Uint16 break_pc=0x0100;
static Uint8 debug_op=0;

static void debug(int i) {
  int j;
  if(i) fprintf(stderr,"Debug(%02X)\n",i);
  fprintf(stderr,"ds:");
  for(i=0;i<ds.p;i++) fprintf(stderr," %02X",ds.d[i]);
  fputc('\n',stderr);
  fprintf(stderr,"rs:");
  for(i=0;i<rs.p;i++) fprintf(stderr," %02X",rs.d[i]);
  fputc('\n',stderr);
  fprintf(stderr,"   0. 1. 2. 3. 4. 5. 6. 7. 8. 9. A. B. C. D. E. F.\n");
  for(i=0;i<16;i++) {
    fprintf(stderr,"%1X:",i);
    for(j=0;j<16;j++) fprintf(stderr," %02X",device[i].d[j]);
    if(device[i].aux) fprintf(stderr," *");
    fputc('\n',stderr);
  }
}

static void uxnerr(int n,int pc) {
  if(use_debug) debug(-1);
  fflush(outf);
  fflush(stderr);
  errx(2,"Uxn error %d at 0x%04X",n,pc);
}

#define Push8(z) ({ s->d[s->p++]=(z); })
#define Push16(z) ({ s->d[s->p++]=(z)>>8; s->d[s->p++]=(z); })
#define Push(z) ({ if(v&0x20) s->d[s->p++]=(z)>>8; s->d[s->p++]=(z); })
#define Pop8(z) ({ z=s->d[0xFF&--k]; if(v<0x80) s->p=k; })
#define Pop16(z) ({ z=s->d[0xFF&--k]; z|=s->d[0xFF&--k]<<8; if(v<0x80) s->p=k; })
#define Pop(z) ({ if(v&0x20) Pop16(z); else Pop8(z); })

static void run(Uint16 pc) {
  Stack*s;
  Uint8 k,v;
  Uint16 x,y,z;
  if(!pc) return;
  start:
  while(v=mem[pc++]) {
    if(pc==breakpoint && use_screen) goto stop;
    s=(v&0x40?&rs:&ds);
    k=s->p;
    switch(v&0x1F?:v) {
      case 0x01: Pop(x); Push(x+1); break;
      case 0x02: Pop(x); break;
      case 0x03: Pop(x); Pop(y); Push(x); break;
      case 0x04: Pop(x); Pop(y); Push(x); Push(y); break;
      case 0x05: Pop(z); Pop(y); Pop(x); Push(y); Push(z); Push(x); break;
      case 0x06: Pop(x); Push(x); Push(x); break;
      case 0x07: Pop(x); Pop(y); Push(y); Push(x); Push(y); break;
      case 0x08: Pop(x); Pop(y); Push8(y==x?1:0); break;
      case 0x09: Pop(x); Pop(y); Push8(y!=x?1:0); break;
      case 0x0A: Pop(x); Pop(y); Push8(y>x?1:0); break;
      case 0x0B: Pop(x); Pop(y); Push8(y<x?1:0); break;
      case 0x0C: Pop(x); jump: if(v&0x20) pc=x; else pc+=(Sint8)x; break;
      case 0x0D: Pop(x); Pop8(y); if(y) goto jump; break;
      case 0x0E: Pop(x); y=pc; goto stash;
      case 0x0F: Pop(x); goto stash;
      case 0x10: Pop8(x); peek: if(v&0x20) x=GET16(mem+x); else x=mem[x]; Push(x); break;
      case 0x11: Pop8(x); Pop(y); poke: if(v&0x20) PUT16(mem+x,y); else mem[x]=y; break;
      case 0x12: Pop8(x); x=pc+(Sint8)x; goto peek; break;
      case 0x13: Pop8(x); Pop(y); x=pc+(Sint8)x; goto poke; break;
      case 0x14: Pop16(x); goto peek; break;
      case 0x15: Pop16(x); Pop(y); goto poke; break;
      case 0x16:
        Pop8(x);
        z=(x>>4)&15; x&=15;
        y=device[z].in(device+z,x);
        if(v&0x20) y=(y<<8)|device[z].in(device+z,x+1);
        Push(y);
        break;
      case 0x17:
        Pop8(x); Pop(y);
        z=(x>>4)&15; x&=15;
        if(v&0x20) {
          device[z].d[x]=y>>8;
          device[z].d[(x+1)&15]=y;
          device[z].out(device+z,x);
          device[z].out(device+z,x+1);
        } else {
          device[z].d[x]=y;
          device[z].out(device+z,x);
        }
        break;
      case 0x18: Pop(x); Pop(y); Push(y+x); break;
      case 0x19: Pop(x); Pop(y); Push(y-x); break;
      case 0x1A: Pop(x); Pop(y); Push(y*x); break;
      case 0x1B: Pop(x); Pop(y); Push(x?y/x:0); break;
      case 0x1C: Pop(x); Pop(y); Push(y&x); break;
      case 0x1D: Pop(x); Pop(y); Push(y|x); break;
      case 0x1E: Pop(x); Pop(y); Push(y^x); break;
      case 0x1F: Pop8(x); Pop(y); z=x>>4; x&=15; Push((y>>x)<<z); break;
      case 0x20: Pop8(x); pc+=x?GET16(mem+pc)+2:2; break;
      case 0x40: pc+=GET16(mem+pc)+2; break;
      case 0x60: Push16(pc+2); pc+=GET16(mem+pc)+2; break;
      case 0x80: case 0xC0: Push8(mem[pc++]); break;
      case 0xA0: case 0xE0: Push8(mem[pc++]); Push8(mem[pc++]); break;
    }
    continue;
    stash:
    s=(v&0x40?&ds:&rs);
    if(v&1) {
      Push(x);
    } else {
      Push16(y);
      if(v&0x20) pc=x; else pc+=(Sint8)x;
    }
  }
  if(pc==breakpoint && use_screen) {
    stop:
    paused=debugger=picture_changed=1;
    break_pc=pc-1;
  }
  fflush(outf);
}

static void load_rom(void) {
  FILE*fp=fopen(rom_name,"r");
  if(!fp) err(1,"Cannot load program file");
  memset(mem,0,MEMSIZE);
  fread(mem+0x0100,1,MEMSIZE-0x0100,fp);
  if(ferror(fp)) err(1,"Cannot load program file");
  fclose(fp);
}

static Uint8 default_in(Device*misc1,Uint8 misc2) { return misc1->d[misc2&15]; }
static void default_out(Device*misc1,Uint8 misc2) {}
static void default_exp(Device*misc1,Uint16 misc2) {}

static Uint8 system_in(Device*dev,Uint8 id) {
  switch(id) {
    case 4: return ds.p;
    case 5: return rs.p;
    default: return dev->d[id];
  }
}

static void system_out(Device*dev,Uint8 id) {
  Uint16 x;
  switch(id) {
    case 3:
      x=GET16(dev->d+2);
      device[mem[x]>>4].exp(device+(mem[x]>>4),x);
      break;
    case 4:
      ds.p=dev->d[4];
      break;
    case 5:
      rs.p=dev->d[5];
      break;
    case 8 ... 13:
      colors[070].r=colors[074].r=(dev->d[8]>>4)*0x11;
      colors[071].r=colors[075].r=(dev->d[8]&15)*0x11;
      colors[072].r=colors[076].r=(dev->d[9]>>4)*0x11;
      colors[073].r=colors[077].r=(dev->d[9]&15)*0x11;
      colors[070].g=colors[074].g=(dev->d[10]>>4)*0x11;
      colors[071].g=colors[075].g=(dev->d[10]&15)*0x11;
      colors[072].g=colors[076].g=(dev->d[11]>>4)*0x11;
      colors[073].g=colors[077].g=(dev->d[11]&15)*0x11;
      colors[070].b=colors[074].b=(dev->d[12]>>4)*0x11;
      colors[071].b=colors[075].b=(dev->d[12]&15)*0x11;
      colors[072].b=colors[076].b=(dev->d[13]>>4)*0x11;
      colors[073].b=colors[077].b=(dev->d[13]&15)*0x11;
      if(screen && palet==7) {
        SDL_SetColors(screen,colors+palet*8,0,8);
        picture_changed=1;
      }
      break;
    case 14: if(dev->d[14] && use_debug) debug(dev->d[14]); break;
    case 15: if(dev->d[15]) exit(dev->d[15]&0x7F); break;
  }
}

static void console_out(Device*dev,Uint8 id) {
  switch(id) {
    case 8: fputc(dev->d[8],outf); break;
    case 9: fputc(dev->d[9],stderr); break;
  }
}

static Uint16 files_stat(int fd,char*name,Uint16 len,Uint8*out) {
  struct stat st;
  if(strlen(name)+7>len || fstatat(fd,name,&st,0)) return 0;
  if(S_ISDIR(st.st_mode)) return snprintf(out,len,"---- %s\n",name);
  else if(st.st_size<0x10000) return snprintf(out,len,"%04x %s\n",(unsigned int)st.st_size,name);
  else return snprintf(out,len,"???? %s\n",name);
  return 0;
}

static Uint32 files_seek(UxnFile*uf,Uint8 mode,Uint32 len) {
  switch(uf->mode) {
    case 1: case 2: case 4:
      switch(mode) {
        case 0: fseek(uf->file,len,SEEK_SET); break;
        case 1: fseek(uf->file,(Sint32)len,SEEK_CUR); break;
        case 2: fseek(uf->file,(Sint32)len,SEEK_END); break;
        case 3: return ftell(uf->file);
      }
      break;
    case 3:
      if(!mode && !len) rewinddir(uf->dir);
      uf->de=0;
      break;
  }
  return 0;
}

static int files_set(Device*dev,int rw) {
  // rw: 0(close) 1(read) 2(write) 3(both)
  // return: 1 if file opened, 0 if not opened
  Uint8 m=dev->d[7];
  UxnFile*uf=dev->aux;
  if(uf->file) fclose(uf->file);
  if(uf->dir) closedir(uf->dir);
  uf->file=0;
  uf->dir=0;
  uf->de=0;
  uf->mode=0;
  dev->d[2]=dev->d[3]=0;
  if(!uf->name[0]) return 0;
  if(rw==1) {
    if(uf->dir=opendir(uf->name)) {
      uf->mode=3;
      return 1;
    } else if(uf->file=fopen(uf->name,"r")) {
      uf->mode=1;
      return 1;
    } else {
      warn("Cannot open file '%s' for reading",uf->name);
    }
  } else if(rw==2 && allow_write) {
    if(uf->file=fopen(uf->name,m?"a":"w")) {
      uf->mode=2;
      return 1;
    } else {
      warn("Cannot open file '%s' for writing",uf->name);
    }
  } else if(rw==3) {
    if(allow_write && (uf->file=fopen(uf->name,"r+"))) {
      uf->mode=4;
      return 1;
    } else if(uf->file=fopen(uf->name,"r")) {
      uf->mode=1;
      return 1;
    } else {
      warn("Cannot open file '%s' for reading/writing",uf->name);
    }
  }
  return 0;
}

static void files_out(Device*dev,Uint8 id) {
  UxnFile*uf=dev->aux;
  Uint16 len=GET16(dev->d+10);
  Uint16 addr,x,y;
  int i;
  switch(id) {
    case 5:
      addr=GET16(dev->d+4);
      dev->d[2]=dev->d[3]=0;
      if(!uf->name[0]) break;
      y=files_stat(AT_FDCWD,uf->name,len,mem+addr);
      if(y<=len) PUT16(dev->d+2,y);
      break;
    case 7:
      if(use_extension) files_set(dev,0);
      break;
    case 9:
      addr=GET16(dev->d+8);
      files_set(dev,0);
      x=GET16(dev->d+8);
      for(i=0;i<4095;i++) {
        if(!(uf->name[i]=mem[x+i])) break;
        // if(mem[x+i]=='/') uf->name[i]=0;
      }
      if(use_debug) fprintf(stderr,"! File: %s\n",uf->name);
      break;
    case 13:
      addr=GET16(dev->d+12);
      dev->d[2]=dev->d[3]=0;
      read:
      if(uf->mode==1 || uf->mode==4) {
        if(!len) break;
        x=fread(mem+addr,1,len,uf->file);
        PUT16(dev->d+2,x);
        if(ferror(uf->file)) {
          warn("Error reading file '%s'",uf->name);
          clearerr(uf->file);
        }
      } else if(uf->mode==3) {
        do {
          if(uf->de) {
            if(x=files_stat(dirfd(uf->dir),uf->de->d_name,len,mem+addr)) {
              addr+=x;
              len-=x;
            } else {
              break;
            }
          }
        } while(uf->de=readdir(uf->dir));
        PUT16(dev->d+2,addr-GET16(dev->d+12));
      } else {
        if(files_set(dev,1)) goto read;
      }
      break;
    case 15:
      addr=GET16(dev->d+14);
      dev->d[2]=dev->d[3]=0;
      if(!allow_write) {
        disallow:
        warnx("Not allowed to write to file '%s'.",uf->name);
        break;
      }
      write:
      if(uf->mode==2 || uf->mode==4) {
        if(!len) break;
        x=fwrite(mem+addr,1,len,uf->file);
        PUT16(dev->d+2,x);
        if(ferror(uf->file)) {
          warn("Error writing file '%s'",uf->name);
          clearerr(uf->file);
        }
      } else {
        if(files_set(dev,2)) goto write;
      }
      break;
  }
}

static Uint8 files_in(Device*dev,Uint8 id) {
  UxnFile*uf=dev->aux;
  int c;
  switch(id) {
    case 12: case 13:
      dev->d[2]=dev->d[3]=0;
      read:
      if(uf->mode==1 || uf->mode==4) {
        c=fgetc(uf->file);
        if(c==EOF) {
          if(ferror(uf->file)) {
            warn("Error reading file '%s'",uf->name);
            clearerr(uf->file);
          }
          c=0;
        } else {
          dev->d[3]=1;
        }
        return c;
      } else if(uf->mode==3) {
        return 0;
      } else {
        if(files_set(dev,1)) goto read;
      }
      return 0;
    default: return dev->d[id];
  }
}

static void files_exp(Device*dev,Uint16 addr) {
  UxnFile*uf=dev->aux;
  Uint32 x;
  if(use_extension) switch(mem[addr]&15) {
    case 0 ... 2:
      x=GET16(mem+addr+1)<<16;
      x|=GET16(mem+addr+3);
      if(!uf->mode) files_set(dev,3);
      files_seek(uf,mem[addr]&15,x);
      break;
    case 3:
      x=files_seek(uf,3,0);
      PUT16(mem+addr+1,x>>16);
      PUT16(mem+addr+3,x);
      break;
  }
}

static Uint8 datetime_in(Device*dev,Uint8 id) {
  time_t t=time(0);
  struct tm r;
  if(use_utc) gmtime_r(&t,&r); else localtime_r(&t,&r);
  switch(id) {
    case 0: return (r.tm_year+1900)>>8;
    case 1: return r.tm_year+1900;
    case 2: return r.tm_mon;
    case 3: return r.tm_mday;
    case 4: return r.tm_hour;
    case 5: return r.tm_min;
    case 6: return r.tm_sec;
    case 7: return r.tm_wday;
    case 8: return r.tm_yday>>8;
    case 9: return r.tm_yday;
    case 10: return r.tm_isdst;
    default: return dev->d[id];
  }
}

static void set_cursor(void) {
  if(hide_cursor) SDL_ShowCursor(paused || !use_mouse);
}

static Uint32 timer_callback(Uint32 interval) {
  if(!timer_expired) {
    SDL_Event e;
    timer_expired=1;
    e.type=SDL_USEREVENT;
    SDL_PushEvent(&e);
    timer_expired=1;
  }
  return interval;
}

static void set_screen_mode(int w,int h) {
  picture_changed=size_changed=0;
  free(bglayer);
  if(screen) SDL_FreeSurface(screen);
  if(w<1) w=1;
  if(h<1) h=1;
  scr_w=w;
  scr_h=h;
  screen=SDL_SetVideoMode(w*zoom,h*zoom,8,scrflags);
  if(!screen) errx(1,"Cannot set screen mode: %s",SDL_GetError());
  if(!bglayer) {
    if(timer_rate) SDL_SetTimer(timer_rate,timer_callback);
    if(audio_option) {
      if(SDL_OpenAudio(&audiospec,0)) errx(1,"Cannot initialize SDL audio: %s",SDL_GetError());
      SDL_PauseAudio(0);
    }
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
    SDL_WM_SetCaption(rom_name,rom_name);
    set_cursor();
    SDL_EnableUNICODE(1);
  }
  SDL_SetColors(screen,colors+palet*8,0,8);
  bglayer=calloc(w*h,2);
  if(!bglayer) err(1,"Memory allocation error");
  fglayer=bglayer+w*h;
}

static void screen_fill(Uint8*p,Uint8 v,Uint16 x0,Uint16 y0,Uint16 x1,Uint16 y1) {
  if(y1>scr_h) y1=scr_h;
  if(x1>scr_w) x1=scr_w;
  if(y0>=y1 || x0>=x1) return;
  p+=y0*scr_w+x0;
  x1-=x0;
  while(y0++<y1) memset(p,v,x1),p+=scr_w;
}

static void draw_sprite(Uint16 x,Uint16 y,Uint16 addr,Uint8 v) {
  Uint8 b=v&0x80;
  Uint8 u;
  Uint16 c;
  Uint8*p=v&0x40?fglayer:bglayer;
  Uint8*m;
  if(x+8>scr_w || y+8>scr_h || addr>0x10000-(b?16:8)) return;
  p+=y*scr_w+x;
  m=mem+addr;
  for(y=0;y<8;y++) {
    c=m[y]|(b?m[y+8]<<8:0);
    for(x=0;x<8;x++) {
      u=(c&1)|((c>>7)&2);
      c>>=1;
      if((u=blend[u][v&0x0F])!=4) p[(y^(v&0x20?7:0))*scr_w+(x^(v&0x10?0:7))]=u;
    }
  }
}

static void screen_out(Device*dev,Uint8 id) {
  int i,fx,fy;
  Uint16 w;
  Uint16 x=GET16(dev->d+8);
  Uint16 y=GET16(dev->d+10);
  Uint8 z=dev->d[id];
  if(id<6 && disallow_size_change) {
    PUT16(dev->d+2,scr_w);
    PUT16(dev->d+4,scr_h);
    return;
  }
  switch(id) {
    case 2: case 3:
      x=GET16(dev->d+2);
      if(x!=scr_w) scr_w=x,size_changed=1;
      break;
    case 4: case 5:
      x=GET16(dev->d+4);
      if(x!=scr_h) scr_h=x,size_changed=1;
      break;
    case 14:
      if(size_changed) set_screen_mode(scr_w,scr_h);
      picture_changed=1;
      if(z<0x80) {
        if(x<scr_w && y<scr_h) (z&0x40?fglayer:bglayer)[x+y*scr_w]=z&3;
        if(dev->d[6]&0x01) PUT16(dev->d+8,x+1);
        if(dev->d[6]&0x02) PUT16(dev->d+10,y+1);
      } else {
        screen_fill(z&0x40?fglayer:bglayer,z&3,dev->d[14]&0x10?0:x,dev->d[14]&0x20?0:y,dev->d[14]&0x10?x:scr_w,dev->d[14]&0x20?y:scr_h);
      }
      break;
    case 15:
      if(size_changed) set_screen_mode(scr_w,scr_h);
      picture_changed=1;
      w=GET16(dev->d+12);
      fx=((z&0x10)&&(screencompat&0x02)?-8:8);
      fy=((z&0x20)&&(screencompat&0x02)?-8:8);
      for(i=0;i<=(dev->d[6]>>4);i++) {
        draw_sprite(x+i*(dev->d[6]&0x02?fx:0),y+i*(dev->d[6]&0x01?fy:0),w,z);
        if(dev->d[6]&0x04) w+=z&0x80?16:8;
      }
      if(dev->d[6]&0x01) PUT16(dev->d+8,x+fx);
      if(dev->d[6]&0x02) PUT16(dev->d+10,y+fy);
      if(dev->d[6]&0x04) PUT16(dev->d+12,w);
      break;
  }
}

static void audio_out(Device*dev,Uint8 id) {
  UxnAudio*a;
  Uint32 s,n;
  if(id!=15) return;
  if(use_screen) SDL_LockAudio();
  a=dev->aux;
  a->env[0]=(dev->d[8]>>4)*audio_option['e'-'a'];
  a->env[1]=(dev->d[8]&15)*audio_option['e'-'a'];
  a->env[2]=(dev->d[9]>>4)*audio_option['e'-'a'];
  a->env[3]=(dev->d[9]&15)*audio_option['e'-'a'];
  a->length=n=GET16(dev->d+10);
  a->pos=s=GET16(dev->d+12);
  a->end=s+n;
  if(dev->d[15]&0x80) a->length=0;
  a->envtime=0;
  a->phase=0;
  s=(dev->d[15]&0x7F)+audio_option['n'-'a'];
  s=notes[s%12]>>(10-s/12);
  if(n<256) s=(n*(unsigned long long)s)>>8;
  if(use_extension && (dev->d[6] || dev->d[7])) s=(((unsigned long long)s)*(0x10000+GET16(dev->d+6)))>>16;
  a->freq=s;
  if(audiospec.channels==1) {
    a->vol[0]=a->vol[1]=((dev->d[14]&15)+(dev->d[14]>>4))/2;
  } else {
    a->vol[0]=dev->d[14]>>4;
    a->vol[1]=dev->d[14]&15;
  }
  if(use_screen) SDL_UnlockAudio();
}

static Uint32 envelope(const UxnAudio*a) {
  // Returns a number from 0 to 64.
  Uint32 t=a->envtime;
  if(!(a->env[0]|a->env[1]|a->env[2]|a->env[3])) return 64;
  if(t<a->env[0]) return ((64*t)/a->env[0])?:1;
  t-=a->env[0];
  if(t<a->env[1]) return 64-((32*t)/a->env[1]);
  t-=a->env[1];
  if(t<a->env[2]) return 32;
  t-=a->env[2];
  if(t<a->env[3]) return 32-((32*t)/a->env[3]);
  return 0;
}

static void audio_callback(void*userdata,Uint8*stream,int len) {
  Uint8 c=audiospec.channels;
  Uint32 vol=audio_option['m'-'a']?0:audio_option['v'-'a'];
  Sint16*buf=(void*)stream;
  UxnAudio*a;
  Uint32 o0,o1,d,n,e;
  len>>=1;
  for(n=0;n<len;n+=c) {
    o0=o1=0;
    for(d=3;d<7;d++) {
      a=device[d].aux;
      if(a->pos>=a->end && !a->length) continue;
      e=envelope(a);
      if(!e) {
        a->pos=a->length=a->end=0;
        continue;
      }
      a->envtime++;
      e*=(Sint8)(mem[a->pos]^0x80);
      a->phase+=a->freq;
      if(a->phase>=0x1000000) {
        a->pos+=a->phase>>24;
        a->phase&=0xFFFFFF;
        while(a->pos>=a->end && a->length) a->pos-=a->length;
      }
      o0+=e*a->vol[0];
      o1+=e*a->vol[1];
    }
    buf[n]=(o0*vol)>>12;
    if(c==2) buf[n+1]=(o1*vol)>>12;
  }
}

static void run_audio(void) {
  Uint8 c=audiospec.channels;
  Uint32 vol=audio_option['m'-'a']?0:audio_option['v'-'a'];
  Uint32 fram=(timer_rate*(long long)audiospec.freq)/1000;
  Uint8 q=0;
  Uint32 f=0;
  UxnAudio*a;
  Uint32 o0,o1,d,e;
  for(;;) {
    if(++f==fram) {
      f=0;
      run(GET16(device[2].d));
    }
    if(q) {
      for(d=3;d<7;d++) if(q&(1<<d)) run(GET16(device[d].d));
      q=0;
    }
    //TODO: check for input from stdin
    o0=o1=0;
    for(d=3;d<7;d++) {
      a=device[d].aux;
      if(a->pos>=a->end && !a->length) {
        end:
        if(a->length) q|=1<<d;
        a->pos=a->length=a->end=0;
        continue;
      }
      e=envelope(a);
      if(!e) goto end;
      a->envtime++;
      e*=(Sint8)(mem[a->pos]^0x80);
      a->phase+=a->freq;
      if(a->phase>=0x1000000) {
        a->pos+=a->phase>>24;
        a->phase&=0xFFFFFF;
        while(a->pos>=a->end && a->length) a->pos-=a->length;
      }
      o0+=e*a->vol[0];
      o1+=e*a->vol[1];
    }
    d=(o0*vol)>>12; putchar(d>>8); putchar(d);
    if(c==2) {
      d=(o1*vol)>>12; putchar(d>>8); putchar(d);
    }
  }
}

static void do_extension_by_uuid(Uint16 addr) {
  static const Uint8 uuid_8color[]="\x80\x17\x51\x32\xE2\x63\x11\xED\xB8\xC9\x00\x26\x18\x74\x54\x16";
  static const Uint8 uuid_screencompat[]="\x97\x0B\x5A\x0C\x2C\x44\x11\xEE\xAC\x3B\x00\x26\x18\x74\x54\x16";
  int i;
  if(mem[addr+1]==0x00 && use_screen && !memcmp(mem+addr+2,uuid_8color,16)) {
    for(i=0;i<8;i++) {
      colors[i+070].r=(mem[addr+19+i+i]>>4)*0x11;
      colors[i+070].g=(mem[addr+19+i+i]&15)*0x11;
      colors[i+070].b=(mem[addr+20+i+i]>>4)*0x11;
    }
    if(palet==7) {
      SDL_SetColors(screen,colors+palet*8,0,8);
      picture_changed=1;
    }
    mem[addr+18]=1;
  } else if(mem[addr+1]==0x20 && use_screen && !memcmp(mem+addr+2,uuid_screencompat,16)) {
    screencompat=mem[addr+19];
    blend[0][0]=(screencompat&0x01?4:0);
    mem[addr+18]=1;
  }
}

static void system_exp(Device*dev,Uint16 addr) {
  Uint32 src,dst;
  Uint16 len,n;
  switch(mem[addr]) {
    case 0x00:
      len=GET16(mem+addr+1);
      dst=(GET16(mem+addr+3)<<16L)|(GET16(mem+addr+5));
      if(dst<MEMSIZE && dst+len<=MEMSIZE) {
        memset(mem+dst,mem[addr+7],len);
      } else {
        if(use_debug) fprintf(stderr,"! Access expanded memory out of range\n");
      }
      break;
    case 0x01: case 0x02:
      len=GET16(mem+addr+1);
      src=(GET16(mem+addr+3)<<16L)|(GET16(mem+addr+5));
      dst=(GET16(mem+addr+7)<<16L)|(GET16(mem+addr+9));
      if(dst<MEMSIZE && src+len<=MEMSIZE && dst+len<=MEMSIZE) {
        if(mem[addr]==0x01) {
          for(n=0;n<len;n++) mem[dst+n]=mem[src+n];
        } else {
          for(n=len;n;n--) mem[dst+n-1]=mem[src+n-1];
        }
      } else {
        if(use_debug) fprintf(stderr,"! Access expanded memory out of range\n");
      }
      break;
    case 0x03:
      if(!use_extension) break;
      mem[addr+18]=0;
      do_extension_by_uuid(addr);
      break;
    case 0x04:
      if(!use_extension) break;
      PUT16(mem+addr+1,EXPANDED_MEMORY);
      break;
  }
}

static void debug_screen(Uint8*s,Uint8 k) {
  static Sint8 page=0;
  static Uint8 port=0;
  static Uint8 din;
  static Uint32 addr=0;
  static Uint32 scrol=0;
  static const char opname[]=
    "?IPNSRDOENGLJJJSLSLSLSDDASMDAOES"
    "?NOIWOUVQETTMCSTDTDTDTEEDUUINROF"
    "?CPPPTPRUQHHPNRHZZRRAAIODBLVDART"
  ;
  char buf[81];
  int i,j;
  Stack*z;
  FILE*f;
  UxnFile*u;
  if(s) {
    memset(s,32,80*25);
    show:
    switch(page) {
      case 0:
        memcpy(s+80*0,buf,snprintf(buf,81,"* %s",rom_name));
        memcpy(s+80*1,buf,snprintf(buf,81,"<c> Console: %s",use_console?"On":"Off"));
        memcpy(s+80*2,buf,snprintf(buf,81,"<d> Debug: %s",use_debug?"On":"Off"));
        memcpy(s+80*3,buf,snprintf(buf,81,"<F2> Sound: %s",audio_option?(audio_option['m'-'a']?"Mute":"On"):"N/A"));
        memcpy(s+80*4,buf,snprintf(buf,81,"<F3> Mouse: %s",use_mouse?"On":"Off"));
        memcpy(s+80*5,buf,snprintf(buf,81,"<F4> Layers: %s",layers?(layers==1?"fg":"bg"):"fg/bg"));
        memcpy(s+80*6,buf,snprintf(buf,81,"<F6/F7> Palette: %d",palet));
        memcpy(s+80*7,buf,snprintf(buf,81,"<F9> Restart"));
        memcpy(s+80*8,buf,snprintf(buf,81,"<F10> Quit"));
        if(audio_option) memcpy(s+80*9,buf,snprintf(buf,81,"<-/+> Volume: %d",(int)audio_option['v'-'a']));
        memcpy(s+80*10,buf,snprintf(buf,81,"<PAUSE> Resume"));
        memcpy(s+80*13,buf,snprintf(buf,81,"Pages: CTRL+"));
        memcpy(s+80*14,buf,snprintf(buf,81," A:Stack   B:Return  C:Device  D:Memory  E:Files "));
        memcpy(s+80*15,buf,snprintf(buf,81," F:Code    G:        N:        O:        P:      "));
        memcpy(s+80*16,buf,snprintf(buf,81," Q:        R:        S:        T:        U:      "));
        memcpy(s+80*17,buf,snprintf(buf,81," V:        W:        X:        Y:        Z:Main  "));
        break;
      case 1: case 2:
        if(page==1) z=&ds; else z=&rs;
        memcpy(s,buf,snprintf(buf,81,"%s Stack (%02x)",page==1?"Data":"Return",z->p));
        for(i=0;i<256;i++) memcpy(s+80*(i/16+1)+(i&15)*4,buf,snprintf(buf,81,"%c%02x%c",z->p==i?'[':' ',z->d[i],z->p==i?']':' '));
        break;
      case 3:
        memcpy(s,buf,snprintf(buf,81,"--   -0  -1  -2  -3  -4  -5  -6  -7  -8  -9  -a  -b  -c  -d  -e  -f"));
        for(i=0;i<16;i++) memcpy(s+80*(i+1),buf,snprintf(buf,81,"%x0:",i));
        for(i=0;i<256;i++) memcpy(s+80*(i/16+1)+(i&15)*4+4,buf,snprintf(buf,81,"%c%02x%c",port==i?'[':' ',device[i>>4].d[i&15],port==i?']':' '));
        memcpy(s+80*17,buf,snprintf(buf,81,"<i> In=%02x  <o> Out  <x> Execute",din));
        break;
      case 4:
        if(addr<0) addr=0;
        if(addr>=MEMSIZE) addr=MEMSIZE-1;
        if(scrol<0) scrol=0;
        if(addr>=0x100 && scrol<addr-0xF0) scrol=(addr-0xF0)&~15;
        if(scrol>addr) scrol=addr&~15;
        memcpy(s,buf,snprintf(buf,81,"-----   -0  -1  -2  -3  -4  -5  -6  -7  -8  -9  -a  -b  -c  -d  -e  -f"));
        for(i=0;i<16;i++) memcpy(s+80*(i+1),buf,snprintf(buf,81,"%05x:",i*16+scrol));
        for(i=0;i<256;i++) if(i+scrol<MEMSIZE) memcpy(s+80*(i/16+1)+(i&15)*4+7,buf,snprintf(buf,81,"%c%02x%c",addr==i+scrol?'[':' ',mem[i+scrol],addr==i+scrol?']':' '));
        memcpy(s+80*17,buf,snprintf(buf,81,"<x> Execute  <s> Save"));
        break;
      case 5:
        for(i=0;i<2;i++) {
          u=i?&uxnfile1:&uxnfile0;
          memcpy(s+80*(7*i+1),buf,snprintf(buf,81,"[File%d]",i));
          memcpy(s+80*(7*i+2),buf,snprintf(buf,81," name=%s",u->name));
          if(u->file) memcpy(s+80*(7*i+3),buf,snprintf(buf,81," File fd=%d offset=0x%lX",fileno(u->file),ftell(u->file)));
          if(u->dir) memcpy(s+80*(7*i+4),buf,snprintf(buf,81," Dir fd=%d offset=0x%lX",dirfd(u->dir),telldir(u->dir)));
          memcpy(s+80*(7*i+5),buf,snprintf(buf,81," mode=0x%02X",u->mode));
          memcpy(s+80*(7*i+6),buf,snprintf(buf,81," <%c> Close  <%c> Rewind","cC"[i],"rR"[i]));
        }
        break;
      case 6:
        j=break_pc;
        for(i=0;i<16 && j<0x10000;i++) {
          memcpy(s+80*i,buf,snprintf(buf,81,"%04x: %c%c %c%c%c%c%c%c",j,breakpoint==j+1?'*':' ',i?' ':'>',
           opname[mem[j]&0x1F],opname[(mem[j]&0x1F)+0x20],opname[(mem[j]&0x1F)+0x40]
           ,mem[j]&0x20?'2':'_',mem[j]&0x40?'r':'_',mem[j]&0x80?'k':'_' ));
          if(mem[j]==0x80 || mem[j]==0xC0) {
            memcpy(s+80*i+9,buf,snprintf(buf,81,"LIT_%c  %02x",mem[j]&0x40?'r':'_',mem[j+1]));
            j+=2;
          } else if(mem[j]==0xA0 || mem[j]==0xE0) {
            memcpy(s+80*i+9,buf,snprintf(buf,81,"LIT2%c  %02x%02x",mem[j]&0x40?'r':'_',mem[j+1],mem[j+2]));
            j+=3;
          } else {
            if(!mem[j]) memcpy(s+80*i+9,"BRK",3);
            j++;
          }
        }
        memcpy(s+80*17,buf,snprintf(buf,81,"ds:%02x rs:%02x",ds.p,rs.p));
        memcpy(s+80*19,buf,snprintf(buf,81,"<SPACE> Step  <RET> Continue  <x> Execute"));
        memcpy(s+80*20,buf,snprintf(buf,81,"<,> Set Breakpoint  <.> Clear Breakpoint"));
        break;
      default:
        if(page<0) page=6; else page=0;
        goto show;
    }
  } else {
    if(k==9 || k==']') {
      ++page;
    } else if(k=='[') {
      --page;
    } else if(k>=1 && k<=7) {
      page=k;
    } else if(k>=14 && k<=25) {
      page=k-6;
    } else if(k==26) {
      page=0;
    } else if(k==27) {
      debugger=0;
      return;
    } else switch(page) {
      case 0:
        if(k=='c') use_console^=1;
        if(k=='d') use_debug^=1;
        if(audio_option) {
          if((k=='-' || k==10) && audio_option['v'-'a']) --audio_option['v'-'a'];
          if(k=='+' || k==11) ++audio_option['v'-'a'];
          if(k==8) audio_option['v'-'a']/=10;
          if(k>='0' && k<='9') audio_option['v'-'a']=10*audio_option['v'-'a']+k-'0';
        }
        break;
      case 1: case 2:
        if(page==1) z=&ds; else z=&rs;
        if(k==8) z->p--;
        if(k==12) z->p++;
        if(k==10) z->p+=16;
        if(k==11) z->p-=16;
        if(k>='0' && k<='9') z->d[z->p]=(z->d[z->p]<<4)+(k-'0');
        if(k>='a' && k<='f') z->d[z->p]=(z->d[z->p]<<4)+(k+10-'a');
        break;
      case 3:
        i=port;
        if(k==8) port--;
        if(k==12) port++;
        if(k==10) port+=16;
        if(k==11) port-=16;
        if(k>='0' && k<='9') device[i>>4].d[i&15]=(device[i>>4].d[i&15]<<4)+(k-'0');
        if(k>='a' && k<='f') device[i>>4].d[i&15]=(device[i>>4].d[i&15]<<4)+(k+10-'a');
        if(k=='i') din=device[i>>4].in(device+(i>>4),i&15);
        if(k=='o') device[i>>4].out(device+(i>>4),i&15);
        if(k=='x') run(GET16(device[i>>4].d));
        break;
      case 4:
        if(k==8) addr--;
        if(k==12) addr++;
        if(k==10) addr+=16;
        if(k==11) addr-=16;
        if(k==',') scrol-=0x10,addr-=0x10;
        if(k=='.') scrol+=0x10,addr+=0x10;
        if(k=='<') scrol-=0x100,addr-=0x100;
        if(k=='>') scrol+=0x100,addr+=0x100;
        if(k=='q') scrol-=0x1000,addr-=0x1000;
        if(k=='w') scrol+=0x1000,addr+=0x1000;
        if(k=='Q') scrol-=0x10000,addr-=0x10000;
        if(k=='W') scrol+=0x10000,addr+=0x10000;
        if(k=='=') scrol=addr=0;
        if(addr>=MEMSIZE+0x1000) scrol=addr=0;
        if(k>='0' && k<='9') mem[addr]=(mem[addr]<<4)+(k-'0');
        if(k>='a' && k<='f') mem[addr]=(mem[addr]<<4)+(k+10-'a');
        if(k=='x' && addr<0x10000) run(addr);
        if(k=='s') {
          f=fopen("_uxn_memory","w");
          if(f) {
            fwrite(mem,1,MEMSIZE,f);
            fprintf(stderr,"Saved 0x%X bytes to file _uxn_memory\n",(int)(MEMSIZE));
            fclose(f);
          } else {
            warn("Cannot open file _uxn_memory");
          }
        }
        break;
      case 5:
        if(k=='c') files_set(device+10,0);
        if(k=='C') files_set(device+11,0);
        if(k=='r') files_seek(&uxnfile0,0,0);
        if(k=='R') files_seek(&uxnfile1,0,0);
        break;
      case 6:
        if(k==8 || k==11) --break_pc;
        if(k==12 || k==10) ++break_pc;
        if(k>='0' && k<='9') break_pc=(break_pc<<4)+(k-'0');
        if(k>='a' && k<='f') break_pc=(break_pc<<4)+(k+10-'a');
        if(k==',') breakpoint=break_pc+1;
        if(k=='.') breakpoint=0;
        if(k=='x') run(break_pc);
        if(k==' ') {
          j=breakpoint;
          breakpoint=break_pc+2;
          i=mem[break_pc];
          if(i==0x80 || i==0xC0) {
            breakpoint=break_pc+3;
          } else if(i==0xA0 || i==0xE0) {
            breakpoint=break_pc+4;
          } else if((i&0x1F)==0x0C || (i&0x1F)==0x0D || (i&0x1F)==0x0E) {
            if((i&0x1F)==0x0D) {
              i&=0x60;
              if(i==0x00 && ds.p>1 && !ds.d[ds.p-2]) goto no_jump;
              if(i==0x20 && ds.p>2 && !ds.d[ds.p-3]) goto no_jump;
              if(i==0x40 && rs.p>1 && !rs.d[rs.p-2]) goto no_jump;
              if(i==0x60 && rs.p>2 && !rs.d[rs.p-3]) goto no_jump;
            }
            i&=0x60;
            if(i==0x00 && ds.p) breakpoint=break_pc+((Sint8)ds.d[ds.p-1])+2;
            else if(i==0x20 && ds.p>1) breakpoint=GET16(ds.d+ds.p-2)+1;
            else if(i==0x40 && rs.p) breakpoint=break_pc+((Sint8)rs.d[rs.p-1])+2;
            else if(i==0x60 && rs.p>1) breakpoint=GET16(rs.d+rs.p-2)+1;
            else goto no_run;
          } else if(i) {
            no_jump:
            breakpoint=break_pc+2;
          } else {
            goto no_run;
          }
          run(break_pc);
          no_run:
          breakpoint=j;
          break;
        }
        if(k==13) {
          paused=debugger=0;
          run(break_pc);
          return;
        }
        break;
    }
  }
}

static void redraw(void) {
  static Uint8*s=0;
  int a,r,u,v,x,y;
  Uint8*p;
  if(debugger) {
    if(!s) s=calloc(80,26);
    if(!s) err(1,"Allocation failed");
    debug_screen(s,0);
  }
  if(size_changed) set_screen_mode(scr_w,scr_h);
  picture_changed=0;
  SDL_LockSurface(screen);
  a=0;
  p=screen->pixels;
  r=screen->pitch;
  if(debugger) {
    for(y=0;y<scr_h*zoom;y++) {
      for(x=0;x<scr_w*zoom;x++) {
        if(y>=25*8-7 || x>=80*8 || (v=s[(y/8)*80+x/8]-32)<=0) p[x]=0;
        else p[x]=(fontdata[v*8+(y&7)]>>(7&~x))&1;
      }
      p+=r;
    }
    if(scr_h*zoom>9) {
      p=screen->pixels+r*(scr_h*zoom-8);
      for(y=0;y<8;y++) {
        for(a=0;a<scr_w*zoom;a++) p[a]=(a>>4)&7;
        p+=r;
      }
    }
  } else if(layers) {
    for(y=0;y<scr_h;y++) {
      for(x=0;x<scr_w;x++) {
        u=(layers==1?fglayer:bglayer)[a];
        for(v=0;v<zoom;v++) memset(p+v*r+x*zoom,u,zoom);
        a++;
      }
      p+=r*zoom;
    }
  } else {
    for(y=0;y<scr_h;y++) {
      for(x=0;x<scr_w;x++) {
        u=fglayer[a]?fglayer[a]+4:bglayer[a];
        for(v=0;v<zoom;v++) memset(p+v*r+x*zoom,u,zoom);
        a++;
      }
      p+=r*zoom;
    }
  }
  SDL_UnlockSurface(screen);
  SDL_Flip(screen);
}

static inline int select_joystick(Uint8 n) {
  if((joy_mask&1) && n==joy_player[0]) return 2;
  if((joy_mask&2) && n==joy_player[1]) return 5;
  if((joy_mask&4) && n==joy_player[2]) return 6;
  if((joy_mask&8) && n==joy_player[3]) return 7;
  return 0;
}

static int run_screen(void) {
  SDL_Event e;
  int i,j,k;
  while(SDL_WaitEvent(&e)) {
    switch(e.type) {
      case SDL_QUIT: return 0;
      case SDL_VIDEOEXPOSE: redraw(); break;
      case SDL_USEREVENT: timer_expired=0; if(!paused) run(GET16(device[2].d)); break;
      case SDL_KEYDOWN:
        i=0;
        switch(e.key.keysym.sym) {
          case SDLK_LCTRL: case SDLK_END: i=0x01; break;
          case SDLK_LALT: i=0x02; break;
          case SDLK_LSHIFT: i=0x04; break;
          case SDLK_HOME: i=0x08; break;
          case SDLK_UP: i=0x10; break;
          case SDLK_DOWN: i=0x20; break;
          case SDLK_LEFT: i=0x40; break;
          case SDLK_RIGHT: i=0x80; break;
          case SDLK_PAGEUP: i=-scroll_size; goto scroll;
          case SDLK_PAGEDOWN: i=scroll_size; goto scroll;
          case SDLK_F1: run(GET16(device[2].d)); timer_expired=0; break;
          case SDLK_F2: if(audio_option) audio_option['m'-'a']^=1; picture_changed=debugger; break;
          case SDLK_F3: use_mouse^=1; set_cursor(); picture_changed=debugger; break;
          case SDLK_F4: layers=(layers+1)%3; redraw(); break;
          case SDLK_F5: redraw(); if(SDL_GetModState()&KMOD_RSHIFT) debug(-1); break;
          case SDLK_F6: palet=(palet+1)&7; SDL_SetColors(screen,colors+palet*8,0,8); redraw(); break;
          case SDLK_F7: palet=(palet-1)&7; SDL_SetColors(screen,colors+palet*8,0,8); redraw(); break;
          case SDLK_F8: paused=picture_changed=debugger=1; break;
          case SDLK_F9: return 1;
          case SDLK_F10: return 0;
          case SDLK_PAUSE: debugger=0; paused^=1; set_cursor(); picture_changed=1; break;
        }
        if(paused) {
          if(debugger) {
            if(i&0xF0) {
              debug_screen(0,i==0x10?11:i==0x20?10:i==0x40?8:i==0x80?12:0);
            } else if(i=e.key.keysym.unicode) {
              debug_screen(0,i);
            }
            picture_changed=1;
          }
          break;
        }
        if(i) {
          if(joypad_repeat || !(device[8].d[2]&i)) {
            device[8].d[2]|=i;
            run(GET16(device[8].d));
          }
        } else if(i=e.key.keysym.unicode) {
          device[8].d[3]=i;
          run(GET16(device[8].d));
          if(!debugger) device[8].d[3]=0; else debug_op=8;
        }
        break;
      scroll:
        if(paused || !i) break;
        PUT16(device[9].d+12,i);
        run(GET16(device[9].d));
        if(!debugger) PUT16(device[9].d+12,0); else debug_op=9;
        break;
      case SDL_KEYUP:
        if(paused) break;
        i=0;
        switch(e.key.keysym.sym) {
          case SDLK_LCTRL: case SDLK_END: i=0x01; break;
          case SDLK_LALT: i=0x02; break;
          case SDLK_LSHIFT: i=0x04; break;
          case SDLK_HOME: i=0x08; break;
          case SDLK_UP: i=0x10; break;
          case SDLK_DOWN: i=0x20; break;
          case SDLK_LEFT: i=0x40; break;
          case SDLK_RIGHT: i=0x80; break;
        }
        if(device[8].d[2]&i) {
          device[8].d[2]&=~i;
          run(GET16(device[8].d));
        }
        break;
      case SDL_MOUSEMOTION:
      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        if(paused || !use_mouse) break;
        k=SDL_GetMouseState(&i,&j);
        i/=zoom; j/=zoom;
        if(i<0 || i>=screen->w || j<0 || j>=screen->h) break;
        if(touch_mode) {
          if(e.type==SDL_MOUSEBUTTONDOWN && !device[9].d[6]) {
            PUT16(device[9].d+2,i);
            PUT16(device[9].d+4,j);
            run(GET16(device[9].d));
          } else if(e.type==SDL_MOUSEBUTTONUP && !k) {
            device[9].d[6]=0;
            run(GET16(device[9].d));
            i=j=0x8000;
          } else if(e.type==SDL_MOUSEMOTION && !k) {
            break;
          }
        }
        PUT16(device[9].d+2,i);
        PUT16(device[9].d+4,j);
        device[9].d[6]=k&7;
        run(GET16(device[9].d));
        break;
      case SDL_USEREVENT+1:
        device[1].d[2]=e.user.code;
        device[1].d[7]=1;
        if(e.user.data1) {
          if(!(device[1].d[6]&1)) break;
          device[1].d[7]=0;
        }
        run(GET16(device[1].d));
        SDL_CondBroadcast(stdin_cond);
        break;
      case SDL_JOYAXISMOTION:
        if(!joy_axis_mode) break;
        if(j=select_joystick(e.jaxis.which)) {
          k=device[8].d[j];
          if(e.jaxis.axis==joy_axis_x) {
            i=e.jaxis.value*(joy_axis_mode&1?-1:1);
            k&=0x3F;
            if(i>=joy_sensitivity) k|=0x80; else if(i<=-joy_sensitivity) k|=0x40;
          } else if(e.jaxis.axis==joy_axis_y) {
            i=e.jaxis.value*(joy_axis_mode&2?-1:1);
            k&=0xCF;
            if(i>=joy_sensitivity) k|=0x10; else if(i<=-joy_sensitivity) k|=0x20;
          }
          trigger:
          if(k!=device[8].d[j]) {
            device[8].d[j]=k;
            if(!paused) run(GET16(device[8].d));
          }
        }
        break;
      case SDL_JOYBUTTONDOWN:
      case SDL_JOYBUTTONUP:
        if(e.jbutton.button&~127) break;
        if(j=select_joystick(e.jbutton.which)) {
          k=device[8].d[j];
          i=joy_button_map[e.jbutton.button];
          if(e.jbutton.state==SDL_PRESSED) k|=i; else k&=~i;
          goto trigger;
        }
        break;
      case SDL_JOYHATMOTION:
        if(!joy_hat_mode) break;
        if(j=select_joystick(e.jhat.which)) {
          k=device[8].d[j]&0x0F;
          // Note that the SDL_HAT_ values are bits counted clockwise from UP.
          i=(e.jhat.value*0x1111)>>joy_hat_mode;
          if(i&SDL_HAT_UP) k|=0x10;
          if(i&SDL_HAT_DOWN) k|=0x20;
          if(i&SDL_HAT_LEFT) k|=0x40;
          if(i&SDL_HAT_RIGHT) k|=0x80;
          goto trigger;
        }
        break;
    }
    if(picture_changed) redraw();
  }
  return 0;
}

static int stdin_thread_fn(void*unused) {
  int c;
  SDL_Event e;
  SDL_mutex*mut=SDL_CreateMutex();
  SDL_LockMutex(mut);
  e.type=SDL_USEREVENT+1;
  e.user.data1=0;
  while((c=fgetc(inf))>=0) {
    e.user.code=c;
    while(SDL_PushEvent(&e)) SDL_CondWaitTimeout(stdin_cond,mut,timer_rate?:10);
  }
  if(use_extension) {
    e.user.code=0;
    e.user.data1="";
    while(SDL_PushEvent(&e)) SDL_CondWaitTimeout(stdin_cond,mut,timer_rate?:10);
  }
  return 0;
}

static void set_datetime(char*s) {
  int i=2;
  if(*s=='@') {
    struct tm r;
    time_t t=strtoll(s+1,0,10);
    if(use_utc) gmtime_r(&t,&r); else localtime_r(&t,&r);
    PUT16(device[12].d,r.tm_year+1900);
    device[12].d[2]=r.tm_mon;
    device[12].d[3]=r.tm_mday;
    device[12].d[4]=r.tm_hour;
    device[12].d[5]=r.tm_min;
    device[12].d[6]=r.tm_sec;
    device[12].d[7]=r.tm_wday;
    PUT16(device[12].d+8,r.tm_yday);
    device[12].d[10]=r.tm_isdst;
  } else {
    i=strtol(s,&s,10);
    PUT16(device[12].d,i);
    if(*s) s++;
    i=2;
    while(*s && i<15) {
      device[12].d[i++]=strtol(s,&s,10);
      if(*s) s++;
    }
  }
}

static void set_audio(char*s) {
  int i;
  audio_option=calloc(sizeof(Sint32),26);
  if(!audio_option) err(1,"Allocation failed");
  audio_option['b'-'a']=2048;
  audio_option['c'-'a']=2;
  audio_option['r'-'a']=44100;
  audio_option['v'-'a']=273;
  while(*s && *s!='.') {
    if(*s<'a' && *s>'z') errx(1,"Invalid audio option");
    audio_option[*s-'a']=strtol(s+1,&s,10);
  }
  audiospec.freq=audio_option['r'-'a'];
  audiospec.channels=audio_option['c'-'a'];
  audiospec.samples=audio_option['b'-'a'];
  audiospec.format=AUDIO_S16SYS;
  audiospec.callback=audio_callback;
  for(i=3;i<7;i++) {
    device[i].aux=calloc(1,sizeof(UxnAudio));
    if(!device[i].aux) err(1,"Allocation failed");
    device[i].out=audio_out;
  }
  if(!audio_option['e'-'a']) audio_option['e'-'a']=audio_option['r'-'a']/15;
}

static void set_joystick(char*s) {
  int c,i,n;
  SDL_Joystick*j;
  if(*s=='?') {
    if(SDL_Init(SDL_INIT_JOYSTICK|SDL_INIT_NOPARACHUTE)) errx(1,"Cannot initialize SDL: %s",SDL_GetError());
    atexit(SDL_Quit);
    if(s[1]) {
      j=SDL_JoystickOpen(i=strtol(s+1,0,10));
      if(!j) errx(1,"Cannot open joystick %d",i);
      printf("Opened Joystick %d\n",i);
      printf("Name: %s\n",SDL_JoystickName(i));
      printf("Axes: %d\n",SDL_JoystickNumAxes(j));
      printf("Buttons: %d\n",SDL_JoystickNumButtons(j));
      printf("Hats: %d\n",SDL_JoystickNumHats(j));
      printf("Balls: %d\n",SDL_JoystickNumBalls(j));
      SDL_JoystickClose(j);
    } else {
      n=SDL_NumJoysticks();
      for(i=0;i<n;i++) printf("%d: %s\n",i,SDL_JoystickName(i));
    }
    exit(0);
    return;
  }
  n=0;
  while(*s) {
    if(*s>='0' && *s<='9') {
      if(n&~3) errx(1,"Too many players");
      joy_player[n]=strtol(s,&s,10);
      joy_mask|=1<<n;
    } else if(*s==',') {
      s++;
      n++;
    } else {
      break;
    }
  }
  if(*s=='/') ++s; else errx(1,"Improper joystick setting");
  while(*s) {
    c=*s++;
    n=strtol(s,&s,10);
    switch(c) {
      case 'A': joy_button_map[n&0x7F]|=0x01; break;
      case 'B': joy_button_map[n&0x7F]|=0x02; break;
      case 'D': joy_button_map[n&0x7F]|=0x20; break;
      case 'H': joy_hat=n; joy_hat_mode=6; break;
      case 'h': joy_hat=n; joy_hat_mode=4; break;
      case 'I': joy_hat=n; joy_hat_mode=5; break;
      case 'i': joy_hat=n; joy_hat_mode=7; break;
      case 'L': joy_button_map[n&0x7F]|=0x40; break;
      case 'R': joy_button_map[n&0x7F]|=0x80; break;
      case 'S': joy_button_map[n&0x7F]|=0x04; break;
      case 'T': joy_button_map[n&0x7F]|=0x08; break;
      case 'U': joy_button_map[n&0x7F]|=0x10; break;
      case 'X': joy_axis_x=n; joy_axis_mode|=1; break;
      case 'Y': joy_axis_y=n; joy_axis_mode|=2; break;
      case 's': joy_sensitivity=n; break;
      case 'x': joy_axis_x=n; joy_axis_mode|=4; break;
      case 'y': joy_axis_y=n; joy_axis_mode|=8; break;
      default: errx(1,"Improper joystick setting");
    }
  }
}

static void set_other_program(const char*s) {
  int pf[4];
  pid_t pi;
  if(pipe(pf) || pipe(pf+2)) err(1,"Pipe failed");
  pi=fork();
  if(pi==(pid_t)(-1)) err(1,"Fork failed");
  if(pi) {
    // Parent
    close(pf[1]);
    close(pf[2]);
    inf=fdopen(pf[0],"r");
    if(!inf) err(1,"Cannot open pipe for input");
    outf=fdopen(pf[3],"w");
    if(!outf) err(1,"Cannot open pipe for output");
  } else {
    // Child
    close(pf[0]);
    close(pf[3]);
    if(dup2(pf[1],1)<0 || dup2(pf[2],0)<0) err(1,"Cannot duplicate file descriptors");
    close(pf[1]);
    close(pf[2]);
    execl("/bin/sh","/bin/sh","-c",s,(char*)0);
    fprintf(stderr,"Cannot exec\n");
    _exit(1);
  }
}

static void set_palette(char*s) {
  int p=palet<<3;
  FILE*f=fmemopen(s,strlen(s),"r");
  if(!f) err(1,"Error with fmemopen during parsing command-line arguments");
  while(p<64 && fscanf(f,"%2hhX%2hhX%2hhX",&colors[p].r,&colors[p].g,&colors[p].b)) {
    p++;
    fscanf(f,"%*[, ]");
  }
  fclose(f);
}

static void set_script(char*s) {
  static size_t siz;
  Script o={0,0,0,0,0};
  if(!scriptf) {
    scriptf=open_memstream((char**)&scriptv,&siz);
    if(!scriptf) err(1,"Error with open_memstream during parsing command-line arguments");
  }
  if(*s=='@') {
    char*line=0;
    size_t line_size=0;
    FILE*fp=fopen(s+1,"r");
    if(!fp) err(1,"Cannot open file \"%s\"",s+1);
    while(getline(&line,&line_size,fp)>0) {
      s=line+strlen(line);
      while(s>line && (s[-1]=='\r' || s[-1]=='\n' || s[-1]==' ' || s[-1]=='\t')) *--s=0;
      if(*s=='@') errx(1,"Nested file in -m not allowed");
      if(*s && *s!='#') set_script(s);
    }
    fclose(fp);
    free(line);
  } else {
    Uint8 b;
    if(*s=='.') o.stage=1,s++;
    if(*s>'f') o.kind=*s++;
    o.addr=strtoll(s,&s,16);
    if(*s=='=') {
      s++;
      if(strlen(s)>=0x1FFFF) errx(1,"Argument too long");
      o.data=malloc(strlen(s)/2+1);
      if(!o.data) err(1,"Allocation failed");
      while(*s && s[1]) {
        if(!sscanf(s,"%2hhX",&b)) break;
        o.data[o.len++]=b;
        s+=2;
      }
      if(*s) errx(1,"Syntax error in -m");
    } else if(*s) {
      errx(1,"Syntax error in -m");
    }
    if(o.kind || o.len) {
      if(!fwrite(&o,sizeof(Script),1,scriptf)) err(1,"Cannot write to memory stream");
      ++scriptc;
    }
  }
}

static void do_script(Uint8 stage) {
  Uint8 u=use_extension;
  Uint16 n,i,j;
  Script*o;
  use_extension=1;
  for(n=0;n<scriptc;n++) if(scriptv[n].stage==stage) {
    o=scriptv+n;
    switch(o->kind) {
      case 0: // Memory write
        memcpy(mem+o->addr,o->data,o->len);
        break;
      case 'p': // Device port (including trigger)
        j=o->addr;
        for(i=0;i<o->len;i++) {
          device[(j>>4)&15].d[j&15]=o->data[i];
          device[(j>>4)&15].out(device+((j>>4)&15),j&15);
          j++;
        }
        break;
      case 's': // Device port (set only)
        j=o->addr;
        for(i=0;i<o->len;i++) {
          device[(j>>4)&15].d[j&15]=o->data[i];
          j++;
        }
        break;
      case 'u': // Extension set
        mem[o->addr]=3;
        memcpy(mem+o->addr+1,o->data,o->len);
        do_extension_by_uuid(o->addr);
        break;
      case 'x': // Execute
        run(o->addr);
        break;
      case 'z': // Others
        if(o->addr==2) {
          if(o->len) screencompat=o->data[0];
          blend[0][0]=(screencompat&0x01?4:0);
        } else {
          errx(1,"Unrecognized command -mz%X",o->addr);
        }
        break;
      default: errx(1,"Unrecognized command -m%c",o->kind);
    }
  }
  use_extension=u;
}

int main(int argc,char**argv) {
  int i,j;
  for(i=0;i<16;i++) {
    device[i].in=default_in;
    device[i].out=default_out;
    device[i].exp=default_exp;
  }
  device[0].in=system_in;
  device[0].out=system_out;
  device[0].exp=system_exp;
  device[10].aux=&uxnfile0;
  device[11].aux=&uxnfile1;
  device[12].in=datetime_in;
  inf=stdin; outf=stdout;
  while((i=getopt(argc,argv,"+ADFIJ:NOQST:YZa:de:h:ijm:np:qs:t:w:xyz:"))>0) switch(i) {
    case 'D': scrflags|=SDL_DOUBLEBUF; break;
    case 'F': scrflags|=SDL_FULLSCREEN; break;
    case 'I': use_thread=1; break;
    case 'J': set_joystick(optarg); break;
    case 'N': scrflags|=SDL_NOFRAME; break;
    case 'O': touch_mode=1; device[9].d[2]=device[9].d[4]=0x80; break;
    case 'Q': use_mouse=0; break;
    case 'S': disallow_size_change=1; break;
    case 'T': device[12].in=default_in; set_datetime(optarg); break;
    case 'Y': allow_write=1; device[10].out=device[11].out=files_out; device[10].in=device[11].in=files_in; device[10].exp=device[11].exp=files_exp; break;
    case 'Z': use_utc=1; break;
    case 'a': set_audio(optarg); break;
    case 'd': use_debug=1; break;
    case 'e': set_other_program(optarg); break;
    case 'h': default_height=strtol(optarg,0,10); break;
    case 'i': hide_cursor=1; break;
    case 'j': joypad_repeat=1; break;
    case 'm': set_script(optarg); break;
    case 'n': use_screen=0; break;
    case 'p': palet=strtol(optarg,0,10)&7; if(*optarg && optarg[1]=='=') set_palette(optarg+2); break;
    case 'q': use_console=0; break;
    case 's': scroll_size=strtol(optarg,0,10); break;
    case 't': timer_rate=strtol(optarg,0,10); break;
    case 'w': default_width=strtol(optarg,0,10); break;
    case 'x': use_extension=1; break;
    case 'y': allow_write=0; device[10].out=device[11].out=files_out; device[10].in=device[11].in=files_in; device[10].exp=device[11].exp=files_exp; break;
    case 'z': zoom=strtol(optarg,0,10); break;
    default: return 1;
  }
  if(optind==argc) errx(1,"Required argument missing");
  rom_name=argv[optind];
  if(use_console) device[1].out=console_out;
  if(scriptf) {
    fclose(scriptf);
    if(!scriptv) err(1,"Allocation failed");
    do_script(0);
  }
  load_rom();
  if(use_screen) {
    if(zoom<1) errx(1,"Zoom out of range");
    if(default_height<1 || default_width<1) errx(1,"Screen size out of range");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|(audio_option?SDL_INIT_AUDIO:0)|(joy_mask?SDL_INIT_JOYSTICK:0))) errx(1,"Cannot initialize SDL: %s",SDL_GetError());
    atexit(SDL_Quit);
    device[2].out=screen_out;
    scr_w=default_width;
    scr_h=default_height;
    PUT16(device[2].d+2,scr_w);
    PUT16(device[2].d+4,scr_h);
    size_changed=1;
    if(joy_mask) {
      if((joy_mask&1) && !SDL_JoystickOpen(joy_player[0])) errx(1,"Cannot open joystick for player I");
      if((joy_mask&2) && !SDL_JoystickOpen(joy_player[1])) errx(1,"Cannot open joystick for player II");
      if((joy_mask&4) && !SDL_JoystickOpen(joy_player[2])) errx(1,"Cannot open joystick for player III");
      if((joy_mask&8) && !SDL_JoystickOpen(joy_player[3])) errx(1,"Cannot open joystick for player IV");
      SDL_JoystickEventState(SDL_ENABLE);
    }
  }
  if(optind+1<argc) strncpy(uxnfile0.name,argv[optind+1],4095);
  if(optind+2<argc) strncpy(uxnfile1.name,argv[optind+2],4095);
  restart:
  size_changed=1;
  ds.p=rs.p=0;
  device[1].d[7]=(optind+1==argc)?0:1;
  if(scriptc) do_script(1);
  run(0x0100);
  if(device[1].d[0] || device[1].d[1]) for(i=optind+1;i<argc;i++) {
    for(j=0;argv[i][j];j++) {
      device[1].d[2]=argv[i][j];
      device[1].d[7]=2;
      run(GET16(device[1].d));
    }
    device[1].d[2]='\n';
    device[1].d[7]=(i==argc-1?4:3);
    run(GET16(device[1].d));
  }
  if(use_screen) {
    if(size_changed) set_screen_mode(scr_w,scr_h);
    if(use_thread) {
      stdin_cond=SDL_CreateCond();
      SDL_CreateThread(stdin_thread_fn,0);
    }
    while(run_screen()) {
      for(i=0;i<15;i++) for(j=0;j<16;j++) device[i].d[j]=0;
      if(scriptc) do_script(0);
      load_rom();
      goto restart;
    }
  } else if(audio_option) {
    run_audio();
  } else {
    device[1].d[7]=1;
    while((device[1].d[0] || device[1].d[1]) && ((i=fgetc(inf))>=0)) {
      device[1].d[2]=i;
      run(GET16(device[1].d));
    }
    if(use_extension && (device[1].d[6]&1)) {
      device[1].d[2]=device[1].d[7]=0;
      run(GET16(device[1].d));
    }
  }
  return 0;
}

