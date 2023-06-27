#if 0
gcc -s -O2 -o ./uxnasm uxnasm.c
exit
#endif

#include <stdio.h>
#include <string.h>

/*
Copyright (c) 2021-2023 Devine Lu Linvega, Andrew Alderwick

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE.

[This file has been modified from the original that the above copyright
notice applies to. The same license conditions apply to the modified
version, although the person who modified it is not the person who is
named in the above copyright notice.]
*/

#define MAXLABELS 0x400
#define MAXMACROS 0x100
#define MAXREFS 0x800
#define MAXEXPS 0x80
#define MAXITEMS 0x40

#define TRIM 0x0100
#define LENGTH 0x10000

typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;
typedef unsigned int Uint32;

typedef struct {
	char name[0x40], items[MAXITEMS][0x40];
	Uint8 len;
} Macro;

typedef struct {
	char name[0x40];
	Uint16 addr, refs;
} Label;

typedef struct {
	char name[0x40], rune;
	Uint16 addr;
} Reference;

typedef struct {
	char name[0x40];
	Uint32 offset,len;
	Uint8 kind;
} ExpItem;

typedef struct {
	Uint8 data[LENGTH];
	unsigned int ptr, length;
	Uint16 llen, mlen, rlen, xlen;
	Label labels[MAXLABELS];
	Macro macros[MAXMACROS];
	Reference refs[MAXREFS];
	ExpItem exps[MAXEXPS];
	char scope[0x40];
	Uint32 total;
} Program;

static Program p;
static char line_comment;
static int ssptr;
static Uint16 sstack[256];
static int bsptr,bcnt;
static Uint16 bstack[256];
static char spec_rune[128];
static Uint16 exstart;
static FILE*subas;

/* clang-format off */

static char ops[][4] = {
	"LIT", "INC", "POP", "NIP", "SWP", "ROT", "DUP", "OVR",
	"EQU", "NEQ", "GTH", "LTH", "JMP", "JCN", "JSR", "STH",
	"LDZ", "STZ", "LDR", "STR", "LDA", "STA", "DEI", "DEO",
	"ADD", "SUB", "MUL", "DIV", "AND", "ORA", "EOR", "SFT"
};

static int   scmp(char *a, char *b, int len) { int i = 0; while(a[i] == b[i]) if(!a[i] || ++i >= len) return 1; return 0; } /* string compare */
static int   sihx(char *s) { int i = 0; char c; while((c = s[i++])) if(!(c >= '0' && c <= '9') && !(c >= 'a' && c <= 'f')) return 0; return i > 1; } /* string is hexadecimal */
static int   shex(char *s) { int n = 0, i = 0; char c; while((c = s[i++])) if(c >= '0' && c <= '9') n = n * 16 + (c - '0'); else if(c >= 'a' && c <= 'f') n = n * 16 + 10 + (c - 'a'); return n; } /* string to num */
static int   slen(char *s) { int i = 0; while(s[i]) i++; return i; } /* string length */
static int   spos(char *s, char c) { Uint8 i = 0, j; while((j = s[i++])) if(j == c) return i; return -1; } /* character position */
static char *scpy(char *src, char *dst, int len) { int i = 0; while((dst[i] = src[i]) && i < len - 2) i++; dst[i + 1] = '\0'; return dst; } /* string copy */
static char *scat(char *dst, const char *src) { char *ptr = dst + slen(dst); while(*src) *ptr++ = *src++; *ptr = '\0'; return dst; } /* string cat */

/* clang-format on */

static int parse(char *w, FILE *f);

static int
error(const char *name, const char *msg)
{
	fprintf(stderr, "%s: %s\n", name, msg);
	return 0;
}

static char *
sublabel(char *src, char *scope, char *name)
{
	return scat(scat(scpy(scope, src, 0x40), "/"), name);
}

static Macro *
findmacro(char *name)
{
	int i;
	for(i = 0; i < p.mlen; i++)
		if(scmp(p.macros[i].name, name, 0x40))
			return &p.macros[i];
	return NULL;
}

static Label *
findlabel(char *name)
{
	int i;
	for(i = 0; i < p.llen; i++)
		if(scmp(p.labels[i].name, name, 0x40))
			return &p.labels[i];
	return NULL;
}

