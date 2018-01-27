#ifndef H_DCLSTORE
#define H_DCLSTORE 1
struct dclitem {
	int indent;
	char *dcl;	/* this should be freeded */
	int dcllen;
	char *ident;
	int identlen;
	char *rest;
};
struct dclstore {
	struct dclitem store[64];
	int len;
};

int dclpush(struct dclstore *dcl, struct fmtline *c);
int dclflush(FILE *outfp, struct dclstore *dcl);
#endif
