#include <vitasdk.h>
#include <psp2/motion.h> 
#include <stdlib.h>

#include "main.h"
#include "renderer.h"
#include "ui.h"
#include "profile.h"
#include "common.h"

#define VERSION				2
#define SUBVERSION			1
#define SUBSUBVERSION		0

#define MENU_MODES          9  // Menu modes num
#define CREDITS_NUM			16

#define COLOR_DEFAULT     0x00FFFFFF
#define COLOR_HEADER      0x00FF00FF
#define COLOR_CURSOR      0x0000FF00
#define COLOR_ACTIVE      0x0000FFFF
#define COLOR_DISABLE     0x000000FF
#define CHA_W  12		//Character size in pexels
#define CHA_H  20
#define L_0    5		//Left margin for text
#define L_1    18		
#define L_2    36

#define LONG_PRESS_TIME   350000	//0.35sec

enum{
	MAIN_MENU = 0,
	REMAP_MENU,
	ANALOG_MENU,
	TOUCH_MENU,
	GYRO_MENU,
	CNTRL_MENU,
	FUNCS_LIST,
	SETTINGS_MENU,
	CREDITS_MENU
};

uint8_t ui_opened = 0;

static uint8_t new_frame = 1;
static int screen_h = 272;
static int screen_w = 480;
static uint32_t ticker;
static int cfg_i = 0;
static int menu_i = MAIN_MENU;

static uint32_t curr_buttons;
static uint32_t old_buttons;
static uint64_t tick;
static uint64_t pressedTicks[PHYS_BUTTONS_NUM];

static char* str_menus[MENU_MODES] = {
	"MAIN MENU", 
	"REMAP MENU", 
	"ANALOG MENU", 
	"TOUCH MENU", 
	"GYRO MENU", 
	"CONNECTED CONTROLLERS", 
	"USED FUNCTIONS",
	"SETTINGS",
	"CREDITS"
};

static char* str_main_menu[] = {
	"Change remap settings",
	"Change analog remap settings",
	"Change touch remap settings",
	"Change gyro remap settings",
	"Setup external gamepads",
	"Show imported functions",
	"Settings",
	"Credits",
	"Return to the game"
};

static char* str_credits[CREDITS_NUM] = {
	"Thanks to ", 
	"  Tain Sueiras, nobodywasishere and RaveHeart",
	"     for their awesome support on Patreon",
	"Special thanks to",
	"  S1ngyy, for his analogs/gyro code",
	"  pablojrl123, for customizable opening buttons",
	"  Cassie, W0lfwang, TheIronUniverse,",
	"  Kiiro Yakumo and mantixero",
	"    for enduring endless crashes",
	"    while testing this thing",
	"  Vita Nuova community",
	"    for all the help and support I got there",
	"",
	"              Created by Rinnegatamante",
	"                       Updated by Mer1e"
};

static char* str_yes_no[] = {
	"No", "Yes"
};

static char* str_settings[] = {
	"Save as Game profile", 
	"Load Game profile", 
	"Save as Global profile", 
	"Load Global profile",
	"Save as PS4 Link profile"
};

static char* str_funcs[HOOKS_NUM-1] = {
	"sceCtrlPeekBufferPositive",
	"sceCtrlPeekBufferPositive2",
	"sceCtrlReadBufferPositive",
	"sceCtrlReadBufferPositive2",
	"sceCtrlPeekBufferPositiveExt",
	"sceCtrlPeekBufferPositiveExt2",
	"sceCtrlReadBufferPositiveExt",
	"sceCtrlReadBufferPositiveExt2",
	"sceCtrlPeekBufferNegative",
	"sceCtrlPeekBufferNegative2",
	"sceCtrlReadBufferNegative",
	"sceCtrlReadBufferNegative2",
	"sceTouchRead",
	"sceTouchRead2",
	"sceTouchPeek",
	"sceTouchPeek2"
};

static char* str_btns[PHYS_BUTTONS_NUM] = {
	"Cross", "Circle", "Triangle", "Square",
	"Start", "Select", 
	"LT/L2", "RT/R2",
	"Up", "Right", "Left", "Down", 
	"L1", "R1", "L3", "R3"
};
static char* str_sections[] = {
	"Buttons", "LStick", "RStick", 
	"FrontTouch", "RearTouch", "Gyroscope",
	"","Disabled"
};
static char* str_analog_directions[] = {
	"Left", "Right", "Up", "Down"
};
static char* str_touch_zones[] = {
	"TopLeft", "TopRight", "BotLeft", "BotRight"
};
static char* str_gyro_directions[] = {
	"Left", "Right", "Up", "Down",
	"Roll Left","Roll Right"
};
static char* str_touch_points[] = {
	"Point A", "Point B", "Point C", "Point D"
};
static char* str_deadband[] = {
	"Game default", "Enable", "Disable"
};

