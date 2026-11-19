#ifndef MOUNTAIN_APP_H
#define MOUNTAIN_APP_H
#include "render.h"
bool mountain_open(const char *asset_dir, int width, int height);
bool mountain_step(void);
void mountain_close(void);
int mountain_sound(void);
int mountain_action(int action);
void mountain_input(float forward, float right);
void mountain_look(float yaw, float pitch);
void mountain_conditions(int season, int weather, float hour);
void mountain_seed(uint32_t seed);
void mountain_pause(int state);
const char *mountain_label(int key);
const char *mountain_option(int kind, int index);
float mountain_height(float x, float z);
float mountain_water(void);
double mountain_value(int key);
void mountain_resize(int w, int h);
void mountain_frame(void);
void mountain_xr_active(int enabled);
void mountain_walk(float dt, float forward, float right);
void mountain_xr_eye(const Mat *projection, const Mat *view, int x, int y, int w, int h);
bool mountain_photo(void);
#endif
