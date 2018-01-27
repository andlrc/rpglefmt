#ifndef H_RPGLEFMT
#define H_RPGLEFMT 1
#define CFG_INDUNSET 	-1
#define CFG_INDLINE	-2
struct rpglecfg {
	int aligndcl;	/* align declarations */
	int indent;	/* default start indent */
	int shiftwidth;	/* number of spaces for a indent */
#ifdef FEAT_ICEBREAK
	int icebreak;	/* use IceBreak specific syntax (continuous comments and
			   multi line interpreted strings */
#endif
	int paren;	/* (de) indent parenthesis */
};
#endif