static Uint8
findopcode(char *s)
{
	int i;
	for(i = 0; i < 0x20; i++) {
		int m = 0;
		if(!scmp(ops[i], s, 3))
			continue;
		if(!i) i |= (1 << 7); /* force keep for LIT */
		while(s[3 + m]) {
			if(s[3 + m] == '2')
				i |= (1 << 5); /* mode: short */
			else if(s[3 + m] == 'r')
				i |= (1 << 6); /* mode: return */
			else if(s[3 + m] == 'k')
				i |= (1 << 7); /* mode: keep */
			else
				return 0; /* failed to match */
			m++;
		}
		return i;
	}
	return 0;
}

static int
makemacro(char *name, FILE *f)
{
	Macro *m;
	char word[0x40];
	if(findmacro(name))
		return error("Macro duplicate", name);
	if(sihx(name) && slen(name) % 2 == 0)
		return error("Macro name is hex number", name);
	if(findopcode(name) || scmp(name, "BRK", 4) || !slen(name))
		return error("Macro name is invalid", name);
	if(p.mlen == MAXMACROS)
		return error("Macros limit exceeded", name);
	m = &p.macros[p.mlen++];
	scpy(name, m->name, 0x40);
	while(fscanf(f, "%63s", word) == 1) {
		if(word[0] == '{') continue;
		if(word[0] == '}') break;
		if(word[0] == '%' || line_comment)
			return error("Macro error", name);
		if(m->len >= MAXITEMS)
			return error("Macro size exceeded", name);
		scpy(word, m->items[m->len++], 0x40);
	}
	return 1;
}

static int
makelabel(char *name)
{
	Label *l;
	if(findlabel(name))
		return error("Label duplicate", name);
	if(sihx(name) && (slen(name) == 2 || slen(name) == 4))
		return error("Label name is hex number", name);
	if(findopcode(name) || scmp(name, "BRK", 4) || !slen(name))
		return error("Label name is invalid", name);
	if(p.llen == MAXLABELS)
		return error("Labels limit exceeded", name);
	l = &p.labels[p.llen++];
	l->addr = p.ptr;
	l->refs = 0;
	scpy(name, l->name, 0x40);
	return 1;
}

static int
makereference(char *scope, char *label, Uint16 addr)
{
	char subw[0x40], parent[0x40];
	Reference *r;
	if(p.rlen == MAXREFS)
		return error("References limit exceeded", label);
	r = &p.refs[p.rlen++];
	if(label[1] == '&' && scope)
		scpy(sublabel(subw, scope, label + 2), r->name, 0x40);
	else {
		int pos = spos(label + 1, '/');
		if(pos > 0) {
			Label *l;
			if((l = findlabel(scpy(label + 1, parent, pos))))
				l->refs++;
		}
		scpy(label + 1, r->name, 0x40);
	}
	r->rune = label[0];
	r->addr = addr;
	return 1;
}

static int makeexp(Uint8 kind,char*name,Uint32 len,Uint8 misc) {
  ExpItem*e;
  if(p.xlen==MAXEXPS) return error("Expansion limit exceeded","");
  e=p.exps+(p.xlen++);
  if(name) scpy(name,e->name,0x40); else e->name[0]=misc,e->name[1]=0;
  e->kind=kind;
  e->len=len;
  return 1;
}

static int
writebyte(Uint8 b)
{
	if(p.ptr < TRIM)
		return error("Writing in zero-page", "");
	else if(p.ptr > 0xffff)
		return error("Writing after the end of RAM", "");
	else if(p.ptr < p.length)
		return error("Memory overwrite", "");
	p.data[p.ptr++] = b;
	p.length = p.ptr;
	return 1;
}

static int
writeopcode(char *w)
{
	Uint8 res;
	res = writebyte(findopcode(w));
	return res;
}

static int
writeshort(Uint16 s, int lit)
{
	if(lit)
		if(!writebyte(findopcode("LIT2"))) return 0;
	return writebyte(s >> 8) && writebyte(s & 0xff);
}

static int
writelitbyte(Uint8 b)
{
	if(!writebyte(findopcode("LIT"))) return 0;
	if(!writebyte(b)) return 0;
	return 1;
}

static int
doinclude(const char *filename)
{
	FILE *f;
	char w[0x40];
	int c;
	if(!(f = fopen(filename, "r")))
		return error("Include missing", filename);
	while(fscanf(f, "%63s", w) == 1) {
		if(!parse(w, f))
			return error("Unknown token", w);
		if(line_comment) {
		  while((c=fgetc(f))>0 && c!='\n');
		  line_comment=0;
		}
	}
	fclose(f);
	line_comment=0;
	return 1;
}

