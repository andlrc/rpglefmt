#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "rpglefmt.h"
#include "fmt.h"
#include "dclstore.h"

int dclpush(struct dclstore *dcl, struct fmtline *c)
{
#define ltrim(p)		\
	while (isspace(*p)) {	\
		p++;		\
	}
	struct dclitem *item;
	char *pline;

	/* empty line */
	if (!c) {
		item = &(dcl->store[dcl->len++]);
		memset(item, 0, sizeof(struct dclitem));
		item->dcl = item->ident = item->rest;
		return 0;
	}

	/*
	 * each line is something like:
	 * dcl-s  ident_long   type(siz);
	 * dcl-ds ident_longer likeds(ident);
	 * dcl-ds ident_super_long;
	 */
	if (!(pline = strdup(c->line)))
		return -1;

	item = &(dcl->store[dcl->len++]);
	memset(item, 0, sizeof(struct dclitem));
	item->indent = c->indent;
	item->dcl = pline;
	item->ident = item->rest = 0;

	/* find end and length of dcl-XX */;
	while (*pline != ';' && !isspace(*pline)) {
		pline++;
		item->dcllen++;
	}

	/* dcl-XX$ */
	if (*pline == '\0')
		return 0;
	*pline++ = '\0';

	ltrim(pline);
	item->ident = pline;
	/* find end and length of the identifier */
	while (*pline != ';' && !isspace(*pline)) {
		pline++;
		item->identlen++;
	}

	/* dcl-XX ident$ | dcl-XX ident; */
	if (*pline == '\0' || *pline == ';')
		return 0;
	*pline++ = '\0';

	ltrim(pline);
	item->rest = pline;

	return 0;
}

int dclflush(FILE *outfp, struct dclstore *dcl)
{
	struct dclitem *item;
	int i, dclmax, identmax;

	dclmax = 0;
	identmax = 0;

	for (i = 0; i < dcl->len; i++) {
		item = &(dcl->store[i]);

		if (item->dcllen > dclmax)
			dclmax = item->dcllen;
		if (item->identlen > identmax)
			identmax = item->identlen;
	}

	for (i = 0; i < dcl->len; i++) {
		item = &(dcl->store[i]);
		if (item->rest)
			fprintf(outfp, "%*s%-*s %-*s %s", item->indent, "", dclmax, item->dcl, identmax, item->ident, item->rest);
		else if (item->ident)
			fprintf(outfp, "%*s%-*s %s", item->indent, "", dclmax, item->dcl, item->ident);
		else if (item->dcl)
			fprintf(outfp, "%*s%-*s", item->indent, "", dclmax, item->dcl);
		else
			fprintf(outfp, "\n");
		free(item->dcl);
	}
	dcl->len = 0;
	return 0;
}
