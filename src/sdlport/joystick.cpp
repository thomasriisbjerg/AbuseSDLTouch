/*
 *  Abuse - dark 2D side-scrolling platform game
 *  Copyright (c) 2001 Anthony Kruize <trandor@labyrinth.net.au>
 *  Copyright (c) 2005-2011 Sam Hocevar <sam@hocevar.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software Foundation,
 *  Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 */

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include "joy.h"
//#include <SDL/sdl.h>
#include <SDL.h>
#include "common.h"
#include "event.h"
#include "setup.h"
#include "jwindow.h"
#include "dev.h"
#include "lisp.h"

#if 0 && defined(__QNXNTO__)
#include <screen/screen.h>
#include <SDL_sysvideo.h>
#endif

extern WindowManager *wm;
extern int xwinres, ywinres;

struct jjoystick;
struct jaxis;
struct jbutton;

enum jaxis_type
{
	AXIS_TYPE_NONE,
	AXIS_TYPE_KEY,
	AXIS_TYPE_AIM,
};
struct jaxis
{
	jaxis() : type(AXIS_TYPE_NONE), min(0), max(0), positive_key(0), negative_key(0) { }
	jaxis_type type;
	int min;
	int max;
	union
	{
		int positive_key;
		int aim_type;
	};
	int negative_key;
};
struct jbutton
{
	jbutton() : key(0) { }
	int key;
};
struct jjoystick
{
	jjoystick() : name(0), axis(0), num_axes(0), button(0), num_buttons(0) { }
	~jjoystick()
	{
		if (axis)
			delete[] axis;
		if (button)
			delete[] button;
	}
	const char *name;
	jaxis *axis;
	int num_axes;
	jbutton *button;
	int num_buttons;
};

static jjoystick *joystick;
static int num_joysticks;

int joy_init( int argc, char **argv )
{
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
	{
		printf("Unable to initialise SDL joystick : %s\n", SDL_GetError());
		return 0;
	}

	num_joysticks = SDL_NumJoysticks();

	if (num_joysticks <= 0)
	{
		printf("No joysticks found\n");
		return 0;
	}
	
	// alloc n joysticks
	joystick = new jjoystick[num_joysticks];

	for (int i = 0; i < num_joysticks; i++)
	{
		SDL_Joystick *j = SDL_JoystickOpen(i);
		const char *joystick_name = SDL_JoystickName(i);

		// if ! joystick_name matches a mapping
		// continue

		joystick[i].name = joystick_name;
		joystick[i].num_axes = SDL_JoystickNumAxes(j);
		joystick[i].axis = new jaxis[joystick[i].num_axes];
		joystick[i].num_buttons = SDL_JoystickNumButtons(j);
		joystick[i].button = new jbutton[joystick[i].num_buttons];

		// if joystick name matches joystick input mapping name
		// open joystick i
		// install mapping for index i
	}

	SDL_JoystickEventState(SDL_ENABLE);
	SDL_Joystick *joystick = SDL_JoystickOpen(0);
		
	return joystick != 0;
}

void joy_eval_button(int device_index, int button_index, int &event_key)
{

}

void joy_eval_axis(int device_index, int axis_index, int axis_value, int &event_type, int &event_key)
{
	if (joystick[device_index].axis[axis_index].type == AXIS_TYPE_KEY)
	{
		if (axis_value < joystick[device_index].axis[axis_index].max && axis_value > joystick[device_index].axis[axis_index].min)
			event_type = EV_KEYRELEASE;
		else
			event_type = EV_KEY;
		event_key = axis_value < 0 ? joystick[device_index].axis[axis_index].negative_key : joystick[device_index].axis[axis_index].positive_key;
	}
	if (joystick[device_index].axis[axis_index].type == AXIS_TYPE_AIM)
	{
		if (axis_value < joystick[device_index].axis[axis_index].max && axis_value > joystick[device_index].axis[axis_index].min)
			event_key = 0;
		else
			event_key = axis_value;
		event_type = joystick[device_index].axis[axis_index].aim_type ? EV_AIM_Y : EV_AIM_X;
	}
}

void joy_status( int &b1, int &b2, int &b3, int &xv, int &yv )
{
    /* Do Nothing */
}

void joy_calibrate()
{
    /* Do Nothing */
}

void create_event(const int key, const int type, Event &ev, bool &first_event)
{
	Event *e = first_event ? &ev : new Event();
	e->type = type;
	e->key = key;
	e->mouse_move = ev.mouse_move;
	e->mouse_button = ev.mouse_button;
	if (!first_event)
		wm->Push(e);
	first_event = false;
}

