/* Copyright 2011 Bert Muennich
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
#include <unistd.h>
#include <Imlib2.h>

//#include "sxiv.h"
//#include "config.h"
/* sxiv.c */
#include "main.h"

/* autoreload.c */
//#include "autoreload.h"

/* commands.c */
//#include "commands.h"

/* options.c */
#include "options.h"

/* thumbs.c */
//#include "thumbs.h"

/* util.c */
#include "util.h"

/* url.c */

/* window.c */
#include "window.h"




opt_t _options;
const opt_t *options = (const opt_t*) &_options;

void
print_usage(void)
{
	printf("usage: sxiv [-abcfhiopqRrtvZ] [-A framerate] [-e wid] [-G gamma] "
	       "[-g geometry] [-N name] [-n num] [-S delay] [-s mode] [-z zoom] "
#if HAVE_LIBCURL
	       "FILES / URLS...\n");
#else
 	       "FILES...\n");
#endif /* HAVE_LIBCURL */
}

void
parse_options(int argc, char **argv)
{
	int n, opt;
	char *end, *s;
	const char *scalemodes = "dfwh";

	progname = strrchr(argv[0], '/');
	progname = progname ? progname + 1 : argv[0];

	_options.from_stdin = 0;
	_options.to_stdout = 0;
	_options.recursive = 0;
	_options.single_r = 0;
	_options.startnum = 0;

	_options.scalemode = SCALE_DOWN;
	_options.zoom = 1.0;
	_options.animate = 0;
	_options.gamma = 0;
	_options.slideshow = 0;
	_options.framerate = 0;

	_options.fullscreen = 0;
	_options.embed = 0;
	_options.hide_bar = 0;
	_options.geometry = NULL;
	_options.res_name = NULL;

	_options.quiet = 0;
	_options.thumb_mode = 0;
	_options.clean_cache = 0;
	_options.private_mode = 0;

	while ((opt = getopt(argc, argv, "A:abce:fG:g:hin:N:opqRrS:s:tvZz:")) != -1) {
		switch (opt) {
			case '?':
				print_usage();
				exit(EXIT_FAILURE);
			case 'A':
				n = strtol(optarg, &end, 0);
				if (*end != '\0' || n <= 0)
					error(EXIT_FAILURE, 0, "Invalid argument for option -A: %s", optarg);
				_options.framerate = n;
				/* fall through */
			case 'a':
				_options.animate = 1;
				break;
			case 'b':
				_options.hide_bar = 1;
				break;
			case 'c':
				_options.clean_cache = 1;
				break;
			case 'e':
				n = strtol(optarg, &end, 0);
				if (*end != '\0')
					error(EXIT_FAILURE, 0, "Invalid argument for option -e: %s", optarg);
				_options.embed = n;
				break;
			case 'f':
				_options.fullscreen = 1;
				break;
			case 'G':
				n = strtol(optarg, &end, 0);
				if (*end != '\0')
					error(EXIT_FAILURE, 0, "Invalid argument for option -G: %s", optarg);
				_options.gamma = n;
				break;
			case 'g':
				_options.geometry = optarg;
				break;
			case 'h':
				print_usage();
				exit(EXIT_SUCCESS);
			case 'i':
				_options.from_stdin = 1;
				break;
			case 'n':
				n = strtol(optarg, &end, 0);
				if (*end != '\0' || n <= 0)
					error(EXIT_FAILURE, 0, "Invalid argument for option -n: %s", optarg);
				_options.startnum = n - 1;
				break;
			case 'N':
				_options.res_name = optarg;
				break;
			case 'o':
				_options.to_stdout = 1;
				break;
			case 'p':
				_options.private_mode = 1;
				break;
			case 'q':
				_options.quiet = 1;
				break;
			case 'R':
				_options.recursive = 1;
				break;
			case 'r':
				_options.single_r = 1;
				break;
			case 'S':
				n = strtof(optarg, &end) * 10;
				if (*end != '\0' || n <= 0)
					error(EXIT_FAILURE, 0, "Invalid argument for option -S: %s", optarg);
				_options.slideshow = n;
				break;
			case 's':
				s = strchr(scalemodes, optarg[0]);
				if (s == NULL || *s == '\0' || strlen(optarg) != 1)
					error(EXIT_FAILURE, 0, "Invalid argument for option -s: %s", optarg);
				_options.scalemode = s - scalemodes;
				break;
			case 't':
				_options.thumb_mode = 1;
				break;
			case 'v':
				puts("sxiv-"VERSION);
				exit(EXIT_SUCCESS);
			case 'Z':
				_options.scalemode = SCALE_ZOOM;
				_options.zoom = 1.0;
				break;
			case 'z':
				n = strtol(optarg, &end, 0);
				if (*end != '\0' || n <= 0)
					error(EXIT_FAILURE, 0, "Invalid argument for option -z: %s", optarg);
				_options.scalemode = SCALE_ZOOM;
				_options.zoom = (float) n / 100.0;
				break;
		}
	}

	_options.filenames = argv + optind;
	_options.filecnt = argc - optind;

	if (_options.filecnt == 1 && STREQ(_options.filenames[0], "-")) {
		_options.filenames++;
		_options.filecnt--;
		_options.from_stdin = 1;
	}
}
