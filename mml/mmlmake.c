#if 0
gcc -s -O2 -o ./mmlmake mmlmake.c
exit
#endif

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define OP_REST 0x03
#define OP_WAIT 0x05
#define OP_INSTRUMENT 0x07
#define OP_GOTO 0x09
#define OP_LOOPSTART 0x0B
#define OP_LOOPEND 0x0D
#define OP_VOLUME 0x0F

typedef struct {
  FILE*file;
  char*data;
  size_t size;
  unsigned short loop;
  unsigned short address;
} Track;

typedef struct {
  const char*name;
  void(*call)(char*);
} Command;

typedef struct {
  char*data;
  size_t size;
  unsigned short address;
  unsigned char env[2];
  char loop;
} Instrument;

static const signed char notes[12]={9,11,0,2,4,5,7};

static unsigned char*driver_data;
static size_t driver_size;
static int divisions=120;
static char stereo=0;
static Track tracks[4];
static Instrument instrum[128];
static int cur_op=0;
static int cur_time=0;
static int cur_track;
static int rested=0;
static int total_time=0;
static char debug=0;

static void cmd_debug(char*arg) {
  debug=1;
}

static void cmd_divisions(char*arg) {
  divisions=strtol(arg,&arg,10);
  if(!divisions || (*arg!='\r' && *arg!='\n' && *arg)) errx(1,"Improper argument for #DIVISIONS");
}

static void cmd_driver(char*arg) {
  FILE*fp=fopen(arg,"rb");
  if(!fp) err(1,"Cannot open driver file (%s)",arg);
  fseek(fp,0,SEEK_END);
  driver_size=ftell(fp);
  if(driver_size<32 || driver_size>0xFF00) errx(1,"Improper size of driver file");
  fseek(fp,0,SEEK_SET);
  driver_data=malloc(driver_size);
  if(!driver_data) err(1,"Allocation failed");
  if(!fread(driver_data,driver_size,1,fp)) err(1,"Error reading driver file");
  fclose(fp);
  if(memcmp(driver_data,"\xA0\x00\x00\xA0\xF8\x00\xA0\x00\xFA\xA0\x00\x00\xA0\xFC\x00\xA0\x00\xFE\x31\x31\x31\x31",0x16)) errx(1,"Wrong driver file");
}

static void cmd_stereo(char*arg) {
  stereo=strtol(arg,&arg,10);
  if(stereo<0 || stereo>15 || (*arg!='\r' && *arg!='\n' && *arg)) errx(1,"Improper argument for #STEREO");
}

static void cmd_tempo(char*arg) {
  int x;
  x=strtol(arg,&arg,10);
  if(x>600 || x<12 || (*arg!='\r' && *arg!='\n' && *arg)) errx(1,"Improper argument for #TEMPO");
  divisions=14400/x;
}

static const Command commands[]={
  {"DEBUG",cmd_debug},
  {"DIVISIONS",cmd_divisions},
  {"DRIVER",cmd_driver},
  {"STEREO",cmd_stereo},
  {"TEMPO",cmd_tempo},
  {0,0}
};

static void do_command(char*c) {
  int i;
  char*s=strchr(c,' ');
  if(s) *s++=0; else errx(1,"Missing argument for #%s",c);
  for(i=0;commands[i].name;i++) if(!strcmp(commands[i].name,c)) {
    commands[i].call(s);
    return;
  }
  errx(1,"Bad command: #%s",c);
}

static void do_music(char*line) {
  char*s=strchr(line,' ');
  int ch;
  if(!s) errx(1,"Improper music line");
  while(line<s) {
    if(*line<'A' || *line>'D') errx(1,"Improper music line");
    ch=*line++-'A';
    if(!tracks[ch].file) {
      tracks[ch].file=open_memstream(&tracks[ch].data,&tracks[ch].size);
      if(!tracks[ch].file) err(1,"Cannot open memory stream");
    }
    fprintf(tracks[ch].file,"%s ",s);
  }
}

