#ifndef BONOBO_FILES_HEADER
#define BONOBO_FILES_HEADER
#include "third-party/md4c-html.h"
extern char *outputdir;
extern char *inputdir;
void recursedir(char *path);
void handlefiles(char *inputfile, char *outputfile);
void output_callback(const MD_CHAR * text, MD_SIZE size, void *userdata);
#endif
