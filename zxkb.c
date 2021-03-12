#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <wiringPi.h>
#include <linux/uinput.h>

// cc zxkb.c -lwiringPi

const int datalines[5] = {26,19,13,6,5};
const int addresslines[8] = {25,24,23,22,27,18,17,4};

const char* keys[8][5] = {	
	{"1","2","3","4","5"},
	{"Q","W","E","R","T"},
	{"A","S","D","F","G"},
	{"0","9","8","7","6"},
	{"P","O","I","U","Y"},
	{"LEFTSHIFT","Z","X","C","V"},
	{"ENTER","L","K","J","H"},
	{"SPACE","LEFTCTRL","M","N","B"}
}; 

const int vkeys[8][5] = {
	{KEY_1,KEY_2,KEY_3,KEY_4,KEY_5},
	{KEY_Q,KEY_W,KEY_E,KEY_R,KEY_T},
	{KEY_A,KEY_S,KEY_D,KEY_F,KEY_G},
	{KEY_0,KEY_9,KEY_8,KEY_7,KEY_6},
	{KEY_P,KEY_O,KEY_I,KEY_U,KEY_Y},
	{KEY_LEFTSHIFT,KEY_Z,KEY_X,KEY_C,KEY_V},
	{KEY_ENTER,KEY_L,KEY_K,KEY_J,KEY_H},
	{KEY_SPACE,KEY_LEFTCTRL,KEY_M,KEY_N,KEY_B}
};

int keytrack[8][5] = {
	{0,0,0,0,0},
	{0,0,0,0,0},
	{0,0,0,0,0},
	{0,0,0,0,0},
	{0,0,0,0,0},
	{0,0,0,0,0},
	{0,0,0,0,0},
	{0,0,0,0,0}
};

void sendkeypress(int fd, int type, int code, int val) {
	struct input_event ie;
	
	ie.type = type;
	ie.code = code;
	ie.value = val;
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;
	
	write(fd, &ie, sizeof(ie));
}

int main(void) {
	// Prepare GPIO
	wiringPiSetupGpio();
	
	for(int i=0;i<8;i++) {
		pinMode(addresslines[i],OUTPUT);
		digitalWrite(addresslines[i],HIGH);
	}
	
	for(int i=0;i<5;i++) {
		pullUpDnControl(datalines[i],2);
	}
	
	int isFree;
	// Prepare virtual keyboard
	struct uinput_setup usetup;
	int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	ioctl(fd, UI_SET_EVBIT, EV_KEY);

	for(int i=0;i<8;i++) {
		for(int j=0;j<5;j++) {
			ioctl(fd, UI_SET_KEYBIT, vkeys[i][j]);
		}
	}

	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0xB00B;
	usetup.id.product = 0xA45E;
	strcpy(usetup.name, "ZXKB");
	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);

	/* Main loop: read status of pins, translate to key presses
	   and send to virtual keyboard */
	while(1) {
		for(int i=0;i<8;i++) {
			digitalWrite(addresslines[i], 0);
			for(int j=0;j<5;j++) {
				isFree = digitalRead(datalines[j]);
				// Key down
				if(isFree == 0 && keytrack[i][j] == 0) {
					printf("Pressed %s\n",keys[i][j]);
					keytrack[i][j] = 1;
					sendkeypress(fd, EV_KEY, vkeys[i][j], 1);
					sendkeypress(fd, EV_SYN, SYN_REPORT, 0);
				}
				// Key up
				else if(isFree == 1 && keytrack[i][j] == 1) {
					printf("Released %s\n",keys[i][j]);
					keytrack[i][j] = 0;
					sendkeypress(fd, EV_KEY, vkeys[i][j], 0);
					sendkeypress(fd, EV_SYN, SYN_REPORT, 0);
				}
			}
			digitalWrite(addresslines[i], 1);
			usleep(60);
		}
	}
     
	ioctl(fd, UI_DEV_DESTROY);
	close(fd);
	return 0;
}
