#if 0
gcc -s -O2 -Wno-unused-result main.c `sdl-config --cflags --libs`
exit
#endif

// This file is public domain.

#include "SDL.h"
#include <dirent.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

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
  void*aux;
} Device;

typedef struct {
  FILE*file;
  DIR*dir;
  char name[4096];
  struct dirent*de;
  Uint8 mode;
} UxnFile;

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
  {0x18,0xFF,0x18,0},
  {0x18,0x18,0xFF,0},
  {0xFF,0x18,0x18,0},
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
  {0x00,0x00,0x00,0},
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

static const Uint8 blend[4][16]={
  {0,0,0,0,1,4,1,1,2,2,4,2,3,3,3,4},
  {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3},
  {1,2,3,1,1,2,3,1,1,2,3,1,1,2,3,1},
  {2,3,1,2,2,3,1,2,2,3,1,2,2,3,1,2},
};

static Uint8 mem[0x30000];
static Stack ds,rs;
static Device device[16];
static Uint16 scr_w,scr_h;
static Uint8*bglayer;
static Uint8*fglayer;
static UxnFile uxnfile0;
static UxnFile uxnfile1;

static const char*rom_name;
static Uint8 use_console=1;
static Uint8 use_screen=1;
static Uint8 use_mouse=1;
static Uint8 use_debug=0;
static Uint8 use_utc=0;
static Uint8 hide_cursor=0;
static Uint8 allow_write=0;
static Uint8 zoom=1;
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
static Uint8 bicycle=0;

static void debug(int i) {
  if(i) fprintf(stderr,"Debug(%02X)\n",i);
  fprintf(stderr,"ds:");
  for(i=0;i<ds.p;i++) fprintf(stderr," %02X",ds.d[i]);
  fputc('\n',stderr);
  fprintf(stderr,"rs:");
  for(i=0;i<rs.p;i++) fprintf(stderr," %02X",rs.d[i]);
  fputc('\n',stderr);
}

static void uxnerr(int n,int pc) {
  if(use_debug) debug(-1);
  fflush(stdout);
  fflush(stderr);
  errx(2,"Uxn error %d at 0x%04X",n,pc);
}

#define Push8(z) ({ if(s->p>254) {v=2; goto error;} s->d[s->p++]=(z); })
#define Push16(z) ({ if(s->p>253) {v=2; goto error;} s->d[s->p++]=(z)>>8; s->d[s->p++]=(z); })
#define Push(z) ({ if(s->p>(v&0x20?253:254)) {v=2; goto error;} if(v&0x20) s->d[s->p++]=(z)>>8; s->d[s->p++]=(z); })
#define Pop8(z) ({ if(!k) {v=1; goto error;} z=s->d[--k]; if(v<0x80) s->p=k; })
#define Pop16(z) ({ if(k<2) {v=1; goto error;} z=s->d[--k]; z|=s->d[--k]<<8; if(v<0x80) s->p=k; })
#define Pop(z) ({ if(v&0x20) Pop16(z); else Pop8(z); })