void create_event(const int key, const int type, Event &ev)
{
	bool first_event = true;
	create_event(key, type, ev, first_event);
}
static ivec2 aim_topleft(-240, -240);
static ivec2 aim_size(240, 240);
static ivec2 move_topleft(0, -240);
static ivec2 move_size(240, 240);
static ivec2 special_topleft(-33, 33);
static ivec2 special_size(33, 33);
static ivec2 pause_topleft(85, 50);
static ivec2 pause_size(150, 100);
static ivec2 statbar_topleft(47, 0);
static ivec2 statbar_size(7*33, 33);

bool touch_base::inside(const ivec2 &v)
{
	return v > top_left && v <= (top_left + size);
}

void touch_base::start_flash(const uint32_t start_tick, int index)
{
	// skip if we're already flashing
	if (current_level && current_level->tick_counter() > flash_start_tick && current_level->tick_counter() < flash_start_tick + flash_ticks)
	{
		//printf("start_flash already flashing from tick %i index %i\n", flash_start_tick, index); fflush(stdout);
		return;
	}

	flash_start_tick = start_tick;
	//printf("start_flash tick %i\n", start_tick); fflush(stdout);
}

void touch_base::stop_flash()
{
	flash_start_tick = 0;
}

bool touch_base::visible()
{
	if (current_level && current_level->tick_counter() >= flash_start_tick && current_level->tick_counter() < flash_start_tick + flash_ticks && ((current_level->tick_counter() - flash_start_tick) & 2))
		return false;
	return true;
}

const int touch_statbar::weapon_icon_width = 33;

bool touch_statbar::mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (wm->m_first == 0 && the_game->state == RUN_STATE && inside(touch) && touch_index == -1)
	{
		touch_index = current_touch_index;

		int dx = touch.x - top_left.x;
		int weapon_index = dx / weapon_icon_width;
		weapon_index = weapon_index < 0 ? 0 : weapon_index;
		weapon_index = weapon_index > 7 ? 7 : weapon_index;

		create_event('1' + weapon_index, EV_KEY, ev);

		selected_weapon = weapon_index;
		return true;
	}
	return false;
}

bool touch_statbar::mouse_motion(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (the_game->state == RUN_STATE && touch_index == current_touch_index)
	{
		int dx = touch.x - top_left.x;
		int weapon_index = dx / weapon_icon_width;
		weapon_index = weapon_index < 0 ? 0 : weapon_index;
		weapon_index = weapon_index > 7 ? 7 : weapon_index;

		if (weapon_index != selected_weapon)
		{
			bool first_event = true;
			create_event('1' + selected_weapon, EV_KEYRELEASE, ev, first_event);
			create_event('1' + weapon_index, EV_KEY, ev, first_event);
			selected_weapon = weapon_index;
		}
		return true;
	}
	return false;
}

bool touch_statbar::mouse_up(const int current_touch_index, Event &ev)
{
	if (touch_index == current_touch_index)
	{
		touch_index = -1;
		create_event('1' + selected_weapon, EV_KEYRELEASE, ev);
		return true;
	}
	return false;
}

void touch_base::resize(const ivec2 &topleft, const ivec2 &s, const float xscale, const float yscale, const int width, const int height)
{
	size.x = s.x * xscale;
	size.y = s.y * yscale;

	top_left.x = topleft.x < 0 ? (width + topleft.x - (size.x - s.x)) : topleft.x;
	top_left.y = topleft.y < 0 ? (height + topleft.y - (size.y - s.y)) : topleft.y;
}

void touch_statbar::resize()
{
	touch_base::resize(statbar_topleft, statbar_size, 1.0f, 1.0f, xres, yres);
}

bool touch_move::mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (wm->m_first == 0 && the_game->state == RUN_STATE && inside(touch) && touch_index == -1)
	{
		touch_index = current_touch_index;

		ivec2 d = touch - (top_left + size / 2);

		bool first_event = true;

		if (d.x > move_threshold.x && !move_right_pressed)
		{
			move_right_pressed = true;
			create_event(get_key_binding("right", 0), EV_KEY, ev, first_event);
		}
		if (d.x < -move_threshold.x && !move_left_pressed)
		{
			move_left_pressed = true;
			create_event(get_key_binding("left", 0), EV_KEY, ev, first_event);
		}
		if (d.y < -move_threshold.y && !move_up_pressed)
		{
			move_up_pressed = true;
			create_event(get_key_binding("up", 0), EV_KEY, ev, first_event);
		}
		if (d.y > move_threshold.y && !move_down_pressed)
		{
			move_down_pressed = true;
			create_event(get_key_binding("down", 0), EV_KEY, ev, first_event);
		}
		return true;
	}
	return false;
}

