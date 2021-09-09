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
#include <libgen.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <X11/keysym.h>
#include <X11/XF86keysym.h>

#include "sxiv.h"
#include "config.h"

typedef struct {
	struct timeval when;
	int active;
	timeout_f handler;
} timeout_t;

/* timeout handler functions: */
void redraw(void);
void reset_cursor(void);
void animate(void);
void slideshow(void);
void clear_resize(void);

appmode_t mode;
arl_t arl;
img_t img;
tns_t tns;
win_t win;

fileinfo_t *files;
int filecnt, fileidx;
int alternate;
int markcnt;
int markidx;

#if HAVE_LIBCURL
char **rmfiles;
int rmcnt, rmidx;
#endif /* HAVE_LIBCURL */

#ifdef ENABLE_COUNT
int prefix;
#endif /* ENABLE_COUNT */
/*What is this?
int extprefix;
int inputting_prefix;
*/
int resized = 0;

typedef struct {
	int err;
	char *cmd;
} extcmd_t;

struct {
	extcmd_t f;
	int fd;
	unsigned int i, lastsep;
	pid_t pid;
} info;

struct {
	extcmd_t f;
	int warned;
} keyhandler;

timeout_t timeouts[] = {
	{ { 0, 0 }, 0, redraw       },
	{ { 0, 0 }, 0, reset_cursor },
	{ { 0, 0 }, 0, animate      },
	{ { 0, 0 }, 0, slideshow    },
	{ { 0, 0 }, 0, clear_resize },
};

cursor_t imgcursor[3] = {
	CURSOR_ARROW, CURSOR_ARROW, CURSOR_ARROW
};

#if HAVE_LIBCURL
CLEANUP void
tmp_unlink(char **rmfiles, int n) {
	while (n--)
		unlink(rmfiles[n]);
}
#endif /* HAVE_LIBCURL */


void
cleanup(void)
{
#if HAVE_LIBCURL
	tmp_unlink(rmfiles, rmidx);
#endif /* HAVE_LIBCURL */
	img_close(&img, 0);
	arl_cleanup(&arl);
	tns_free(&tns);
	win_close(&win);
}

char *
check_and_get_path(char *filename)
{
	char *path;
	if (access(filename, R_OK) < 0 ||
		(path = realpath(filename, NULL)) == NULL)
		error(0, errno, "%s", filename);
	return(path);
}

#if HAVE_LIBCURL
static void
internal_check_add_file(char *filename, char *url, int given)
#else
void
check_add_file(char *filename, int given)
#endif /* HAVE_LIBCURL */
{
	char *path;
	int i;

	if (*filename == '\0')
		return;

	if (access(filename, R_OK) < 0 || (path = realpath(filename, NULL)) == NULL) {
		if (given)
			error(0, errno, "%s", filename);
		return;
	}

	/* If file is already present in files array, don't duplicate it */
	for (i = 0; i < fileidx; i++) {
		if (!strcmp(files[i].path, path))
			return;
	}

	if (fileidx == filecnt) {
		filecnt *= 2;
		files = erealloc(files, filecnt * sizeof(*files));
		memset(&files[filecnt/2], 0, filecnt/2 * sizeof(*files));
	}

	files[fileidx].name = estrdup(filename);
	files[fileidx].path = path;
#if HAVE_LIBCURL
	if (url != NULL) {
		files[fileidx].url = estrdup(url);
		if (rmidx == rmcnt) {
			rmcnt *= 2;
			rmfiles = erealloc(rmfiles, rmcnt * sizeof(char*));
			memset(&rmfiles[rmcnt/2], 0, rmcnt/2 * sizeof(char*));
		}
		rmfiles[rmidx++] = path;
	}
#endif /* HAVE_LIBCURL */
	if (given)
		files[fileidx].flags |= FF_WARN;
	fileidx++;
}


#if HAVE_LIBCURL
void
check_add_file(char *filename, int given)
{
	internal_check_add_file(filename, NULL, given);
}

void
check_add_url(char *filename, char *url, int given)
{
	internal_check_add_file(filename, url, given);
}
#endif /* HAVE_LIBCURL */

