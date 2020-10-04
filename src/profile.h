#ifndef _PROFILE_H_
#define _PROFILE_H_

#define TARGET_REMAPS               42 // Supported target remaps num

#define PROFILE_REMAP_NUM           38 // Supported buttons num
#define PROFILE_ANALOG_NUM          8
#define PROFILE_GYRO_NUM			11
#define PROFILE_TOUCH_NUM			18
#define PROFILE_CONTROLLER_NUM		3
#define PROFILE_SETTINGS_NUM		4

#define PROFILE_REMAP_DEF           16
extern const uint8_t PROFILE_ANALOG_DEF[PROFILE_ANALOG_NUM];
extern const uint16_t PROFILE_TOUCH_DEF[PROFILE_TOUCH_NUM];
extern const uint8_t PROFILE_GYRO_DEF[PROFILE_GYRO_NUM];
extern const uint8_t PROFILE_CONTROLLER_DEF[PROFILE_CONTROLLER_NUM];
extern const uint8_t PROFILE_SETTINGS_DEF[PROFILE_SETTINGS_NUM];

extern uint8_t profile_remap[PROFILE_REMAP_NUM];
extern uint8_t profile_analog[PROFILE_ANALOG_NUM];
extern uint8_t profile_gyro[PROFILE_GYRO_NUM];
extern uint16_t profile_touch[PROFILE_TOUCH_NUM];
extern uint8_t profile_controller[PROFILE_CONTROLLER_NUM];
extern uint8_t profile_settings[PROFILE_SETTINGS_NUM];

extern void profile_resetRemap();
extern void profile_resetAnalog();
extern void profile_resetTouch();
extern void profile_resetGyro();
extern void profile_resetController();
extern void profile_resetSettings();

extern void profile_saveSettings();
extern void profile_loadSettings();
extern void profile_saveGlobal();
extern void profile_saveLocal();
extern void profile_loadGlobal();
extern void profile_loadLocal();
extern void profile_savePS4Link();

#endif