static int doinclude_binary(const char*filename) {
  FILE*f;
  int c;
  if(!(f = fopen(filename, "r"))) return error("Include missing", filename);
  while((c=fgetc(f))!=EOF) writebyte(c);
  fclose(f);
  return 1;
}

static int special_calc(const char*w) {
  const char*loop=w;
  Uint16 a,b;
  while(*w) switch(*w++) {
    case '_': loop=w+1; break;
    case '0' ... '9': if(ssptr>255) return 0; sstack[ssptr++]=w[-1]-'0'; break;
    case 'a' ... 'f': if(ssptr>255) return 0; sstack[ssptr++]=w[-1]+10-'a'; break;
    case '\'': if(ssptr<2) return 0; a=sstack[--ssptr]; a+=sstack[--ssptr]<<4; sstack[ssptr++]=a; break;
    case '\"': if(ssptr<4) return 0; a=sstack[--ssptr]; a+=sstack[--ssptr]<<4; a+=sstack[--ssptr]<<8; a+=sstack[--ssptr]<<12; sstack[ssptr++]=a; break;
    case '!': if(ssptr<1) return 0; p.ptr=sstack[--ssptr]; break;
    case '@': if(ssptr>255) return 0; sstack[ssptr++]=p.ptr; break;
    case '#': if(ssptr<1) return 0; p.length=sstack[--ssptr]; break;
    case '$': if(ssptr>255) return 0; sstack[ssptr++]=p.length; break;
    case '+': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=a+b; break;
    case '-': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=a-b; break;
    case '*': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=a*b; break;
    case '/': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; if(!b) return 0; sstack[ssptr++]=a/b; break;
    case '%': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; if(!b) return 0; sstack[ssptr++]=a%b; break;
    case '&': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=a&b; break;
    case '|': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=a|b; break;
    case '^': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=a^b; break;
    case '<': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=(b&~15?0:a<<b); break;
    case '>': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=(b&~15?0:a>>b); break;
    case ':': if(ssptr<1 || ssptr>255) return 0; a=sstack[ssptr-1]; sstack[ssptr++]=a; break;
    case ',': if(ssptr<2) return 0; b=sstack[--ssptr]; a=sstack[--ssptr]; sstack[ssptr++]=b; sstack[ssptr++]=a; break;
    case '.': if(ssptr<1) return 0; --ssptr; break;
    case 'D': fprintf(stderr,"Depth #%x\n",ssptr); if(ssptr) fprintf(stderr,"Debug #%x\n",sstack[ssptr-1]); break;
    case 'E': if(ssptr) fprintf(stderr,"User error #%x\n",sstack[ssptr-1]); return 0;
    case 'g': if(ssptr<1) return 0; a=sstack[--ssptr]; sstack[ssptr++]=p.data[a]; break;
    case 'G': if(ssptr<1) return 0; a=sstack[--ssptr]; if(a==0xFFFF) return 0; sstack[ssptr++]=(p.data[a]<<8)|p.data[a+1]; break;
    case 'k': if(ssptr<1) return 0; if(--sstack[ssptr-1]) w=loop; else --ssptr; break;
    case 'o': if(ssptr<1) return 0; if(sstack[ssptr-1]--) w=loop; else --ssptr; break;
    case 'p': if(ssptr<2) return 0; a=sstack[--ssptr]; b=sstack[--ssptr]; p.data[a]=b; break;
    case 'P': if(ssptr<2) return 0; a=sstack[--ssptr]; b=sstack[--ssptr]; if(a==0xFFFF) return 0; p.data[a]=b>>8; p.data[a+1]=b&255; break;
    case 'w': if(ssptr<1) return 0; a=sstack[--ssptr]; writebyte(a); break;
    case 'W': if(ssptr<1) return 0; a=sstack[--ssptr]; writeshort(a,0); break;
    case 'x': if(ssptr<1) return 0; a=sstack[--ssptr]; if(!makeexp(0,0,a,0)) return 0; break;
    case '(': if(ssptr<1) return 0; if(!sstack[--ssptr]) break; while(*w && *w!=')') w++; break;
    case ')': /* do nothing */ break;
    case '[': if(ssptr<1) return 0; if(sstack[--ssptr]) break; while(*w && *w!=']') w++; break;
    case ']': /* do nothing */ break;
    default: fprintf(stderr,"Unrecognized special calculation command: %c\n",w[-1]); return 0;
  }
  return 1;
}