bool touch_move::mouse_motion(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (the_game->state == RUN_STATE && touch_index == current_touch_index)
	{
		ivec2 d = touch - (top_left + size / 2);

		bool first_event = true;

		if ((d.x > move_threshold.x) != move_right_pressed)
		{
			move_right_pressed = !move_right_pressed;
			create_event(get_key_binding("right", 0), move_right_pressed ? EV_KEY : EV_KEYRELEASE, ev, first_event);
		}
		if ((d.x < -move_threshold.x) != move_left_pressed)
		{
			move_left_pressed = !move_left_pressed;
			create_event(get_key_binding("left", 0), move_left_pressed ? EV_KEY : EV_KEYRELEASE, ev, first_event);
		}
		if ((d.y < -move_threshold.y) != move_up_pressed)
		{
			move_up_pressed = !move_up_pressed;
			create_event(get_key_binding("up", 0), move_up_pressed ? EV_KEY : EV_KEYRELEASE, ev, first_event);
		}
		if ((d.y > move_threshold.y) != move_down_pressed)
		{
			move_down_pressed = !move_down_pressed;
			create_event(get_key_binding("down", 0), move_down_pressed ? EV_KEY : EV_KEYRELEASE, ev, first_event);
		}
		return true;
	}
	return false;
}

bool touch_move::mouse_up(const int current_touch_index, Event &ev)
{
	if (touch_index == current_touch_index)
	{
		touch_index = -1;

		bool first_event = true;

		if (move_right_pressed)
		{
			move_right_pressed = false;
			create_event(get_key_binding("right", 0), EV_KEYRELEASE, ev, first_event);
		}
		if (move_left_pressed)
		{
			move_left_pressed = false;
			create_event(get_key_binding("left", 0), EV_KEYRELEASE, ev, first_event);
		}
		if (move_up_pressed)
		{
			move_up_pressed = false;
			create_event(get_key_binding("up", 0), EV_KEYRELEASE, ev, first_event);
		}
		if (move_down_pressed)
		{
			move_down_pressed = false;
			create_event(get_key_binding("down", 0), EV_KEYRELEASE, ev, first_event);
		}
		return true;
	}
	return false;
}

void touch_move::resize(const float xscale, const float yscale)
{
	touch_base::resize(move_topleft, move_size, xscale, yscale, xwinres, ywinres);
	move_threshold = size / 6;
}

int _best_angle; // this is read from video.cpp update_window_done() and written from cop.cpp top_ai()

bool touch_aim::mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (wm->m_first == 0 && the_game->state == RUN_STATE && inside(touch) && touch_index == -1)
	{
		touch_index = current_touch_index;

		ivec2 d = touch - (top_left + size / 2);
		int dlen2 = d.x * d.x + d.y * d.y;

		if (dlen2 < aim_threshold * aim_threshold)
			d = ivec2(0, 0);

		bool first_event = true;
		create_event(d.x, EV_AIM_X, ev, first_event);
		create_event(d.y, EV_AIM_Y, ev, first_event);
		create_event(get_key_binding("b2", 0), EV_KEY, ev, first_event);
		return true;
	}
	return false;
}

bool touch_aim::mouse_motion(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (the_game->state == RUN_STATE && touch_index == current_touch_index)
	{
		ivec2 d = touch - (top_left + size / 2);
		int dlen2 = d.x * d.x + d.y * d.y;

		if (dlen2 < aim_threshold * aim_threshold)
			d = ivec2(0, 0);

		bool first_event = true;
		create_event(d.x, EV_AIM_X, ev, first_event);
		create_event(d.y, EV_AIM_Y, ev, first_event);
		return true;
	}
	return false;
}

bool touch_aim::mouse_up(const int current_touch_index, Event &ev)
{
	if (touch_index == current_touch_index)
	{
		touch_index = -1;
		bool first_event = true;
		create_event(0, EV_AIM_X, ev, first_event);
		create_event(0, EV_AIM_Y, ev, first_event);
		create_event(get_key_binding("b2", 0), EV_KEYRELEASE, ev, first_event);
		return true;
	}
	return false;
}