void
remove_file(int n, int manual)
{
	if (n < 0 || n >= filecnt)
		return;

	if (filecnt == 1) {
		if (!manual)
			fprintf(stderr, "sxiv: no more files to display, aborting\n");
		exit(manual ? EXIT_SUCCESS : EXIT_FAILURE);
	}
	if (files[n].flags & FF_MARK)
		markcnt--;

	if (files[n].path != files[n].name)
		free((void*) files[n].path);
	free((void*) files[n].name);

	if (n + 1 < filecnt) {
		if (tns.thumbs != NULL) {
			memmove(tns.thumbs + n, tns.thumbs + n + 1, (filecnt - n - 1) *
			        sizeof(*tns.thumbs));
			memset(tns.thumbs + filecnt - 1, 0, sizeof(*tns.thumbs));
		}
		memmove(files + n, files + n + 1, (filecnt - n - 1) * sizeof(*files));
	}
	filecnt--;
	if (fileidx > n || fileidx == filecnt)
		fileidx--;
	if (alternate > n || alternate == filecnt)
		alternate--;
	if (markidx > n || markidx == filecnt)
		markidx--;
}

void
set_timeout(timeout_f handler, int time, int overwrite)
{
	int i;

	for (i = 0; i < ARRLEN(timeouts); i++) {
		if (timeouts[i].handler == handler) {
			if (!timeouts[i].active || overwrite) {
				gettimeofday(&timeouts[i].when, 0);
				TV_ADD_MSEC(&timeouts[i].when, time);
				timeouts[i].active = 1;
			}
			return;
		}
	}
}

void
reset_timeout(timeout_f handler)
{
	int i;

	for (i = 0; i < ARRLEN(timeouts); i++) {
		if (timeouts[i].handler == handler) {
			timeouts[i].active = 0;
			return;
		}
	}
}

int
check_timeouts(struct timeval *t)
{
	int i = 0, tdiff, tmin = -1;
	struct timeval now;

	while (i < ARRLEN(timeouts)) {
		if (timeouts[i].active) {
			gettimeofday(&now, 0);
			tdiff = TV_DIFF(&timeouts[i].when, &now);
			if (tdiff <= 0) {
				timeouts[i].active = 0;
				if (timeouts[i].handler != NULL)
					timeouts[i].handler();
				i = tmin = -1;
			} else if (tmin < 0 || tdiff < tmin)
				tmin = tdiff;
		}
		i++;
	}
	if (tmin > 0 && t != NULL)
		TV_SET_MSEC(t, tmin);
	return tmin > 0;
}

void
close_info(void)
{
	if (info.fd != -1) {
		kill(info.pid, SIGTERM);
		close(info.fd);
		info.fd = -1;
	}
}

void
open_info(void)
{
	int pfd[2];
	char w[12], h[12];

	if (info.f.err != 0 || info.fd >= 0 || win.bar.h == 0)
		return;
	win.bar.l.buf[0] = '\0';
	if (pipe(pfd) < 0)
		return;
	if ((info.pid = fork()) == 0) {
		close(pfd[0]);
		dup2(pfd[1], 1);
		snprintf(w, sizeof(w), "%d", img.w);
		snprintf(h, sizeof(h), "%d", img.h);
		execl(info.f.cmd, info.f.cmd, files[fileidx].name, w, h, NULL);
		//execl(info.f.cmd, info.f.cmd, basename((char *)files[fileidx].name), w, h, NULL);
		error(EXIT_FAILURE, errno, "exec: %s", info.f.cmd);
	}
	close(pfd[1]);
	if (info.pid < 0)
		close(pfd[0]);
	else {
		fcntl(pfd[0], F_SETFL, O_NONBLOCK);
		info.fd = pfd[0];
		info.i = info.lastsep = 0;
	}
}

void
read_info(void)
{
	ssize_t i, n;
	char buf[BAR_L_LEN];

	while (1) {
		n = read(info.fd, buf, sizeof(buf));
		if (n < 0 && errno == EAGAIN)
			return;
		else if (n == 0)
			goto end;
		for (i = 0; i < n; i++) {
			if (buf[i] == '\n') {
				if (info.lastsep == 0) {
					win.bar.l.buf[info.i++] = ' ';
					info.lastsep = 1;
				}
			} else {
				win.bar.l.buf[info.i++] = buf[i];
				info.lastsep = 0;
			}
			if (info.i + 1 == win.bar.l.size)
				goto end;
		}
	}
end:
	info.i -= info.lastsep;
	win.bar.l.buf[info.i] = '\0';
	win_draw(&win);
	close_info();
}

