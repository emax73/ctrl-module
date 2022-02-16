#include "uart.h"
#include "interrupts.h"

#define HW_UART_R() *(volatile unsigned int *)(0xFFFFFA0C)
#define HW_UART_W(a) *(volatile unsigned int *)(0xFFFFFA0C) = a

#define HW_UART_R_DATA        0xFF
#define HW_UART_R_TXREADY     0x100
#define HW_UART_R_RXFIFOFULL  0x200
#define HW_UART_R_RXFIFOEMPTY 0x400

#define HW_UART_W_DATA        0xFF
#define HW_UART_W_TXGO        0x100
#define HW_UART_W_RXFIFOREAD  0x200
#define HW_UART_W_RXFIFORESET 0x400

// WRITE
// uart_txdata <= mem_write(7 downto 0);
// uart_txgo <= mem_write(8);
// uartrxfifo_read <= mem_write(9);
// uartrxfifo_reset <= mem_write(10);

// READ
// mem_read(7 downto 0) <= uartrxfifo_data(7 downto 0);
// mem_read(10 downto 8) <= uartrxfifo_empty & uartrxfifo_full & uart_txready;

#if 0
typedef struct {
  unsigned char d[256];
  unsigned short c;
  unsigned char l;
  unsigned char r;
} buffer_t;

static int get(buffer_t *b) {
  if (b->c > 0) {
    b->c--;
    return b->d[b->l++];
  }
  return -1;
}

static void put(buffer_t *b, unsigned char c);

static void putsafe(buffer_t *b, unsigned char c) {
  DisableInterrupts();
  put(b, c);
  EnableInterrupts();
}

static int getsafe(buffer_t *b) {
  int r;
  DisableInterrupts();
  r = get(b);
  EnableInterrupts();
  return r;
}

static void put(buffer_t *b, unsigned char c) {
  if (b->c < 256) {
    b->c++;
    b->d[b->r++] = c;
  }
}

static void init(buffer_t *b) {
  b->l = b->r = b->c = 0;
}

buffer_t rx, tx;

void UartISR(void) {
  unsigned int sr = HW_UART_R();
  if (sr & HW_UART_R_RXINT) {
    put(&rx, sr & HW_UART_R_DATA);
  }

  if (sr & HW_UART_R_TXREADY && tx.c) {
    HW_UART_W(HW_UART_W_TXGO | get(&tx));
  } else if (sr & HW_UART_R_TXREADY) {
    HW_UART_W(0);
  }

}
#endif

void UartInit(void) {
  HW_UART_W(HW_UART_W_RXFIFORESET);
  HW_UART_W(0);
}

void UartTx(const char *data, int len) {
  int i;

  for (i=0; i<len; i++) {
    HW_UART_W(HW_UART_W_TXGO | data[i]);
    while ((HW_UART_R() & HW_UART_R_TXREADY) == 0)
      HW_UART_W(data[i]);
      ;
  }
}

int UartRx(char *data, int maxlen) {
  int i = 0;
  while (i < maxlen) {
    if ((HW_UART_R() & HW_UART_R_RXFIFOEMPTY) == HW_UART_R_RXFIFOEMPTY)
      break;

    data[i++] = HW_UART_R() & HW_UART_R_DATA;
    HW_UART_W(HW_UART_W_RXFIFOREAD);
    HW_UART_W(0);
  }

  return i;
}

int UartRxChar(void) {
  int c = -1;

  if ((HW_UART_R() & HW_UART_R_RXFIFOEMPTY) != HW_UART_R_RXFIFOEMPTY) {
    c = HW_UART_R() & HW_UART_R_DATA;
    HW_UART_W(HW_UART_W_RXFIFOREAD);
    HW_UART_W(0);
  }
  return c;
}

void UartLoop(void) {
  char str[16];
  int len;

  len = UartRx(str, sizeof str);
  UartTx(str, len);
}