char* getControllerName(int id){
	if 		(id == 	SCE_CTRL_TYPE_UNPAIRED) return "Unpaired controller";
	else if (id == 	SCE_CTRL_TYPE_PHY) 		return "Physical controller for VITA";
	else if (id == 	SCE_CTRL_TYPE_VIRT) 	return "Virtual controller for PSTV";
	else if (id == 	SCE_CTRL_TYPE_DS3) 		return "DualShock 3";
	else if (id == 	SCE_CTRL_TYPE_DS4) 		return "DualShock 4";
	else 									return "Unknown controller";
}

//Calculate starting index for scroll menu
int calcStartingIndex(int idx, int entriesNum, int screenEntries){
	int bottom_l = 3;
	int ret = max(0, idx - (screenEntries - bottom_l));
	while (ret > 0 && (entriesNum - ret - 2) < screenEntries) ret--;
	return ret;
}
// Config Menu Renderer
void drawConfigMenu() {
	char* _blank = "                                                                                ";
	
	//DRAW HEADER
	drawStringF(0, 0, _blank);
	drawStringF(0, CHA_H, _blank);
	setTextColor(COLOR_HEADER);
	drawStringF(L_0, 10, "remaPSV2 v.%hhu.%hhu.%hhu  %s", 
		VERSION, SUBVERSION, SUBSUBVERSION, str_menus[menu_i]);
	drawString(screen_w - CHA_W*strlen(titleid) - 10, 10, titleid);
	
	//DRAW MENU
	uint8_t slim_mode = 0;//Mode for low res framebuffers;
	if (screen_w < 850)
		slim_mode = 1;
	int i, y = CHA_H;
	int screen_entries = ((float)screen_h - 10) / CHA_H - 1;
	int avaliable_entries = screen_entries - 4 - (slim_mode ? 1 : 0);
	char *footer1 ="", *footer2="";
	switch (menu_i){
	case MAIN_MENU:
		for (i = calcStartingIndex(cfg_i, sizeof(str_main_menu)/sizeof(char*), avaliable_entries); i < sizeof(str_main_menu)/sizeof(char*); i++) {
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + CHA_H, (ticker % 16 < 8) ? "x" : "X");
			}
			
			setTextColor((i == cfg_i) ? COLOR_CURSOR : COLOR_DEFAULT);
			drawStringF(L_1, y += CHA_H, "%s", str_main_menu[i]);
			if (y + 40 > screen_h) break;
		}
		footer1 = "(X):select";
		footer2 = "(O):close";
		break;
	case REMAP_MENU:
		for (i = calcStartingIndex(cfg_i, PROFILE_REMAP_NUM, avaliable_entries); i < PROFILE_REMAP_NUM; i++) {
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0 + CHA_W*10, y + CHA_H, (ticker % 16 < 8) ? "<" : ">");
			}
			
			char *srcSection = "", *srcAction = "", *targetSection = "", *targetAction = "";
			//Source section
			if (i == 0) srcSection = str_sections[0];
			else if (i == PHYS_BUTTONS_NUM)  srcSection = str_sections[3];
			else if (i == PHYS_BUTTONS_NUM + 4)  srcSection = str_sections[4];
			else if (i == PHYS_BUTTONS_NUM + 8) srcSection = str_sections[1];
			else if (i == PHYS_BUTTONS_NUM + 12) srcSection = str_sections[2];
			else if (i == PHYS_BUTTONS_NUM + 16) srcSection = str_sections[5];
			//Source  Action
			if (i < PHYS_BUTTONS_NUM) srcAction = str_btns[i];
			else if (i < PHYS_BUTTONS_NUM + 4)  srcAction = str_touch_zones[i - PHYS_BUTTONS_NUM];
			else if (i < PHYS_BUTTONS_NUM + 8)  srcAction = str_touch_zones[i - PHYS_BUTTONS_NUM-4];
			else if (i < PHYS_BUTTONS_NUM + 12) srcAction = str_analog_directions[i - PHYS_BUTTONS_NUM-8];
			else if (i < PHYS_BUTTONS_NUM + 16) srcAction = str_analog_directions[i - PHYS_BUTTONS_NUM-12];
			else if (i < PHYS_BUTTONS_NUM + 22) srcAction = str_gyro_directions[i - PHYS_BUTTONS_NUM-16];
			//Target Section
			if (profile_remap[i] < PHYS_BUTTONS_NUM) targetSection = str_sections[0];
			else if (profile_remap[i] == PHYS_BUTTONS_NUM)  targetSection = str_sections[6];
			else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1)  targetSection = str_sections[7];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 6)  targetSection = str_sections[1];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 10)  targetSection = str_sections[2];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 18)  targetSection = str_sections[3];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 26)  targetSection = str_sections[4];
			//Target  Action
			if (profile_remap[i] < PHYS_BUTTONS_NUM) targetAction = str_btns[profile_remap[i]];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 2)  targetAction = "";
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 6) targetAction = str_analog_directions[profile_remap[i] - PHYS_BUTTONS_NUM-2];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 10) targetAction = str_analog_directions[profile_remap[i] - PHYS_BUTTONS_NUM-6];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 14) targetAction = str_touch_zones[profile_remap[i] - PHYS_BUTTONS_NUM-10];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 18) targetAction = str_touch_points[profile_remap[i] - PHYS_BUTTONS_NUM-14];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 22) targetAction = str_touch_zones[profile_remap[i] - PHYS_BUTTONS_NUM-18];
			else if (profile_remap[i] < PHYS_BUTTONS_NUM + 26) targetAction = str_touch_points[profile_remap[i] - PHYS_BUTTONS_NUM-22];
	
			setTextColor(COLOR_HEADER);
			drawString(L_0, y += CHA_H, srcSection);
			
			if (i == cfg_i) setTextColor(COLOR_CURSOR);
			else if (profile_remap[i] == PHYS_BUTTONS_NUM) setTextColor(COLOR_DEFAULT);
			else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1) setTextColor(COLOR_DISABLE);
			else setTextColor(COLOR_ACTIVE);
			drawString(L_0 + CHA_W*11, y, srcAction);
			
			if (profile_remap[i] == PHYS_BUTTONS_NUM) setTextColor(COLOR_DEFAULT);
			else if (profile_remap[i] == PHYS_BUTTONS_NUM + 1) setTextColor(COLOR_DISABLE);
			else setTextColor(COLOR_ACTIVE);
			if (profile_remap[i] != PHYS_BUTTONS_NUM)
				drawString(L_0 + CHA_W*21, y, " -> ");
			
			drawString(L_0 + CHA_W*25, y, targetSection);
			drawString(L_0 + CHA_W*36, y, targetAction);
			if (y + 60 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		footer1 = "(<)(>):change  (LT)(RT):section  ([]):reset";
		footer2 = " (start):reset all  (O):back";
		break;
	case ANALOG_MENU:
		for (i = calcStartingIndex(cfg_i, PROFILE_ANALOG_NUM, avaliable_entries); i < PROFILE_ANALOG_NUM; i++) {				
			if (y + 60 > screen_h) break;
			
			if (!(i % 4)){	//Headers
				setTextColor(COLOR_HEADER);
				drawString(L_0, y+CHA_H, (i == 0) ? "Deadzone" : "Force digital");
			}
			
			if (i == cfg_i) setTextColor(COLOR_CURSOR);
			else if (profile_analog[i] != PROFILE_ANALOG_DEF[i]) setTextColor(COLOR_ACTIVE);
			else setTextColor(COLOR_DEFAULT);
			drawStringF(L_0+14*CHA_W, y+=CHA_H, "%s", 
				!(i % 2) ? (((i / 2) % 2 ) ? "Right Analog" : "Left Analog "): "");
			if (i < 4)
				drawStringF(L_0+27*CHA_W, y, "[%s axis]: %hhu", 
					(i % 2) ? "Y" : "X",
					profile_analog[i]);
			else 
				drawStringF(L_0+27*CHA_W, y, "[%s axis]: %s", 
					(i % 2) ? "Y" : "X",
					str_yes_no[profile_analog[i]]);	
					
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0 + 36*CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
			}
		}
		footer1 = "(<)(>):change  ([]):reset  (start):reset all" ;
		footer2 = "(O):back";
		break;
	case TOUCH_MENU:
		for (i = calcStartingIndex(cfg_i, PROFILE_TOUCH_NUM, avaliable_entries); i < PROFILE_TOUCH_NUM; i++) {				
			if (y + 60 > screen_h) break;
			
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0+ ((i<16) ? 16*CHA_W : 32*CHA_W), y + CHA_H, (ticker % 16 < 8) ? "<" : ">");
			}
			
			if (i == 0 || i == 8){	//Headers
				setTextColor(COLOR_HEADER);
				drawString(L_0, y+CHA_H, (i == 0) ? "Front" : "Rear");
			}
			
			if (i < 16){ //Points
				if (i == cfg_i) setTextColor(COLOR_CURSOR);
				else if (profile_touch[i] != PROFILE_TOUCH_DEF[i]) 
					setTextColor(COLOR_ACTIVE);
				else setTextColor(COLOR_DEFAULT);
				if (i < 16){
					if (!(i % 2)) 
						drawString(L_0+6*CHA_W, y+CHA_H, str_touch_points[(i % 8)/2]);
					drawStringF(L_0+14*CHA_W, y+=CHA_H, "%s:", !(i % 2) ? "x" : "y");
					drawStringF(L_0+17*CHA_W, y, "%hu", profile_touch[i]);
				}
				if (y + 60 > screen_h) break;
			}
			
			if (i == 16){ //Front touch mode
				if (16 == cfg_i) setTextColor(COLOR_CURSOR);
				else if (profile_touch[16] == PROFILE_TOUCH_DEF[i]) setTextColor(COLOR_DEFAULT);
				else setTextColor(COLOR_ACTIVE);
				drawString(L_0, y+=CHA_H, "Disable Front touch if remapped:");
				drawString(L_0+33*CHA_W, y, str_yes_no[profile_touch[16]]);
				if (y + 60 > screen_h) break;
			}
			
			if (i==17){ //Rear touch mode
				if (17 == cfg_i) setTextColor(COLOR_CURSOR);
				else if (profile_touch[17] == PROFILE_TOUCH_DEF[i]) setTextColor(COLOR_DEFAULT);
				else setTextColor(COLOR_ACTIVE);
				drawString(L_0, y+=CHA_H, "Disable Rear touch  if remapped:");
				drawString(L_0+33*CHA_W, y, str_yes_no[profile_touch[17]]);
				if (y + 60 > screen_h) break;
			}
		}
		footer1 = "(<)(>)[TOUCH](RS):change  ([]):reset  (start):reset all";                          
		footer2 = "(O): back";
		break;
	case GYRO_MENU:
		for (i = calcStartingIndex(cfg_i, PROFILE_GYRO_NUM, avaliable_entries); i < PROFILE_GYRO_NUM; i++) {
			if (y + 60 > screen_h) break;

			if (i < 6 && !(i % 3)) {	//Draw Headers
				setTextColor(COLOR_HEADER);
				drawString(L_1, y + CHA_H, (i == 0) ? "Sensivity" : (i == 3) ? "Deadzone" : "Mode");
			}

			if (i == cfg_i) setTextColor(COLOR_CURSOR);
			else if (profile_gyro[i] != PROFILE_GYRO_DEF[i]) setTextColor(COLOR_ACTIVE);
			else setTextColor(COLOR_DEFAULT);
			if (i < 6){ 		//Draw sens and deadzone option
				drawStringF(L_1 + 17 * CHA_W, y += CHA_H, "%s axis: %hhu",
					((i % 3) == 2) ? "Z" : ((i % 3) ? "Y" : "X"),
					profile_gyro[i]);
			
			} else if (i == 6) { //Draw deadband option
				drawStringF(L_1, y += CHA_H, "Deadband mode          : %s", 
					str_deadband[profile_gyro[i]]);
				
			} else if (i == 7) { //Draw wheel mode option
				drawStringF(L_1, y += CHA_H, "Wheel mode [WIP]       : %s", 
					str_yes_no[profile_gyro[i]]);
			
			} else if (i < 10) { //Draw reset button options
				drawStringF(L_1, y += CHA_H, "Wheel reset %s key : %s", 
					(i == 8) ? "first " : "second", 
					str_btns[profile_gyro[i]]);
			
			} else if (i == 10) { //Draw manual reset option
				drawString(L_1, y += CHA_H, "Manual wheel reset");
			}
			
			if (cfg_i == i) {//Draw cursor
				setTextColor(COLOR_CURSOR);
				if (cfg_i == 10)
					drawString(L_0, y, (ticker % 16 < 8) ? "x" : "X");
				else
					drawString(L_1 + 24 * CHA_W, y, (ticker % 16 < 8) ? "<" : ">");
			}
		}
		footer1 = "(<)(>):change  ([]):reset  (start):reset all";                          
		footer2 = "(O): back";
		break;
	case CNTRL_MENU:;		
		SceCtrlPortInfo pi;
		int res = sceCtrlGetControllerPortInfo(&pi);
		if (res != 0){//Should not ever trigger
			setTextColor(COLOR_DISABLE);
			drawString(L_1, y+= CHA_H, "Error getting controllers info");
		} else {
			//Cursor
			setTextColor(COLOR_CURSOR);
			drawString(L_0, y + CHA_H + CHA_H * cfg_i, (ticker % 16 < 8) ? "<" : ">");
			
			//Use external controller
			setTextColor(cfg_i == 0 ? COLOR_CURSOR : 
				(profile_controller[0] == PROFILE_CONTROLLER_DEF[0] ? COLOR_DEFAULT : COLOR_ACTIVE));
			drawStringF(L_1, y += CHA_H, "Use external controller: %s", str_yes_no[profile_controller[0]]);
			
			//Port selection
			setTextColor(cfg_i == 1 ? COLOR_CURSOR : 
				(profile_controller[1] == PROFILE_CONTROLLER_DEF[1] ? COLOR_DEFAULT : COLOR_ACTIVE));
			drawStringF(L_1, y += CHA_H, "Selected port: {%i} %s %s", 
				profile_controller[1],
				getControllerName(pi.port[profile_controller[1]]), 
				profile_controller[1] ? "" : "[DEFAULT]");
			
			//Button swap
			setTextColor(cfg_i == 2 ? COLOR_CURSOR : 
				(profile_controller[2] == PROFILE_CONTROLLER_DEF[2] ? COLOR_DEFAULT : COLOR_ACTIVE));
			drawStringF(L_1, y += CHA_H, "Swap L1<>LT R1<>RT     : %s", str_yes_no[profile_controller[2]]);
			
			//Ports stats
			y+=CHA_H;
			setTextColor(COLOR_DEFAULT);
			drawString(L_1, y+= CHA_H, "Detected controllers:");
			for (int i = max(0, cfg_i - (avaliable_entries + 1)); i < 5; i++){
				setTextColor((L_1 == cfg_i) ? COLOR_CURSOR : ((pi.port[i] != SCE_CTRL_TYPE_UNPAIRED) ? COLOR_ACTIVE : COLOR_DEFAULT));
				drawStringF(L_1, y += CHA_H, "Port %i: %s", i, getControllerName(pi.port[i]));
				if (y + 40 > screen_h) break;
			}	
		}
		footer1 = "(<)(>):change  ([]):reset  (start):reset all";
		footer2 = "(O): back";  
		break;
	case FUNCS_LIST:
		for (i = calcStartingIndex(cfg_i, HOOKS_NUM - 1, avaliable_entries); i < HOOKS_NUM - 1; i++) {
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + CHA_H, "-");
			}
			setTextColor((i == cfg_i) ? COLOR_CURSOR : (used_funcs[i] ? COLOR_ACTIVE : COLOR_DEFAULT));
			drawStringF(L_1, y += CHA_H, "%s : %s", str_funcs[i], used_funcs[i] ? "Yes" : "No");
			if (y + 40 > screen_h) break;
		}
		setTextColor(COLOR_HEADER);
		footer2 = "(O):back";                                                           
		break;  
	case SETTINGS_MENU:;	
		//Cursor
		setTextColor(COLOR_CURSOR);
		if (cfg_i <= 2)
			drawString(L_0, y + CHA_H + CHA_H * cfg_i, (ticker % 16 < 8) ? "<" : ">");
		else
			drawString(L_0, y + CHA_H + CHA_H * cfg_i, (ticker % 16 < 8) ? "x" : "X");
		//Menu trigger keys
		setTextColor(cfg_i == 0 ? COLOR_CURSOR : 
			(profile_settings[0] == PROFILE_SETTINGS_DEF[0] ? COLOR_DEFAULT : COLOR_ACTIVE));
		drawStringF(L_1, y += CHA_H, "Menu trigger first key    : %s", 
			str_btns[profile_settings[0]]);
		setTextColor(cfg_i == 1 ? COLOR_CURSOR : 
			(profile_settings[1] == PROFILE_SETTINGS_DEF[1] ? COLOR_DEFAULT : COLOR_ACTIVE));
		drawStringF(L_1, y += CHA_H, "            second key    : %s", 
			str_btns[profile_settings[1]]);
		
		//Save game profile on close
		setTextColor(cfg_i == 2 ? COLOR_CURSOR : 
			(profile_settings[2] == PROFILE_SETTINGS_DEF[2] ? COLOR_DEFAULT : COLOR_ACTIVE));
		drawStringF(L_1, y += CHA_H, "Save Game profile on close: %s", str_yes_no[profile_settings[2]]);
		
		//Startup delay
		setTextColor(cfg_i == 3 ? COLOR_CURSOR : 
			(profile_settings[3] == PROFILE_SETTINGS_DEF[3] ? COLOR_DEFAULT : COLOR_ACTIVE));
		drawStringF(L_1, y += CHA_H, "Startup delay             : %hhu seconds", profile_settings[3]);
		
		//Profile management
		for (int i = 0; i <	sizeof(str_settings)/sizeof(char*); i++){
			setTextColor((cfg_i == (4 + i)) ? COLOR_CURSOR : COLOR_DEFAULT);
			drawString(L_1, y += CHA_H, str_settings[i]);
		}
		
		//Footer
		footer1 = "(<)(>):change  ([]):reset  (start):reset all";
		footer2 = "(O): back";  
		break; 
	case CREDITS_MENU:
		//y+=CHA_H;
		for (i = calcStartingIndex(cfg_i, CREDITS_NUM, avaliable_entries); i < CREDITS_NUM; i++) {	
			if (cfg_i == i){//Draw cursor
				setTextColor(COLOR_CURSOR);
				drawString(L_0, y + CHA_H, "-");
			}
			
			setTextColor(COLOR_DEFAULT);
			drawStringF(L_2, y += CHA_H, "%s", str_credits[i]);
			if (y + 40 > screen_h) break;
		}
		footer2 = "(O):back";                                                           
		break;                                                             
	default:
		break;
	}
	
	//DRAW FOOTER
	setTextColor(COLOR_HEADER);
	if (!slim_mode){
		drawStringF(0, screen_h-CHA_H*1.5, _blank);
		drawStringF(0, screen_h-CHA_H, _blank);
		drawStringF(0, screen_h-CHA_H, _blank);                                                                 
		drawStringF(10, screen_h-CHA_H, footer1);
		drawStringF(screen_w - CHA_W*strlen(footer2), screen_h-CHA_H, footer2);
	} else {
		drawStringF(0, screen_h-CHA_H*2.5, _blank);
		drawStringF(0, screen_h-CHA_H*2, _blank);
		drawStringF(0, screen_h-CHA_H, _blank);
		drawStringF(10, screen_h-CHA_H*2, footer1);
		drawStringF(screen_w - CHA_W*strlen(footer2) - 10, screen_h-CHA_H, footer2);
	}
		
	//DRAW TOUCH POINTER over everything else
	if (menu_i != TOUCH_MENU || cfg_i >= 16)
		return;
	int left = profile_touch[cfg_i - (cfg_i % 2)] - 8;
	left *= (float)screen_w / ((cfg_i < 8) ? TOUCH_SIZE[0] : TOUCH_SIZE[2]);
	left = min((max(0, left)), screen_w);
	int top = profile_touch[cfg_i - (cfg_i % 2) + 1] - 10;
	top *= (float)screen_h / ((cfg_i < 8) ? TOUCH_SIZE[1] : TOUCH_SIZE[3]); //Scale to framebuffer size
	top = min((max(0, top)), screen_h);//limit into screen
	setTextColor((ticker % 4) ? COLOR_CURSOR : COLOR_DISABLE);
	drawString(left, top, (ticker % 2) ? "" : "@");
}

