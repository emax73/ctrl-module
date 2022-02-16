#include <stdio.h>

int r;

#include "../swap.c"
#include "commontests.h"


int main(void) {
  passifeq(0xdeadbeef, SwapBBBB(0xefbeadde), "should swap properly");
  passifeq(0x00000682, SwapBBBB(0x82060000), "should swap properly");
  infotests();
  return tests_run - tests_passed;
}
