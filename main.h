
/*******************************************************************
 * Annotation for functions called in cleanup().
 * These functions are not allowed to call error(!0, ...) or exit().
 ********************************************************************/
#define CLEANUP

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#define ARRLEN(a)		(sizeof(a) / sizeof a[0])
#define STREQ(s1,s2)		(strcmp((s1), (s2)) == 0)
#define TV_DIFF(t1,t2)		(((t1)->tv_sec  - (t2)->tv_sec ) * 1000 + \
				 ((t1)->tv_usec - (t2)->tv_usec) / 1000)
#define TV_SET_MSEC(tv,t)	{ (tv)->tv_sec  = (t) / 1000;           \
				  (tv)->tv_usec = (t) % 1000 * 1000; }
#define TV_ADD_MSEC(tv,t)	{ (tv)->tv_sec  += (t) / 1000;          \
				  (tv)->tv_usec += (t) % 1000 * 1000; }
typedef enum {
	BO_BIG_ENDIAN,
	BO_LITTLE_ENDIAN
} byteorder_t;

typedef enum {
	MODE_IMAGE,
	MODE_THUMB
} appmode_t;

typedef enum {
	DIR_LEFT  = 1,
	DIR_RIGHT = 2,
	DIR_UP    = 4,
	DIR_DOWN  = 8
} direction_t;

typedef enum {
	DEGREE_90  = 1,
	DEGREE_180 = 2,
	DEGREE_270 = 3
} degree_t;

typedef enum {
	FLIP_HORIZONTAL = 1,
	FLIP_VERTICAL   = 2
} flipdir_t;

typedef enum {
	SCALE_DOWN,
	SCALE_FIT,
	SCALE_FILL,
	SCALE_WIDTH,
	SCALE_HEIGHT,
	SCALE_ZOOM
} scalemode_t;

typedef enum {
	DRAG_RELATIVE,
	DRAG_ABSOLUTE
} dragmode_t;

typedef enum {
	CURSOR_ARROW,
	CURSOR_DRAG,
	CURSOR_WATCH,
	CURSOR_LEFT,
	CURSOR_RIGHT,
	CURSOR_NONE,

	CURSOR_COUNT
} cursor_t;

typedef enum {
	FF_WARN    = 1,
	FF_MARK    = 2,
	FF_TN_INIT = 4
} fileflags_t;

typedef struct {
	const char *name; /* as given by user */
	const char *path; /* always absolute */
#ifdef HAVE_LIBCURL
        const char *url;
#endif /* HAVE_LIBCURL */
	fileflags_t flags;
} fileinfo_t;

/* timeouts in milliseconds: */
enum {
	TO_REDRAW_RESIZE = 75,
	TO_REDRAW_THUMBS = 200,
	TO_CURSOR_HIDE   = 1200,
	TO_DOUBLE_CLICK  = 300
};

typedef void (*timeout_f)(void);

typedef struct arl arl_t;
typedef struct img img_t;
typedef struct opt opt_t;
typedef struct tns tns_t;
typedef struct win win_t;

