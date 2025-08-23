#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "metadata.h"
#include "files.h"
char *outputdir = NULL;
char *inputdir = NULL;
metadata defaults;
void printusage(char *programname)
{
    fprintf(stderr,
	    "Usage: %s -i <inputdir> -o <outputdir>\nMetadata Defaults, if somethings not specified in the file metadata defaults to these values\n\t-s\tDeclares what to use as the default stylesheet (defaults to /style.css)\n\t-l\tDeclares what to use as the default language (defaults to \"en\"\n",
	    programname);
}

int main(int argc, char *argv[])
{
    int sflag, lflag;
    sflag = lflag = 0;
    int opt;
    while ((opt = getopt(argc, argv, "i:o:s:l:")) != -1) {
	switch (opt) {
	case 'i':
	    inputdir = optarg;
	    break;
	case 'o':
	    outputdir = optarg;
	    break;
	case 's':
	    strncpy(defaults.css, optarg, 401);
	    if (defaults.css[400] != '\0') {
		fprintf(stderr,
			"your css path is too long - greater than 400 characters\n");
		return 1;
	    }
	    sflag = 1;
	    break;
	case 'l':
	    strncpy(defaults.language, optarg, 11);
	    if(defaults.language[10] != '\0') {
		fprintf(stderr, "Language codes aren't supposed to be this long. examples of valid language codes: en, es-CO, uz-Cyrl-UZ\n see https://datatracker.ietf.org/doc/html/rfc5646 for more information\n");\
		return 1;
	    }
	    lflag = 1;
	    break;
	default:
	    printusage(argv[0]);
	    return 1;
	}
    }
    if(!sflag){
	strncpy(defaults.css, "/style.css", sizeof("/style.css")+1);
    }
    if(!lflag){
	defaults.language[0] = 'e';
	defaults.language[1] = 'n';
	defaults.language[2] = '\0';
    }
    if (inputdir == NULL || outputdir == NULL) {
	fprintf(stderr, "Needs both an input dir and outputdir specified\n");
	printusage(argv[0]);
	return 1;
    }
    /* I hope this if statement never results to true but idk with you people. Doesn't hurt to be extra safe */
    if (strlen(inputdir) > 4096 || strlen(outputdir) > 4096) {
	fprintf(stderr,
		"There is no reason for your path length to be this long, check what you're doing\n");
	return 1;
    }
    /* the recursedir() function will use the inputdir variable */
    recursedir(NULL);
    return 0;
}
