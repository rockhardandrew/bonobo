#include <stdlib.h>
#include <stdio.h>
#include "files.h"
#include "metadata.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <string.h>
/* static memory buffers used for buffers */
static char inpath[4096];
static char outpath[4096];
void output_callback(const MD_CHAR *text, MD_SIZE size, void *userdata)
{
    FILE *out = (FILE *) userdata;
    fwrite(text, 1, size, out);
}

/* create directory if it doesn't exist 
  returns 0 if directory exists, 1 if directory created succesfully, and -1 if creating directory failed */
int cdir(char *path)
{
    struct stat st = { 0 };
    if (stat(path, &st) == -1) {
	printf("creating directory %s\n", path);
	if (mkdir(path, 0770) == -1) {
	    perror("Error creating directory");
	    return -1;
	}
	return 1;
    }
    if (!S_ISDIR(st.st_mode)) {
	fprintf(stderr, "Path exists but is not a directory: %s\n", path);
	return -1;
    }
    return 0;
}

void handlefiles(char *inputfile, char *outputfile)
{
    printf("opening file: %s ", inputfile);
    FILE *fp = fopen(inputfile, "r");
    if (fp == NULL) {
	fprintf(stderr, "failed to open\n");
	return;
    }
    fseek(fp, 0, SEEK_END);
    size_t filesize = ftell(fp);
    rewind(fp);
    char *file = malloc(filesize + 1);
    if (file == NULL) {
	fprintf(stderr, "malloc() failed\n");
	return;
    }
    size_t bytesread = fread(file, 1, filesize, fp);
    if (bytesread != filesize) {
	fprintf(stderr, "failed to read\n");
	return;
    }
    fclose(fp);
    file[filesize] = '\0';
    int seperator = seperatemetadata(file, filesize);
    filelayout fl = splitfile(file, seperator);
    metadata metad;
    switch (seperator) {
    case -1:
	fprintf(stderr, "syntax error parsing metadata\n");
	free(file);
	return;
    case 0:
	printf("- no metadata found\n");
	metad = defaults;
	break;
    default:
	metad = parsemetadata(fl.json, seperator);
	break;
    }
    FILE *output = fopen(outputfile, "w");
    if (output == NULL) {
	fprintf(stderr, "failed to open file %s\n", outputfile);
	free(file);
	return;
    }
    fprintf(output,
	    "<!DOCTYPE html>\n<html lang=\"%s\">\n<head>\n\t<meta name=\"generator\" content=\"Bonobo 0.1\">\n\t<title>%s</title>\n\t<link rel='stylesheet' type='text/css' href='%s'>\n\t<meta name=\"description\" content=\"%s\">\n\t<meta charset=\"utf-8\">\n</head>\n<body>\n<main>\n<article>\n",
	    metad.language, metad.title, metad.css, metad.description);
    int result = md_html(fl.markdown,
			 filesize - seperator - 1,
			 output_callback,
			 output,
			 MD_FLAG_TABLES |
			 MD_FLAG_TASKLISTS |
			 MD_FLAG_STRIKETHROUGH |
			 MD_FLAG_PERMISSIVEAUTOLINKS | MD_FLAG_UNDERLINE,
			 0);
    fprintf(output, "</article>\n</main>\n</body>\n</html>\n");
    fclose(output);
    free(file);
    printf("succesfully wrote file %s\n", outputfile);
    return;
}

/* similar to strncpy but you can specify which index to start at 
 * I created this function because sometimes you need to concatinate while also removing data */
void strwrite(char *dest, char *src, int start, int maxsize)
{
    int size = strlen(src);
    for (int i = 0; i < maxsize; i++) {
	if (i > size) {
	    dest[i + start] = '\0';
	} else {
	    dest[i + start] = src[i];
	}
    }
}

void addfilename(char *filename)
{
    int inlen = strlen(inpath);
    int outlen = strlen(outpath);
    int filenlen = strlen(filename);
    inpath[inlen] = '/';
    outpath[outlen] = '/';
    strwrite(inpath, filename, inlen + 1, 4095 - inlen - 1);
    strwrite(outpath, filename, outlen + 1, 4095 - outlen - 1);
    strncat(inpath, ".md", 3);
    strncat(outpath, ".html", 5);
}

/* recursedir uses a static char buffer, the leadingpath variable is used for recursion purposes*/
void recursedir(char *leadingpath)
{

    if (leadingpath == NULL) {
	strncpy(inpath, inputdir, strlen(inputdir));
	strncpy(outpath, outputdir, strlen(outputdir));
    } else {
	int insize = strlen(inpath);
	int outsize = strlen(outpath);
	inpath[insize] = '/';
	outpath[outsize] = '/';
	strwrite(inpath, leadingpath, insize + 1, strlen(inpath) - insize);
	strwrite(outpath, leadingpath, outsize + 1, strlen(outpath) - outsize);
    }
    if (inpath[4095] != '\0' || outpath[4095] != '\0') {
	fprintf(stderr, "memory error\n");
    }
    struct dirent *de;
    DIR *dr = opendir(inpath);
    if (dr == NULL) {
	fprintf(stderr, "Could not open directory %s\n", inpath);
	return;
    }
    int err = cdir(outpath);
    if (err == -1) {
	fprintf(stderr,
		"Failed creating directory %s - try different -o option\n",
		outpath);
	return;
    }
    while ((de = readdir(dr)) != NULL) {
	if (strcmp(de->d_name, ".") == 0) {
	    /* do nothing */
	} else if (strcmp(de->d_name, "..") == 0) {
	    /* do nothing */
	} else if (de->d_type == DT_DIR) {
	    recursedir(de->d_name);
	} else {
	    char *filetype = strrchr(de->d_name, '.');
	    if (strcmp(filetype, ".md") == 0) {
		*filetype = '\0';
		addfilename(de->d_name);
		handlefiles(inpath, outpath);
		*strrchr(inpath, '/') = '\0';
		*strrchr(outpath, '/') = '\0';
	    }
	}
    }
    closedir(dr);
    return;
}