static void do_instrument(char*s) {
  int i;
  int n=strtol(s,&s,10);
  FILE*fp;
  if(n&~127) errx(1,"Instrument number out of range");
  if(instrum[n].data) errx(1,"Instrument %d already defined",n);
  while(*s==' ' || *s=='\t') ++s;
  if(*s=='[') {
    ++s;
    i=strtol(s,&s,10); if(i&~15) errx(1,"Improper envelope");
    instrum[n].env[0]=i>>4;
    i=strtol(s,&s,10); if(i&~15) errx(1,"Improper envelope");
    instrum[n].env[0]|=i;
    i=strtol(s,&s,10); if(i&~15) errx(1,"Improper envelope");
    instrum[n].env[1]=i>>4;
    i=strtol(s,&s,10); if(i&~15) errx(1,"Improper envelope");
    instrum[n].env[1]|=i;
    while(*s==' ' || *s=='\t') ++s;
    if(*s!=']') errx(1,"Improper instrument definition (%d): %s",n,s);
    ++s;
    while(*s==' ' || *s=='\t') ++s;
  }
  if(*s=='$') {
    ++s;
    instrum[n].loop=0x80;
    while(*s==' ' || *s=='\t') ++s;
  }
  if(*s!='=') errx(1,"Improper instrument definition (%d): %s",n,s);
  ++s;
  while(*s==' ' || *s=='\t') ++s;
  fp=fopen(s,"rb");
  if(!fp) err(1,"Cannot open data file for instrument %d",n);
  fseek(fp,0,SEEK_END);
  i=ftell(fp);
  if(i<2 || i>0xFF00) errx(1,"Improper size of data file for instrument %d",n);
  instrum[n].data=malloc(instrum[n].size=i);
  if(!instrum[n].data) err(1,"Allocation failed");
  fseek(fp,0,SEEK_SET);
  if(!fread(instrum[n].data,i,1,fp)) err(1,"Cannot read instrument data");
  fclose(fp);
}

static void do_line(char*line) {
  char*s=line+strlen(line);
  while(s>line && (s[-1]=='\r' || s[-1]=='\n' || s[-1]==' ' || s[-1]=='\t')) *--s=0;
  switch(*line) {
    case 'A': case 'B': case 'C': case 'D': do_music(line); break;
    case '#': do_command(line+1); break;
    case '@': do_instrument(line+1); break;
    case 0: case ';': break;
    default: errx(1,"Bad command: %s",line);
  }
}

static void instrument_address(void) {
  int n;
  int a=driver_size+0x0100;
  for(n=0;n<128;n++) if(instrum[n].data) {
    instrum[n].address=a;
    a+=instrum[n].size+5;
  }
}

static void send_time(void) {
  int i;
  if(!cur_time) return;
  total_time+=cur_time;
  while(cur_time) {
    if(cur_time>255) i=255; else i=cur_time;
    cur_time-=i;
    fputc(cur_op,tracks[cur_track].file);
    fputc(-i,tracks[cur_track].file);
    cur_op=OP_WAIT;
  }
}

static int note_length(char**ss,int n) {
  char*s=*ss;
  int i;
  if(i=strtol(s,&s,10)) n=divisions/i;
  i=n;
  while(*s=='.') n+=i/=2;
  *ss=s;
  if(n<1) errx(1,"Improper note length");
  return n;
}

static char*add_note(char*s,int n,int t) {
  int v=notes[s[-1]-'a']+t;
  int i;
  send_time();
  while(*s=='-') s++,v--;
  while(*s=='+') s++,v++;
  while(*s==',' || *s=='_') s++,v-=12;
  while(*s=='\'') s++,v+=12;
  n=note_length(&s,n);
  if(v<0 || v>127) errx(1,"Note out of range");
  cur_op=v+0x80;
  cur_time=n;
  return s;
}

static char*add_rest(char*s,int n) {
  send_time();
  n=note_length(&s,n);
  cur_op=OP_REST;
  cur_time=n;
  return s;
}

static char*add_wait(char*s,int n) {
  n=note_length(&s,n);
  cur_time+=n;
  return s;
}

static char*set_instrument(char*s,FILE*fp) {
  int n;
  if(*s<'0' || *s>'9') errx(1,"Improper music command (@%c)",*s);
  n=strtol(s,&s,10);
  if(n&~127) errx(1,"Instrument number out of range");
  if(!instrum[n].size || !instrum[n].address) errx(1,"Undefined instrument");
  n=instrum[n].address;
  fputc(OP_INSTRUMENT,fp);
  fputc(n>>8,fp);
  fputc(n&255,fp);
  return s;
}

static char*set_volume(char*s,FILE*fp,int y) {
  int n;
  if(*s<'0' || *s>'9') errx(1,"Improper music command (v%c)",*s);
  n=strtol(s,&s,10);
  if(n&~15) errx(1,"Volume number out of range");
  fputc(OP_VOLUME,fp);
  fputc(n*0x11-(((stereo*n)/15)<<(y?0:4)),fp);
  return s;
}

