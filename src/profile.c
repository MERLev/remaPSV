#include <vitasdk.h>
#include <taihen.h>
#include <libk/stdio.h>
#include "main.h"
#include "profile.h"

const uint8_t PROFILE_ANALOG_DEF[PROFILE_ANALOG_NUM] = {
	30, 30, 30, 30,		//Deadzone
	0, 0, 0, 0			//Force digital
};

const uint16_t PROFILE_TOUCH_DEF[PROFILE_TOUCH_NUM] = {
	600,  272,	//Front TL
	1280, 272,	//		TR
	600,  816,	//		BL
	1280, 816,	//		BR
	600,  272,	//Rear	TL
	1280, 272,	//		TR
	600,  608,	//		BL
	1280, 608,	//		BR
	1, 1		//Touch mode
};

const uint8_t PROFILE_GYRO_DEF[PROFILE_GYRO_NUM] = {
	127, 127, 127,	//Sensivity
	15, 15, 15,		//Deadzone
	0,				//Deadband
	0,				//Whel mode
	6,				//Reset wheel btn 1 | LTrigger
	7,				//Reset wheel btn 2 | RTrigger
	0
};

const uint8_t PROFILE_CONTROLLER_DEF[PROFILE_CONTROLLER_NUM] = {
	0, 0, 0
};

const uint8_t PROFILE_SETTINGS_DEF[PROFILE_SETTINGS_NUM] = {
	4, 3, //Opening keys
	1,	  //Autosave game profile
	10	  //Startup delay
};

uint8_t profile_remap[PROFILE_REMAP_NUM];
uint8_t profile_analog[PROFILE_ANALOG_NUM];
uint8_t profile_gyro[PROFILE_GYRO_NUM];
uint16_t profile_touch[PROFILE_TOUCH_NUM];
uint8_t profile_controller[PROFILE_CONTROLLER_NUM];
uint8_t profile_settings[PROFILE_SETTINGS_NUM];

static char fname[128];

// Reset options per-menu
void profile_resetRemap(){
	for (int i = 0; i < PROFILE_REMAP_NUM; i++)
		profile_remap[i] = PROFILE_REMAP_DEF;
}
void profile_resetAnalog(){
	for (int i = 0; i < PROFILE_ANALOG_NUM; i++)
		profile_analog[i] = PROFILE_ANALOG_DEF[i];
}
void profile_resetTouch(){
	for (int i = 0; i < PROFILE_TOUCH_NUM; i++)
		profile_touch[i] = PROFILE_TOUCH_DEF[i];
}
void profile_resetGyro() {
	for (int i = 0; i < PROFILE_GYRO_NUM; i++)
		profile_gyro[i] = PROFILE_GYRO_DEF[i];
}
void profile_resetController(){
	for (int i = 0; i < PROFILE_CONTROLLER_NUM; i++)
		profile_controller[i] = PROFILE_CONTROLLER_DEF[i];
}
void profile_resetSettings(){
	for (int i = 0; i < PROFILE_SETTINGS_NUM; i++)
		profile_settings[i] = PROFILE_SETTINGS_DEF[i];
}

