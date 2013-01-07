/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 1995 Crack dot Com
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This software was released into the Public Domain. As with most public
 *  domain software, no warranty is made or implied by Crack dot Com, by
 *  Jonathan Clark, or by Sam Hocevar.
 */

#ifndef __JOYSTICK_HPP_
#define __JOYSTICK_HPP_

#include "common.h"
#include "event.h"

int joy_init(int argc, char **argv); // returns 0 if no joystick is available
void joy_status(int &b1, int &b2, int &b3, int &xv, int &yv);
void joy_calibrate();

void joy_eval_axis(int device_index, int axis_index, int axis_value, int &event_type, int &event_key);
void joy_eval_button(int device_index, int button_index, int &event_key);

struct touch_base
{
	touch_base() : touch_index(-1), top_left(0), size(-1), alpha(1.0f), flash_start_tick(0) { }
	bool inside(const ivec2 &v);
	void resize(const ivec2 &topleft, const ivec2 &s, const float xscale, const float yscale, const int width, const int height);
	void start_flash(const uint32_t start_tick, int index);
	void stop_flash();
	bool visible();

	int touch_index;
	ivec2 top_left;
	ivec2 size;
	float alpha;
	uint32_t flash_start_tick;

	static const uint32_t flash_ticks = 10;
};

struct touch_statbar : public touch_base
{
	touch_statbar() : touch_base() { }
	void mouse_motion(const int current_touch_index, const ivec2 &touch, Event &ev);
	void mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev);
	void mouse_up(const int current_touch_index, Event &ev);
	void resize();

	static const int weapon_icon_width;
	int selected_weapon;
};

struct touch_move : public touch_base
{
	touch_move() : touch_base() { }
	void mouse_motion(const int current_touch_index, const ivec2 &touch, Event &ev);
	void mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev);
	void mouse_up(const int current_touch_index, Event &ev);
	void resize(float xscale, float yscale);

	ivec2 move_threshold;
	bool move_up_pressed;
	bool move_down_pressed;
	bool move_left_pressed;
	bool move_right_pressed;
};

struct touch_aim : public touch_base
{
	touch_aim() : touch_base() { }
	void mouse_motion(const int current_touch_index, const ivec2 &touch, Event &ev);
	void mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev);
	void mouse_up(const int current_touch_index, Event &ev);
	void resize(float xscale, float yscale);

	int aim_threshold;
};

struct touch_pause : public touch_base
{
	touch_pause() : touch_base() { }
	void mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev);
	void mouse_up(const int current_touch_index, Event &ev);
	void resize();
};

struct touch_special : public touch_base
{
	touch_special() : touch_base() { }
	void mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev);
	void mouse_up(const int current_touch_index, Event &ev);
	void resize();

	bool pressed;
};

struct touch_controls
{
	touch_controls();
	~touch_controls();
	void mouse_motion(const int current_touch_index, const ivec2 &touchgame, const ivec2 &touchview, Event &ev);
	void mouse_down(const int current_touch_index, const ivec2 &touchgame, const ivec2 &touchview, Event &ev);
	void mouse_up(const int current_touch_index, Event &ev);
	void resize();
	bool visible();
	void get_dpi(int &xdpi, int &ydpi);
	void flash(const int index);
	void show_help(const char *text);

	touch_move move;
	touch_aim aim;
	touch_statbar statbar;
	touch_pause pause;
	touch_special special;

	float alpha;

	int last_flash_index;

	static const size_t num_train_messages = 12;
	char * train_messages[num_train_messages];
};

extern touch_controls touch;

#endif
