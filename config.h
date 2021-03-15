#ifdef _WINDOW_CONFIG

/* default window dimensions (overwritten via -g option): */
enum {
	WIN_WIDTH  = 800,
	WIN_HEIGHT = 600
};

/* colors and font are configured with 'background', 'foreground' and
 * 'font' X resource properties or by using wal (pywal).
 * See X(7) section Resources and xrdb(1) for more information.
 */

#endif
#ifdef _IMAGE_CONFIG

/* levels (in percent) to use when zooming via '-' and '+':
 * (first/last value is used as min/max zoom level)
 */
static const float zoom_levels[] = {
//	 	  0.1,  /* https://github.com/muennich/sxiv/issues/273 */
	 12.5,  25.0,  50.0,  75.0,
	100.0, 150.0, 200.0, 400.0, 800.0
};

/* default slideshow delay (in sec, overwritten via -S option): */
enum { SLIDESHOW_DELAY = 3 };

/* gamma correction: the user-visible ranges [-GAMMA_RANGE, 0] and
 * (0, GAMMA_RANGE] are mapped to the ranges [0, 1], and (1, GAMMA_MAX].
 * */
static const double GAMMA_MAX   = 10.0;
static const int    GAMMA_RANGE = 32;

/* command i_scroll pans image 1/PAN_FRACTION of screen width/height */
static const int PAN_FRACTION = 8;

/* if false, pixelate images at zoom level != 100%,
 * toggled with 'a' key binding
 */
static const bool ANTI_ALIAS = true;

/* if true, use a checkerboard background for alpha layer,
 * toggled with 'A' key binding
 */
static const bool ALPHA_LAYER = false;

#endif
#ifdef _THUMBS_CONFIG

/* thumbnail sizes in pixels (width == height): */
static const int thumb_sizes[] = { 32, 64, 96, 128, 286, 361, 412 };
/*added configurable box model with thumbnail padding, margin and borde… THIS IS INSANEEE
i-tsvetkov/sxiv this person made it*/
const int THUMB_BORDERS[] = { 7 };	//thickness of the selction and mark color(an array in case you want different sizes on differents zoom levels
const int THUMB_MARGIN = 0;		//margins between images
const int THUMB_PADDING = -4;		//padding of the highlight or mark
/* thumbnail size at startup, index into thumb_sizes[]: */
static const int THUMB_SIZE= 4;

#endif
#ifdef _MAPPINGS_CONFIG

