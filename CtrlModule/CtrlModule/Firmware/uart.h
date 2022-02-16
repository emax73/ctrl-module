#ifndef _UART_H
#define _UART_H

extern void UartInit(void);
extern void UartTx(const char *data, int len);
extern int UartRx(char *data, int maxlen);
extern int UartRxChar(void);
extern void UartLoop(void);

#endif