#ifdef ENABLE_COUNT
int
evaluate_prefix(void)
{
	extern int prefix;

	if (prefix_keys)
		return -1;
	else
		return prefix;
}
#endif /* ENABLE_COUNT */

void
load_image(int new)
{
	int prev = new < fileidx;
	static int current;

	if (new < 0 || new >= filecnt)
		return;

	if (win.xwin != None)
		win_set_cursor(&win, CURSOR_WATCH);
	reset_timeout(slideshow);

	if (new != current)
		alternate = current;

	img_close(&img, 0);
	while (!img_load(&img, &files[new])) {
		remove_file(new, 0);
		if (new >= filecnt)
			new = filecnt - 1;
		else if (new > 0 && prev)
			new--;
	}
	files[new].flags &= ~FF_WARN;
	fileidx = current = new;

	close_info();
	open_info();
	arl_setup(&arl, files[fileidx].path);

	if (img.multi.cnt > 0 && img.multi.animate)
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, 1);
	else
		reset_timeout(animate);
}

int
mark_image(int n, int on)
{
	markidx = n;
	if (!!(files[n].flags & FF_MARK) != on) {
		files[n].flags ^= FF_MARK;
		markcnt += on ? 1 : -1;
		if (mode == MODE_THUMB)
			tns_mark(&tns, n, on);
		return 1;
	}
	return 0;
}

void
bar_put(win_bar_t *bar, const char *fmt, ...)
{
	size_t len = bar->size - (bar->p - bar->buf), n;
	va_list ap;

	va_start(ap, fmt);
	n = vsnprintf(bar->p, len, fmt, ap);
	bar->p += MIN(len, n);
	va_end(ap);
}

#define BAR_SEP "  "

void
update_info(void)
{
	unsigned int i, fn, fw;
	const char *mark;
	win_bar_t *l = &win.bar.l, *r = &win.bar.r;

	/* update bar contents */
	if (win.bar.h == 0)
		return;
	for (fw = 0, i = filecnt; i > 0; fw++, i /= 10);
	mark = files[fileidx].flags & FF_MARK ? " " : "";
	l->p = l->buf;
	r->p = r->buf;
	if (mode == MODE_THUMB) {
			//show name even when caching
		if (tns.loadnext < tns.end) {
			bar_put(r, "Loading...  %0*d" BAR_SEP, fw, tns.loadnext + 1);
			strncpy(l->buf, basename((char *)files[fileidx].name), l->size); }
	//bar_put(r, "[%s%d] %0*d/%d", mark, markcnt, fw, fileidx + 1, filecnt, "Loading... %0*d", fw, tns.loadnext + 1);
		else if (tns.initnext < filecnt) {
			bar_put(r, "Caching...  %0*d" BAR_SEP, fw, tns.initnext + 1);
		strncpy(l->buf, basename((char *)files[fileidx].name), l->size);}
			//bar_put(r, "[%s%d] %0*d/%d", mark, markcnt, fw, fileidx + 1, filecnt, "Caching... %0*d", fw, tns.initnext + 1);
		else {	//Show only the image basename() in titlebar, not the entire path(./)

#if HAVE_LIBCURL
			if (files[fileidx].url != NULL)
				strncpy(l->buf, files[fileidx].url, l->size);
			else {
#endif /* HAVE_LIBCURL */
				//strncpy(l->buf, files[fileidx].name, l->size);
			strncpy(l->buf, basename((char *)files[fileidx].name), l->size);
			bar_put(r, "[%s%d] %0*d/%d", mark, markcnt, fw, fileidx + 1, filecnt);
#if HAVE_LIBCURL
			}
#endif /* HAVE_LIBCURL */
		}

//			strncpy(l->buf, basename((char *)files[fileidx].name), l->size);
//			bar_put(r, "[%s%d] %0*d/%d", mark, markcnt, fw, fileidx + 1, filecnt);
//			}
	} else {//show marked imgs on bar(only if there's at least one image marked)
		//But this doens't update...?
		if (markcnt != 0)
			bar_put(r, "[%s%d]" BAR_SEP, mark, markcnt);
		if (img.ss.on) {
			if (img.ss.delay % 10 != 0)
				bar_put(r, "%2.1fs" BAR_SEP, (float)img.ss.delay / 10);
			else
				bar_put(r, "%ds" BAR_SEP, img.ss.delay / 10);
		}
		if (img.gamma != 0)
			bar_put(r, "Gamma%+d" BAR_SEP, img.gamma);
		bar_put(r, "%3d%%" BAR_SEP, (int) (img.zoom * 100.0));
		if (img.multi.cnt > 0) {
			for (fn = 0, i = img.multi.cnt; i > 0; fn++, i /= 10);
			bar_put(r, "%0*d/%d" BAR_SEP, fn, img.multi.sel + 1, img.multi.cnt);
		}
		bar_put(r, "%0*d/%d", fw, fileidx + 1, filecnt);
//		if (info.f.err)
		//	strncpy(l->buf, files[fileidx].name, l->size);
		if (info.f.err) {
#if HAVE_LIBCURL
			if (files[fileidx].url != NULL)
				strncpy(l->buf, files[fileidx].url, l->size);
			else {
#endif /* HAVE_LIBCURL */
				strncpy(l->buf, files[fileidx].name, l->size);
#if HAVE_LIBCURL
			}
#endif /* HAVE_LIBCURL */
		}
	}
}