static int make_tile(int h,const char*w) {
  int i,c,v0,v1;
  v0=v1=0;
  for(i=0;i<8;i++) {
    switch(w[i]) {
      case '0': case '.': c=0; break;
      case '1': case '#': c=1; break;
      case '2': case 'X': c=2; break;
      case '3': case '*': c=3; break;
      default: return 0;
    }
    if(c>1 && !h) return 0;
    if(c&1) v0|=0x80>>i;
    if(c&2) v1|=0x80>>i;
  }
  if(p.ptr>0xFFF7) return error("Writing after the end of RAM","");
  p.data[p.ptr]=v0;
  if(h) p.data[p.ptr+8]=v1;
  p.ptr++;
  if(w[8]==';') {
    if(h) p.ptr+=8;
  } else if(w[8]) {
    return 0;
  }
  if(p.length<p.ptr) p.length=p.ptr;
  return 1;
}

static int exp_file(char*w) {
  FILE*f=fopen(w,"rb");
  if(!f) return error("Cannot open file",w);
  fseek(f,0,SEEK_END);
  if(!makeexp(2,w,ftell(f),0)) return 0;
  fclose(f);
  return 1;
}

static int
parse(char *w, FILE *f)
{
	int i;
	char word[0x40], subw[0x40], c;
	Macro *m;
	Label*l;
	if(slen(w) >= 63)
		return error("Invalid token", w);
	if(spec_rune[*w]) goto defa;
	switch(w[0]) {
	case '(': /* comment */
		if(slen(w) != 1) fprintf(stderr, "-- Malformed comment: %s\n", w);
		i = 1; /* track nested comment depth */
		while(fscanf(f, "%63s", word) == 1) {
			if(slen(word) != 1)
				continue;
			else if(word[0] == '(')
				i++;
			else if(word[0] == ')' && --i < 1)
				break;
		}
		break;
	case '~': /* include */
		if(!doinclude(w + 1))
			return error("Invalid include", w);
		break;
	case '%': /* macro */
		spec_rune[w[1]&127]=1;
		if(!makemacro(w + 1, f))
			return error("Invalid macro", w);
		break;
	case '|': /* pad-absolute */
		if(sihx(w + 1))
			p.ptr = shex(w + 1);
		else if(w[1] == '&') {
			if(!sublabel(subw, p.scope, w + 2) || !(l = findlabel(subw)))
				return error("Invalid padding", w);
			p.ptr = l->addr;
		} else {
			if(!(l = findlabel(w + 1)))
				return error("Invalid padding", w);
			p.ptr = l->addr;
		}
		break;
	case '$': /* pad-relative */
		if(sihx(w + 1))
			p.ptr += shex(w + 1);
		else if(w[1] == '&') {
			if(!sublabel(subw, p.scope, w + 2) || !(l = findlabel(subw)))
				return error("Invalid padding", w);
			p.ptr += l->addr;
		} else {
			if(!(l = findlabel(w + 1)))
				return error("Invalid padding", w);
			p.ptr += l->addr;
		}
		break;
	case '@': /* label */
		if(w[1]=='\\') break;
		if(!makelabel(w + 1))
			return error("Invalid label", w);
		scpy(w + 1, p.scope, 0x40);
		break;
	case '&': /* sublabel */
		if(!makelabel(sublabel(subw, p.scope, w + 1)))
			return error("Invalid sublabel", w);
		break;
	case '#': /* literals hex */
		if(!sihx(w + 1) || (slen(w) != 3 && slen(w) != 5))
			return error("Invalid hex literal", w);
		if(slen(w) == 3) {
			if(!writelitbyte(shex(w + 1))) return 0;
		} else if(slen(w) == 5) {
			if(!writeshort(shex(w + 1), 1)) return 0;
		}
		break;
	case '.': /* literal byte zero-page */
		makereference(p.scope, w, p.ptr);
		if(!writelitbyte(0xff)) return 0;
		break;
	case ',': /* literal byte relative */
		makereference(p.scope, w, p.ptr);
		if(!writelitbyte(0xff)) return 0;
		break;
	case ';': /* literal short absolute */
		makereference(p.scope, w, p.ptr);
		if(!writeshort(0xffff, 1)) return 0;
		break;
	case ':': case '=': /* raw short absolute */
		makereference(p.scope, w, p.ptr);
		if(!writeshort(0xffff, 0)) return 0;
		break;
	case '-': /* raw byte zero-page */
		makereference(p.scope, w, p.ptr);
		if(!writebyte(0xff)) return 0;
		break;
	case '_': /* raw byte relative */
		makereference(p.scope, w, p.ptr);
		if(!writebyte(0xff)) return 0;
		break;
	case '"': /* raw string */
		i = 0;
		while((c = w[++i]))
			if(!writebyte(c)) return 0;
		break;
	case '?': /* JCI */
		makereference(p.scope, w, p.ptr + 1);
		return writebyte(0x20) && writeshort(0xffff, 0);
	case '!': /* JMI */
		makereference(p.scope, w, p.ptr + 1);
		return writebyte(0x40) && writeshort(0xffff, 0);
	case '\\': /* specials */
		switch(w[1]) {
		  case '\\': line_comment=1; break;
		  case '~': if(!doinclude_binary(w+2)) return error("Invalid include",w); break;
		  case '.': case ',': makereference(p.scope,w+1,p.ptr-1); if(!writebyte(0xff)) return 0; break;
		  case ':': spec_ref:
		    if(!(l=findlabel(w+2))) return error("Invalid reference",w);
		    if(ssptr==256) return error("Stack overflow",w);
		    sstack[ssptr++]=l->addr;
		    break;
		  case '0' ... '9': case 'a' ... 'f':
		    if(!sihx(w+1)) return error("Invalid hex literal",w);
		    if(ssptr==256) return error("Stack overflow",w);
		    sstack[ssptr++]=shex(w+1);
		    break;
		  case '@':
		    if(!ssptr) return error("Stack underflow",w);
		    i=p.ptr;
		    p.ptr=sstack[--ssptr];
		    if(!makelabel(w+2)) return error("Invalid label",w);
		    p.ptr=i;
		    break;
		  case '_': if(!special_calc(w+2)) return error("Special calculation error",w); break;
		  case '-': case '=': if(!make_tile(w[1]=='=',w+2)) return error("Tile error",w); break;
		  case '`': makereference(0,w+1,p.ptr); break;
		  case '*': if(!makeexp('*',w+2,0,0)) return 0; break;
		  case 'A': case 'C': case 'D':
		    if(!sihx(w+1)) return error("Invalid hex literal",w);
		    if(!makeexp(w[1],w+2,shex(w+2),0)) return 0;
		    break;
		  case 'B': case 'E': case 'F':
		    if(w[2]>='0' && w[2]<='9') i=w[2]-'0';
		    else if(w[2]>='a' && w[2]<='u') i=w[2]+10-'a';
		    else return error("Invalid expanded memory alignment",w);
		    if(!makeexp(w[1],0,0,i)) return 0;
		    break;
		  case '^':
		    if(!exp_file(w+2)) return 0;
		    break;
		  case '+': w[1]='x'; makereference(0,w+1,1); break;
		  default: return error("Invalid special",w);
		}
		break;
	case '^': /* sub-assembly */
		if(!makeexp('^',w+1,0,0)) return 0;
		p.exps[p.xlen-1].len=p.ptr;
		break;
	case '[':
	case ']':
		if(slen(w) == 1) break;
		if((w[1]=='.' || w[1]==',' || w[1]==':' || w[1]==';' || w[1]=='@' || w[1]=='\\' || w[1]=='?' || w[1]=='!' || w[1]=='&') && !w[2]) {
		  if(*w=='[') {
		    if(bsptr==256) return error("Stack overflow",w);
		    bstack[bsptr++]=i=bcnt++;
		  } else if(*w==']') {
		    if(!bsptr) return error("Stack underflow",w);
		    i=bstack[--bsptr];
		  }
		  snprintf(w+2,10,"\\%x\\",i);
		  switch(w[1]) {
		    case '@': if(!makelabel(w+2)) return error("Invalid label",w); break;
		    case '\\': goto spec_ref;
		    case '&': w[1]='!'; makereference(p.scope,w+1,p.ptr+1); return writebyte(0x60) && writeshort(0xffff, 0);
		    default: return parse(w+1,f);
		  }
		  break;
		} else if(w[1]=='[' && w[0]==']' && !w[2]) {
		  if(!bsptr) return error("Stack underflow",w);
		  i=bstack[bsptr-1];
		  bstack[bsptr-1]=bcnt++;
		  snprintf(w,10,"!\\%x\\",bstack[bsptr-1]);
		  if(!parse(w,f)) return 0;
		  snprintf(w,10,"\\%x\\",i);
		  if(!makelabel(w)) return error("Invalid label",w);
		  break;
		} else if(w[1]=='^' && !w[2]) {
		  if(*w=='[') {
		    exstart=p.ptr;
		  } else {
		    if(p.ptr<exstart || p.ptr-exstart>0x40) return error("Too long inline expanded memory data",w);
		    if(!makeexp(1,0,p.ptr-exstart,0)) return 0;
		    memcpy(p.exps[p.xlen-1].name,p.data+exstart,p.ptr-exstart);
		    p.ptr=p.length=exstart;
		  }
		  break;
		}
		/* fall through */
	default: defa:
		/* opcode */
		if(findopcode(w) || scmp(w, "BRK", 4)) {
			if(!writeopcode(w)) return 0;
		}
		/* raw byte */
		else if(sihx(w) && slen(w) == 2) {
			if(!writebyte(shex(w))) return 0;
		}
		/* raw short */
		else if(sihx(w) && slen(w) == 4) {
			if(!writeshort(shex(w), 0)) return 0;
		}
		/* macro */
		else if((m = findmacro(w))) {
			for(i = 0; i < m->len; i++)
				if(!parse(m->items[i], f))
					return 0;
			line_comment=0;
			return 1;
		} else {
			*word='!';
			scpy(w,word+1,62);
			makereference(p.scope, word, p.ptr + 1);
			return writebyte(0x60) && writeshort(0xffff, 0);
			// return error("Unknown token", w);
		}
	}
	return 1;
}

