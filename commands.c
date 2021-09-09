/* Copyright 2011, 2012, 2014 Bert Muennich
 *
 * This file is part of sxiv.
 *
 * sxiv is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * sxiv is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with sxiv.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include "sxiv.h"
#include "config.h"

void remove_file(int, int);
void load_image(int);
int mark_image(int, int);
void close_info(void);
void open_info(void);
int ptr_third_x(void);
void redraw(void);
void reset_cursor(void);
void animate(void);
void slideshow(void);
void set_timeout(timeout_f, int, int);
void reset_timeout(timeout_f);

extern appmode_t mode;
extern img_t img;
extern tns_t tns;
extern win_t win;

extern fileinfo_t *files;
extern int filecnt, fileidx;
extern int alternate;
extern int markcnt;
extern int markidx;

#ifdef ENABLE_COUNT
extern int prefix;
#endif /* ENABLE_COUNT */
//extern int extprefix;	/* Not needed! */
int
img_frame_goto(img_t *, int);

int
cg_switch_mode(arg_t _)
{
	if (mode == MODE_IMAGE) {
		if (tns.thumbs == NULL)
			tns_init(&tns, files, &filecnt, &fileidx, &win);
		img_close(&img, 0);
		reset_timeout(reset_cursor);
		if (img.ss.on) {
			img.ss.on = 0;
			reset_timeout(slideshow);
		}
		tns.dirty = 1;
		mode = MODE_THUMB;
	} else {
		load_image(fileidx);
		mode = MODE_IMAGE;
	}
	return 1;
}

int
cg_toggle_fullscreen(arg_t _)
{
	win_toggle_fullscreen(&win);
	set_timeout(redraw, TO_REDRAW_RESIZE, 0);	/* redraw after next ConfigureNotify event */
	if (mode == MODE_IMAGE)
		img.checkpan = img.dirty = 1;
	else
		tns.dirty = 1;
	return 0;
}

int
cg_toggle_bar(arg_t _)
{
	win_toggle_bar(&win);
	if (mode == MODE_IMAGE) {
		if (win.bar.h > 0)
			open_info();
		else
			close_info();
		img.checkpan = img.dirty = 1;
	} else
		tns.dirty = 1;
	return 1;
}
/* Not needed
int cg_prefix_external(arg_t _)
{
	extprefix = 1;
	return 0;
}
*/
int
cg_reload_image(arg_t _)
{
	if (mode == MODE_IMAGE)
		load_image(fileidx);
	else {
		win_set_cursor(&win, CURSOR_WATCH);
		if (!tns_load(&tns, fileidx, 1, 0)) {
			remove_file(fileidx, 0);
			tns.dirty = 1;
		}
	}
	return 1;
}

int
cg_remove_image(arg_t _)
{
	remove_file(fileidx, 1);
	if (mode == MODE_IMAGE)
		load_image(fileidx);
	else
		tns.dirty = 1;
	return 1;
}

int
cg_first(arg_t _)
{
	if (mode == MODE_IMAGE && fileidx != 0) {
		load_image(0);
		return 1;
	} else if (mode == MODE_THUMB && fileidx != 0) {
		fileidx = 0;
		tns.dirty = 1;
		return 1;
	} else
		return 0;
}