/* url.c */
#if HAVE_LIBCURL
#include <curl/curl.h>

int
is_url(const char *url) {
	if ((!strncmp(url, "http://", 7))
			|| (!strncmp(url, "https://", 8))
			|| (!strncmp(url, "gopher://", 9))
			|| (!strncmp(url, "gophers://", 10))
			|| (!strncmp(url, "ftp://", 6))
			|| (!strncmp(url, "file://", 7)))
		return 1;
	return 0;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

int
get_url(const char *url, char **out) {
	CURL *curl;
	CURLcode ret;
	char tmp[1024] = { 0 };
	FILE *file = NULL;
	size_t j;

	for (j = strlen(url); j != 0 && url[j] != '/'; j--);
	if (j != 0)
		j++;

	snprintf(tmp, sizeof(tmp), "/tmp/sxiv-%s", url + j);
	file = fopen(tmp, "wb");

	if (file == NULL)
		return -1;

	*out = strdup(tmp);
	if (*out == NULL)
		return -1;

	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

	ret = curl_easy_perform(curl);

	if (ret != CURLE_OK) {
		printf("Curl error: %s\n", curl_easy_strerror(ret));
		return -1;
	}
	fclose(file);
	curl_easy_cleanup(curl);
	curl_global_cleanup();

	return 0;
}
#endif /* HAVE_LIBCURL */


int
ptr_third_x(void)
{
	int x, y;

	win_cursor_pos(&win, &x, &y);
	return MAX(0, MIN(2, (x / (win.w * 0.33))));
}

void
redraw(void)
{
	int t;

	if (mode == MODE_IMAGE) {
		img_render(&img);
		if (img.ss.on) {
			t = img.ss.delay * 100;
			if (img.multi.cnt > 0 && img.multi.animate)
				t = MAX(t, img.multi.length);
			set_timeout(slideshow, t, 0);
		}
	} else
		tns_render(&tns);
	update_info();
	win_draw(&win);
	reset_timeout(redraw);
	reset_cursor();
}

void
reset_cursor(void)
{
	int c, i;
	cursor_t cursor = CURSOR_NONE;

	if (mode == MODE_IMAGE) {
		for (i = 0; i < ARRLEN(timeouts); i++) {
			if (timeouts[i].handler == reset_cursor) {
				if (timeouts[i].active) {
					c = ptr_third_x();
					//c = MAX(fileidx > 0 ? 0 : 1, c);
					//c = MIN(fileidx + 1 < filecnt ? 2 : 1, c);
					cursor = imgcursor[c];
				}
				break;
			}
		}
	} else {
		if (tns.loadnext < tns.end || tns.initnext < filecnt)
			cursor = CURSOR_WATCH;
		else
			cursor = CURSOR_ARROW;
	}
	win_set_cursor(&win, cursor);
}

void
animate(void)
{
	if (img_frame_animate(&img)) {
		redraw();
		set_timeout(animate, img.multi.frames[img.multi.sel].delay, 1);
	}
}

void
slideshow(void)
{
	load_image(fileidx + 1 < filecnt ? fileidx + 1 : 0);
	redraw();
}

void
clear_resize(void)
{
	resized = 0;
}

int
is_input_ev(Display *dpy, XEvent *ev, XPointer arg)
{
	return ev->type == ButtonPress || ev->type == KeyPress;
}

void
run_key_handler(const char *key, unsigned int mask)
{
	pid_t pid;
	FILE *pfs;
	int marked = markcnt > 0;
	int changed = 0;
	int f, i, pfd[2];
	int fcnt = marked ? markcnt : 1;
	char kstr[32], oldbar[BAR_L_LEN];
	struct stat *oldst, st;
	XEvent dump;

	if (keyhandler.f.err != 0) {
		if (!keyhandler.warned) {
			error(0, keyhandler.f.err, "%s", keyhandler.f.cmd);
			keyhandler.warned = 1;
		}
		return;
	}
	if (key == NULL)
		return;

	if (pipe(pfd) < 0) {
		error(0, errno, "pipe");
		return;
	}
	if ((pfs = fdopen(pfd[1], "w")) == NULL) {
		error(0, errno, "open pipe");
		close(pfd[0]), close(pfd[1]);
		return;
	}
	oldst = emalloc(fcnt * sizeof(*oldst));

	close_info();
	memcpy(oldbar, win.bar.l.buf, sizeof(oldbar));

	/* How to put the text on the midlle? */
	snprintf(win.bar.l.buf, win.bar.l.size, "%s >_ Keyhandler...", oldbar);
	win_draw(&win);
	win_set_cursor(&win, CURSOR_WATCH);

	snprintf(kstr, sizeof(kstr), "%s%s%s%s",
	    mask & ControlMask ? "C-" : "",
	    mask & Mod1Mask    ? "M-" : "",
	    mask & ShiftMask   ? "S-" : "", key);

	if ((pid = fork()) == 0) {
		close(pfd[1]);
		dup2(pfd[0], 0);
		execl(keyhandler.f.cmd, keyhandler.f.cmd, kstr, NULL);
		error(EXIT_FAILURE, errno, "exec: %s", keyhandler.f.cmd);
	}
	close(pfd[0]);
	if (pid < 0) {
		error(0, errno, "fork");
		fclose(pfs);
		goto end;
	}

	for (f = i = 0; f < fcnt; i++) {
		if ((marked && (files[i].flags & FF_MARK)) || (!marked && i == fileidx)) {
			stat(files[i].path, &oldst[f]);
			fprintf(pfs, "%s\n", files[i].name);
			f++;
		}
	}
	fclose(pfs);

	while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);

	for (f = i = 0; f < fcnt; i++) {
		if ((marked && (files[i].flags & FF_MARK)) || (!marked && i == fileidx)) {
			if (stat(files[i].path, &st) != 0 ||
				  memcmp(&oldst[f].st_mtime, &st.st_mtime, sizeof(st.st_mtime)) != 0)
			{
				if (tns.thumbs != NULL) {
					tns_unload(&tns, i);
					tns.loadnext = MIN(tns.loadnext, i);
				}
				changed = 1;
			}
			f++;
		}
	}
	/* drop user input events that occurred while running the key handler */
	while (XCheckIfEvent(win.env.dpy, &dump, is_input_ev, NULL));

end:
	if (mode == MODE_IMAGE) {
		if (changed) {
			img_close(&img, 1);
			load_image(fileidx);
		} else
			open_info();
	}
	free(oldst);
	reset_cursor();
	redraw();
}

