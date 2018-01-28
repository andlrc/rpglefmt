#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "rpglefmt.h"
#include "fmt.h"
#include "dclstore.h"

#define ROOTINDENT 7

/*
 * startwith and contains both checks word boundaries, i.e
 * startwith("abc", "ab") is false
 * contains("dcl-ds abc likeds(", "likeds") is true
 */
#define startwith(s, q)					\
	(s &&						\
	 strncasecmp(s, q, sizeof(q) - 1) == 0 &&	\
	 !isalnum(s[sizeof(q) - 1]))

#define contains(s, q)					\
	_contains(s, q, sizeof(q) - 1)
static int _contains(char *s, char *q, int qlen)
{
	char *p;

	if (!s)
		return 0;

	for (p = strstr(s, q); p; p = strstr(p + 1, q)) {
		if ((p == s || !isalnum(*(p - 1))) && !isalnum(p[qlen]))
			return 1;
	}

	return 0;
}

static int getindent(struct rpglecfg *cfg, struct fmtline *c, struct fmtline *p)
{
	char *ptmp;
	int indent, hint;

	hint = 0;

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

	switch ((int) c->state) {
#ifdef FEAT_ICEBREAK
	case STATE_IBCOMMENT:
		/*
		 * indent the '*' in continues comments
		 */
		if (startwith(c->line, "*") && !startwith(p->line, "*")) {
			indent = p->indent + 1;
			goto finish;
		}
		if (!startwith(c->line, "*") && startwith(p->line, "*")) {
			indent = p->indent - 1;
			goto finish;
		}
		break;
#endif
	case STATE_NORM:
#ifdef FEAT_ICEBREAK
		if (cfg->icebreak) {
			/*
			 * de indent after the indent created by the '*' in
			 * continues comments
			 */
			if (startwith(p->line, "*") &&
			    contains(p->line, "*/")) {
				hint = -1;
			}
		}
#endif
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
		break;
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
			else if (cfg->icebreak && ptmp[1] == '*')
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
		memcpy(c->parenpos, p->parenpos, sizeof(int) * FMTMAXPAREN);
	}

	return indent + hint;
}

int fmt(struct rpglecfg *cfg, FILE *outfp, FILE *infp)
{
	char *linebuf, *pline;
	size_t linesiz;
	int lineno;
	struct dclstore dcl;
	struct fmtline c;	/* current line */
	struct fmtline p;	/* Previous non blank line */

	linebuf = 0;
	linesiz = 0;

	memset(&dcl, 0, sizeof(struct dclstore));

	memset(&c, 0, sizeof(struct fmtline));
	memset(&p, 0, sizeof(struct fmtline));
	c.line = p.line = 0;

	for (lineno = 0; getline(&linebuf, &linesiz, infp) != -1; lineno++) {
		for (pline = linebuf, c.spaces = 0;
		     isspace(*pline); pline++, c.spaces++)
			/* remove leading indentation */;

		if (*pline == '\0') {	/* ignore empty lines */
			if (cfg->aligndcl) {
				if (dclpush(&dcl, 0) == -1)
					return -1;
			} else {
				fprintf(outfp, "%s", linebuf);
			}
			continue;
		}

		c.lineno = lineno;
		if (!(c.line = strdup(pline)))
			return -1;
		c.indent = getindent(cfg, &c, &p);
		if (cfg->relindent && (
#ifdef FEAT_ICEBREAK
		    p.state == STATE_IBSTR ||
#endif
		    p.state == STATE_STR)) {
			c.xindent += (c.spaces - p.spaces);
		} else if (cfg->paren && p.parencnt) {
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
				if (dclpush(&dcl, &c) == -1)
					return -1;
			} else {
				if (dcl.len) {
					dclflush(outfp, &dcl);
				}
				fprintf(outfp, "%*s%s", indent, "",
					c.line);
			}
		} else {
			fprintf(outfp, "%*s%s", indent, "", c.line);
		}

		free(p.line);
		memcpy(&p, &c, sizeof(struct fmtline));
	}

	if (dcl.len)
		dclflush(outfp, &dcl);

	free(p.line);
	free(linebuf);

	return 0;
}
