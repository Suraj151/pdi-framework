/******************************** Regex Match *********************************
This file is part of the pdi stack.

This is free software. you can redistribute it and/or modify it but without any
warranty.

Author          : Suraj I.
created Date    : 28th May 2026
******************************************************************************/

#include "RegexMatch.h"


static int token_length(const char *p, int plen) {
	if (plen <= 0) return 0;
	if (*p == '\\' && plen >= 2) return 2;
	if (*p == '[') {
		int i = 1;
		if (i < plen && p[i] == '^') i++;
		while (i < plen && p[i] != ']') i++;
		return (i < plen) ? (i + 1) : 1;
	}
	return 1;
}


static bool token_matches(const char *p, int plen, char ch) {
	if (plen <= 0) return false;
	if (*p == '.') return true;
	if (*p == '\\' && plen >= 2) return ch == p[1];
	if (*p == '[') {
		bool negate = false;
		int i = 1;
		if (i < plen && p[i] == '^') { negate = true; i++; }
		bool found = false;
		while (i < plen && p[i] != ']') {
			if (i + 2 < plen && p[i+1] == '-' && p[i+2] != ']') {
				if (ch >= p[i] && ch <= p[i+2]) found = true;
				i += 3;
			} else {
				if (ch == p[i]) found = true;
				i++;
			}
		}
		return found != negate;
	}
	return ch == *p;
}


static bool match_here(const char *p, int plen, const char *t, int tlen, int tpos);


static bool match_quant(const char *p, int tokenlen, char quant,
                        int plen_after_quant, const char *t, int tlen, int tpos) {
	int minMatches = (quant == '+') ? 1 : 0;
	int maxMatches = (quant == '?') ? 1 : 32767;

	int count = 0;
	while (count < maxMatches && (tpos + count) < tlen &&
	       token_matches(p, tokenlen, t[tpos + count])) {
		count++;
	}

	while (count >= minMatches) {
		if (match_here(p + tokenlen + 1, plen_after_quant, t, tlen, tpos + count)) return true;
		if (count == 0) break;
		count--;
	}
	return false;
}


static bool match_here(const char *p, int plen, const char *t, int tlen, int tpos) {
	if (plen <= 0) return true;
	if (plen == 1 && *p == '$') return tpos == tlen;

	int tokenlen = token_length(p, plen);
	if (tokenlen == 0) return true;

	if (tokenlen < plen) {
		char nextCh = p[tokenlen];
		if (nextCh == '*' || nextCh == '+' || nextCh == '?') {
			return match_quant(p, tokenlen, nextCh, plen - tokenlen - 1, t, tlen, tpos);
		}
	}

	if (tpos < tlen && token_matches(p, tokenlen, t[tpos])) {
		return match_here(p + tokenlen, plen - tokenlen, t, tlen, tpos + 1);
	}
	return false;
}


int regex_match(const char *pattern, const char *text, int textlen) {
	if (nullptr == pattern || nullptr == text) return -1;

	int plen = 0;
	while (pattern[plen]) plen++;

	if (plen > 0 && pattern[0] == '^') {
		return match_here(pattern + 1, plen - 1, text, textlen, 0) ? 0 : -1;
	}

	for (int i = 0; i <= textlen; i++) {
		if (match_here(pattern, plen, text, textlen, i)) return i;
	}
	return -1;
}
