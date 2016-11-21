// Inspired by https://github.com/kentonv/dvorak-qwerty

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <X11/Xlib.h>
#include <assert.h>
#include <time.h>

#define arraysize(array) (sizeof(array) / sizeof((array)[0]))

char layouts[][16] = {"us,us", "ru,us", "us"};
//                              ^
// Write there your native(not "us" or "us,us") layout. This layout should be installed on your os.
// First layout used for grab to dvorak and will be "us" or "us,us". Otherwise you have to change the characters_qwerty variable.
// You can change last layout to other. This layout should be installed to your os.

// Keycodes of keys used for switch to layout by index with alt+shift
const unsigned int switch_keycodes[] = {
	10, 11, 12
}; // keycode 10 - "1" or "!", keycode 11 - "2" or "@", keycode 12 - "3" or "#"
// By default:
//	alt+shift+1 - swap to us,us with grabbed letters to dvorak
//	alt+shift+2 - swap to ru,us
//	alt+shift+3 - swap to us

// 
const int layout_ind_for_dvorak = 0;

const unsigned int letters_keykodes[] = {
	                                       20, 21,
	24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
	 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	  52, 53, 54, 55, 56, 57, 58, 59, 60, 61
};


const char characters_qwerty[] =
	          "-="
	"qwertyuiop[]"
	"asdfghjkl;'"
	 "zxcvbnm,./";

const char characters_dvorak[] =
	          "[]"
	"',.pyfgcrl/="
	"aoeuidhtns-"
	 ";qjkxbmwvz";

int dvorak_mapping[128];

int getCurrentLayoutIndex() {
	FILE *fp;
	char lin[64];

	fp = popen("setxkbmap -query", "r");
	if (fp == NULL) {
		printf("Failed to run \"setxkbmap -query\"" );
		return -2;
	}
	int i;
	char layout_line_start[] = "layout:";
	char current_layout[16];
	while (fgets(lin, sizeof(lin)-1, fp) != NULL) {
		i = 0;
		for(; i < 7; i++) {
			if (lin[i] != layout_line_start[i]) {
				break;
			}
		}
		if (i == 7) {
			int start_layout_on_line = 0;
			for (; i < sizeof(lin)-1; i++) {
				if (start_layout_on_line != 0 || lin[i] != ' ') {
					if (lin[i] != '\n' && lin[i] != '\r' && lin[i] != '\0') {
						if (start_layout_on_line == 0) {
							start_layout_on_line = i;
						}
						current_layout[i - start_layout_on_line] = lin[i];
					}
				}
			}
			break;
		}
	}
	for (i = 0; i < arraysize(layouts); i++) {
		if (strcmp(current_layout, layouts[i]) == 0) {
			return i;
		}
	}
	return -1;
}

int main(int argc, char* argv[]) {

	int count_layouts = arraysize(layouts);
	
	// Set modifier keys
	unsigned int only_text_modifiers[] = {AnyKey, ShiftMask};
	unsigned int switch_text_modifiers[] = {Mod1Mask | ShiftMask};
	
	// Create replaces map for dvorak
	int size = arraysize(letters_keykodes);
	int en_to_keycode[128];
	memset(en_to_keycode, 0, sizeof(en_to_keycode));
	for (int i = 0; i < size; i++) {
		en_to_keycode[(int) characters_qwerty[i]] = letters_keykodes[i];
	}
	memset(dvorak_mapping, 0, sizeof(dvorak_mapping));
	for (int i = 0; i < size; i++) {
		assert(en_to_keycode[(int) characters_dvorak[i]] != 0);
		dvorak_mapping[letters_keykodes[i]] = en_to_keycode[(int) characters_dvorak[i]];
	}

	// Get display and window
	Display* display = XOpenDisplay(NULL);
	if (display == NULL) {
		printf("Couldn't open display.\n");
		exit(1);
	}

	Window window = DefaultRootWindow(display);

	// Grab keycodes
	for (int i = 0; i < arraysize(letters_keykodes); i++) {
		for (int j = 0; j < arraysize(only_text_modifiers); j++) {
			XGrabKey(display, letters_keykodes[i], only_text_modifiers[j], window, True, GrabModeAsync, GrabModeAsync);
		}
	}
	for (int i = 0; i < arraysize(switch_keycodes); i++) {
		for (int j = 0; j < arraysize(switch_text_modifiers); j++) {
			XGrabKey(display, switch_keycodes[i], switch_text_modifiers[j], window, True, GrabModeAsync, GrabModeAsync);
		}
	}

	XSync(display, true);
	int current_layout_index = getCurrentLayoutIndex();
	bool is_dvorak_now = false;
	clock_t prev_get_layout_clock = clock();
	XEvent event;
	// Start loop for grab key events
	while(1) {
		XNextEvent(display, &event);
		clock_t t = (double)(clock() - prev_get_layout_clock);
		if (t > 1000) {
			int new_layout_index = getCurrentLayoutIndex();
			if (new_layout_index > -2) {
				current_layout_index = new_layout_index;
				is_dvorak_now = current_layout_index == layout_ind_for_dvorak;
			}
			prev_get_layout_clock = t;
		}
		if (event.xkey.keycode >= 0 && event.xkey.keycode < arraysize(dvorak_mapping)) {
			// Check current laуout
			bool isSwitch = false;
			// Test if it is switching layout
			for(int i = 0; i < count_layouts; i++) {
				if (event.xkey.keycode == switch_keycodes[i]) {
					isSwitch = true;
					is_dvorak_now = false;
					current_layout_index = i;
					is_dvorak_now = current_layout_index == layout_ind_for_dvorak;
					char command[64];
					char* commandStart = "sleep .2; setxkbmap -layout ";
					strcpy( command, commandStart );
					strcat( command, layouts[current_layout_index] );
					int systemRet = system(command);
					break;
				}
			}
			// Change keycode by map
			int new_keycode = dvorak_mapping[event.xkey.keycode];
			if(!is_dvorak_now) {
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
