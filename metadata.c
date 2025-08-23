#include "metadata.h"
#include "third-party/jsmn.h"
#include <string.h>

/* shows where metadata ends and markdown file starts
 * returns -1 if no metadata found
 * returns 0 if no metadata found */
int seperatemetadata(char *file, size_t filelength)
{
    if (file[0] != '{') {
	return 0;
    }
    int quotes = 0;
    int braces = 1;
    int i = 1;
    while (braces != 0) {
	/* if we reach end of file return error */
	if (i == filelength) {
	    return -1;
	}
	switch (file[i]) {
	case '"':
	    quotes ^= 1;
	    break;
	case '{':
	    if (!quotes) {
		braces++;
	    }
	    break;
	case '}':
	    if (!quotes) {
		braces--;
	    }
	    break;
	default:
	    /* do nothing */
	    break;
	}
	i++;
    }
    return i;
}

/* this function is used for debugging more than anything */
void printstring(char *json, int start, int end)
{
    for (int i = start; i < end; i++) {
	putchar(json[i]);
    }
}

int matchobjectwithstring(char *json, int start, int end, char *string)
{
/* first check if strings are same length */
    if (end - start != strlen(string)) {
	return 0;
    }
    int matches = 1;
    for (int i = 0; i < end - start; i++) {
	if (json[i + start] != string[i]) {
	    matches = 0;
	}
    }
    return matches;
}

/* split (destructive) file into json and markdown using the seperator */
filelayout splitfile(char *file, int position)
{
    filelayout fl;
    file[position] = '\0';
    fl.json = file;
    fl.markdown = file + position + 1;
    return fl;
}


/* parse metadata json */
metadata parsemetadata(char *json, int size)
{
    enum parser { NEITHER = 0, TITLE = 1, DESCRIPTION = 2, LANGUAGE = 3, CSS = 4
    };
    enum parser state = NEITHER;
    metadata metad;
    jsmn_parser p;
    jsmn_init(&p);
    jsmntok_t tokens[128];
    int r = jsmn_parse(&p, json, size, tokens, 128);
    if (r < 0) {
	fprintf(stderr, "failed to parse metadata\n");
	return defaults;
    }
    int titlefound, descriptionfound, languagefound, cssfound;
    titlefound = descriptionfound = languagefound = cssfound = 0;
    for (int i = 0; i < 128; i++) {
	if (tokens[i].end == 0) {
	    i = 128;
	} else if (tokens[i].type == JSMN_STRING && state == NEITHER) {
	    if (!titlefound) {
		int matchestitle =
		    matchobjectwithstring(json, tokens[i].start, tokens[i].end,
					  "title");
		if (matchestitle) {
		    titlefound = 1;
		    state = TITLE;
		}
	    }
	    if (!descriptionfound && state == NEITHER) {
		int matchesdesc =
		    matchobjectwithstring(json, tokens[i].start, tokens[i].end,
					  "description");
		if (matchesdesc) {
		    descriptionfound = 1;
		    state = DESCRIPTION;
		}
	    }
	    if (!languagefound && state == NEITHER) {
		int matcheslanguage =
		    matchobjectwithstring(json, tokens[i].start, tokens[i].end,
					  "language");
		if (matcheslanguage) {
		    languagefound = 1;
		    state = LANGUAGE;
		}
	    }
	    if (!cssfound && state == NEITHER) {
		int matchescss =
		    matchobjectwithstring(json, tokens[i].start, tokens[i].end,
					  "css");
		if (matchescss) {
		    cssfound = 1;
		    state = CSS;
		}
	    }
	} else if (tokens[i].type == JSMN_STRING && state == TITLE) {
	    int length = tokens[i].end - tokens[i].start;
	    if (length > 100) {
		fprintf
		    (stderr,
		     "warning: your title is too long - past 100 characters long. Bonobo trunicates at 100 characters\nRecommended title length is <60 characters for search engine indexing purposes\n");
		length = 100;
	    }
	    for (int y = 0; y < length; y++) {
		metad.title[y] = json[y + tokens[i].start];
	    }
	    metad.title[length] = '\0';
	    state = NEITHER;
	} else if (tokens[i].type == JSMN_STRING && state == DESCRIPTION) {
	    int length = tokens[i].end - tokens[i].start;
	    if (length > 300) {
		fprintf
		    (stderr,
		     "warning: your description is too long - past 300 characters long. Bonobo trunicates at 300 characters\nRecommended description length is <60 characters for search engine indexing purposes\n");
		length = 300;
	    }
	    for (int y = 0; y < length; y++) {
		metad.description[y] = json[y + tokens[i].start];
	    }
	    metad.description[length] = '\0';
	    state = NEITHER;
	} else if (tokens[i].type == JSMN_STRING && state == LANGUAGE) {
	    int length = tokens[i].end - tokens[i].start;
	    if (length > 10) {
		fprintf
		    (stderr,
		     "warning: language code too long, past 10 characters\nexamples of valid language codes: en, es-CO, uz-Cyrl-UZ\n see https://datatracker.ietf.org/doc/html/rfc5646 for more information. Using default language\n");
		languagefound = 0;
	    } else {
		for (int y = 0; y < length; y++) {
		    metad.language[y] = json[y + tokens[i].start];
		}
		metad.language[length] = '\0';
	    }
	    state = NEITHER;
	} else if (tokens[i].type == JSMN_STRING && state == CSS) {
	    int length = tokens[i].end - tokens[i].start;
	    if (length > 400) {
		fprintf(stderr,
			"warning: CSS path too long - past 400 characters. Just using default CSS path now\n");
		languagefound = 0;
	    } else {
		for (int y = 0; y < length; y++) {
		    metad.css[y] = json[y + tokens[i].start];
		}
		metad.css[length] = '\0';
	    }
	    state = NEITHER;
	} else {
	    /* do nothing */
	}
    }
    /* use defaults if nothing defined */
    if (!languagefound) {
	strncpy(metad.language, defaults.language, sizeof(metad.language));
    }
    if (!cssfound) {
	strncpy(metad.css, defaults.css, sizeof(metad.css));
    }
    if (!descriptionfound) {
	metad.description[0] = '\0';
    }
    if (!titlefound) {
	metad.title[0] = '\0';
    }
    return metad;
}