static int
resolve(void)
{
	Label *l;
	int i,a;
	for(i = 0; i < p.rlen; i++) {
		Reference *r = &p.refs[i];
		switch(r->rune) {
		case '_':
			if(!(l = findlabel(r->name)))
				return error("Unknown relative reference", r->name);
			p.data[r->addr] = (Sint8)(l->addr - r->addr - 2);
			if((Sint8)p.data[r->addr] != (l->addr - r->addr - 2))
				return error("Relative reference is too far", r->name);
			l->refs++;
			break;
		case '.':
			if(!(l = findlabel(r->name)))
				return error("Unknown zero-page reference", r->name);
			p.data[r->addr + 1] = l->addr & 0xff;
			l->refs++;
			break;
		case ',':
			if(!(l = findlabel(r->name)))
				return error("Unknown relative reference", r->name);
			p.data[r->addr + 1] = (Sint8)(l->addr - r->addr - 3);
			if((Sint8)p.data[r->addr + 1] != (l->addr - r->addr - 3))
				return error("Relative reference is too far", r->name);
			l->refs++;
			break;
		case '-':
			if(!(l = findlabel(r->name)))
				return error("Unknown absolute reference", r->name);
			p.data[r->addr] = l->addr & 0xff;
			l->refs++;
			break;
		case ';':
			if(!(l = findlabel(r->name)))
				return error("Unknown absolute reference", r->name);
			p.data[r->addr + 1] = l->addr >> 0x8;
			p.data[r->addr + 2] = l->addr & 0xff;
			l->refs++;
			break;
		case ':': case '=':
			if(!(l = findlabel(r->name)))
				return error("Unknown absolute reference", r->name);
			p.data[r->addr + 0] = l->addr >> 0x8;
			p.data[r->addr + 1] = l->addr & 0xff;
			l->refs++;
			break;
		case '!': case '?':
			if(!(l = findlabel(r->name)))
				return error("Unknown instant jump reference", r->name);
			a = l->addr - r->addr - 2;
			p.data[r->addr] = a >> 0x8;
			p.data[r->addr + 1] = a & 0xff;
			l->refs++;
			break;
		case '`':
			p.ptr=r->addr;
			if(!special_calc(r->name)) return error("Special calculation error", r->name);
			break;
		case 'x':
			if(l=findlabel(r->name)) l->refs++;
			break;
		default:
			return error("Unknown reference", r->name);
		}
	}
	return 1;
}

