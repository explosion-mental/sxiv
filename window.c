/* Copyright 2011-2013 Bert Muennich
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
#include <locale.h>
#include <unistd.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/Xresource.h>
#include <Imlib2.h>

//#include "sxiv.h"

/* sxiv.c */
#include "main.h"

/* autoreload.c */
//#include "autoreload.h"

/* commands.c */
#include "commands.h"

/* options.c */
#include "options.h"

/* thumbs.c */
//#include "thumbs.h"

/* util.c */
#include "util.h"

/* url.c */

/* window.c */
#include "window.h"


#include "drw.h"

//#define _WINDOW_CONFIG
#include "config.h"
#include "icon/data.h"
#include "utf8.h"
//#include "drw.h"
enum {	H_TEXT_PAD = 5,
	V_TEXT_PAD = 1 };

static struct {
	int name;
	Cursor icon;
} cursors[CURSOR_COUNT] = {
	{ XC_left_ptr }, { XC_dotbox }, { XC_watch },
	{ XC_sb_left_arrow }, { XC_sb_right_arrow }
};

static Display *dpy;
static Drw *drw;
static int screen;
static Window root, wmcheckwin;
static int sw, sh;           /* X display screen geometry width, height */

static Visual *visual;
static int depth;
static Colormap cmap;

static GC gc;
//static GC = drw->gc;
static Clr **scheme;
static XftFont *font;
static int fontheight;
static double fontsize;
static int barheight;

Atom atoms[ATOM_COUNT];

void
win_init_font(const win_env_t *e, const char *fontstr)
{
	if ((font = XftFontOpenName(dpy, screen, fontstr)) == NULL)
		error(EXIT_FAILURE, 0, "Error loading font '%s'", fontstr);
	fontheight = font->ascent + font->descent;
	FcPatternGetDouble(font->pattern, FC_SIZE, 0, &fontsize);
	barheight = fontheight + 2 * V_TEXT_PAD;
}

void
win_alloc_color(const win_env_t *e, const char *name, XftColor *col)
{
	if (!XftColorAllocName(dpy, visual,
	                       cmap, name, col))
		error(EXIT_FAILURE, 0, "Error allocating color '%s'", name);
}
		/* database          name                      def*/
const char *
win_res(XrmDatabase db, const char *name, const char *def)
{
	char *type;
	XrmValue ret;

	if (db != None &&
	    XrmGetResource(db, name, name, &type, &ret) &&
	    STREQ(type, "String"))
	{
		return ret.addr;
	} else {
		return def;
		//if (def == NULL)
		//defaults colors in case 'x'_COLOR is not defined...
	}
}

#define INIT_ATOM_(atom) atoms[ATOM_##atom] = XInternAtom(dpy, #atom, False);
#define RES_CLASS "Sxiv"
#define LENGTH(X)               (sizeof X / sizeof X[0])
void
win_init(win_t *win)
{
	win_env_t *e;
	const char *bg, *fg, *mark, *sel, *f;
	char *res_man;
	XrmDatabase db;

	memset(win, 0, sizeof(win_t));

	e = &win->env;
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		error(EXIT_FAILURE, 0, "Error opening X display");

	//screen = DefaultScreen(dpy);
	//e->dpy = DisplayWidth(dpy, screen);
	//e->dpy = DisplayHeight(dpy, screen);
	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);

	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (int i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);

	visual = DefaultVisual(dpy, screen);
	depth = DefaultDepth(dpy, screen);
	cmap = DefaultColormap(dpy, screen);
	//e->vis = DefaultVisual(dpy, screen);
	//e->cmap = DefaultColormap(dpy, screen);
	//e->depth = DefaultDepth(dpy, screen);

	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	//if (setlocale(LC_CTYPE, "") == NULL || XSupportsLocale() == 0)
	//	error(0, 0, "No locale support");

	XrmInitialize();
	res_man = XResourceManagerString(dpy);
	db = res_man != NULL ? XrmGetStringDatabase(res_man) : None;

	//f = win_res(db, RES_CLASS "unifont-9", "monospace-8");
	f = win_res(db, RES_CLASS ".font", leterfont);
	win_init_font(e, f);

