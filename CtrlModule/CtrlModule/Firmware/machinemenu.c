#ifndef NO_MACHINE_MENU
#include "machinemenu.h"
#include "uart.h"
#include "menu.h"
#include "misc.h"
#include "osd.h"

extern struct menu_entry topmenu[];

#define MAX_MACHINE 128
#define NR_IMAGES_PAGE		10

static int base = 0;
static int nextpage = 0;
char names[1024];
const char ready[] = "Ready";

void MachineInit() {
	base = 0;
	nextpage = 0;
}

void MachineStart(int row);
struct menu_entry machinemenu[MAX_MACHINE] = {
	{MENU_ENTRY_SUBMENU,"Machine menu.",MENU_ACTION(machinemenu)},
	{MENU_ENTRY_SUBMENU,"Back",MENU_ACTION(&topmenu)},
	{MENU_ENTRY_NULL,0,0}
};

void MachineStart(int row) {
	UartInit();
	UartTx("!PlEaSeLeTmEjTaG!", 17);
	if (!waitFor("Ready")) {
		return;
	}

	UartInit();
	UartTx("g", 1);
	UartTx(machinemenu[row].label, 4);
	UartTx("j", 1);
}

int waitFor(const char *s) {
	int c;
	int cnt = 0;
	while (s[cnt]) {
		while ((c=UartRxChar()) >= 0 && s[cnt]) {
			if (s[cnt] == c) cnt++;
			else cnt = 0;
		}
	}
	return s[cnt] == '\0';
}

int readLine(char *b, int maxline) {
	int c;
	int cnt = 0;
	while (c != '\r') {
		while ((c=UartRxChar()) >= 0 && c != '\r') {
			if (cnt < maxline && c != '\n') b[cnt++] = c;
		}
	}
	return cnt;
}

int containsString(const char *s, int sl, const char *match, int len) {
	int m = 0, i=0;
	while (i < sl) {
		if (match[m] == s[i]) m++;
		else m = 0;
		if (m == len) return 1;
		i++;
	}
	return 0;
}

int hex2bin(char c) {
	if (c >= '0' && c <= '9') return c-'0';
	if (c >= 'a' && c <= 'f') return c-'a'+10;
	if (c >= 'A' && c <= 'F') return c-'A'+10;
	return 0;
}

int hex2binstr(char *c, int l) {
	int v = 0, i;
	for (i=0; i<l; i++) {
		v = (v << 4) | hex2bin(c[i]);
	}
	return v;
}

void MachineMenu(int row) {
	char buff[256];
	char cmd[10];
	char *s = names;
	int l;

	UartTx("xxx", 3);
	UartInit();
	UartTx("!PlEaSeLeTmEjTaG!", 17);
	if (!waitFor("Ready")) {
		return;
	}

	UartInit();
	
	// get images
	UartTx("kffffffff\r", 10);
	l = readLine(buff, sizeof buff);
	int nrImages = hex2binstr(buff, l);
	l = readLine(buff, sizeof buff); // gulp Ready

	int i=0;
	machinemenu[i].type = MENU_ENTRY_SUBMENU;
	machinemenu[i].label = "Machine menu.";
	machinemenu[i].action = MENU_ACTION(machinemenu);
	i ++;

	while (i <= NR_IMAGES_PAGE) {
		UartInit();
		cmd[0] = 'k';
		cmd[9] = '\r';
		intToStringNoPrefix(&cmd[1], base+i-1);
		UartTx(cmd, 10);
		

		machinemenu[i].type = MENU_ENTRY_CALLBACK;
		machinemenu[i].label = s;
		machinemenu[i].action = MENU_ACTION_CALLBACK(MachineStart);

		l = readLine(s, sizeof names - (s-names));
		if (l > 0) {
			if (containsString(s, l, "Ready", 5))
				break;
			machinemenu[i].type = MENU_ENTRY_CALLBACK;
			machinemenu[i].label = s;
			machinemenu[i].action = MENU_ACTION_CALLBACK(MachineStart);

			i ++;
			s[l] = '\0';
			s += l + 1;
			
			if (!waitFor("Ready")) {
				break;
			}
		}
	}

	// back link
	if ((base + NR_IMAGES_PAGE) >= nrImages) {
		base = 0;
	} else {
		base += NR_IMAGES_PAGE;
	}

	machinemenu[i].type = MENU_ENTRY_CALLBACK;
	machinemenu[i].label = "Next";
	machinemenu[i].action = MENU_ACTION_CALLBACK(MachineMenu);
	i ++;

	machinemenu[i].type = MENU_ENTRY_SUBMENU;
	machinemenu[i].label = "Back";
	machinemenu[i].action = MENU_ACTION(topmenu);
	i ++;

	// endstop
	machinemenu[i].type = MENU_ENTRY_NULL;
	machinemenu[i].label = NULL;
	machinemenu[i].action = 0;
	Menu_Set(machinemenu);
	UartTx("xxx", 3);
}
#endif

/*
!PlEaSeLeTmEjTaG!
l
mjs1 - (0000002F sectors)
bbcm - (0000001D sectors)
samc - (0000001F sectors)
elec - (0000001B sectors)
Ready
x

!PlEaSeLeTmEjTaG!
g
mjs1 - (0000002F sectors)
bbcm - (0000001D sectors)
samc - (0000001F sectors)
elec - (0000001B sectors)
Ready
x

!PlEaSeLeTmEjTaG!
gbbcm
Ready
j


*/
