#ifndef _MISC_H
#define _MISC_H

#define NULL ((void *)0)

extern void _memcpy(char *d, char *s, int len);
extern void _memset(char *d, char c, int len);
// char _toupper(char c);
extern void _strcpy(char *d, char *s);
extern void _strcat(char *d, char *s);
extern void intToString(char *s, unsigned int n);
extern void intToStringNoPrefix(char *s, unsigned int n);
extern void MutateFilename(char *buff, int n);
extern void GuessFilename(char *buff, const char *offset, const char *deftext);

#define memcpy _memcpy
#define memset _memset
// #define toupper _toupper
#define strcpy _strcpy
#define strcat _strcat

#define tolower(x) ((x)>='A' && (x)<='Z' ? ((x)|32) : (x))
#define toupper(x) ((x)>='a' && (x)<='z' ? ((x)&~32) : (x))

#ifndef debug
#define debug(a) {}
#endif

#endif
