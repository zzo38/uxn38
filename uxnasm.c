#include <stdio.h>

/*
Copyright (c) 2021 Devine Lu Linvega

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
#define MAXITEMS 0x40

#define TRIM 0x0100
#define LENGTH 0x10000

typedef unsigned char Uint8;
typedef signed char Sint8;
typedef unsigned short Uint16;

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
	Uint8 data[LENGTH];
	unsigned int ptr, length;
	Uint16 llen, mlen, rlen;
	Label labels[MAXLABELS];
	Macro macros[MAXMACROS];
	Reference refs[MAXREFS];
	char scope[0x40];
} Program;

static Program p;
static char line_comment;
static int ssptr;
static Uint16 sstack[256];
static int bsptr,bcnt;
static Uint16 bstack[256];

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
	if(label[1] == '&')
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
    case '(': if(ssptr<1) return 0; if(!sstack[--ssptr]) break; while(*w && *w!=')') w++; break;
    case ')': /* do nothing */ break;
    case '[': if(ssptr<1) return 0; if(sstack[--ssptr]) break; while(*w && *w!=']') w++; break;
    case ']': /* do nothing */ break;
    default: fprintf(stderr,"Unrecognized special calculation command: %c\n",w[-1]); return 0;
  }
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
		if(!makemacro(w + 1, f))
			return error("Invalid macro", w);
		break;
	case '|': /* pad-absolute */
		if(!sihx(w + 1))
			return error("Invalid padding", w);
		p.ptr = shex(w + 1);
		break;
	case '$': /* pad-relative */
		if(!sihx(w + 1))
			return error("Invalid padding", w);
		p.ptr += shex(w + 1);
		break;
	case '@': /* label */
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
	case ':': /* raw short absolute */
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
		  default: return error("Invalid special",w);
		}
		break;
	case '[':
	case ']':
		if(slen(w) == 1) break; /* else fallthrough */
		if((w[1]=='.' || w[1]==',' || w[1]==':' || w[1]==';' || w[1]=='@' || w[1]=='\\') && !w[2]) {
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
		    default: return parse(w+1,f);
		  }
		  break;
		}
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
		} else
			return error("Unknown token", w);
	}
	return 1;
}

static int
resolve(void)
{
	Label *l;
	int i;
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
		case ':':
			if(!(l = findlabel(r->name)))
				return error("Unknown absolute reference", r->name);
			p.data[r->addr + 0] = l->addr >> 0x8;
			p.data[r->addr + 1] = l->addr & 0xff;
			l->refs++;
			break;
		default:
			return error("Unknown reference", r->name);
		}
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
		"Assembled %s in %d bytes(%.2f%% used), %d labels, %d macros.\n",
		filename,
		p.length<TRIM?0:p.length-TRIM,
		p.length<TRIM?0:((p.length - TRIM) / 652.80),
		p.llen,
		p.mlen);
}

static void send_name_output(FILE*dst) {
  int i;
  for(i=0;i<p.llen;i++) fprintf(dst,"|%04x @%s\n",p.labels[i].addr,p.labels[i].name);
}

int
main(int argc, char *argv[])
{
	FILE *src, *dst;
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
	review(argv[2]);
	if(argc>3 && argv[3][0]) {
	  fclose(dst);
	  if(!(dst=fopen(argv[3],"w"))) return !error("Invalid name output",argv[3]);
	  send_name_output(dst);
	}
	return 0;
}