#define MODMASK(mask) ((mask) & (ShiftMask|ControlMask|Mod1Mask))
void
on_keypress(XKeyEvent *kev)
{
	int i;
	unsigned int sh = 0;
	KeySym ksym, shksym;
	char dummy, key;
	int bound = 0;
	int dirty = 0;

	XLookupString(kev, &key, 1, &ksym, NULL);

	if (kev->state & ShiftMask) {
		kev->state &= ~ShiftMask;
		XLookupString(kev, &dummy, 1, &shksym, NULL);
		kev->state |= ShiftMask;
		if (ksym != shksym)
			sh = ShiftMask;
	}
	if (IsModifierKey(ksym))
		return;
#ifdef ENABLE_COUNT
		/* I dont need this */
	else if (extprefix) {
		run_key_handler(XKeysymToString(ksym), kev->state & ~sh);
		extprefix = False;
	} else if (key >= '0' && key <= '9') {
		/* number prefix for commands */
		prefix = prefix * 10 + (int) (key - '0');
		return;
	}
#endif /* ENABLE_COUNT */
	else for (i = 0; i < ARRLEN(keys); i++) {
		if (keys[i].ksym == ksym &&
		    MODMASK(keys[i].mask | sh) == MODMASK(kev->state) &&
		    keys[i].cmd >= 0 && keys[i].cmd < CMD_COUNT &&
		    (cmds[keys[i].cmd].mode < 0 || cmds[keys[i].cmd].mode == mode))
		{
			if (cmds[keys[i].cmd].func(keys[i].arg))
				dirty = 1;
			bound = 1;
		}
	}
	if (!bound)
		run_key_handler(XKeysymToString(ksym), kev->state & ~sh);
	if (dirty)
		redraw();
#ifdef ENABLE_COUNT
	prefix = 0;
#endif /* ENABLE_COUNT */
}

