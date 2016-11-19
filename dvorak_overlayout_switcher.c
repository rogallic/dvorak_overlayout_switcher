// Inspired by https://github.com/kentonv/dvorak-qwerty

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/Xlib.h>
#include <assert.h>
#include <time.h>

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))

// Write there your layouts
const char layouts[][16] = {"us", "ru,us"};
// Set index of qwerty layout for dvorak modification
const int layoutIndForDvorak = 0;

const int dvorakKeycodes[] = {
	                                       20, 21,
	24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
	 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	  52, 53, 54, 55, 56, 57, 58, 59, 60, 61
};

const int switchKeycodes[] = {
	10, 11, 12, 13
};

const char kQwerty[] =
	          "-="
	"qwertyuiop[]"
	"asdfghjkl;'"
	 "zxcvbnm,./";

const char kDvorak[] =
	          "[]"
	"',.pyfgcrl/="
	"aoeuidhtns-"
	 ";qjkxbmwvz";

int dvorakMapping[256];

int getCurrentLayoutIndex() {
	FILE *fp;
	char lin[64];

	fp = popen("setxkbmap -query", "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}
	int i;
	char layoutLineStart[] = "layout:";
	char currentLayout[16];
	while (fgets(lin, sizeof(lin)-1, fp) != NULL) {
		i = 0;
		for(; i < 7; i++) {
			if (lin[i] != layoutLineStart[i]) {
				break;
			}
		}
		if (i == 7) {
			int startLayoutOnLine = 0;
			for (; i < sizeof(lin)-1; i++) {
				if (startLayoutOnLine != 0 || lin[i] != ' ') {
					if (lin[i] != '\n' && lin[i] != '\r' && lin[i] != '\0') {
						if (startLayoutOnLine == 0) {
							startLayoutOnLine = i;
						}
						currentLayout[i - startLayoutOnLine] = lin[i];
					}
				}
			}
			break;
		}
	}
	for (i = 0; i < arraysize(layouts); i++) {
		if (strcmp(currentLayout, layouts[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int main(int argc, char* argv[]) {
	
	// Set modifier keys
	unsigned int onlyTextModifiers[] = {AnyKey, ShiftMask};
	unsigned int switchTextModifiers[] = {Mod1Mask | ShiftMask};
	
	// Create replaces map for dvorak
	int size = arraysize(dvorakKeycodes);
	int en_to_keycode[128];
	memset(en_to_keycode, 0, sizeof(en_to_keycode));
	for (int i = 0; i < size; i++) {
		en_to_keycode[(int) kQwerty[i]] = dvorakKeycodes[i];
	}
	memset(dvorakMapping, 0, sizeof(dvorakMapping));
	for (int i = 0; i < size; i++) {
		assert(en_to_keycode[(int) kDvorak[i]] != 0);
		dvorakMapping[dvorakKeycodes[i]] = en_to_keycode[(int) kDvorak[i]];
	}

	// Get display and window
	Display* display = XOpenDisplay(NULL);
	if (display == NULL) {
		printf("Couldn't open display.\n");
		exit(1);
	}

	Window window = DefaultRootWindow(display);

	// Grab keycodes
	for (int i = 0; i < arraysize(dvorakKeycodes); i++) {
		for (int j = 0; j < arraysize(onlyTextModifiers); j++) {
			XGrabKey(display, dvorakKeycodes[i], onlyTextModifiers[j], window, True, GrabModeAsync, GrabModeAsync);
		}
	}
	for (int i = 0; i < arraysize(switchKeycodes); i++) {
		for (int j = 0; j < arraysize(switchTextModifiers); j++) {
			XGrabKey(display, switchKeycodes[i], switchTextModifiers[j], window, True, GrabModeAsync, GrabModeAsync);
		}
	}

	XSync(display, True);
	bool isDvorak = false;
	clock_t tPrev = clock();
	XEvent event;
	// Start loop for grab key events
	while(1) {
		if (isDvorak) {
			clock_t t = (double)(clock() - tPrev);
			if (t > 1000) {
				int currentLayoutIndex = getCurrentLayoutIndex();
				if (currentLayoutIndex != layoutIndForDvorak) {
					isDvorak = false;
				}
				tPrev = t;
			}
		}
		XNextEvent(display, &event);
		if (event.xkey.keycode >= 0 && event.xkey.keycode < arraysize(dvorakMapping)) {
			// Check current laуout
			bool isSwitch = false;
			// Test if it is switching layout
			if (event.xkey.keycode > 9 && event.xkey.keycode < 14) {
				isSwitch = true;
				isDvorak = false;
				int toLayout = -1;
				if (event.xkey.keycode > 11) {
					isDvorak = true;
					toLayout = layoutIndForDvorak;
				} else {
					toLayout = event.xkey.keycode - 10;
				}
				char command[64];
				char* commandStart = "sleep .2; setxkbmap -layout ";
				strcpy( command, commandStart );
				strcat( command, layouts[toLayout] );
				int systemRet = system(command);
			}
			// Change keycode by map
			int new_keycode = dvorakMapping[event.xkey.keycode];
			if(!isDvorak || isSwitch) {
				new_keycode = event.xkey.keycode;
			}
			if (new_keycode != 0) {
				event.xkey.keycode = new_keycode;
			}
		}

		// Send changed event
		int junk;
		XGetInputFocus(display, &event.xkey.window, &junk);
		XSendEvent(display, event.xkey.window, False, 0, &event);
	}
}
