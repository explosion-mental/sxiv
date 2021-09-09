#include <dirent.h>

typedef struct {
	DIR *dir;
	char *name;
	int d;
	int recursive;

	char **stack;
	int stcap;
	int stlen;
} r_dir_t;

extern const char *progname;

void *emalloc(size_t);
void *erealloc(void*, size_t);
char *estrdup(const char*);
void error(int, int, const char*, ...);
void size_readable(float*, const char**);
int r_opendir(r_dir_t*, const char*, int);
int r_closedir(r_dir_t*);
char *r_readdir(r_dir_t*, int);
int r_mkdir(char*);