/* keyboard mappings for image and thumbnail mode: */
/* Disable all keys that I don't need and are on the key-handler since I patch sxiv so it doesn't need a "Modkey" to use external commands */
static const keymap_t keys[] = {
	/* modifiers    key               function              argument */
				/* Navigation */
	{ 0,            XK_h,             i_scroll_or_navigate, DIR_LEFT },
//	{ 0,            XK_Left,          i_scroll_or_navigate, DIR_LEFT },
	{ 0,            XK_j,             i_scroll_or_navigate, DIR_DOWN },
//	{ 0,            XK_Down,          i_scroll_or_navigate, DIR_DOWN },
	{ 0,            XK_k,             i_scroll_or_navigate, DIR_UP },
//	{ 0,            XK_Up,            i_scroll_or_navigate, DIR_UP },
	{ 0,            XK_l,             i_scroll_or_navigate, DIR_RIGHT },
//	{ 0,            XK_Right,         i_scroll_or_navigate, DIR_RIGHT },
//	{ 0,            XK_j,             i_navigate,           +1 },
//	{ 0,            XK_k,             i_navigate,           -1 },
	{ 0,            XK_comma,         i_navigate,           +1 },
	{ 0,            XK_comma,         i_scroll_to_edge,     DIR_LEFT | DIR_UP },
	{ 0,            XK_period,        i_navigate,           -1 },
	{ 0,            XK_period,        i_scroll_to_edge,     DIR_LEFT | DIR_UP },
//	{ 0,            XK_space,         i_navigate,           +1 },
//	{ 0,            XK_BackSpace,     i_navigate,           -1 },
	{ 0,            XK_bracketright,  i_navigate,           +3 },
	{ 0,            XK_bracketleft,   i_navigate,           -3 },
	{ 0,		XK_Tab,           i_alternate,          None },
	{ ControlMask,  XK_j,             i_navigate_frame,     -1 },
	{ ControlMask,  XK_k,             i_navigate_frame,     +1 },
	{ 0,            XK_g,             g_first,              None },
	{ 0,            XK_G,             g_n_or_last,          None },
	{ 0,		XK_grave,         i_random_navigate,    None },

				/* Scroll */
//	{ 0,            XK_h,             i_scroll,             DIR_LEFT },
//	{ 0,            XK_j,             i_scroll,             DIR_DOWN },
//	{ 0,            XK_k,             i_scroll,             DIR_UP },
//	{ 0,            XK_l,             i_scroll,             DIR_RIGHT },
	{ 0,            XK_Left,          i_scroll,             DIR_LEFT },
	{ 0,            XK_Down,          i_scroll,             DIR_DOWN },
	{ 0,            XK_Up,            i_scroll,             DIR_UP },
	{ 0,            XK_Right,         i_scroll,             DIR_RIGHT },
	{ 0,            XK_H,             i_scroll_to_edge,     DIR_LEFT },
	{ 0,            XK_J,             i_scroll_to_edge,     DIR_DOWN },
	{ 0,            XK_K,             i_scroll_to_edge,     DIR_UP },
	{ 0,            XK_L,             i_scroll_to_edge,     DIR_RIGHT },
	//what are these?? useless?
//	{ ControlMask,  XK_h,             g_scroll_screen,      DIR_LEFT },
	{ ControlMask,  XK_Left,          g_scroll_screen,      DIR_LEFT },
//	{ ControlMask,  XK_j,             g_scroll_screen,      DIR_DOWN },
	{ ControlMask,  XK_Down,          g_scroll_screen,      DIR_DOWN },
//	{ ControlMask,  XK_k,             g_scroll_screen,      DIR_UP },
	{ ControlMask,  XK_Up,            g_scroll_screen,      DIR_UP },
//	{ ControlMask,  XK_l,             g_scroll_screen,      DIR_RIGHT },
	{ ControlMask,  XK_Right,         g_scroll_screen,      DIR_RIGHT },

					/* Zoom */
	{ 0,            XK_o,             g_zoom,               +1 },
	{ 0,            XK_i,             g_zoom,               -1 },
	{ 0,            XK_equal,         g_zoom,               +1 },
	{ 0,            XK_minus,         g_zoom,               -1 },
//	{ 0,            XK_K,             g_zoom,               +1 },
//	{ ControlMask,  XK_k,             g_zoom,               +1 },
//	{ ControlMask,  XK_j,             g_zoom,               -1 },
//	{ 0,            XK_J,             g_zoom,               -1 },
//	{ 0,            XK_equal,         i_set_zoom,           100 },
//	{ 0,            XK_equal,         i_set_zoom,           100 },
//	{ 0,            XK_9,		  i_set_zoom,           100 },
//	{ 0,            XK_plus,          g_zoom,               +1 },
//	{ 0,            XK_KP_Add,        g_zoom,               +1 },
//	{ 0,            XK_KP_Subtract,   g_zoom,               -1 },

				/* Marks */
	{ 0,            XK_m,             g_toggle_image_mark,  None },
//	{ 0,            XK_M,             g_mark_range,         None },
//	{ ControlMask,  XK_m,             g_reverse_marks,      None },
	{ 0,            XK_t,             g_mark_range,         None },
	{ 0,	        XK_M,             g_reverse_marks,      None },
//	{ ControlMask,  XK_u,             g_unmark_all,         None },
	{ 0,  		XK_x,             g_unmark_all,         None },
	{ 0,            XK_n,             g_navigate_marked,    +1 },
	{ 0,            XK_N,             g_navigate_marked,    -1 },
	{ 0,            XK_braceleft,     g_change_gamma,       -1 },
	{ 0,            XK_braceright,    g_change_gamma,       +1 },
	{ ControlMask,  XK_g,             g_change_gamma,        0 },

				/* Thumbnail movement */
	{ 0,            XK_h,             t_move_sel,           DIR_LEFT },
	{ 0,            XK_Left,          t_move_sel,           DIR_LEFT },
	{ 0,            XK_j,             t_move_sel,           DIR_DOWN },
	{ 0,            XK_Down,          t_move_sel,           DIR_DOWN },
	{ 0,            XK_k,             t_move_sel,           DIR_UP },
	{ 0,            XK_Up,            t_move_sel,           DIR_UP },
	{ 0,            XK_l,             t_move_sel,           DIR_RIGHT },
	{ 0,            XK_Right,         t_move_sel,           DIR_RIGHT },

				/* Scaling */
	{ 0,            XK_W,             i_fit_to_win,         SCALE_DOWN },
	{ 0,            XK_w,             i_fit_to_win,         SCALE_FIT },
	{ 0,            XK_0,             i_fit_to_win,         SCALE_FIT },
	{ 0,            XK_9,             i_fit_to_win,         SCALE_WIDTH },
	{ 0,            XK_e,             i_fit_to_win,         SCALE_WIDTH },
	{ 0,            XK_E,             i_fit_to_win,         SCALE_HEIGHT },
 	{ 0,            XK_F,             i_fit_to_win,         SCALE_FILL },

				/* Rotation */
	{ 0,            XK_less,          i_rotate,             DEGREE_270 },
	{ 0,            XK_greater,       i_rotate,             DEGREE_90 },
//	{ 0,            XK_question,      i_rotate,             DEGREE_180 },
	{ 0,            XK_u,	          i_rotate,             DEGREE_180 },
//	{ 0,            XK_bar,           i_flip,               FLIP_HORIZONTAL },
	{ 0,            XK_backslash,     i_flip,               FLIP_HORIZONTAL },
	{ 0,            XK_bar,           i_flip,               FLIP_VERTICAL },

				/* Miscelaneous */
//	{ 0,            XK_Return,        g_switch_mode,        None },
	{ 0,            XK_space,         g_switch_mode,        None },
	{ 0,            XK_f,             g_toggle_fullscreen,  None },
	{ 0,            XK_b,             g_toggle_bar,         None },
	{ 0,            XK_Q,             g_dmenu_output,       None },
//	{ ControlMask,  XK_x,             g_prefix_external,    None },
//	{ 0,	        XK_comma,         g_prefix_external,    None },
	{ 0,            XK_F5,            g_reload_image,       None },
	{ ControlMask,  XK_R,             t_reload_all,         None },
//	{ ControlMask,  XK_R,             t_reload_all,         None },
	{ 0,            XK_D,             g_remove_image,       None },
	{ 0,            XK_a,             i_toggle_antialias,   None },
	{ 0,            XK_A,             i_toggle_alpha,       None },
	{ 0,            XK_S,             i_slideshow,          None },
//	{ 0,            XK_s,             i_slideshow,          None },
//	{ ControlMask,  XK_space,         i_toggle_animation,   None },
	{ 0,		XK_p,      	  i_toggle_animation,   None },
	{ 0,            XK_q,             g_quit,               None },
};

/* mouse button mappings for image mode: */
static const button_t buttons[] = {
	/* modifiers    button            function              argument */
	{ 0,            1,                i_cursor_navigate,    None },
	{ 0,            2,                i_drag,               DRAG_ABSOLUTE },
	{ 0,            3,                g_switch_mode,        None },
	{ 0,            4,                g_zoom,               +1 },
	{ 0,            5,                g_zoom,               -1 },
};

#endif