static Uint32 expanded_block_length(int x) {
  ExpItem*e;
  Uint32 t=0;
  int i;
  for(i=x;i<p.xlen;i++) {
    e=p.exps+i;
    if(e->kind>='A' && e->kind<='F') break;
    t+=e->len;
  }
  return t;
}

static int do_subassembly(ExpItem*e) {
  Program ps=p;
  Uint16 st=e->len;
  Label*la;
  int i,j;
  if(!subas) {
    subas=fopen(".uxn_subassembly","wb");
    if(!subas) return error("Cannot open subassembly temporary file",".uxn_subassembly");
  }
  p.ptr=p.length=st;
  p.llen=p.mlen=p.rlen=0;
  for(i=0;i<ps.rlen;i++) if(ps.refs[i].rune=='x') {
    for(j=0;j<ps.mlen;j++) if(scmp(ps.macros[j].name,ps.refs[i].name,0x40)) {
      if(p.mlen==MAXMACROS) return error("Too many exports","");
      p.macros[p.mlen++]=ps.macros[j];
      goto found;
    }
    for(j=0;j<ps.llen;j++) if(scmp(ps.labels[j].name,ps.refs[i].name,0x40)) {
      if(p.llen==MAXLABELS) return error("Too many exports","");
      ps.labels[j].refs=1;
      p.labels[p.llen++]=ps.labels[j];
      goto found;
    }
    found: ;
  }
  if(!doinclude(e->name)) return error("Invalid include",e->name);
  if(!resolve()) return error("Error resolving sub-assembly",e->name);
  e->len=p.length-st;
  if(e->len&~0xFFFF) return error("Length confusion error",e->name);
  fwrite(p.data+st,e->len,1,subas);
  for(i=0;i<p.rlen;i++) if(p.refs[i].rune=='x') {
    la=findlabel(p.refs[i].name);
    if(!la) return error("Invalid export from sub-assembly",p.refs[i].name);
    if(ps.llen==MAXLABELS) return error("Too many exports from sub-assembly",e->name);
    la->refs=1;
    ps.labels[ps.llen++]=*la;
  }
  for(i=ps.xlen;i<p.xlen;i++) ps.exps[i]=p.exps[i];
  ps.xlen=p.xlen;
  st=e->len;
  p=ps;
  e->kind=3;
  e->len=st;
  return 1;
}