//	fg = win_res(db, RES_CLASS ".foreground", "color8");
	bg   = win_res(db, RES_CLASS ".background", bgcolor);	/* background */
	fg   = win_res(db, RES_CLASS ".color2",     fgcolor);	/* bar */
	mark = win_res(db, RES_CLASS ".color2",   markcolor);	/* mark */
	sel  = win_res(db, RES_CLASS ".color8",    selcolor);	/* highlight */
	//colors[SchemeNorm][ColBg];

	win_alloc_color(e, bg, &win->bg);
	win_alloc_color(e, fg, &win->fg);
 	win_alloc_color(e, mark, &win->markcol);
 	win_alloc_color(e, sel, &win->selcol);

	win->bar.l.size = BAR_L_LEN;
	win->bar.r.size = BAR_R_LEN;
	/* 3 padding bytes needed by utf8_decode */
	win->bar.l.buf = emalloc(win->bar.l.size + 3);
	win->bar.l.buf[0] = '\0';
	win->bar.r.buf = emalloc(win->bar.r.size + 3);
	win->bar.r.buf[0] = '\0';
	win->bar.h = options->hide_bar ? 0 : barheight;

	INIT_ATOM_(WM_DELETE_WINDOW);
	INIT_ATOM_(_NET_WM_NAME);
	INIT_ATOM_(_NET_WM_ICON_NAME);
	INIT_ATOM_(_NET_WM_ICON);
	INIT_ATOM_(_NET_WM_STATE);
	INIT_ATOM_(_NET_WM_STATE_FULLSCREEN);
	INIT_ATOM_(_NET_WM_PID);
}

