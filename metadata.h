#ifndef BONOBO_METADATA_HEADER
#define BONOBO_METADATA_HEADER
#include <stdio.h>
typedef struct {
    char *json;
    char *markdown;
} filelayout;
/* it'll automatically trunicate and warn you if your title or description are too long */
typedef struct {
    char title[101];
    char description[301];
    char language[11];		/* even though most language codes fit in 5 bytes (es-CO for example), some don't (uz-Cyrl-UZ) */
    char css[401];		/* pls don't make your css path too long thats dumb */
} metadata;
int seperatemetadata(char *file, size_t filelength);
filelayout splitfile(char *file, int position);
metadata parsemetadata(char *json, int size);
metadata setdefaults();
extern metadata defaults;
#endif