void
on_buttonpress(XButtonEvent *bev)
{
	int i, sel;
	int dirty = 0;
	static Time firstclick;

	if (mode == MODE_IMAGE) {
		set_timeout(reset_cursor, TO_CURSOR_HIDE, 1);	/* Clock */
		reset_cursor();

		for (i = 0; i < ARRLEN(buttons); i++) {
			if (buttons[i].button == bev->button &&
			    MODMASK(buttons[i].mask) == MODMASK(bev->state) &&
			    buttons[i].cmd >= 0 && buttons[i].cmd < CMD_COUNT &&
			    (cmds[buttons[i].cmd].mode < 0 || cmds[buttons[i].cmd].mode == mode))
			{
				if (cmds[buttons[i].cmd].func(buttons[i].arg))
					dirty = 1;
			}
		}
		if (dirty)
			redraw();
	} else {
		/* thumbnail mode (hard-coded) */
		switch (bev->button) {
			case Button1:
				if ((sel = tns_translate(&tns, bev->x, bev->y)) >= 0) {
					if (sel != fileidx) {
						tns_highlight(&tns, fileidx, 0);
						tns_highlight(&tns, sel, 1);
						fileidx = sel;
						firstclick = bev->time;
						redraw();
					} else if (bev->time - firstclick <= TO_DOUBLE_CLICK) {
						mode = MODE_IMAGE;
						set_timeout(reset_cursor, TO_CURSOR_HIDE, 1);
						load_image(fileidx);
						redraw();
					} else
						firstclick = bev->time;
				}
				break;
			case Button3:
				if ((sel = tns_translate(&tns, bev->x, bev->y)) >= 0) {
					int on = !(files[sel].flags & FF_MARK);
					XEvent e;

					for (;;) {
						if (sel >= 0 && mark_image(sel, on))
							redraw();
						XMaskEvent(win.env.dpy,
						           ButtonPressMask | ButtonReleaseMask | PointerMotionMask, &e);
						if (e.type == ButtonPress || e.type == ButtonRelease)
							break;
						while (XCheckTypedEvent(win.env.dpy, MotionNotify, &e));
						sel = tns_translate(&tns, e.xbutton.x, e.xbutton.y);
					}
				}
				break;
			case Button4:
			case Button5:
				if (tns_scroll(&tns, bev->button == Button4 ? DIR_UP : DIR_DOWN,
				               (bev->state & ControlMask) != 0))
					redraw();
				break;
		}
	}
#ifdef ENABLE_COUNT
	prefix = 0;
#endif /* ENABLE_COUNT */
}

const struct timespec ten_ms = {0, 10000000};

