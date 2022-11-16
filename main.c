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

static const Uint8 blend[4][16]={
  {0,0,0,0,1,4,1,1,2,2,4,2,3,3,3,4},
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

static const char*rom_name;
static Uint8 use_console=1;
static Uint8 use_screen=1;
static Uint8 use_mouse=1;
static Uint8 use_debug=0;
static Uint8 use_utc=0;
static Uint8 use_extension=0;
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

static Sint32*audio_option;
static SDL_AudioSpec audiospec;

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
    PUT16(device[0].d,0);
    ds.p=rs.p=0;
    goto start;
  } else {
    uxnerr(v,pc);
  }
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
    case 15: if(dev->d[15]) exit(0); break;
  }
}

static Uint8 extension_in(Device*dev,Uint8 id) {
  switch(id) {
    case 8: return EXPANDED_MEMORY;
    default: return dev->d[id];
  }
}

static void extension_out(Device*dev,Uint8 id) {
  Uint16 x,y,len;
  switch(id) {
    case 8:
      switch(dev->d[8]>>4) {
        case 0:
          x=GET16(dev->d+2);
          y=GET16(dev->d+4);
          len=GET16(dev->d+6);
          if(x+len>0x10000 || y+len>0x10000 || (dev->d[8]&15)>=EXPANDED_MEMORY) uxnerr(5,x);
          memcpy(mem+y+((dev->d[8]&15)<<16)+0x10000,mem+x,len);
          break;
        case 1:
          x=GET16(dev->d+2);
          y=GET16(dev->d+4);
          len=GET16(dev->d+6);
          if(x+len>0x10000 || y+len>0x10000 || (dev->d[8]&15)>=EXPANDED_MEMORY) uxnerr(5,x);
          memcpy(mem+x,mem+y+((dev->d[8]&15)<<16)+0x10000,len);
          break;
        case 2:
          if(!screen) break;
          if(dev->d[8]&1) memset(bglayer,0,scr_w*scr_h);
          if(dev->d[8]&2) memset(fglayer,0,scr_w*scr_h);
          break;
      }
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

static void files_seek(UxnFile*uf,Uint8 mode,Uint32 len) {
  switch(mode&3) {
    case 0: len=0; break;
    case 1: /* do nothing */ break;
    case 2: len<<=16; break;
    case 3: return;
  }
  switch(uf->mode) {
    case 1: case 2: case 4:
      switch((mode>>2)&3) {
        case 0: fseek(uf->file,len,SEEK_CUR); break;
        case 1: fseek(uf->file,-len,SEEK_CUR); break;
        case 2: fseek(uf->file,len,SEEK_SET); break;
        case 3: fseek(uf->file,-len,SEEK_END); break;
      }
      break;
  }
}

static int files_set(Device*dev,int rw) {
  // rw: 0(close) 1(read) 2(write)
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
  if(m && !use_extension) m=1;
  if((m&15)==2 && rw && allow_write) {
    if(rw==1) m&=15;
    if(uf->file=fopen(uf->name,m&16?"w+x":"r+")) {
      uf->mode=4;
      return 1;
    } else if(uf->file=fopen(uf->name,m&16?"w+x":"w+")) {
      uf->mode=4;
      return 1;
    } else {
      warn("Cannot open file '%s' for reading and writing",uf->name);
    }
  } else if(rw==1) {
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
    if(uf->file=fopen(uf->name,m==1?"a":m==17?"ax":m==16?"wx":"w")) {
      uf->mode=2;
      return 1;
    } else {
      warn("Cannot open file '%s' for writing",uf->name);
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
      y=files_stat(uf->name,len,mem+addr);
      if(y<=len) PUT16(dev->d+2,y);
      break;
    case 6:
      if(!use_extension) break;
      dev->d[2]=dev->d[3]=0;
      switch(dev->d[6]) {
        case 0x10 ... 0x12: // Close/open
          if(dev->d[6]==0x12 && !allow_write) goto disallow;
          dev->d[3]=files_set(dev,dev->d[6]&0x0F);
          break;
        case 0x80 ... 0x8F: // Seek
          files_seek(uf,dev->d[6]&0x0F,len);
          break;
      }
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
        //TODO
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
          case SDLK_F2: if(audio_option) audio_option['m'-'a']^=1; break;
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
  while((i=getopt(argc,argv,"+ABDFNQT:YZa:dh:inp:qt:w:xyz:"))>0) switch(i) {
    case 'B': bicycle=1; device[0].in=default_in; break;
    case 'D': scrflags|=SDL_DOUBLEBUF; break;
    case 'F': scrflags|=SDL_FULLSCREEN; break;
    case 'N': scrflags|=SDL_NOFRAME; break;
    case 'Q': use_mouse=0; break;
    case 'T': device[12].in=default_in; set_datetime(optarg); break;
    case 'Y': allow_write=1; device[10].out=device[11].out=files_out; device[10].in=device[11].in=files_in; break;
    case 'Z': use_utc=1; break;
    case 'a': set_audio(optarg); break;
    case 'd': use_debug=1; break;
    case 'h': default_height=strtol(optarg,0,10); break;
    case 'i': hide_cursor=1; break;
    case 'n': use_screen=0; break;
    case 'p': palet=strtol(optarg,0,10)&7; break;
    case 'q': use_console=0; break;
    case 't': timer_rate=strtol(optarg,0,10); break;
    case 'w': default_width=strtol(optarg,0,10); break;
    case 'x': use_extension=1; device[15].in=extension_in; device[15].out=extension_out; break;
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
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|(audio_option?SDL_INIT_AUDIO:0))) errx(1,"Cannot initialize SDL: %s",SDL_GetError());
    atexit(SDL_Quit);
    device[2].out=screen_out;
    scr_w=default_width;
    scr_h=default_height;
    PUT16(device[2].d+2,scr_w);
    PUT16(device[2].d+4,scr_h);
    size_changed=1;
  }
  if(use_console) device[1].out=console_out;
  if(device[12].d[15]) device[12].d[15]=0; else device[12].in=datetime_in;
  if(optind+1<argc) strncpy(uxnfile0.name,argv[optind+1],4095);
  if(optind+2<argc) strncpy(uxnfile1.name,argv[optind+2],4095);
  restart:
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
    if(size_changed) set_screen_mode(scr_w,scr_h);
    while(run_screen()) {
      for(i=0;i<15;i++) for(j=0;j<16;j++) device[i].d[j]=0;
      load_rom();
      goto restart;
    }
  } else if(audio_option) {
    run_audio();
  } else {
    while((device[1].d[0] || device[1].d[1]) && ((i=getchar())>=0)) {
      device[1].d[2]=i;
      run(GET16(device[1].d));
    }
    if(device[15].out==extension_out) run(GET16(device[15].d));
  }
  return 0;
}