int
cg_n_or_last(arg_t _)
{
#ifdef ENABLE_COUNT
	int n = prefix != 0 && prefix - 1 < filecnt ? prefix - 1 : filecnt - 1;
	if (mode == MODE_IMAGE && fileidx != n) {
		load_image(n);
		return 1;
	} else if (mode == MODE_THUMB && fileidx != n) {
		fileidx = n;
#else
	if (mode == MODE_IMAGE) {
		load_image(filecnt - 1);
		return 1;
	} else if (mode == MODE_THUMB && fileidx != -1) {
		fileidx = filecnt - 1;
#endif /* ENABLE_COUNT */
		tns.dirty = 1;
		return 1;
	} else
		return 0;
}

int
cg_scroll_screen(arg_t dir)
{
	if (mode == MODE_IMAGE)
#ifdef ENABLE_COUNT
		return img_pan(&img, dir, -1);
#else
		return img_pan(&img, dir);
#endif /* ENABLE_COUNT */
	else
		return tns_scroll(&tns, dir, 1);
}

int
cg_zoom(arg_t d)
{
	if (mode == MODE_THUMB)
		return tns_zoom(&tns, d);
	else if (d > 0)
		return img_zoom_in(&img);
	else if (d < 0)
		return img_zoom_out(&img);
	else
		return 0;
}

int
cg_toggle_image_mark(arg_t _)
{
	return mark_image(fileidx, !(files[fileidx].flags & FF_MARK));
}

int
cg_reverse_marks(arg_t _)
{
	int i;

	for (i = 0; i < filecnt; i++) {
		files[i].flags ^= FF_MARK;
		markcnt += files[i].flags & FF_MARK ? 1 : -1;
	}
	if (mode == MODE_THUMB)
		tns.dirty = 1;
	return 1;
}

int
cg_mark_range(arg_t _)
{
	int d = markidx < fileidx ? 1 : -1, end, i;
	int dirty = 0, on = !!(files[markidx].flags & FF_MARK);

	for (i = markidx + d, end = fileidx + d; i != end; i += d)
		dirty |= mark_image(i, on);
	return dirty;
}

int
cg_unmark_all(arg_t _)
{
	int i;

	for (i = 0; i < filecnt; i++)
		files[i].flags &= ~FF_MARK;
	markcnt = 0;
	if (mode == MODE_THUMB)
		tns.dirty = 1;
	return 1;
}

int
cg_navigate_marked(arg_t n)
{
	int d, i;
	int new = fileidx;

#ifdef ENABLE_COUNT
	if (prefix > 0)
		n *= prefix;
#endif /* ENABLE_COUNT */
	d = n > 0 ? 1 : -1;
	if (markcnt == 0)	/* Preventing crash if no image is mark */
		return 0;
	/* Loop through all marked images */
	//for (i = fileidx + d; n != 0 && i >= 0 && i < filecnt; i += d) { /*This doen't work?*/
	for (i = fileidx + d; n != 0; i += d) {
 		if (i < 0)
 			i += filecnt;
 		else if (i >= filecnt)
 			i -= filecnt;
		if (files[i].flags & FF_MARK) {
			n -= d;
			new = i;
		}
	}
	if (new != fileidx) {
		if (mode == MODE_IMAGE)
			load_image(new);
		else {
			fileidx = new;
			tns.dirty = 1;
		}
		return 1;
	} else
		return 0;
}

int
cg_change_gamma(arg_t d)
{
#ifdef ENABLE_COUNT
	if (img_change_gamma(&img, d * (prefix > 0 ? prefix : 1))) {
#else
	if (img_change_gamma(&img, d)) {
#endif /* ENABLE_COUNT */
		if (mode == MODE_THUMB)
			tns.dirty = 1;
		return 1;
	} else
		return 0;
}

int
ci_navigate(arg_t n)
{
#ifdef ENABLE_COUNT
	if (prefix > 0)
		n *= prefix;
#endif /* ENABLE_COUNT */
	n += fileidx;
	if (n < 0)
		n = filecnt - 1;
	if (n >= filecnt)
		n = 0;

	if (n != fileidx) {
		load_image(n);
		return 1;
	} else
		return 0;
}

int
ci_cursor_navigate(arg_t _)
{
	return ci_navigate(ptr_third_x() - 1);
}

int
ci_alternate(arg_t _)
{
	load_image(alternate);
	return 1;
}

int
ci_navigate_frame(arg_t d)//How to toggle animation if navigate frame is activated?
{
	int frame;
#ifdef ENABLE_COUNT
	if (prefix > 0)
		d *= prefix;
#endif /* ENABLE_COUNT */
//	return !img.multi.animate && img_frame_navigate(&img, d);
	if (img.multi.cnt > 0) {
		frame = (img.multi.sel + d) % img.multi.cnt;
		while (frame < 0)
			frame += img.multi.cnt;
/* This one doen't care if it's playing the animaton */
		//return img_frame_goto(&img,frame);
/* This one cares if it's playing, so will only navigate if animation is paused */
		return !img.multi.animate && img_frame_goto(&img, frame);
	} else
		//return img_frame_navigate(&img, d);
		return !img.multi.animate && img_frame_navigate(&img, d);
}

int
ci_toggle_animation(arg_t _)
{
	int dirty = 0;

	if (img.multi.cnt > 0) {
		img.multi.animate = !img.multi.animate;
		if (img.multi.animate) {
			dirty = img_frame_animate(&img);
			set_timeout(animate, img.multi.frames[img.multi.sel].delay, 1);
		} else
			reset_timeout(animate);
	}
	return dirty;
}

int
ci_scroll(arg_t dir)
{
#ifdef ENABLE_COUNT
	return img_pan(&img, dir, prefix);
#else
	return img_pan(&img, dir);
#endif /* ENABLE_COUNT */
}

int
ci_scroll_to_edge(arg_t dir)
//If zoomed, then navigate else scroll to edge
/* If not zoomed, zoom one float on the array of zoom on config.h */
{
//	return img_pan_edge(&img, dir);
	//if (img_zoom_diff(&img, NULL) >= 0) {
	//return img_zoom(&img, 110.0 / 100.0) && img_pan_edge(&img, dir);
	//return img_zoom_in(&img);
	//return img_fit_win(&img, SCALE_WIDTH) && img_pan_edge(&img, dir);
		//if (img_fit_win(&img, SCALE_WIDTH))
		//return img_pan_edge(&img, dir);
		//else return img_zoom(&img, 110.0 / 100.0) && img_pan_edge(&img, dir);
//    arg_t n;
//    switch (dir) {
//		case DIR_UP:	n = -1; break;
//		case DIR_DOWN:	n =  1; break;
//		default:	n =  0; break;
//    }
//    return ci_navigate(n);
//  } else {
	//return ci_scroll(dir);
	return img_pan_edge(&img, dir);
  //}
}

int
ci_scroll_or_navigate(arg_t dir)
{
	if (img_zoom_diff(&img, NULL) >= 0) {
		arg_t n;
		switch (dir) {
			case DIR_UP:	n = -1; break;
			case DIR_DOWN:	n =  1; break;
			default:	n =  0; break;
		}
		return ci_navigate(n);
	} else
		return ci_scroll(dir);
}

int
ci_drag(arg_t mode)
{
	int x, y, ox, oy;
	float px, py;
	XEvent e;

	if ((int)(img.w * img.zoom) <= win.w && (int)(img.h * img.zoom) <= win.h)
		return 0;

	win_set_cursor(&win, CURSOR_DRAG);

	win_cursor_pos(&win, &x, &y);
	ox = x;
	oy = y;

	for (;;) {
		if (mode == DRAG_ABSOLUTE) {
			px = MIN(MAX(0.0, x - win.w*0.1), win.w*0.8) / (win.w*0.8)
			   * (win.w - img.w * img.zoom);
			py = MIN(MAX(0.0, y - win.h*0.1), win.h*0.8) / (win.h*0.8)
			   * (win.h - img.h * img.zoom);
		} else {
			px = img.x + x - ox;
			py = img.y + y - oy;
		}

		if (img_pos(&img, px, py)) {
			img_render(&img);
			win_draw(&win);
		}
		XMaskEvent(win.env.dpy,
		           ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &e);
		if (e.type == ButtonPress || e.type == ButtonRelease)
			break;
		while (XCheckTypedEvent(win.env.dpy, MotionNotify, &e));
		ox = x;
		oy = y;
		x = e.xmotion.x;
		y = e.xmotion.y;
	}
	set_timeout(reset_cursor, TO_CURSOR_HIDE, 1);
	reset_cursor();

	return 1;
}

int
ci_set_zoom(arg_t zl)
{
#ifdef ENABLE_COUNT
	return img_zoom(&img, (prefix ? prefix : zl) / 100.0);
#else
	return img_zoom(&img, zl / 100.0);
#endif /* ENABLE_COUNT */
}

int
ci_fit_to_win(arg_t sm)
{
	return img_fit_win(&img, sm);
}

int
ci_rotate(arg_t degree)
{
	img_rotate(&img, degree);
	return 1;
}

int
ci_flip(arg_t dir)
{
	img_flip(&img, dir);
	return 1;
}

int
ci_toggle_antialias(arg_t _)
{
	img_toggle_antialias(&img);
	return 1;
}

int
ci_toggle_alpha(arg_t _)
{
	img.alpha = !img.alpha;
	img.dirty = 1;
	return 1;
}

int
ci_slideshow(arg_t _)
{
#ifdef ENABLE_COUNT
	if (prefix > 0) {
		img.ss.on = 1;
		img.ss.delay = prefix * 10;
		set_timeout(slideshow, img.ss.delay * 100, 1);
	} else
#endif /* ENABLE_COUNT */
	if (img.ss.on) {
		img.ss.on = 0;
		reset_timeout(slideshow);
	} else
		img.ss.on = 1;
	return 1;
}

int
ct_move_sel(arg_t dir)
{
#ifdef ENABLE_COUNT
	return tns_move_selection(&tns, dir, prefix);
#else
	return tns_move_selection(&tns, dir, -1); //How to disable 'prefix' function on this command?
#endif /* ENABLE_COUNT */
}

int
ct_reload_all(arg_t _)
{
	tns_free(&tns);
	tns_init(&tns, files, &filecnt, &fileidx, &win);
	tns.dirty = 1;
	return 1;
}

	/* Customs */
int
cg_quit(arg_t _)
{
	unsigned int i;

	if (options->to_stdout && markcnt > 0) {
		for (i = 0; i < filecnt; i++) {
			if (files[i].flags & FF_MARK)
				printf("%s\n", files[i].name);
		}
	}
	exit(EXIT_SUCCESS);
}


int
ci_random_navigate(arg_t _)
{
	int n = rand() % filecnt;

	if (n != fileidx) {
		load_image(n);
		return 1;
	} else
		return 0;
}

int
cg_dmenu_output(arg_t _)
{
	printf("%s\n", files[fileidx].name);
	//printf("%s\n", files[fileidx].path);
        exit(EXIT_SUCCESS);
}
/*
int
cg_toggle_squarethumb(arg_t _)
{
	squarethumb = !squarethumb;
	//return 0;
}
*/


#undef  G_CMD
#define G_CMD(c) { -1, cg_##c },
#undef  I_CMD
#define I_CMD(c) { MODE_IMAGE, ci_##c },
#undef  T_CMD
#define T_CMD(c) { MODE_THUMB, ct_##c },

const cmd_t cmds[CMD_COUNT] = {
#include "commands.lst"
};