static int align_expanded(void) {
  ExpItem*e;
  Uint32 at=p.length-TRIM;
  Uint32 x;
  int i,j;
  for(i=0;i<p.xlen;i++) {
    e=p.exps+i;
    e->offset=at;
    if(e->kind>='A' && e->kind<='F') {
      for(j=i+1;j<p.xlen;j++) {
        if(p.exps[j].kind=='*') scpy(p.exps[j].name,p.scope,0x40);
        if(p.exps[j].kind=='^' && !do_subassembly(p.exps+j)) return 0;
        if(p.exps[j].kind>='A' && p.exps[j].kind<='F') break;
      }
    }
    switch(e->kind) {
      case '*': // label
        p.ptr=at;
        makelabel(e->name);
        p.ptr=at>>16;
        j=slen(e->name);
        if(j>=0x3F) return error("Too long expanded memory label name",e->name);
        e->name[j]='^';
        e->name[j+1]=0;
        makelabel(e->name);
        break;
      case '^': // sub-assembly
        return error("Sub-assembly outside of expanded memory block",e->name);
      case 'A': // absolute address
        if(at>e->len) return error("Expanded memory absolute address overlapping",e->name);
        at=e->len;
        break;
      case 'B': // align to beginning
        if(at&((1<<*e->name)-1)) at=(at+(1<<*e->name))&-(1<<*e->name);
        break;
      case 'C': // fit into number of pages
        x=expanded_block_length(i+1);
        if(x>(e->len<<16)) return error("Expanded memory block is too big",e->name);
        if(x+(at&0xFFFF)>(e->len<<16)) at=(at+0x10000)&-0x10000;
        break;
      case 'D': // start on page
        if((at>>16)>e->len) return error("Expanded memory page overlapping",e->name);
        if((at>>16)<e->len) at=e->len<<16;
        break;
      case 'E': // align to end
        x=expanded_block_length(i+1);
        if((at+x)&((1<<*e->name)-1)) at=((at+x+(1<<*e->name))&-(1<<*e->name))-x;
        break;
      case 'F': // align to fit
        x=expanded_block_length(i+1);
        if(x>(1<<*e->name)) return error("Expanded memory block does not fit in specified size","");
        if((at&((1<<*e->name)-1))!=((at+x)&((1<<*e->name)-1))) at=(at+((1<<*e->name)-1))&-(1<<*e->name);
        break;
      default: at+=e->len;
    }
  }
  p.total=at;
  if(subas) {
    fclose(subas);
    subas=fopen(".uxn_subassembly","rb");
    if(!subas) return error("Cannot open subassembly temporary file",".uxn_subassembly");
  }
  return 1;
}