void
win_open(win_t *win)
{
	int c, i, j, n;
	long parent;
	win_env_t *e;
	XClassHint classhint;
	unsigned long *icon_data;
	XColor col;
	Cursor *cnone = &cursors[CURSOR_NONE].icon;
	char none_data[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	Pixmap none;
	int gmask;
	XSizeHints sizehints;
	XWMHints hints;

	e = &win->env;
	parent = options->embed != 0 ? options->embed : RootWindow(dpy, screen);

	sizehints.flags = PWinGravity;
	sizehints.win_gravity = NorthWestGravity;

	/* determine window offsets, width & height */
	if (options->geometry == NULL)
		gmask = 0;
	else
		gmask = XParseGeometry(options->geometry, &win->x, &win->y,
		                       &win->w, &win->h);
	if ((gmask & WidthValue) != 0)
		sizehints.flags |= USSize;
	else
		win->w = WIN_WIDTH;
	if ((gmask & HeightValue) != 0)
		sizehints.flags |= USSize;
	else
		win->h = WIN_HEIGHT;
	if ((gmask & XValue) != 0) {
		if ((gmask & XNegative) != 0) {
			win->x += sw - win->w;
			sizehints.win_gravity = NorthEastGravity;
		}
		sizehints.flags |= USPosition;
	} else
		win->x = 0;

	if ((gmask & YValue) != 0) {
		if ((gmask & YNegative) != 0) {
			win->y += sh - win->h;
			sizehints.win_gravity = sizehints.win_gravity == NorthEastGravity
			                      ? SouthEastGravity : SouthWestGravity;
		}
		sizehints.flags |= USPosition;
	} else
		win->y = 0;

	win->xwin = XCreateWindow(dpy, parent,
	                          win->x, win->y, win->w, win->h, 0,
	                          e->depth, InputOutput, e->vis, 0, NULL);
	if (win->xwin == None)
		error(EXIT_FAILURE, 0, "Error creating X window");

	/* set the _NET_WM_PID */
    	pid_t pid = getpid();
    	XChangeProperty(dpy, win->xwin,
		atoms[ATOM__NET_WM_PID], XA_CARDINAL, sizeof(pid_t) * 8,
		PropModeReplace, (unsigned char *) &pid, 1);

	/* set the WM_CLIENT_MACHINE */
	char hostname[255];
	if (gethostname(hostname, sizeof(hostname)) == 0) {
		XTextProperty tp;
		tp.value = (unsigned char *)hostname;
		tp.nitems = strlen(hostname);
		tp.encoding = XA_STRING;
		tp.format = 8;
		XSetWMClientMachine(dpy, win->xwin, &tp);
    	}

	XSelectInput(dpy, win->xwin,
	             ButtonReleaseMask | ButtonPressMask | KeyPressMask |
	             PointerMotionMask | StructureNotifyMask);

	for (i = 0; i < ARRLEN(cursors); i++) {
		if (i != CURSOR_NONE)
			cursors[i].icon = XCreateFontCursor(dpy, cursors[i].name);
	}
	if (XAllocNamedColor(dpy, DefaultColormap(dpy, screen), "black",
	                     &col, &col) == 0)
		error(EXIT_FAILURE, 0, "Error allocating color 'black'");
	none = XCreateBitmapFromData(dpy, win->xwin, none_data, 8, 8);
	*cnone = XCreatePixmapCursor(dpy, none, none, &col, &col, 0, 0);

	// GC
	//gc = XCreateGC(dpy, win->xwin, 0, None);

	n = icons[ARRLEN(icons)-1].size;
	icon_data = emalloc((n * n + 2) * sizeof(*icon_data));

	for (i = 0; i < ARRLEN(icons); i++) {
		n = 0;
		icon_data[n++] = icons[i].size;
		icon_data[n++] = icons[i].size;

		for (j = 0; j < icons[i].cnt; j++) {
			for (c = icons[i].data[j] >> 4; c >= 0; c--)
				icon_data[n++] = icon_colors[icons[i].data[j] & 0x0F];
		}
		XChangeProperty(dpy, win->xwin,
		                atoms[ATOM__NET_WM_ICON], XA_CARDINAL, 32,
		                i == 0 ? PropModeReplace : PropModeAppend,
		                (unsigned char *) icon_data, n);
	}
	free(icon_data);

	win_set_title(win);

	classhint.res_class = RES_CLASS;
	classhint.res_name = options->res_name != NULL ? options->res_name : "sxiv";
	XSetClassHint(dpy, win->xwin, &classhint);

	XSetWMProtocols(dpy, win->xwin, &atoms[ATOM_WM_DELETE_WINDOW], 1);

	sizehints.width = win->w;
	sizehints.height = win->h;
	sizehints.x = win->x;
	sizehints.y = win->y;
	XSetWMNormalHints(win->env.dpy, win->xwin, &sizehints);

	hints.flags = InputHint | StateHint;
	hints.input = 1;
	hints.initial_state = NormalState;
	XSetWMHints(win->env.dpy, win->xwin, &hints);

//	if (topbar)
//		win->h += win->bar.h;
//	else
//	win->h -= win->bar.h;
//	c->height

//	win->h += sh;
	win->buf.w = sw;
	win->buf.h = sh;
	win->buf.pm = XCreatePixmap(dpy, win->xwin,
	                            win->buf.w, win->buf.h, e->depth);
	//               dpy     gc  scheme
	//XSetForeground(dpy, gc, win->bg.pixel);
	//		dpy	drawable	gc x y  w           h
	//XFillRectangle(dpy, win->buf.pm, gc, 0, 0, win->buf.w, win->buf.h);
	drw_rect(drw, 0, 0, win->buf.w, win->buf.h, 1, 0);
	XSetWindowBackgroundPixmap(dpy, win->xwin, win->buf.pm);

	XMapWindow(dpy, win->xwin);
	XFlush(dpy);

	if (options->fullscreen)
		win_toggle_fullscreen(win);
}
void
setup(void)
{
}

CLEANUP void
win_close(win_t *win)
{
	int i;

	for (i = 0; i < ARRLEN(cursors); i++)
		XFreeCursor(win->env.dpy, cursors[i].icon);

	XFreeGC(win->env.dpy, drw->gc);
	XDestroyWindow(win->env.dpy, win->xwin);
	XCloseDisplay(win->env.dpy);
}

int
win_configure(win_t *win, XConfigureEvent *c)
{
	int changed;

	changed = win->w != c->width || win->h + win->bar.h != c->height;

	win->x = c->x;
	win->y = c->y;
	win->w = c->width;
	win->h = c->height - win->bar.h;
	win->bw = c->border_width;

	return changed;
}

void
win_toggle_fullscreen(win_t *win)
{
	XEvent ev;
	XClientMessageEvent *cm;

	memset(&ev, 0, sizeof(ev));
	ev.type = ClientMessage;

	cm = &ev.xclient;
	cm->window = win->xwin;
	cm->message_type = atoms[ATOM__NET_WM_STATE];
	cm->format = 32;
	cm->data.l[0] = 2; // toggle
	cm->data.l[1] = atoms[ATOM__NET_WM_STATE_FULLSCREEN];

	XSendEvent(win->env.dpy, DefaultRootWindow(win->env.dpy), False,
	           SubstructureNotifyMask | SubstructureRedirectMask, &ev);
}

void
win_toggle_bar(win_t *win)
{
	if (topbar) {
		if (win->bar.h != 0) {
			win->h -= win->bar.h;
			win->bar.h = 0;
			//win->h = win->bar.h - win->h;
			//win->bar.h = 0;
		} else {
			win->bar.h = barheight;
			win->h += win->bar.h;
			//win->bar.h = barheight;
			//win->h = win->bar.h + win->h;
		}
	} else {
		if (win->bar.h != 0) {
			win->h += win->bar.h;
			win->bar.h = 0;
		} else {
			win->bar.h = barheight;
			win->h -= win->bar.h;
		}
	}
}

void
win_clear(win_t *win)
{
	win_env_t *e;

	e = &win->env;

	if (win->w > win->buf.w || win->h + win->bar.h > win->buf.h) {
		XFreePixmap(dpy, win->buf.pm);
		win->buf.w = MAX(win->buf.w, win->w);
		win->buf.h = MAX(win->buf.h, win->h + win->bar.h);
		win->buf.pm = XCreatePixmap(dpy, win->xwin,
		                            win->buf.w, win->buf.h, e->depth);
	}
	//XSetForeground(dpy, gc, win->bg.pixel);
	//XFillRectangle(dpy, win->buf.pm, gc, 0, 0, win->buf.w, win->buf.h);
	drw_rect(drw, 0, 0, win->buf.w, win->buf.h, 1, 0);
}

#define TEXTWIDTH(win, text, len) \
	win_draw_text(win, NULL, NULL, 0, 0, text, len, 0)

int
win_draw_text(win_t *win, XftDraw *d, const XftColor *color, int x, int y, char *text, int len, int w)
{
	int err, tw = 0;
	char *t, *next;
	uint32_t rune;
	XftFont *f;
	FcCharSet *fccharset;
	XGlyphInfo ext;

	for (t = text; t - text < len; t = next) {
		next = utf8_decode(t, &rune, &err);
		if (XftCharExists(win->env.dpy, font, rune))
			f = font;
		else { /* fallback font */
			fccharset = FcCharSetCreate();
			FcCharSetAddChar(fccharset, rune);
			f = XftFontOpen(win->env.dpy, win->env.scr, FC_CHARSET, FcTypeCharSet,
			                fccharset, FC_SCALABLE, FcTypeBool, FcTrue,
			                FC_SIZE, FcTypeDouble, fontsize, NULL);
			FcCharSetDestroy(fccharset);
		}
		XftTextExtentsUtf8(win->env.dpy, f, (XftChar8*)t, next - t, &ext);
		tw += ext.xOff;
		if (tw <= w) {
			XftDrawStringUtf8(d, color, f, x, y, (XftChar8*)t, next - t);
			x += ext.xOff;
		}
		if (f != font)
			XftFontClose(win->env.dpy, f);
	}
	return tw;
}

void
win_draw_bar(win_t *win)
{
	int len, x, y, w, tw;
	win_env_t *e;
	win_bar_t *l, *r;
	XftDraw *d;

	if ((l = &win->bar.l)->buf == NULL || (r = &win->bar.r)->buf == NULL)
		return;

	e = &win->env;
	if (topbar)
		y = font->ascent + V_TEXT_PAD;
	else
		y = win->h + font->ascent + V_TEXT_PAD;
	w = win->w - 2*H_TEXT_PAD;
	d = XftDrawCreate(dpy, win->buf.pm, DefaultVisual(dpy, screen),
			DefaultColormap(dpy, screen));

	XSetForeground(dpy, drw->gc, win->fg.pixel);
	/* bar */
	if (topbar)
			    /*display	drawable    gc  x	y	wid     height	*/
		XFillRectangle(dpy, win->buf.pm, drw->gc, 0, 0, win->w, win->bar.h);
	else
		XFillRectangle(dpy, win->buf.pm, drw->gc, 0, win->h, win->w, win->bar.h);

	XSetForeground(dpy, drw->gc, win->bg.pixel);
	XSetBackground(dpy, drw->gc, win->fg.pixel);

	/* right text */
	if ((len = strlen(r->buf)) > 0) {
		if ((tw = TEXTWIDTH(win, r->buf, len)) > w)
			return;
		x = win->w - tw - H_TEXT_PAD;
		w -= tw;
		win_draw_text(win, d, &win->bg, x, y, r->buf, len, tw);
	}
	/* left text */
	if ((len = strlen(l->buf)) > 0) {
		x = H_TEXT_PAD;
		w -= 2 * H_TEXT_PAD; /* gap between left and right parts */
		win_draw_text(win, d, &win->bg, x, y, l->buf, len, w);
	}
	XftDrawDestroy(d);
}

void
win_draw(win_t *win)
{
	if (win->bar.h > 0)
		win_draw_bar(win);

	XSetWindowBackgroundPixmap(win->env.dpy, win->xwin, win->buf.pm);
	XClearWindow(win->env.dpy, win->xwin);
	XFlush(win->env.dpy);
}

//Highlight thumb
void
win_draw_rect(win_t *win, int x, int y, int w, int h, int fill, int lw, unsigned long col)
{
	XGCValues gcval;

	gcval.line_width = lw;
	gcval.foreground = col;
	XChangeGC(win->env.dpy, drw->gc, GCForeground | GCLineWidth, &gcval);

	if (fill)
		XFillRectangle(win->env.dpy, win->buf.pm, drw->gc, x, y, w, h);
	else
		XDrawRectangle(win->env.dpy, win->buf.pm, drw->gc, x, y, w, h);
}

void
win_set_title(win_t *win)
{
	XStoreName(win->env.dpy, win->xwin, win->title);
	XSetIconName(win->env.dpy, win->xwin, win->title);

	XChangeProperty(win->env.dpy, win->xwin, atoms[ATOM__NET_WM_NAME],
	                XInternAtom(win->env.dpy, "UTF8_STRING", False), 8,
	                PropModeReplace, (unsigned char *) win->title, strlen(win->title));
	XChangeProperty(win->env.dpy, win->xwin, atoms[ATOM__NET_WM_ICON_NAME],
	                XInternAtom(win->env.dpy, "UTF8_STRING", False), 8,
	                PropModeReplace, (unsigned char *) win->title, strlen(win->title));
}

void
win_set_cursor(win_t *win, cursor_t cursor)
{
	if (cursor >= 0 && cursor < ARRLEN(cursors)) {
		XDefineCursor(win->env.dpy, win->xwin, cursors[cursor].icon);
		XFlush(win->env.dpy);
	}
}

void
win_cursor_pos(win_t *win, int *x, int *y)
{
	int i;
	unsigned int ui;
	Window w;

	if (!XQueryPointer(win->env.dpy, win->xwin, &w, &w, &i, &i, x, y, &ui))
		*x = *y = 0;
}
