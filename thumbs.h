

typedef struct {
	Imlib_Image im;
	int w;
	int h;
	int x;
	int y;
} thumb_t;

struct tns {
	fileinfo_t *files;
	thumb_t *thumbs;
	const int *cnt;
	int *sel;
	int initnext;
	int loadnext;
	int first, end;
	int r_first, r_end;

	win_t *win;
	int x;
	int y;
	int cols;
	int rows;
	int zl;
	int bw;
	int dim;
	/* Customs */
	//int max_scale;
	//int autozoom_threshold;


	int dirty;
};

void tns_clean_cache(tns_t*);
void tns_init(tns_t*, fileinfo_t*, const int*, int*, win_t*);
CLEANUP void tns_free(tns_t*);
int tns_load(tns_t*, int, int, int);
void tns_unload(tns_t*, int);
void tns_render(tns_t*);
void tns_mark(tns_t*, int, int);
//void tns_highlight(tns_t*, int, int);
void tns_highlight(tns_t *tns, int n, int hl);
int tns_move_selection(tns_t*, direction_t, int);
int tns_scroll(tns_t*, direction_t, int);
int tns_zoom(tns_t*, int);
int tns_translate(tns_t*, int, int);
/* Customs */
//int tns_zoom(tns_t*, int);
