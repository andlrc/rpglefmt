#define VERSION "0.1.0"

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include "rpglefmt.h"
#include "fmt.h"

char *get_program_name(char *s)
{
	char *p;
	if ((p = strrchr(s, '/')))
		return strdup(p + 1);
	else
		return strdup(s);
}

void print_help(char *program_name)
{
	printf("Usage %s [OPTION]... [FILE]...\n"
	       "\n"
	       "  -d         align declaration specifications\n"
	       "  -i indent  the default indentation, overrides -u\n"
#ifdef FEAT_ICEBREAK
	       "  -I         support IceBreak extensions\n"
#endif
	       "  -p         try to be clever when indenting multi line parenthesis,\n"
	       "             use multiply times for different styles\n"
	       "  -r         preserve relative indentation in multi line string\n"
	       "  -s width   number of spaces to indent with\n"
	       "  -u         use indentation from first line, overrides -i\n"
	       "  -h         show this help and exit\n"
	       "  -V         display version information\n", program_name);
}

void print_version(char *program_name)
{
	printf("%s: " VERSION "\n"
#ifdef FEAT_ICEBREAK
	       "+icebreak\n"
#endif
	       "", program_name);
}

int main(int argc, char **argv)
{
	struct rpglecfg cfg;
	int argind;
	FILE *infp;
	char *infile;
	int c;

	char *program_name = get_program_name(*argv);

	memset(&cfg, 0, sizeof(struct rpglecfg));

	/* defaults */
	cfg.shiftwidth = 2;
	cfg.indent = CFG_INDUNSET;

	opterr = 0;
	while ((c = getopt(argc, argv, "dIi:prs:uhV")) != -1) {
		switch (c) {
		case 'd':	/* align declarations */
			cfg.aligndcl++;
			break;
#ifdef FEAT_ICEBREAK
		case 'I':	/* icebreak support */
			cfg.icebreak++;
			break;
#endif
		case 'i':	/* default indent */
			cfg.indent = atoi(optarg);
			break;
		case 'p':	/* indent parenthesis */
			cfg.paren++;
			break;
		case 'r':	/* preserve relative indentation in strings */
			cfg.relindent++;
			break;
		case 's':	/* shiftwidth */
			cfg.shiftwidth = atoi(optarg);
			break;
		case 'u':	/* use indent from first line */
			cfg.indent = CFG_INDLINE;
			break;
		case 'h':
			print_help(program_name);
			return 0;
		case 'V':
			print_version(program_name);
			return 0;
		default:
			fprintf(stderr, "%s: invalid option -- '%c'\n",
				program_name, optopt ? optopt : c);
			return 2;
		}
	}

	argind = optind;
	infile = "-";

	do {
		if (argind < argc)
			infile = argv[argind];

		if (strcmp(infile, "-") == 0) {
			infp = stdin;
		} else if (!(infp = fopen(infile, "r"))) {
			fprintf(stderr, "%s: %s: %s\n", program_name,
				infile, strerror(errno));
			continue;
		}

		if (fmt(&cfg, stdout, infp) == -1) {
			fclose(infp);
			fprintf(stderr, "%s: %s: %s\n", program_name,
				infile, strerror(errno));
			continue;
		}

		if (infp != stdin)
			fclose(infp);
	} while (++argind < argc);
}
