

struct opt {
	/* file list: */
	char **filenames;
	int from_stdin;
	int to_stdout;
	int recursive;
	int single_r;
	int filecnt;
	int startnum;

	/* image: */
	scalemode_t scalemode;
	float zoom;
	int animate;
	int gamma;
	int slideshow;
	int framerate;

	/* window: */
	int fullscreen;
	int hide_bar;
	long embed;
	char *geometry;
	char *res_name;

	/* misc flags: */
	int quiet;
	int thumb_mode;
	int clean_cache;
	int private_mode;
};

extern const opt_t *options;

void print_usage(void);
void parse_options(int, char**);
