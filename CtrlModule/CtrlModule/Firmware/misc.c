#include "misc.h"

void _memcpy(char *d, char *s, int len) {
	int i;
	for (i=0; i<len; i++) {
		d[i] = s[i];
	}
}

void _memset(char *d, char c, int len) {
  int i;
	for (i=0; i<len; i++) {
		d[i] = c;
	}
}

// char _toupper(char c) {
//   if (c >= 'a' && c <= 'z') return c-('a'-'A');
//   else return c;
// }

void _strcpy(char *d, char *s) {
  while (*d++ = *s++);
}

void _strcat(char *d, char *s) {
  while (*d++);
	d--;
  while (*d++ = *s++);
}

const char hex[] = "0123456789abcdef";

void intToStringNoPrefix(char *s, unsigned int n) {
  int i;
  for (i=0; i<8; i++) {
    s[i] = hex[(n >> 28)&0xf];
    n <<= 4;
  }
}

void intToString(char *s, unsigned int n) {
  s[0] = '0';
  s[1] = 'x';
  intToStringNoPrefix(s+2, n);
}

void MutateFilename(char *buff, int n) {
  char fn[9];
  int i;

  memset(fn, '_', 8);
  fn[8] = '\0';

  for (i=7; i>=5; i--) {
    fn[i] = hex[n&0xf];
    n >>= 4;
  }

  for (i=0; i<5 && buff[i]; i++) {
    fn[i] = buff[i];
  }

  memcpy(buff, fn, 9);
}

void GuessFilename(char *buff, const char *offset, const char *deftext) {
  int i,c;
  int len;
  int lastOffset = -1;
  int lastMax = 0;
  int current = 0;

  for (i=0; i<256; i++) {
    int c = offset[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') || (c == ' ') || (c == '.')) {
      current ++;
    } else if (current) {
      if (current > lastMax) {
        lastOffset = i - current;
        lastMax = current;
      }
      current = 0;
    }
  }

  if (lastOffset >= 0) {
    for (c=i=0; i<lastMax && c<8; i++) {
      int q = offset[lastOffset + i];
      if (q != ' ' && q != '.')
        buff[c++] = toupper(q);
    }
    buff[c] = '\0';
  } else {
    strcpy(buff, deftext);
  }
}
