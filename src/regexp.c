/**
 * \file regexp.c
 */
#include <pcre.h>
#include <stdio.h>
#include <memory.h>
#include <regexp.h>
#include <log.h>


// Patterns for all regular expressions
const char *patterns[smtp_re_count] = {
	"^220(?:\\s+.+)?\\r\\n",
	"^221(?:\\s+.+)?\\r\\n",
	"^250(?:\\s+.+)?\\r\\n",
	"^354(?:\\s+.+)?\\r\\n",
	"^DATA\\r\\n",
	"^MAIL FROM:\\s*<.+>\\r\\n",
	"^RCPT TO:\\s*<(.+@.+)>\\r\\n",
	"^RCPT TO:\\s*<.+@(.+)>\\r\\n",
	"MX\\s+\\d+\\s*(.+)\\.$",
	"MX\\s+(\\d+)\\s*.+\\.$"
};


// Array of all compiled regular expressions
pcre *smtp_re[smtp_re_count];


// Compiles one REGEXP; internal use only
int re_compile(smtp_re_name re_name) {
	int options = PCRE_CASELESS;
	int erroffset;
	const char *error;

	smtp_re[re_name] = pcre_compile((char *)patterns[re_name], options, &error, &erroffset, NULL);

	if (!smtp_re[re_name]) {
		ELOG("REGEXP compilation error.");
		return 1;
	}

	return 0;
}


// Compiles all REGEXPs; returns 1 on success, 0 on failure
int re_init() {
	int code = 0;

	for (int re_name = 0; re_name < smtp_re_count; ++re_name) {
		code |= re_compile(re_name);
	}

	return !code;
}


// Frees memory of all REGEXPs
int re_final() {
	for (int re_name = 0; re_name < smtp_re_count; ++re_name) {
		free(smtp_re[re_name]);
	}

	return 0;
}


// Returns 1 if string matches REGEXP; 0 otherwise
int re_match(smtp_re_name re_name, char *str, int length) {
	int ovector[30];
	int count = pcre_exec(smtp_re[re_name], NULL, (char *)str, length, 0, 0, ovector, 30);

	if (count < 0) {
		return 0;
	} else {
		return count;
	}
}


// Allocates and returns string for first substring in REGEXP; if there
// is no match, returns 0
char* re_match_and_alloc_substring(smtp_re_name re_name, char *str, int length) {
	int ovector[30];
	int count = pcre_exec(smtp_re[re_name], NULL, (char *)str, length, 0, 0, ovector, 30);

	if (count < 0) {
		return 0;
	} else {
		int start = ovector[2], end = ovector[3], length = end - start;
		char *new_str = malloc(length + 1);
		memcpy(new_str, str + start, length + 1);
		new_str[length] = '\0';
		return new_str;
	}
}


// Fills string by first substring in REGEXP and returns 1; if there is
// no match, returns 0
int re_match_and_fill_substring(smtp_re_name re_name, char *str, int length, char *substr) {
	int ovector[30];
	int count = pcre_exec(smtp_re[re_name], NULL, (char *)str, length, 0, 0, ovector, 30);

	if (count < 0) {
		return 0;
	} else {
		int start = ovector[2], end = ovector[3], length = end - start;
		memcpy(substr, str + start, length + 1);
		substr[length] = '\0';
		return 1;
	}
}


// Returns first matched REGEXP for string, or 0 if there are aren't any
smtp_re_name re_match_any(char *str, int length) {
	for (int re_name = 0; re_name < smtp_re_count; ++re_name) {
		if (pcre_exec(smtp_re[re_name], NULL, (char *)str, length, 0, 0, 0, 0) >= 0) return re_name;
	}

	return -1;
}
