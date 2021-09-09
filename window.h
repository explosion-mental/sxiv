

#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>

enum {
	BAR_L_LEN = 512,
	BAR_R_LEN = 64
};

enum {
	ATOM_WM_DELETE_WINDOW,
	ATOM__NET_WM_NAME,
	ATOM__NET_WM_ICON_NAME,
	ATOM__NET_WM_ICON,
	ATOM__NET_WM_STATE,
	ATOM__NET_WM_PID,
	ATOM__NET_WM_STATE_FULLSCREEN,
	ATOM_COUNT
};

typedef struct {
	Display *dpy;
	int scr;
	int scrw, scrh;
	Visual *vis;
	Colormap cmap;
	int depth;
} win_env_t;

typedef struct {
	size_t size;
	char *p;
	char *buf;
} win_bar_t;

struct win {
	Window xwin;
	win_env_t env;

	XftColor bg;
	XftColor fg;
	XftColor selcol;
	XftColor markcol;

    const char *title;

	int x;
	int y;
	unsigned int w;
	unsigned int h; /* = win height - bar height */
	unsigned int bw;

	struct {
		int w;
		int h;
		Pixmap pm;
	} buf;

	struct {
		unsigned int h;
		win_bar_t l;
		win_bar_t r;
	} bar;
};

extern Atom atoms[ATOM_COUNT];

void win_init(win_t*);
void win_open(win_t*);
CLEANUP void win_close(win_t*);
int win_configure(win_t*, XConfigureEvent*);
void win_toggle_fullscreen(win_t*);
void win_toggle_bar(win_t*);
void win_clear(win_t*);
void win_draw(win_t*);
void win_draw_rect(win_t*, int, int, int, int, int, int, unsigned long);
void win_set_title(win_t*);
void win_set_cursor(win_t*, cursor_t);
void win_cursor_pos(win_t*, int*, int*);