static void run(Uint16 pc) {
  Stack*s;
  Uint8 k,v;
  Uint16 x,y,z;
  if(!pc) return;
  start:
  while(v=mem[pc++]) {
    // if(use_debug) printf("! pc=%04X v=%02X ds=%02X rs=%02X\n",pc-1,v,ds.p,rs.p);
    s=(v&0x40?&rs:&ds);
    k=s->p;
    switch(v&0x1F) {
      case 0x00: Push8(mem[pc++]); if(v&0x20) Push8(mem[pc++]); break;
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
      case 0x1B: Pop(x); Pop(y); if(!x) {v=3; goto error;} Push(y/x); break;
      case 0x1C: Pop(x); Pop(y); Push(y&x); break;
      case 0x1D: Pop(x); Pop(y); Push(y|x); break;
      case 0x1E: Pop(x); Pop(y); Push(y^x); break;
      case 0x1F: Pop8(x); Pop(y); z=x>>4; x&=15; Push((y>>x)<<z); break;
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
  fflush(stdout);
  if(bicycle) {
    if(k=device[0].d[2]) {
      memcpy(mem+(k<<8),ds.d,254);
      mem[(k<<8)+0xFF]=ds.p;
    }
    if(k=device[0].d[3]) {
      memcpy(mem+(k<<8),rs.d,254);
      mem[(k<<8)+0xFF]=rs.p;
    }
  }
  return;
  error:
  if(bicycle && (device[0].d[0] || device[0].d[1])) {
    if(device[0].d[2]) mem[(device[0].d[2]<<8)+0xFE]=v; else fprintf(stderr,"Uxn error %d at 0x%04X\n",v,pc);
    pc=GET16(device[0].d);
    ds.p=rs.p=0;
    goto start;
  } else {
    uxnerr(v,pc);
  }
}

static void load_rom(void) {
  FILE*fp=fopen(rom_name,"r");
  if(!fp) err(1,"Cannot load program file");
  memset(mem,0,0x30000);
  fread(mem+0x0100,1,0x30000-0x0100,fp);
  if(ferror(fp)) err(1,"Cannot load program file");
  fclose(fp);
}

static Uint8 default_in(Device*misc1,Uint8 misc2) { return misc1->d[misc2&15]; }
static void default_out(Device*misc1,Uint8 misc2) {}

static Uint8 system_in(Device*dev,Uint8 id) {
  switch(id) {
    case 2: return ds.p;
    case 3: return rs.p;
    default: return dev->d[id];
  }
}

static void system_out(Device*dev,Uint8 id) {
  switch(id) {
    case 2:
      if(bicycle) {
        if(dev->d[2]) {
          memcpy(ds.d,mem+(dev->d[2]<<8),256);
          ds.p=mem[(dev->d[2]<<8)+0xFF];
        } else {
          ds.p=0;
        }
      } else {
        ds.p=dev->d[2];
      }
      break;
    case 3:
      if(bicycle) {
        if(dev->d[3]) {
          memcpy(ds.d,mem+(dev->d[3]<<8),256);
          rs.p=mem[(dev->d[3]<<8)+0xFF];
        } else {
          rs.p=0;
        }
      } else {
        rs.p=dev->d[3];
      }
      break;
    case 8 ... 13:
      if(!screen) break;
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
      if(palet==7) {
        SDL_SetColors(screen,colors+palet*8,0,8);
        picture_changed=1;
      }
      break;
    case 14: if(dev->d[14] && use_debug) debug(dev->d[14]); break;
    case 15: if(dev->d[15]) exit(0); break;
  }
}

static void extension_out(Device*dev,Uint8 id) {
  Uint8*p;
  Uint8*q;
  Uint16 x,y;
  Uint16 len;
  switch(id) {
    case 8:
      if((dev->d[8]>>4)==0) {
        x=GET16(dev->d+2);
        y=GET16(dev->d+4);
        len=GET16(dev->d+6);
        if(dev->d[8]&4) len=256;
        if(x+len>0x10000 || y+len>0x10000) uxnerr(5,x);
        *(dev->d[8]&1?&p:&q)=mem+x;
        *(dev->d[8]&1?&q:&p)=mem+y+(dev->d[8]&2?0x20000:0x10000);
        memcpy(q,p,len);
      }
      dev->d[8]=0;
      break;
    case 11: case 13: case 15:
      dev->d[9]=0;
      break;
  }
}

static void console_out(Device*dev,Uint8 id) {
  switch(id) {
    case 8: putchar(dev->d[8]); break;
    case 9: fputc(dev->d[9],stderr); break;
  }
}

static Uint16 files_stat(char*name,Uint16 len,Uint8*out) {
  struct stat st;
  if(strlen(name)+7>len || stat(name,&st)) return 0;
  if(st.st_size<0x10000) return snprintf(out,len,"%04x %s\n",(unsigned int)st.st_size,name);
  else return snprintf(out,len,"???? %s\n",name);
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
      y=files_stat(uf->name,len,mem+addr);
      if(y<=len) PUT16(dev->d+2,y);
      break;
    case 9:
      addr=GET16(dev->d+8);
      if(uf->file) fclose(uf->file);
      if(uf->dir) closedir(uf->dir);
      uf->file=0;
      uf->dir=0;
      uf->de=0;
      uf->mode=0;
      dev->d[2]=dev->d[3]=0;
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
      if(uf->mode==1) {
        if(!len) break;
        x=fread(mem+addr,1,len,uf->file);
        PUT16(dev->d+2,x);
        if(ferror(uf->file)) {
          warn("Error reading file");
          clearerr(uf->file);
        }
      } else if(uf->mode==3) {
        //TODO
      } else {
        if(uf->file) fclose(uf->file);
        if(uf->dir) closedir(uf->dir);
        uf->file=0;
        uf->dir=0;
        uf->de=0;
        if(!uf->name[0]) break;
        if(uf->name[0]=='.' && !uf->name[1] && (uf->dir=opendir(uf->name))) {
          uf->mode=3;
          goto read;
        } else if(uf->file=fopen(uf->name,"r")) {
          uf->mode=1;
          goto read;
        } else {
          warn("Cannot open file for reading");
        }
      }
      break;
    case 15:
      addr=GET16(dev->d+14);
      dev->d[2]=dev->d[3]=0;
      if(!allow_write) break;
      write:
      //TODO
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
      if(uf->mode==1) {
        c=fgetc(uf->file);
        if(c==EOF) {
          if(ferror(uf->file)) {
            warn("Error reading file");
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
        if(uf->file) fclose(uf->file);
        if(uf->dir) closedir(uf->dir);
        uf->file=0;
        uf->dir=0;
        uf->de=0;
        if(!uf->name[0]) break;
        if(uf->name[0]=='.' && !uf->name[1] && (uf->dir=opendir(uf->name))) {
          uf->mode=3;
          goto read;
        } else if(uf->file=fopen(uf->name,"r")) {
          uf->mode=1;
          goto read;
        } else {
          warn("Cannot open file for reading");
        }
      }
      return 0;
    default: return dev->d[id];
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
  SDL_SetColors(screen,colors+palet*8,0,8);
  bglayer=calloc(w*h,2);
  if(!bglayer) err(1,"Memory allocation error");
  fglayer=bglayer+w*h;
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
  int i;
  Uint16 w;
  Uint16 x=GET16(dev->d+8);
  Uint16 y=GET16(dev->d+10);
  Uint8 z=dev->d[id];
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
      if(x<scr_w && y<scr_h) (z&0x40?fglayer:bglayer)[x+y*scr_w]=z&3;
      if(dev->d[6]&0x01) PUT16(dev->d+8,x+1);
      if(dev->d[6]&0x02) PUT16(dev->d+10,y+1);
      break;
    case 15:
      if(size_changed) set_screen_mode(scr_w,scr_h);
      picture_changed=1;
      w=GET16(dev->d+12);
      for(i=0;i<=(dev->d[6]>>4);i++) {
        draw_sprite(x+i*(dev->d[6]&0x02?8:0),y+i*(dev->d[6]&0x01?8:0),w,z);
        if(dev->d[6]&0x04) w+=z&0x80?16:8;
      }
      if(dev->d[6]&0x01) PUT16(dev->d+8,x+8);
      if(dev->d[6]&0x02) PUT16(dev->d+10,y+8);
      if(dev->d[6]&0x04) PUT16(dev->d+12,w);
      break;
  }
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

static void redraw(void) {
  int a,r,u,v,x,y;
  Uint8*p;
  picture_changed=0;
  SDL_LockSurface(screen);
  a=0;
  p=screen->pixels;
  r=screen->pitch;
  if(layers) {
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

static void set_cursor(void) {
  if(hide_cursor) SDL_ShowCursor(paused || !use_mouse);
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
          case SDLK_F1: run(GET16(device[2].d)); timer_expired=0; break;
          case SDLK_F3: use_mouse^=1; set_cursor(); break;
          case SDLK_F4: layers=(layers+1)%3; redraw(); break;
          case SDLK_F5: redraw(); if(SDL_GetModState()&KMOD_RSHIFT) debug(-1); break;
          case SDLK_F6: palet=(palet+1)&7; SDL_SetColors(screen,colors+palet*8,0,8); redraw(); break;
          case SDLK_F7: palet=(palet-1)&7; SDL_SetColors(screen,colors+palet*8,0,8); redraw(); break;
          case SDLK_F9: return 1;
          case SDLK_F10: return 0;
          case SDLK_PAUSE: paused^=1; set_cursor(); break;
        }
        if(paused) break;
        if(i) {
          if(!(device[8].d[2]&i)) {
            device[8].d[2]|=i;
            run(GET16(device[8].d));
          }
        } else if(i=e.key.keysym.unicode) {
          device[8].d[3]=i;
          run(GET16(device[8].d));
          device[8].d[3]=0;
        }
        break;
      case SDL_KEYUP:
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
        if(paused) break;
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
        PUT16(device[9].d+2,i);
        PUT16(device[9].d+4,j);
        device[9].d[6]=k&7;
        run(GET16(device[9].d));
        break;
    }
    if(picture_changed) redraw();
  }
  return 0;
}

static void set_datetime(char*s) {
  int i=2;
  i=strtol(s,&s,10);
  PUT16(device[12].d,i);
  if(*s) s++;
  i=2;
  while(*s && i<15) {
    device[12].d[i++]=strtol(s,&s,10);
    if(*s) s++;
  }
}

int main(int argc,char**argv) {
  int i,j;
  for(i=0;i<16;i++) {
    device[i].in=default_in;
    device[i].out=default_out;
  }
  device[0].in=system_in;
  device[0].out=system_out;
  device[10].aux=&uxnfile0;
  device[11].aux=&uxnfile1;
  device[12].in=datetime_in;
  while((i=getopt(argc,argv,"+ABDFNQT:YZdh:inp:qt:w:xyz:"))>0) switch(i) {
    case 'B': bicycle=1; device[0].in=default_in; break;
    case 'D': scrflags|=SDL_DOUBLEBUF; break;
    case 'F': scrflags|=SDL_FULLSCREEN; break;
    case 'N': scrflags|=SDL_NOFRAME; break;
    case 'Q': use_mouse=0; break;
    case 'T': device[12].in=default_in; set_datetime(optarg); break;
    case 'Y': allow_write=1; device[10].out=device[11].out=files_out; device[10].in=device[11].in=files_in; break;
    case 'Z': use_utc=1; break;
    case 'd': use_debug=1; break;
    case 'h': default_height=strtol(optarg,0,10); break;
    case 'i': hide_cursor=1; break;
    case 'n': use_screen=0; break;
    case 'p': palet=strtol(optarg,0,10)&7; break;
    case 'q': use_console=0; break;
    case 't': timer_rate=strtol(optarg,0,10); break;
    case 'w': default_width=strtol(optarg,0,10); break;
    case 'x': device[15].out=extension_out; break;
    case 'y': allow_write=0; device[10].out=device[11].out=files_out; device[10].in=device[11].in=files_in; break;
    case 'z': zoom=strtol(optarg,0,10); break;
    default: return 1;
  }
  if(optind==argc) errx(1,"Required argument missing");
  rom_name=argv[optind];
  load_rom();
  if(use_screen) {
    if(zoom<1) errx(1,"Zoom out of range");
    if(default_height<1 || default_width<1) errx(1,"Screen size out of range");
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER)) errx(1,"Cannot initialize SDL: %s",SDL_GetError());
    atexit(SDL_Quit);
    set_screen_mode(default_width,default_height);
    SDL_EnableUNICODE(1);
    device[2].out=screen_out;
    PUT16(device[2].d+2,default_width);
    PUT16(device[2].d+4,default_height);
    if(timer_rate) SDL_SetTimer(timer_rate,timer_callback);
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY,SDL_DEFAULT_REPEAT_INTERVAL);
    SDL_WM_SetCaption(rom_name,rom_name);
    set_cursor();
  }
  if(use_console) device[1].out=console_out;
  if(device[12].d[15]) device[12].d[15]=0; else device[12].in=datetime_in;
  if(optind+1<argc) strncpy(uxnfile0.name,argv[optind+1],4095);
  if(optind+2<argc) strncpy(uxnfile1.name,argv[optind+2],4095);
  run(0x0100);
  if(device[1].d[0] || device[1].d[1]) for(i=optind+1;i<argc;i++) {
    for(j=0;argv[i][j];j++) {
      device[1].d[2]=argv[i][j];
      run(GET16(device[1].d));
    }
    device[1].d[2]='\n';
    run(GET16(device[1].d));
  }
  if(device[15].out==extension_out) run(GET16(device[15].d));
  if(use_screen) {
    while(run_screen()) {
      for(i=0;i<15;i++) for(j=0;j<16;j++) device[i].d[j]=0;
      load_rom();
      PUT16(device[2].d+2,default_width);
      PUT16(device[2].d+4,default_height);
      set_screen_mode(default_width,default_height);
      run(0x0100);
    }
  } else {
    while((device[1].d[0] || device[1].d[1]) && ((i=getchar())>=0)) {
      device[1].d[2]=i;
      run(GET16(device[1].d));
    }
  }
  return 0;
}

