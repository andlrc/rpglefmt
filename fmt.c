#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "rpglefmt.h"

#define MAXPAREN 64
enum {
	STATE_NORM = 0,
#ifdef FEAT_ICEBREAK
	STATE_IBSTR,		/* Multi line IceBreak string */
	STATE_IBCOMMENT,	/* Multi line IceBreak comment */
#endif
	STATE_COMMENT,		/* Single line comment */
	STATE_STR		/* Multi line string */
} state;
struct line {
	int state;		/* parser state */
	int indent;		/* indent for the line */
	int spaces;		/* numbers of leading spaces */
	int xindent;		/* extra indent for a line */
	int lineno;
	char *line;
	int parencnt;
	int parenpos[MAXPAREN];
	int argvalid;
};

#define ROOTINDENT 7

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

static int pushdcl(struct dclstore *dcl, struct line *c)
{
#define ltrim(p)		\
	while (isspace(*p)) {	\
		p++;		\
	}
	struct dclitem *item;
	char *pline;

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

static int flushdcl(FILE *outfp, struct dclstore *dcl)
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
		else
			fprintf(outfp, "%*s%-*s %s", item->indent, "", dclmax, item->dcl, item->ident);
		free(item->dcl);
	}
	dcl->len = 0;
	return 0;
}

/*
 * startwith and contains both checks word boundaries, i.e
 * startwith("abc", "ab") is false
 * contains("dcl-ds abc likeds(", "likeds") is true
 */
#define startwith(s, q)					\
	(s &&						\
	 strncasecmp(s, q, sizeof(q) - 1) == 0 &&	\
	 !isalnum(s[sizeof(q) - 1]))

static int contains(char *s, char *q)
{
	char *p;
	int qlen;

	if (!s)
		return 0;

	qlen = strlen(q);

	for (p = strstr(s, q); p; p = strstr(p + 1, q)) {
		if ((p == s || !isalnum(*(p - 1))) && !isalnum(p[qlen]))
			return 1;
	}

	return 0;
}

static int getindent(struct rpglecfg *cfg, struct line *c, struct line *p,
		     struct line *pp)
{
	char *ptmp;
	int indent;

	/*
	 * there is no lines to determinate indent, so use what is set in
	 * "cfg.indent", check if "**FREE" is present or default to 7.
	 */
	if (c->lineno == 0) {
		switch (cfg->indent) {
		case CFG_INDUNSET:
			if (startwith(c->line, "**FREE"))
				indent = 0;
#ifdef FEAT_ICEBREAK
			else if (cfg->icebreak)
				indent = 0;
#endif
			else
				indent = ROOTINDENT;
			break;
		case CFG_INDLINE:
			indent = c->spaces;
			break;
		default:
			indent = cfg->indent;
		}
		goto finish;
	}

	if (c->state == STATE_NORM) {
		/*
		 * a "when" which follows a "select" should be indented: All other
		 * "when" should be de indented
		 */
		if (startwith(c->line, "when")) {
			if (startwith(p->line, "select"))
				indent = p->indent + cfg->shiftwidth;
			else
				indent = p->indent - cfg->shiftwidth;
			goto finish;
		}

		/*
		 * "dcl-pi", "dcl-pr", and "dcl-ds" with no parameters should not
		 * indent the "end-xx"
		 */
		if ((startwith(p->line, "dcl-pi") && startwith(c->line, "end-pi")) ||
		    (startwith(p->line, "dcl-pr") && startwith(c->line, "end-pr")) ||
		    (startwith(p->line, "dcl-ds") && startwith(c->line, "end-ds"))) {
			indent = p->indent;
			goto finish;
		}

		/* 
		 * "dcl-ds" with "likeds" on the same line doesn't take a definition and
		 * should not do any indent
		 */
		if (startwith(p->line, "dcl-ds") && contains(p->line, "likeds")) {
			indent = p->indent;
			goto finish;
		}

		/*
		 * add indent for opening keywords
		 */
		if (startwith(p->line, "if") ||
		    startwith(p->line, "else") ||
		    startwith(p->line, "elseif") ||
		    startwith(p->line, "dou") ||
		    startwith(p->line, "dow") ||
		    startwith(p->line, "for") ||
		    startwith(p->line, "monitor") ||
		    startwith(p->line, "on-error") ||
		    startwith(p->line, "when") ||
		    startwith(p->line, "other") ||
		    startwith(p->line, "dcl-proc") ||
		    startwith(p->line, "begsr") ||
		    startwith(p->line, "dcl-pi") ||
		    startwith(p->line, "dcl-pr") ||
		    startwith(p->line, "dcl-ds")) {
			indent = p->indent + cfg->shiftwidth;
			goto finish;
		}

		/*
		 * remove indent for closing keywords
		 */
		if (startwith(c->line, "endif") ||
		    startwith(c->line, "enddo") ||
		    startwith(c->line, "endfor") ||
		    startwith(c->line, "endmon") ||
		    startwith(c->line, "other") ||
		    startwith(c->line, "else") ||
		    startwith(c->line, "elseif") ||
		    startwith(c->line, "on-error") ||
		    startwith(c->line, "end-pi") ||
		    startwith(c->line, "end-proc") ||
		    startwith(c->line, "endsr") ||
		    startwith(c->line, "end-pr") ||
		    startwith(c->line, "end-ds")) {
			indent = p->indent - cfg->shiftwidth;
			goto finish;
		}

		/*
		 * "endsl" have to de indent two levels
		 */
		if (startwith(c->line, "endsl")) {
			indent = p->indent - cfg->shiftwidth * 2;
			goto finish;
		}
	}

