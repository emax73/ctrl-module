#ifndef _TAPE_H
#define _TAPE_H

#ifdef SAMCOUPE
#define DISABLE_TAPE
#endif

#ifndef DISABLE_TAPE
extern char tapeLoadSize[32];
extern char tapeSaveSize[32];

extern void TapeUpdateStatus(void);
extern void TapeInit(void);
extern void TapeErase(void);
extern int TapeLoad(const char *fn, DIRENTRY *p);
extern int TapeSave(void);
extern void TapeRewind(void);
extern void TapeUseRecordBuffer(void);
extern void TapeStop(void);
extern void TapeLoadBlock();
#endif

#endif /* _TAPE_H */