static void do_track(int ch) {
  int notelen=divisions/4;
  int octave=5;
  int transpose=0;
  int looptime=0;
  Track*tr=tracks+ch;
  char*s;
  if(tr->file) {
    fclose(tr->file);
    s=tr->data;
  } else {
    s="Lw1 ";
  }
  if(!s) err(1,"Unknown error");
  tr->data=0;
  tr->size=0;
  tr->file=open_memstream(&tr->data,&tr->size);
  if(!tr->file) err(1,"Cannot open memory stream");
  cur_track=ch;
  cur_time=total_time=0;
  cur_op=OP_WAIT;
  rested=0;
  while(*s) switch(*s++) {
    case ' ': case '\r': case '\n': case '\t': break;
    case 'a' ... 'g': rested=0; s=add_note(s,notelen,octave*12+transpose); break;
    case 'l': notelen=note_length(&s,notelen); break;
    case 'n': send_time(); cur_op=strtol(s,&s,10)+transpose+0x80; cur_time=notelen; break;
    case 'o': octave=strtol(s,&s,10); if(octave<0 || octave>9) errx(1,"Improper octave (%d)",octave); break;
    case 'r': s=rested?add_wait(s,notelen):add_rest(s,notelen); rested=1; break;
    case 'v': send_time(); s=set_volume(s,tr->file,ch&1); break;
    case 'w': case '^': s=add_wait(s,notelen); break;
    case 'K': transpose=strtol(s,&s,10); break;
    case 'L': send_time(); looptime=total_time; tr->loop=ftell(tr->file); rested=0; break;
    case 'X': send_time(); fputc(strtol(s,&s,0),tr->file); rested=0; break;
    case '<': if(octave) --octave; break;
    case '>': if(octave<9) ++octave; break;
    case '@': send_time(); s=set_instrument(s,tr->file); break;
    case '[': send_time(); rested=0; fputc(OP_LOOPSTART,tr->file); break;
    case ']': send_time(); rested=0; fputc(OP_LOOPEND,tr->file); fputc(strtol(s,&s,10)-2,tr->file); break;
    default: errx(1,"Improper music command (%c)",s[-1]);
  }
  send_time();
  fprintf(stderr,"%c %d %d\n",ch+'A',looptime,total_time);
  fclose(tr->file);
  if(!tr->data) err(1,"Allocation failed");
  if(tr->size<1 || tr->size>=0xFFFF) errx(1,"Too big output size");
}

static void do_output(void) {
  static const char z[8]={1,5,10,14,2,7,11,16};
  int a=driver_size+0x0100;
  int n;
  if(debug) fprintf(stderr,"Data start address = 0x%04X\n",a);
  for(n=0;n<128;n++) if(instrum[n].data) a+=instrum[n].size+5;
  for(n=0;n<4;n++) {
    if(debug) fprintf(stderr,"Track %c address = 0x%04X\n",n+'A',a);
    tracks[n].address=a;
    driver_data[z[n]]=a>>8;
    driver_data[z[n+4]]=a&255;
    a+=tracks[n].size+3;
    if(a<0 || a>0xFFFF) errx(1,"Too big output size");
  }
  if(debug) fprintf(stderr,"Data end address = 0x%04X\n",a);
  fwrite(driver_data,1,driver_size,stdout);
  for(n=0;n<128;n++) if(instrum[n].data) {
    if(debug) fprintf(stderr,"Instrument %d address = 0x%04X\n",n,instrum[n].address);
    fwrite(instrum[n].env,1,2,stdout);
    putchar(instrum[n].size>>8);
    putchar(instrum[n].size&255);
    putchar(instrum[n].loop);
    fwrite(instrum[n].data,1,instrum[n].size,stdout);
  }
  for(n=0;n<4;n++) {
    fwrite(tracks[n].data,1,tracks[n].size,stdout);
    putchar(OP_GOTO);
    a=tracks[n].address+tracks[n].loop;
    putchar(a>>8);
    putchar(a&255);
  }
}

int main(int argc,char**argv) {
  char*linebuf=0;
  size_t linesize=0;
  while(getline(&linebuf,&linesize,stdin)>0) do_line(linebuf);
  if(!driver_size) errx(1,"Missing #DRIVER command");
  free(linebuf);
  instrument_address();
  do_track(0);
  do_track(1);
  do_track(2);
  do_track(3);
  do_output();
  return 0;
}