void profile_saveSettings(){
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Opening settings config file and saving the config
	SceUID fd = sceIoOpen("ux0:/data/remaPSV/settings.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_settings, PROFILE_SETTINGS_NUM);
	sceIoClose(fd);
}

void profile_loadSettings(){
	profile_resetSettings();
	
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Loading config file for the selected app if exists
	SceUID fd = sceIoOpen("ux0:/data/remaPSV/settings.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_settings, PROFILE_SETTINGS_NUM);
		sceIoClose(fd);
	}
}

void profile_saveGlobal() {
	SceUID fd;
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Opening remap config file and saving it
	fd = sceIoOpen("ux0:/data/remaPSV/remap.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_remap, PROFILE_REMAP_NUM);
	sceIoClose(fd);
	
	// Opening analog config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_analog, PROFILE_ANALOG_NUM);
	sceIoClose(fd);
	
	// Opening touch config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/touch.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_touch, PROFILE_TOUCH_NUM*2);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_gyro, PROFILE_GYRO_NUM);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	fd = sceIoOpen("ux0:/data/remaPSV/controllers.bin", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_controller, PROFILE_CONTROLLER_NUM);
	sceIoClose(fd);
}

void profile_saveLocal() {
	SceUID fd;
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	sprintf(fname, "ux0:/data/remaPSV/%s", titleid);
	sceIoMkdir(fname, 0777);
	
	// Opening remap config file and saving it
	sprintf(fname, "ux0:/data/remaPSV/%s/remap.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_remap, PROFILE_REMAP_NUM);
	sceIoClose(fd);
	
	// Opening analog config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/analogs.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_analog, PROFILE_ANALOG_NUM);
	sceIoClose(fd);
	
	// Opening touch config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/touch.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_touch, PROFILE_TOUCH_NUM*2);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/gyro.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_gyro, PROFILE_GYRO_NUM);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/%s/controllers.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_controller, PROFILE_CONTROLLER_NUM);
	sceIoClose(fd);
}

void profile_savePS4Link() {
	SceUID fd;
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	sprintf(fname, "ux0:/data/remaPSV/NPXS10013");
	sceIoMkdir(fname, 0777);
	
	// Opening remap config file and saving it
	sprintf(fname, "ux0:/data/remaPSV/NPXS10013/remap.bin");
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_remap, PROFILE_REMAP_NUM);
	sceIoClose(fd);
	
	// Opening analog config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/NPXS10013/analogs.bin");
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_analog, PROFILE_ANALOG_NUM);
	sceIoClose(fd);
	
	// Opening touch config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/NPXS10013/touch.bin");
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_touch, PROFILE_TOUCH_NUM*2);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/NPXS10013/gyro.bin");
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_gyro, PROFILE_GYRO_NUM);
	sceIoClose(fd);
	
	// Opening gyro config file and saving the config
	sprintf(fname, "ux0:/data/remaPSV/NPXS10013/controllers.bin");
	fd = sceIoOpen(fname, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	sceIoWrite(fd, profile_controller, PROFILE_CONTROLLER_NUM);
	sceIoClose(fd);
}

void profile_loadGlobal() {
	profile_resetRemap();
	profile_resetAnalog();
	profile_resetTouch();
	profile_resetGyro();
	profile_resetController();
	
	SceUID fd;
	
	// Just in case the folder doesn't exist
	sceIoMkdir("ux0:/data/remaPSV", 0777); 
	
	// Loading config file for the selected app if exists
	fd = sceIoOpen("ux0:/data/remaPSV/remap.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_remap, PROFILE_REMAP_NUM);
		sceIoClose(fd);
	}
	
	// Loading analog config file
	fd = sceIoOpen("ux0:/data/remaPSV/analogs.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_analog, PROFILE_ANALOG_NUM);
		sceIoClose(fd);
	}
	
	// Loading touch config file
	fd = sceIoOpen("ux0:/data/remaPSV/touch.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_touch, PROFILE_TOUCH_NUM*2);
		sceIoClose(fd);
	}
	
	// Loading gyro config file
	fd = sceIoOpen("ux0:/data/remaPSV/gyro.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_gyro, PROFILE_GYRO_NUM);
		sceIoClose(fd);
	}
	
	// Loading controllers config file
	fd = sceIoOpen("ux0:/data/remaPSV/controllers.bin", SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_controller, PROFILE_CONTROLLER_NUM);
		sceIoClose(fd);
	}
}

void profile_loadLocal() {
	// Check if folder exists
	SceIoStat stat = {0};
	sprintf(fname, "ux0:/data/remaPSV/%s", titleid);
    int ret = sceIoGetstat(fname, &stat);
	if (ret < 0)
		return;
	
	profile_resetRemap();
	profile_resetAnalog();
	profile_resetTouch();
	profile_resetGyro();
	profile_resetController();
	
	SceUID fd;
	
	// Loading remap config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/remap.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_remap, PROFILE_REMAP_NUM);
		sceIoClose(fd);
	}
	
	// Loading analog config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/analogs.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_analog, PROFILE_ANALOG_NUM);
		sceIoClose(fd);
	}
	
	// Loading touch config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/touch.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_touch, PROFILE_TOUCH_NUM*2);
		sceIoClose(fd);
	}
	
	// Loading gyro config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/gyro.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_gyro, PROFILE_GYRO_NUM);
		sceIoClose(fd);
	}
	
	// Loading gyro config file for the selected app if exists
	sprintf(fname, "ux0:/data/remaPSV/%s/controllers.bin", titleid);
	fd = sceIoOpen(fname, SCE_O_RDONLY, 0777);
	if (fd >= 0){
		sceIoRead(fd, profile_controller, PROFILE_CONTROLLER_NUM);
		sceIoClose(fd);
	}
}