void
run(void)
{
	int xfd;
	fd_set fds;
	struct timeval timeout;
	int discard, init_thumb, load_thumb, to_set;
	XEvent ev, nextev;

	while (1) {
		to_set = check_timeouts(&timeout);
		init_thumb = mode == MODE_THUMB && tns.initnext < filecnt;
		load_thumb = mode == MODE_THUMB && tns.loadnext < tns.end;

		if ((init_thumb || load_thumb || to_set || info.fd != -1 ||
			   arl.fd != -1) && XPending(win.env.dpy) == 0)
		{
			if (load_thumb) {
				set_timeout(redraw, TO_REDRAW_THUMBS, 0);
				if (!tns_load(&tns, tns.loadnext, 0, 0)) {
					remove_file(tns.loadnext, 0);
					tns.dirty = 1;
				}
				if (tns.loadnext >= tns.end)
					redraw();
			} else if (init_thumb) {
				set_timeout(redraw, TO_REDRAW_THUMBS, 0);
				if (!tns_load(&tns, tns.initnext, 0, 1))
					remove_file(tns.initnext, 0);
			} else {
				xfd = ConnectionNumber(win.env.dpy);
				FD_ZERO(&fds);
				FD_SET(xfd, &fds);
				if (info.fd != -1) {
					FD_SET(info.fd, &fds);
					xfd = MAX(xfd, info.fd);
				}
				if (arl.fd != -1) {
					FD_SET(arl.fd, &fds);
					xfd = MAX(xfd, arl.fd);
				}
				select(xfd + 1, &fds, 0, 0, to_set ? &timeout : NULL);
				if (info.fd != -1 && FD_ISSET(info.fd, &fds))
					read_info();
				if (arl.fd != -1 && FD_ISSET(arl.fd, &fds)) {
					if (arl_handle(&arl)) {
						/* when too fast, imlib2 can't load the image */
						nanosleep(&ten_ms, NULL);
						img_close(&img, 1);
						load_image(fileidx);
						redraw();
					}
				}
			}
			continue;
		}

		do {
			XNextEvent(win.env.dpy, &ev);
			discard = 0;
			if (XEventsQueued(win.env.dpy, QueuedAlready) > 0) {
				XPeekEvent(win.env.dpy, &nextev);
				switch (ev.type) {
					case ConfigureNotify:
					case MotionNotify:
						discard = ev.type == nextev.type;
						break;
					case KeyPress:
						discard = (nextev.type == KeyPress || nextev.type == KeyRelease)
						          && ev.xkey.keycode == nextev.xkey.keycode;
						break;
				}
			}
		} while (discard);

		switch (ev.type) {
			/* handle events */
			case ButtonPress:
				on_buttonpress(&ev.xbutton);
				break;
			case ClientMessage:
				if ((Atom) ev.xclient.data.l[0] == atoms[ATOM_WM_DELETE_WINDOW])
					cmds[g_quit].func(0);
				break;
			case ConfigureNotify:
				if (win_configure(&win, &ev.xconfigure)) {
					if (mode == MODE_IMAGE) {
						img.dirty = 1;
						img.checkpan = 1;
					} else
						tns.dirty = 1;
					if (!resized) {
						redraw();
						set_timeout(clear_resize, TO_REDRAW_RESIZE, 0);
						resized = 1;
					} else
						set_timeout(redraw, TO_REDRAW_RESIZE, 0);
				}
				break;
			case KeyPress:
				on_keypress(&ev.xkey);
				break;
			case MotionNotify:
				if (mode == MODE_IMAGE) {
					set_timeout(reset_cursor, TO_CURSOR_HIDE, 1);
					reset_cursor();
				}
				break;
		}
	}
}

int
fncmp(const void *a, const void *b)
{
	return strcoll(((fileinfo_t*) a)->name, ((fileinfo_t*) b)->name);
}

