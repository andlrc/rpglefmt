#ifndef H_FMT
#define H_FMT 1
enum fmtstate {
	STATE_NORM = 0,
#ifdef FEAT_ICEBREAK
	STATE_IBSTR,		/* Multi line IceBreak string */
	STATE_IBCOMMENT,	/* Multi line IceBreak comment */
#endif
	STATE_COMMENT,		/* Single line comment */
	STATE_STR		/* Multi line string */
};

#define FMTMAXPAREN 64
struct fmtline {
	enum fmtstate state;	/* parser state */
	int indent;		/* indent for the line */
	int spaces;		/* numbers of leading spaces */
	int xindent;		/* extra indent for a line */
	int lineno;
	char *line;
	int parencnt;
	int parenpos[FMTMAXPAREN];
	int argvalid;
};

int fmt(const struct rpglecfg *cfg, FILE *outfp, FILE *infp);
#endif