	/*
	 * indent havn't been found, use the previous line's indent
	 */
	indent = p->indent;

      finish:

	/*
	 * Count parenthesis and calculate indentation based on it
	 */
	for (ptmp = c->line; *ptmp; ptmp++) {
		switch (*ptmp) {
		case '/':
			if (ptmp[1] == '/')
				c->state = STATE_COMMENT;
#ifdef FEAT_ICEBREAK
			else if (ptmp[1] == '*')
				c->state = STATE_IBCOMMENT;
#endif
			break;
		case '(':
			if (c->state == STATE_NORM)
				c->parenpos[c->parencnt++] = (ptmp - c->line);
			break;
		case ')':
			if (c->state == STATE_NORM) {
				c->parencnt--;
				c->argvalid = 0;
			}
			break;
		case '\'':
			if (c->state == STATE_NORM) {
				c->state = STATE_STR;
			} else if (c->state == STATE_STR) {
				if (ptmp[1] == '\'')
					ptmp++;
				else
					c->state = STATE_NORM;
			}
			break;
#ifdef FEAT_ICEBREAK
		case '`':
			if (cfg->icebreak) {
				if (c->state == STATE_NORM) {
					c->state = STATE_IBSTR;
				} else if (c->state == STATE_IBSTR) {
					if (ptmp[1] == '`')
						ptmp++;
					else
						c->state = STATE_NORM;
				}
			}
			break;
		case '*':
			if (cfg->icebreak) {
				if (c->state == STATE_IBCOMMENT && ptmp[1] == '/')
					c->state = STATE_NORM;
			}
			break;
#endif
		case ':':
			if (c->state == STATE_NORM)
				c->argvalid = c->parencnt > 0;
			break;
		}
	}

	if (c->state == STATE_COMMENT)
		c->state = STATE_NORM;

	if (c->parencnt < 0) {
		c->parencnt = p->parencnt + c->parencnt;
		memcpy(c->parenpos, p->parenpos, sizeof(int) * MAXPAREN);
	}

	return indent;
}

int fmt(struct rpglecfg *cfg, FILE *outfp, FILE *infp)
{
	char *linebuf, *pline;
	size_t linesiz;
	int lineno;
	struct dclstore dcl;
	struct line c;	/* current line */
	struct line p;	/* Previous non blank line */
	struct line pp;	/* Previous previous non blank line */

	linebuf = 0;
	linesiz = 0;

	memset(&dcl, 0, sizeof(struct dclstore));

	memset(&c, 0, sizeof(struct line));
	memset(&p, 0, sizeof(struct line));
	memset(&pp, 0, sizeof(struct line));
	c.line = p.line = pp.line = 0;

	for (lineno = 0; getline(&linebuf, &linesiz, infp) != -1; lineno++) {
		for (pline = linebuf, c.spaces = 0;
		     isspace(*pline); pline++, c.spaces++)
			/* remove leading indentation */;

		if (*pline == '\0') {	/* ignore empty lines */
			fprintf(outfp, "%s", linebuf);
			continue;
		}

		c.lineno = lineno;
		if (!(c.line = strdup(pline)))
			return -1;
		c.indent = getindent(cfg, &c, &p, &pp);
		if (cfg->paren && p.parencnt) {
			if (p.argvalid && cfg->paren >= 2)
				c.xindent = p.parenpos[p.parencnt - 1] + 1;
			else
				c.xindent = p.parencnt * cfg->shiftwidth;

			/*
			 * Remove the indentation that is added via being after
			 * an opener (if, else, dow, ...) and only keep the part
			 * the parenthesis have added.
			 * TODO: Should this be togglable
			 */
			c.xindent -= (c.indent - p.indent);
		} else {
			c.xindent = 0;
		}
		int indent = c.indent + c.xindent;

		if (indent < 0)
			indent = 0;

		if (cfg->aligndcl) {
			if (startwith(c.line, "dcl-s") ||
			    startwith(c.line, "dcl-c") ||
			    startwith(c.line, "dcl-ds") ||
			    startwith(c.line, "dcl-pr")) {
				if (pushdcl(&dcl, &c) == -1)
					return -1;
			} else {
				if (dcl.len) {
					flushdcl(outfp, &dcl);
				}
				fprintf(outfp, "%*s%s", indent, "", c.line);
			}
		} else {
			fprintf(outfp, "%*s%s", indent, "", c.line);
		}

		free(pp.line);
		memcpy(&pp, &p, sizeof(struct line));
		memcpy(&p, &c, sizeof(struct line));
	}

	if (dcl.len)
		flushdcl(outfp, &dcl);

	free(pp.line);
	free(p.line);
	free(linebuf);

	return 0;
}