void
sigchld(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void
setup_signal(int sig, void (*handler)(int sig))
{
	struct sigaction sa;

	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
	if (sigaction(sig, &sa, 0) == -1)
		error(EXIT_FAILURE, errno, "signal %d", sig);
}

int
main(int argc, char **argv)
{
	int i, start;
	size_t n;
	ssize_t len;
	char *filename, dirn[PATH_MAX], savedname[PATH_MAX] = {0}, title[] = "Sxiv - ";
	const char *homedir, *dsuffix = "";
	struct stat fstats;
	r_dir_t dir;

	setup_signal(SIGCHLD, sigchld);
	setup_signal(SIGPIPE, SIG_IGN);

	setlocale(LC_COLLATE, "");

	parse_options(argc, argv);

	if (options->clean_cache) {
		tns_init(&tns, NULL, NULL, NULL, NULL);
		tns_clean_cache(&tns);
		exit(EXIT_SUCCESS);
	}

	if (options->filecnt == 0 && !options->from_stdin) {
		print_usage();
		exit(EXIT_FAILURE);
	}

	if (options->recursive || options->single_r || options->from_stdin)
		filecnt = 1024;
	else
		filecnt = options->filecnt;

	files = emalloc(filecnt * sizeof(*files));
	memset(files, 0, filecnt * sizeof(*files));
	fileidx = 0;
#if HAVE_LIBCURL
	rmcnt = 16;
	rmfiles = emalloc(rmcnt * sizeof(char*));
	rmidx = 0;
#endif /* HAVE_LIBCURL */

	if (options->from_stdin) {
		n = 0;
		filename = NULL;
		while ((len = getline(&filename, &n, stdin)) > 0) {
			if (filename[len-1] == '\n')
				filename[len-1] = '\0';
			check_add_file(filename, 1);
		}
		free(filename);
	}

	for (i = 0; i < options->filecnt; i++) {
		filename = options->filenames[i];

		if (stat(filename, &fstats) < 0) {
#if HAVE_LIBCURL
			if (is_url(filename)) {
				char *tmp;

				if (get_url(filename, &tmp) == 0) {
					check_add_url(tmp, filename, 1);
					free(tmp);
					continue;
				} else
					error(0, errno, "%s", filename);
			}
#endif /* HAVE_LIBCURL */
			error(0, errno, "%s", filename);
			continue;
		}
		if (!S_ISDIR(fstats.st_mode)) {
			/* -r */
			if (options->single_r) {
				/* Display the image given first */
				memcpy(savedname, check_and_get_path(filename), sizeof(savedname));
				filename = dirname(filename);
			} else {
				check_add_file(filename, 1);
				continue;
			}
		}
		if (r_opendir(&dir, filename, options->recursive) < 0) {
			error(0, errno, "%s", filename);
			continue;
		}
		start = fileidx;
		while ((filename = r_readdir(&dir, 1)) != NULL) {
			check_add_file(filename, 0);
			free((void*) filename);
		}
		r_closedir(&dir);
		if (fileidx - start > 1)
			qsort(files + start, fileidx - start, sizeof(fileinfo_t), fncmp);
	}

	if (fileidx == 0)
		error(EXIT_FAILURE, 0, "No valid image file given, aborting");

	filecnt = fileidx;
	/* If -n has been given, use it. Else, display the first argument file */
	if (options->startnum > 0)
		fileidx = options->startnum < filecnt ? options->startnum : 0;
	else {
		for (i = 0; i < filecnt; i++) {
			if (!strcmp(files[i].path, savedname)) {
				fileidx = i;
				break;
			}
			fileidx = 0;
		}
	}

	for (i = 0; i < ARRLEN(buttons); i++) {
		if (buttons[i].cmd == i_cursor_navigate) {
			imgcursor[0] = CURSOR_LEFT;
			imgcursor[2] = CURSOR_RIGHT;
			break;
		}
	}

	win_init(&win);
	img_init(&img, &win);
	arl_init(&arl);

	/* Set window title to 'Sxiv - [First file's directory's basename]' */
	strncpy(dirn, files[0].path, sizeof(dirn)-1);
	strncat(title, basename(dirname(dirn)), PATH_MAX);
	//strncat(title, dirname(dirn), PATH_MAX);
	win.title = title;

    	/* The executable files shouldn't be always on '.config/' but on the current dir */
	if ((homedir = getenv("XDG_CONFIG_HOME")) == NULL || homedir[0] == '\0') {
		homedir = getenv("HOME");
		dsuffix = "/.config";
	}
	if (homedir != NULL) {
		extcmd_t *cmd[] = { &info.f, &keyhandler.f };
		/* Name */
		const char *name[] = { "image-info", "key-handler" };

		for (i = 0; i < ARRLEN(cmd); i++) {
			n = strlen(homedir) + strlen(dsuffix) + strlen(name[i]) + 12;
			cmd[i]->cmd = (char*) emalloc(n);
			snprintf(cmd[i]->cmd, n, "%s%s/sxiv/exec/%s", homedir, dsuffix, name[i]);
			if (access(cmd[i]->cmd, X_OK) != 0)
				cmd[i]->err = errno;
		}
	} else
		error(0, 0, "Exec directory not found");
	info.fd = -1;

	if (options->thumb_mode) {
		mode = MODE_THUMB;
		tns_init(&tns, files, &filecnt, &fileidx, &win);
		while (!tns_load(&tns, fileidx, 0, 0))
			remove_file(fileidx, 0);
	} else {
		mode = MODE_IMAGE;
		tns.thumbs = NULL;
		load_image(fileidx);
	}
	win_open(&win);
	win_set_cursor(&win, CURSOR_WATCH);

	atexit(cleanup);

	set_timeout(redraw, 25, 0);

	run();

	return 0;
}
