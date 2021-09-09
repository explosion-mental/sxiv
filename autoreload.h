

struct arl {
	int fd;
	int wd_dir;
	int wd_file;
	char *filename;
};

void arl_init(arl_t*);
void arl_cleanup(arl_t*);
void arl_setup(arl_t*, const char* /* result of realpath(3) */);
int arl_handle(arl_t*);