void touch_aim::resize(const float xscale, const float yscale)
{
	touch_base::resize(aim_topleft, aim_size, xscale, yscale, xwinres, ywinres);
	aim_threshold = size.x / 8;
}

bool touch_pause::mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (wm->m_first == 0 && (the_game->state == RUN_STATE || the_game->state == PAUSE_STATE) && inside(touch) && touch_index == -1)
	{
		touch_index = current_touch_index;
		create_event('p', EV_KEY, ev);
		return true;
	}
	return false;
}

bool touch_pause::mouse_up(const int current_touch_index, Event &ev)
{
	if (touch_index == current_touch_index)
	{
		touch_index = -1;
		create_event('p', EV_KEYRELEASE, ev);
		return true;
	}
	return false;
}

void touch_pause::resize()
{
	touch_base::resize(pause_topleft, pause_size, 1.0f, 1.0f, xres, yres);
}

bool touch_special::mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (wm->m_first == 0 && the_game->state == RUN_STATE && inside(touch) && touch_index == -1)
	{
		touch_index = current_touch_index;
		create_event(get_key_binding("b1", 0), pressed ? EV_KEYRELEASE : EV_KEY, ev);

		pressed = !pressed;
		return true;
	}
	return false;
}

bool touch_special::mouse_up(const int current_touch_index, Event &ev)
{
	if (touch_index == current_touch_index)
	{
		touch_index = -1;
		return true;
	}
	return false;
}

void touch_special::resize()
{
	touch_base::resize(special_topleft, special_size, 1.0f, 1.0f, xres, yres);
}

bool touch_fullscreen::mouse_down(const int current_touch_index, const ivec2 &touch, Event &ev)
{
	if (enabled && touch_index == -1)
	{
		touch_index = current_touch_index;
		create_event(JK_SPACE, EV_KEY, ev);
		return true;
	}
	return false;
}

bool touch_fullscreen::mouse_up(const int current_touch_index, Event &ev)
{
	if (touch_index == current_touch_index)
	{
		touch_index = -1;
		create_event(JK_SPACE, EV_KEYRELEASE, ev);
		enabled = false;
		return true;
	}
	return false;
}

void touch_fullscreen::enable(const bool value)
{
	if (enabled && value == false && touch_index != -1)
	{
		Event *e = new Event();
		e->type = EV_KEYRELEASE;
		e->key = JK_SPACE;
		e->mouse_button = 0;
		e->mouse_move = ivec2(0);
		wm->Push(e);
	}
	enabled = value;
}

void touch_controls::mouse_motion(const int current_touch_index, const ivec2 &touchgame, const ivec2 &touchview, Event &ev)
{
	statbar.mouse_motion(current_touch_index, touchgame, ev) ||
	move.mouse_motion(current_touch_index, touchview, ev) ||
	aim.mouse_motion(current_touch_index, touchview, ev);
}

void touch_controls::mouse_down(const int current_touch_index, const ivec2 &touchgame, const ivec2 &touchview, Event &ev)
{
	fullscreen.mouse_down(current_touch_index, touchgame, ev) ||
	statbar.mouse_down(current_touch_index, touchgame, ev) ||
	pause.mouse_down(current_touch_index, touchgame, ev) ||
	special.mouse_down(current_touch_index, touchgame, ev) ||
	move.mouse_down(current_touch_index, touchview, ev) ||
	aim.mouse_down(current_touch_index, touchview, ev);
}

void touch_controls::mouse_up(const int current_touch_index, Event &ev)
{
	fullscreen.mouse_up(current_touch_index, ev) ||
	statbar.mouse_up(current_touch_index, ev) ||
	pause.mouse_up(current_touch_index, ev) ||
	special.mouse_up(current_touch_index, ev) ||
	move.mouse_up(current_touch_index, ev) ||
	aim.mouse_up(current_touch_index, ev);
}

touch_controls::touch_controls() :
		move(),
		aim(),
		statbar(),
		pause(),
		special(),
		fullscreen(),
		alpha(1.0f),
		last_flash_index(-1)
{
	for (unsigned int i = 0; i < num_train_messages; i++)
		train_messages[i] = 0;
}

touch_controls::~touch_controls()
{
	for (unsigned int i = 0; i < num_train_messages; i++)
		if (train_messages[i])
		{
			free(train_messages[i]);
			train_messages[i] = 0;
		}
}