uint8_t isBtnActive(uint8_t btnNum){
	return ((curr_buttons & HW_BUTTONS[btnNum]) && !(old_buttons & HW_BUTTONS[btnNum])) 
		|| (pressedTicks[btnNum] != 0 && tick - pressedTicks[btnNum] > LONG_PRESS_TIME);
}

//Set custom touch point xy using RS
void analogTouchPicker(SceCtrlData *ctrl){
	if (cfg_i >= 16)
		return;
	int o_idx1 = cfg_i - (cfg_i % 2);
	int shiftX = ((float)(ctrl->rx - 127)) / 8;
	int shiftY = ((float)(ctrl->ry - 127)) / 8;
	if (abs(shiftX) > 30 / 8)
		profile_touch[o_idx1] = lim(profile_touch[o_idx1] + shiftX, 
			0, TOUCH_SIZE[(o_idx1 < 8) ? 0 : 2]);
	if (abs(shiftY) > 30 / 8)
		profile_touch[o_idx1+1] = lim(profile_touch[o_idx1+1] + shiftY, 
			0, TOUCH_SIZE[((o_idx1+1) < 8) ? 1 : 3]);
}

//Set custom touch point xy using touch
void touchPicker(int padType){
	if ((padType == SCE_TOUCH_PORT_FRONT && cfg_i >= 8) ||
		(padType == SCE_TOUCH_PORT_BACK && (cfg_i < 8 || cfg_i >= 16)))
		return;
	SceTouchData std;
	internal_touch_call = 1;
	int ret = sceTouchRead(padType, &std, 1);
	internal_touch_call = 0;
	if (ret && std.reportNum){
		profile_touch[cfg_i - (cfg_i % 2)] = std.report[0].x;
		profile_touch[cfg_i - (cfg_i % 2) + 1] = std.report[0].y;
	}
}