static int
assemble(FILE *f)
{
	char w[0x40];
	int c;
	scpy("on-reset", p.scope, 0x40);
	while(fscanf(f, "%63s", w) == 1) {
		if(!parse(w, f))
			return error("Unknown token", w);
		if(line_comment) {
		  while((c=fgetc(f))>0 && c!='\n');
		  line_comment=0;
		}
	}
	p.total=p.length;
	if(p.xlen && !align_expanded()) return 0;
	return resolve();
}

static void
review(char *filename)
{
	int i;
	for(i = 0; i < p.llen; i++)
		if(p.labels[i].name[0] >= 'A' && p.labels[i].name[0] <= 'Z')
			continue; /* Ignore capitalized labels(devices) */
		else if(!p.labels[i].refs)
			fprintf(stderr, "-- Unused label: %s\n", p.labels[i].name);
	fprintf(stderr,
		"Assembled %s in %d bytes(%.2f%% used), %d labels, %d macros, %d references.\n",
		filename,
		p.length<TRIM?0:p.length-TRIM,
		p.length<TRIM?0:((p.length - TRIM) / 652.80),
		p.llen,
		p.mlen,
		p.rlen);
	if(p.xlen) fprintf(stderr,"Expanded memory: %u bytes (%u pages), %d items.\nTotal: %u bytes.\n",p.total-p.length,(((p.total+0xFFFF)>>16)?:1)-1,p.xlen,p.total-TRIM);
}

static void send_name_output(FILE*dst) {
  int i;
  for(i=0;i<p.llen;i++) fprintf(dst,"|%04x @%s\n",p.labels[i].addr,p.labels[i].name);
}

static void send_expanded(FILE*dst) {
  FILE*f;
  ExpItem*e;
  Uint32 at=p.length;
  Uint32 x;
  int i;
  for(i=0;i<p.xlen;i++) {
    e=p.exps+i;
    if(e->offset>at) {
      x=e->offset-at;
      while(x--) fputc(0,dst);
    }
    at=e->offset;
    switch(e->kind) {
      case 1: // inline data
        fwrite(e->name,e->len,1,dst);
        at+=e->len;
        break;
      case 2: // data from file
        f=fopen(e->name,"rb");
        if(f) {
          x=e->len;
          while(x--) fputc(fgetc(f),dst);
          at+=e->len;
          fclose(f);
        } else {
          error("Cannot open file",e->name);
        }
        break;
      case 3: // sub-assembly
        if(!fread(p.data,e->len,1,subas)) error("Error reading temporary file",".uxn_subassembly");
        fwrite(p.data,e->len,1,dst);
        at+=e->len;
        break;
    }
  }
}

int
main(int argc, char *argv[])
{
	FILE *src, *dst;
	if(argc==2 && argv[1][0]=='@') printf("Program:%u Macro:%u Label:%u Reference:%u ExpItem:%u\n"
	 ,(int)sizeof(Program),(int)sizeof(Macro),(int)sizeof(Label),(int)sizeof(Reference),(int)sizeof(ExpItem));
	if(argc < 3)
		return !error("usage", "input.tal output.rom [output.nam]");
	if(!(src = fopen(argv[1], "r")))
		return !error("Invalid input", argv[1]);
	if(!assemble(src))
		return !error("Assembly", "Failed to assemble rom.");
	if(!(dst = fopen(argv[2], "wb")))
		return !error("Invalid output", argv[2]);
	if(p.length <= TRIM)
		error("Assembly", "Output rom is empty.");
	else
		fwrite(p.data + TRIM, p.length - TRIM, 1, dst);
	if(p.xlen) send_expanded(dst);
	review(argv[2]);
	if(argc>3 && argv[3][0]) {
	  fclose(dst);
	  if(!(dst=fopen(argv[3],"w"))) return !error("Invalid name output",argv[3]);
	  send_name_output(dst);
	}
	return 0;
}
