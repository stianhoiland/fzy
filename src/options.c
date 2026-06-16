#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"

#include "../config.h"

static const char *usage_str =
    ""
    "Usage: fzy [OPTION]...\n"
    " -l, --lines=LINES        Specify how many lines to use (default 10)\n"
    " -p, --prompt=PROMPT      Input prompt (default '> ')\n"
    " -q, --query=QUERY        Use QUERY as the initial search string\n"
    " -e, --show-matches=QUERY Immediately output the sorted matches of QUERY\n"
    " -t, --tty=TTY            Specify file to use as TTY device (default /dev/tty)\n"
    " -s, --show-scores        Show the scores of each match\n"
    " -0, --read-null          Read input delimited by ASCII NUL characters\n"
    " -j, --workers NUM        Use NUM workers for searching. (default is # of CPUs)\n"
    " -i, --show-info          Show selection info line\n"
    " -k, --no-clear           Don't clear choices when exiting\n"
    " -o, --no-sort            Don't sort matches scored by search string\n"
    " -f, --no-fuzz            Do normal string matching (ignores ANSI colors)\n"
    " -g, --highlight          Color sub-string matches (expensive)\n"
    " -m, --multi-select       Select and output multiple choices\n"
    " -M, --enter-clears       Enter selects match and clears query. Output on\n"
	"                          empty query. Also enables -m.\n"
	" -S, --select=STRING      Preselect first occurrence of STRING among choices\n"
	" -x, --exact-match        Output query unless a choice is specifically\n"
	"                          selected.\n"
    " -h, --help     Display this help and exit\n"
    " -v, --version  Output version information and exit\n";

static void usage(const char *argv0) {
	fprintf(stdout, usage_str, argv0);
	exit(EXIT_SUCCESS);
}

static void die(const char *argv0) {
	fprintf(stderr, usage_str, argv0);
	exit(EXIT_FAILURE);
}

static struct option longopts[] = {{"show-matches", required_argument, NULL, 'e'},
				   {"query", required_argument, NULL, 'q'},
				   {"lines", required_argument, NULL, 'l'},
				   {"tty", required_argument, NULL, 't'},
				   {"prompt", required_argument, NULL, 'p'},
				   {"show-scores", no_argument, NULL, 's'},
				   {"read-null", no_argument, NULL, '0'},
				   {"version", no_argument, NULL, 'v'},
				   {"benchmark", optional_argument, NULL, 'b'},
				   {"workers", required_argument, NULL, 'j'},
				   {"show-info", no_argument, NULL, 'i'},
				   {"no-clear", no_argument, NULL, 'k'},
				   {"no-sort", no_argument, NULL, 'o'},
				   {"no-fuzz", no_argument, NULL, 'f'},
				   {"highlight", no_argument, NULL, 'g'},
				   {"multi-select", no_argument, NULL, 'm'},
				   {"enter-clears", no_argument, NULL, 'M'},
				   {"select", required_argument, NULL, 'S'},
				   {"exact-match", no_argument, NULL, 'x'},
				   {"help", no_argument, NULL, 'h'},
				   {NULL, 0, NULL, 0}};

void options_init(options_t *options) {
	/* set defaults */
	options->benchmark       = 0;
	options->filter          = NULL;
	options->init_search     = NULL;
	options->show_scores     = 0;
	options->scrolloff       = 1;
	options->tty_filename    = DEFAULT_TTY;
	options->num_lines       = DEFAULT_NUM_LINES;
	options->prompt          = DEFAULT_PROMPT;
	options->workers         = DEFAULT_WORKERS;
	options->input_delimiter = '\n';
	options->show_info       = DEFAULT_SHOW_INFO;
	options->clear_choices     = 1;
	options->sort_matches      = 1;
	options->fuzzy_search      = 1;
	options->highlight_matches = 0;
	options->multi_select      = 0;
	options->enter_clears      = 0;
	options->preselection      = "";
	options->exact_match       = 0;
}

void options_parse(options_t *options, int argc, char *argv[]) {
	options_init(options);

	int c;
	while ((c = getopt_long(argc, argv, "vhs0e:q:l:t:p:j:ikofgmMS:x", longopts, NULL)) != -1) {
		switch (c) {
			case 'v':
				printf("%s " VERSION ", Stian's fork\n", argv[0]);
				printf("Copyright 2014 John Hawthorn\n");
				exit(EXIT_SUCCESS);
				break;
			case 's':
				options->show_scores = 1;
				break;
			case '0':
				options->input_delimiter = '\0';
				break;
			case 'q':
				options->init_search = optarg;
				break;
			case 'e':
				options->filter = optarg;
				break;
			case 'b':
				if (optarg) {
					if (sscanf(optarg, "%d", &options->benchmark) != 1) {
						die(argv[0]);
					}
				} else {
					options->benchmark = 100;
				}
				break;
			case 't':
				options->tty_filename = optarg;
				break;
			case 'p':
				options->prompt = optarg;
				break;
			case 'j':
				if (sscanf(optarg, "%u", &options->workers) != 1) {
					die(argv[0]);
				}
				break;
			case 'l': {
				int l;
				if (!strcmp(optarg, "max")) {
					l = INT_MAX;
				} else if (sscanf(optarg, "%d", &l) != 1 || l < 1) {
					fprintf(stderr, "Invalid format for --lines: %s\n", optarg);
					fprintf(stderr, "Must be integer in range 3..\n");
					die(argv[0]);
				}
				options->num_lines = l;
			} break;
			case 'i':
				options->show_info = 1;
				break;
			case 'k':
				options->clear_choices = 0;
				break;
			case 'o':
				options->sort_matches = 0;
				break;
			case 'f':
				options->fuzzy_search = 0;
				break;
			case 'g':
				options->highlight_matches = 1;
				break;
			case 'm':
				options->multi_select = 1;
				break;
			case 'M':
				options->multi_select = 1;
				options->enter_clears = 1;
				break;
			case 'S':
				options->preselection = optarg;
				break;
			case 'x':
				options->exact_match = 1;
				break;
			case 'h':
				usage(argv[0]);
				break;
			default:
				die(argv[0]);
				break;
		}
	}
	if (optind != argc) {
		die(argv[0]);
	}
}