// Input Handler for the Config Menu
void ui_inputHandler(SceCtrlData *ctrl) {
	if ((ctrl->buttons & HW_BUTTONS[profile_settings[0]]) 
			&& (ctrl->buttons & HW_BUTTONS[profile_settings[1]]))
		return; //Menu trigger butoons should not trigger any menu actions on menu open
	if (new_frame) {
		new_frame = 0;
		int menu_entries = 0;
		switch (menu_i) {
		case MAIN_MENU:
			menu_entries = sizeof(str_main_menu) / sizeof(char*);
			break;
		case REMAP_MENU:
			menu_entries = PROFILE_REMAP_NUM;
			break;
		case ANALOG_MENU:
			menu_entries = PROFILE_ANALOG_NUM;
			break;
		case TOUCH_MENU:
			menu_entries = PROFILE_TOUCH_NUM;
			touchPicker(SCE_TOUCH_PORT_FRONT);
			touchPicker(SCE_TOUCH_PORT_BACK);
			analogTouchPicker(ctrl);
			break;
		case GYRO_MENU:
			menu_entries = PROFILE_GYRO_NUM;
			break;
		case CNTRL_MENU:
			menu_entries = PROFILE_CONTROLLER_NUM;
			break;
		case FUNCS_LIST:
			menu_entries = HOOKS_NUM - 1;
			break;
		case CREDITS_MENU:
			menu_entries = CREDITS_NUM;
			break;
		case SETTINGS_MENU:
			menu_entries = PROFILE_SETTINGS_NUM + 4;
			break;
		default:
			break;
		}
		tick = ctrl->timeStamp;
		curr_buttons = ctrl->buttons;
		for (int i = 0; i < PHYS_BUTTONS_NUM; i++){
			if ((curr_buttons & HW_BUTTONS[i]) && !(old_buttons & HW_BUTTONS[i]))
				pressedTicks[i] = tick;
			else if (!(curr_buttons & HW_BUTTONS[i]) && (old_buttons & HW_BUTTONS[i]))
				pressedTicks[i] = 0;
			
			if (!isBtnActive(i))
				continue;
			
			switch (HW_BUTTONS[i]) {
			case SCE_CTRL_DOWN:
				cfg_i = (cfg_i + 1) % menu_entries;
				break;
			case SCE_CTRL_UP:
				if (--cfg_i < 0) cfg_i = menu_entries  -1;
				break;
			case SCE_CTRL_RIGHT:
				switch (menu_i){
				case REMAP_MENU: 
					profile_remap[cfg_i] = (profile_remap[cfg_i] + 1) % TARGET_REMAPS;
					break;
				case ANALOG_MENU:
					if (cfg_i < 4) profile_analog[cfg_i] = (profile_analog[cfg_i] + 1) % 128;
					else profile_analog[cfg_i] = !profile_analog[cfg_i];
					break;
				case TOUCH_MENU:
					if (cfg_i < 8)//Front Points xy
						profile_touch[cfg_i] = (profile_touch[cfg_i] + 1) 
							% ((cfg_i % 2) ? TOUCH_SIZE[1] : TOUCH_SIZE[0]);
					else if (cfg_i < 16)//Rear Points xy
						profile_touch[cfg_i] = (profile_touch[cfg_i] + 1)
							% ((cfg_i % 2) ? TOUCH_SIZE[3] : TOUCH_SIZE[2]);
					else 			//yes/no otion
						profile_touch[cfg_i] = !profile_touch[cfg_i];
					break;
				case GYRO_MENU:
					if (cfg_i < 6) //Sens & deadzone
						profile_gyro[cfg_i] = (profile_gyro[cfg_i] + 1) % 200;
					else if (cfg_i == 6) // Deadband
						profile_gyro[cfg_i] = min(2, profile_gyro[cfg_i] + 1);
					else if (cfg_i == 7) // Wheel mode
						profile_gyro[cfg_i] = (profile_gyro[cfg_i] + 1) % 2;
					else if (cfg_i < 10) // Reset wheel buttons
						profile_gyro[cfg_i] 
							= min(PHYS_BUTTONS_NUM - 1, profile_gyro[cfg_i] + 1);
					break;
				case CNTRL_MENU:
					if (cfg_i == 1)
						profile_controller[cfg_i] = min(5, profile_controller[cfg_i] + 1);
					else
						profile_controller[cfg_i] = !profile_controller[cfg_i];
					break;
				case SETTINGS_MENU:
					if (cfg_i < 2)
						profile_settings[cfg_i] 
							= min(PHYS_BUTTONS_NUM - 1, profile_settings[cfg_i] + 1);
					else if (cfg_i == 2)
						profile_settings[cfg_i] = !profile_settings[cfg_i];
					else if (cfg_i == 3)
						profile_settings[cfg_i] 
							= min(60, profile_settings[cfg_i] + 1);
					break;
				}
				break;
			case SCE_CTRL_LEFT:
				switch (menu_i){
				case REMAP_MENU:
					if (profile_remap[cfg_i]) 	
						profile_remap[cfg_i]--;
					else
						profile_remap[cfg_i] = TARGET_REMAPS - 1;
					break;
				case ANALOG_MENU:
					if (profile_analog[cfg_i]) 	
						profile_analog[cfg_i]--;
					else
						profile_analog[cfg_i] = cfg_i < 4 ? 127 : 1;
					break;
				case TOUCH_MENU:
					if (profile_touch[cfg_i]) 	
						profile_touch[cfg_i]--;
					else {
						if (cfg_i < 8)//front points xy
							profile_touch[cfg_i] = ((cfg_i % 2) ? TOUCH_SIZE[1] - 1 : TOUCH_SIZE[0] - 1);
						if (cfg_i < 16)//rear points xy
							profile_touch[cfg_i] = ((cfg_i % 2) ? TOUCH_SIZE[3] - 1 : TOUCH_SIZE[2] - 1);
						else //yes/no options
							profile_touch[cfg_i] = !profile_touch[cfg_i];
					}
					break;
				case GYRO_MENU:
					if (profile_gyro[cfg_i]) 	
						profile_gyro[cfg_i]--;
					else {
						if (cfg_i < 6) //Sens & deadzone
							profile_gyro[cfg_i] = 199;
						else if (cfg_i == 6) // deadband
							profile_gyro[cfg_i] = max(0, profile_gyro[cfg_i] - 1);
						else if (cfg_i == 7) //Wheel mode
							profile_gyro[cfg_i] = 1;
						else if (cfg_i < 10)  // Reset wheel btns
							profile_gyro[cfg_i] = max(0, profile_gyro[cfg_i] - 1);
					}
					break;
				case CNTRL_MENU:
					if (cfg_i == 1)
						profile_controller[cfg_i] = max(0, profile_controller[cfg_i] - 1);
					else
						profile_controller[cfg_i] = !profile_controller[cfg_i];
					break;
				case SETTINGS_MENU:
					if (cfg_i < 2)
						profile_settings[cfg_i] 
							= max(0, profile_settings[cfg_i] - 1);
					else if (cfg_i == 2)
						profile_settings[cfg_i] = !profile_settings[cfg_i];
					else if (cfg_i == 3)
						profile_settings[cfg_i] 
							= max(0, profile_settings[cfg_i] - 1);
					break;
				}
				break;
			case SCE_CTRL_LTRIGGER:
			case SCE_CTRL_L1:
				if (menu_i == REMAP_MENU){ //Sections navigation
					if (profile_remap[cfg_i] < 16)
						profile_remap[cfg_i] = 38;	//Rear touch custom
					else if (profile_remap[cfg_i] < 17)
						profile_remap[cfg_i] = 0;	//HW Buttons
					else if (profile_remap[cfg_i] < 18)
						profile_remap[cfg_i] = 16;	//Original
					else if (profile_remap[cfg_i] < 22)
						profile_remap[cfg_i] = 17;	//Disabled
					else if (profile_remap[cfg_i] < 26)
						profile_remap[cfg_i] = 18;	//Left stick
					else if (profile_remap[cfg_i] < 30)
						profile_remap[cfg_i] = 22;	//Right stick
					else if (profile_remap[cfg_i] < 34)
						profile_remap[cfg_i] = 26;	//Front touch default
					else if (profile_remap[cfg_i] < 38)
						profile_remap[cfg_i] = 30;	//Front touch custom
					else 
						profile_remap[cfg_i] = 34;	//Rear touch default
				}
				break;
			case SCE_CTRL_RTRIGGER:
			case SCE_CTRL_R1:
				if (menu_i == REMAP_MENU){ //Sections navigation
					if (profile_remap[cfg_i] < 16)
						profile_remap[cfg_i] = 16;	//Original
					else if (profile_remap[cfg_i] < 17)
						profile_remap[cfg_i] = 17;	//Disabled
					else if (profile_remap[cfg_i] < 18)
						profile_remap[cfg_i] = 18;	//Left stick
					else if (profile_remap[cfg_i] < 22)
						profile_remap[cfg_i] = 22;	//Right stick
					else if (profile_remap[cfg_i] < 26)
						profile_remap[cfg_i] = 26;	//Front touch default
					else if (profile_remap[cfg_i] < 30)
						profile_remap[cfg_i] = 30;	//Front touch custom
					else if (profile_remap[cfg_i] < 34)
						profile_remap[cfg_i] = 34;	//Rear touch default
					else if (profile_remap[cfg_i] < 38)
						profile_remap[cfg_i] = 38;	//Rear touch custom
					else 
						profile_remap[cfg_i] = 0;	//HW Buttons
				}
				break;
			case SCE_CTRL_SQUARE:
				switch (menu_i){
				case REMAP_MENU: 
					profile_remap[cfg_i] = PHYS_BUTTONS_NUM;
					break;
				case ANALOG_MENU:
					profile_analog[cfg_i] = PROFILE_ANALOG_DEF[cfg_i];
					break;
				case TOUCH_MENU: 
					profile_touch[cfg_i] = PROFILE_TOUCH_DEF[i];
					break;
				case GYRO_MENU:
					profile_gyro[cfg_i] = PROFILE_GYRO_DEF[cfg_i];
					break;
				case CNTRL_MENU:
					profile_controller[cfg_i] = PROFILE_CONTROLLER_DEF[cfg_i];
					break;
				case SETTINGS_MENU:
					if (cfg_i <= 2)
						profile_settings[cfg_i] = PROFILE_SETTINGS_DEF[cfg_i];
					break;
				}
				break;
			case SCE_CTRL_START:
				switch (menu_i){
				case REMAP_MENU: 
					profile_resetRemap();
					break;
				case ANALOG_MENU: 
					profile_resetAnalog();
					break;
				case TOUCH_MENU: 
					profile_resetTouch();
					break;
				case GYRO_MENU: 
					profile_resetGyro();
					break;
				case CNTRL_MENU: 
					profile_resetController();
					break;
				case SETTINGS_MENU: 
					profile_resetSettings();
					break;
				}
				break;
			case SCE_CTRL_CROSS:
				if (menu_i == MAIN_MENU){
					if (cfg_i == menu_entries-1) {
						ui_opened = 0;
						profile_saveLocal();
					} else {					
						menu_i = cfg_i + 1;
						cfg_i = 0;
					}
				} else if (menu_i == SETTINGS_MENU){
					if (cfg_i == PROFILE_SETTINGS_NUM) {
						profile_saveLocal();	
					} else if (cfg_i == PROFILE_SETTINGS_NUM + 1) {
						profile_loadLocal();			
					} else if (cfg_i == PROFILE_SETTINGS_NUM + 2) {
						profile_saveGlobal();			
					} else if (cfg_i == PROFILE_SETTINGS_NUM + 3) {
						profile_loadGlobal();			
					}
				} else if (menu_i == GYRO_MENU) {
					if (cfg_i == 10)
						sceMotionReset();
				}
				break;
			case SCE_CTRL_CIRCLE:
				if (menu_i == MAIN_MENU) {
					ui_opened = 0;
					profile_saveSettings();
					if (profile_settings[0])
						profile_saveLocal();
					delayedStart();
				} else {
					menu_i = MAIN_MENU;
					cfg_i = 0;
				}
				break;
			}
		}
		old_buttons = curr_buttons;
	}
	
}

void ui_draw(const SceDisplayFrameBuf *pParam){
	if (ui_opened) {
		new_frame = 1;
		ticker++;
		screen_h = pParam->height;
		screen_w = pParam->width;
		updateFramebuf(pParam);
		drawConfigMenu();	
	}
}

void ui_open(const SceDisplayFrameBuf *pParam){
	ui_opened = 1;
	cfg_i = 0;
}
