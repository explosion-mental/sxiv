

typedef int arg_t;
typedef int (*cmd_f)(arg_t);

#define G_CMD(c) g_##c,
#define I_CMD(c) i_##c,
#define T_CMD(c) t_##c,

typedef enum {
	#include "commands.lst"
	CMD_COUNT
} cmd_id_t;

typedef struct {
	int mode;
	cmd_f func;
} cmd_t;

typedef struct {
	unsigned int mask;
	KeySym ksym;
	//void (*func)(const Arg *);
	cmd_id_t cmd;
	arg_t arg;
} keymap_t;

typedef struct {
	unsigned int mask;
	unsigned int button;
	cmd_id_t cmd;
	arg_t arg;
} button_t;

extern const cmd_t cmds[CMD_COUNT];


/* image.c */

typedef struct {
	Imlib_Image im;
	unsigned int delay;
} img_frame_t;

typedef struct {
	img_frame_t *frames;
	int cap;
	int cnt;
	int sel;
	int animate;
	int framedelay;
	int length;
} multi_img_t;

struct img {
	Imlib_Image im;
	int w;
	int h;

	win_t *win;
	float x;
	float y;

	scalemode_t scalemode;
	float zoom;

	int checkpan;
	int dirty;
	int aa;
	int alpha;

	Imlib_Color_Modifier cmod;
	int gamma;

	struct {
		int on;
		int delay;
	} ss;

	multi_img_t multi;
};

void img_init(img_t*, win_t*);
int img_load(img_t*, const fileinfo_t*);
CLEANUP void img_close(img_t*, int);
void img_render(img_t*);
int img_fit_win(img_t*, scalemode_t);
int img_zoom(img_t*, float);
int img_zoom_in(img_t*);
int img_zoom_out(img_t*);
int img_pos(img_t*, float, float);
int img_move(img_t*, float, float);
#ifdef ENABLE_COUNT
int img_pan(img_t*, direction_t, int);
#else
int img_pan(img_t*, direction_t);
#endif /* ENABLE_COUNT */
int img_pan_edge(img_t*, direction_t);
void img_rotate(img_t*, degree_t);
void img_flip(img_t*, flipdir_t);
void img_toggle_antialias(img_t*);
int img_change_gamma(img_t*, int);
int img_frame_navigate(img_t*, int);
int img_frame_animate(img_t*);
int  img_zoom_diff(img_t*, float*);