void touch_controls::get_dpi(int &xdpi, int &ydpi)
{
#if defined(__QNXNTO__)
#if 0
	const int defaultDPI = 170;
	screen_display_t screen_disp;
	int rc;
	rc = screen_get_window_property_pv(current_video->hidden->screenWindow, SCREEN_PROPERTY_DISPLAY, (void **)&screen_disp);
	if (rc) {
		xdpi = defaultDPI;
		ydpi = defaultDPI;
	}
	int screen_phys_size[2];
	rc = screen_get_display_property_iv(screen_disp, SCREEN_PROPERTY_PHYSICAL_SIZE, screen_phys_size);
	if (rc) {
		xdpi = defaultDPI;
		ydpi = defaultDPI;
	}
	//Simulator will return 0,0 for physical size of the screen, so use 170 as default dpi
	if ((screen_phys_size[0] == 0) && (screen_phys_size[1] == 0)) {
		xdpi = defaultDPI;
		ydpi = defaultDPI;
	} else {
		int screen_resolution[2];
		rc = screen_get_display_property_iv(screen_disp, SCREEN_PROPERTY_SIZE, screen_resolution);
		if (rc) {
			xdpi = defaultDPI;
			ydpi = defaultDPI;
		}
		xdpi = int(0.0393700787f * float(screen_resolution[0]) / float(screen_phys_size[0]));
		ydpi = int(0.0393700787f * float(screen_resolution[1]) / float(screen_phys_size[1]));
	}
#else
	xdpi = 170;
	ydpi = 170;
#endif
#elif defined(__ANDROID__)
	/*DisplayMetrics dm = new DisplayMetrics();
	getWindowManager().getDefaultDisplay().getMetrics(dm);

	// will either be DENSITY_LOW, DENSITY_MEDIUM or DENSITY_HIGH
	int dpiClassification = dm.densityDpi;

	// these will return the actual dpi horizontally and vertically
	float xDpi = dm.xdpi;
	float yDpi = dm.ydpi;*/
	xdip = 160;
	ydpi = 160;
#else
	xdpi = 132;
	ydpi = 132;
#endif
}

void touch_controls::resize()
{
	/*int xdpi = 170;
	int ydpi = 170;
	get_dpi(xdpi, ydpi);
	float xscale = xdpi / 170.0f;
	float yscale = ydpi / 170.0f;*/
	float xscale = 1.33f;
	float yscale = 1.33f;
	move.resize(xscale, yscale);
	aim.resize(xscale, yscale);
	statbar.resize();
	pause.resize();
	special.resize();

	for (unsigned int i = 0; i < num_train_messages; i++)
	{
		const size_t commandsize = 128;
		char command[commandsize];
		char const *c = command;
		snprintf(command, commandsize, "(get_train_msg %i)", i);
		LObject *o = LObject::Compile(c);
		train_messages[i] = strdup(lstring_value(o->Eval()));
	}
}

bool touch_controls::visible()
{
	return (wm && wm->m_first == 0) && playing_state(the_game->state);
}

void touch_controls::flash(const int index)
{
	//printf("touch flash %i\n", index); fflush(stdout);

	if (index != last_flash_index)
	{
		switch (last_flash_index)
		{
		case 0:
			move.stop_flash(); // all keys
			aim.stop_flash();
			break;
		case 2:
		case 3:
		case 4:
		case 8:
		case 11:
			move.stop_flash();
			break;
		case 5:
			special.stop_flash();
			break;
		case 6:
			statbar.stop_flash();
			break;
		case 7:
			move.stop_flash();
			break;
		}
	}

	uint32_t current_tick = 0;
	if (current_level)
		current_tick = current_level->tick_counter();

	switch (index)
	{
	case 0:
		move.start_flash(current_tick, index); // all keys
		aim.start_flash(current_tick, index);
		break;
	case 2:
	case 3:
	case 4:
	case 8:
	case 11:
		move.start_flash(current_tick, index); // down key
		break;
	case 5:
		special.start_flash(current_tick, index);
		break;
	case 6:
		statbar.start_flash(current_tick, index);
		break;
	case 7:
		move.start_flash(current_tick, index); // up key
		break;
	}

	last_flash_index = index;
}

void touch_controls::show_help(const char *text)
{
	for (unsigned int i = 0; i < num_train_messages; i++)
		if (strcmp(train_messages[i], text) == 0)
			flash(i);
}

touch_controls touch;
