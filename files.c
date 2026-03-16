#include <stdlib.h>
#include <stdio.h>
#include "files.h"
#include "metadata.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
/* static memory buffers used for buffers */
void strwrite(char *dest, char *src, int start, int maxsize);
static char inpath[4096];
static char outpath[4096];
void output_callback(const MD_CHAR *text, MD_SIZE size, void *userdata)
{
    FILE *out = (FILE *) userdata;
    fwrite(text, 1, size, out);
}

struct Filenode {
    char *url;
    metadata metad;
    time_t date;
    struct Filenode *next;
};
struct Filelist {
    struct Filenode *head;
    size_t count;
};
struct Filelist list;
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

int appendlist(char *url, metadata metad, time_t date)
{
    struct Filenode *node = malloc(sizeof(struct Filenode));
    node->next = list.head;
    node->url = url;
    node->metad = metad;
    node->date = date;
    list.head = node;
    list.count++;
    return 0;
}

void freelist()
{
    struct Filenode *start = list.head;
    struct Filenode *next = list.head;
    for (int i = 0; i < list.count; i++) {
	free(next->url);
	next = next->next;
	free(start);
	start = next;
    }
}

void printlist()
{
    struct Filenode *node = list.head;
    for (int i = 0; i < list.count; i++) {
	printf("url: %s title: %s date: %s\n", node->url, node->metad.title,
	       ctime(&node->date));
	node = node->next;
    }
    return;
}

struct Filenode *merge(struct Filenode *first, struct Filenode *second)
{
    if (first == NULL)
	return second;
    if (second == NULL)
	return first;
    if (first->date >= second->date) {
	first->next = merge(first->next, second);
	return first;
    } else {
	second->next = merge(first, second->next);
	return second;
    }
}

void splitList(struct Filenode *source, struct Filenode **front,
	       struct Filenode **back)
{
    struct Filenode *fast;
    struct Filenode *slow;
    if (source == NULL || source->next == NULL) {
	*front = source;
	*back = NULL;
    } else {
	slow = source;
	fast = source->next;

	while (fast != NULL) {
	    fast = fast->next;
	    if (fast != NULL) {
		slow = slow->next;
		fast = fast->next;
	    }
	}

	*front = source;
	*back = slow->next;
	slow->next = NULL;
    }
}

void mergesort(struct Filenode **headRef)
{
    struct Filenode *head = *headRef;
    struct Filenode *a;
    struct Filenode *b;
    if (head == NULL || head->next == NULL)
	return;
    splitList(head, &a, &b);
    mergesort(&a);
    mergesort(&b);
    *headRef = merge(a, b);
}

void genrss()
{
    mergesort(&list.head);
    int dirlen = strlen(outputdir);
    /* stupid way to write a string? yes. smart way to write a string? i think so */
    outpath[dirlen++] = '/';
    outpath[dirlen++] = 'r';
    outpath[dirlen++] = 's';
    outpath[dirlen++] = 's';
    outpath[dirlen++] = '.';
    outpath[dirlen++] = 'x';
    outpath[dirlen++] = 'm';
    outpath[dirlen++] = 'l';
    outpath[dirlen++] = '\0';
    FILE *fp = fopen(outpath, "w");
    if (fp == NULL) {
	printf("failed to open file %s\n", outpath);
	return;
    }
    if (rsstitle == NULL) {
	rsstitle = url + 6;
    }
    if (rssdesc == NULL) {
	rssdesc = "Placeholder Description";
    }
    fprintf(fp,
	    "<rss version=\"2.0\">\n <channel>\n\t<title>%s</title>\n\t<link>%s</link>\n\t<description>%s</description>",
	    rsstitle, url, rssdesc);
    struct Filenode *node = list.head;
    for (int i = 0; i < list.count; i++) {
	char *description;
	char *title;
	struct tm *timestruc;
	char timebuf[80];
	timestruc = gmtime(&node->date);
	strftime(timebuf, sizeof(timebuf), "%a, %d %b %Y %H:%M:%S GMT",
		 timestruc);
	if (node->metad.description[0] == '\0') {
	    description = "placeholder description";
	} else {
	    description = node->metad.description;
	}
	if (node->metad.title[0] == '\0') {
	    title = "placeholder title";
	} else {
	    title = node->metad.title;
	}
	fprintf(fp,
		"\n\t<item>\n\t\t<title>%s</title>\n\t\t<link>%s</link>\n\t\t<language>%s</language>\n\t\t<pubDate>%s</pubdate>\n\t\t<description>%s</description>\n\t</item>\n",
		title, node->url, node->metad.language, timebuf, description);
	node = node->next;
    }
    fprintf(fp, " </channel>\n</rss>\n");
    fclose(fp);
    printf("wrote RSS file %s\n", outpath);
    return;
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
    if (createrss) {
	size_t urlsize = strlen(url) + strlen(outputfile) - strlen(outputdir);
	char *urlpath = malloc(urlsize + 2);
	snprintf(urlpath, urlsize + 1, "%s%s", url,
		 outputfile + strlen(outputdir));
	time_t date;
	if (metad.date[0] != '\0') {
	    date = datetotime(metad.date);
	}
	if (metad.date[0] == '\0' || date == -1) {
	    /* if date isn't found in file metadata we use the input file modification date */
	    struct stat fst;
	    int fd = fileno(fp);
	    fstat(fd, &fst);
	    date = fst.st_mtime;
	}
	appendlist(urlpath, metad, date);
    }
    fclose(fp);
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

void addfilename(char *filename, int html)
{
    int inlen = strlen(inpath);
    int outlen = strlen(outpath);
    int filenlen = strlen(filename);
    inpath[inlen] = '/';
    outpath[outlen] = '/';
    strwrite(inpath, filename, inlen + 1, 4095 - inlen - 1);
    strwrite(outpath, filename, outlen + 1, 4095 - outlen - 1);
    if (html) {
	strncat(inpath, ".md", 3);
	strncat(outpath, ".html", 5);
    }
}

void copyfile()
{

    int sfd, dfd;
    char *src, *dest;
    size_t filesize;

    /* SOURCE */
    sfd = open(inpath, O_RDONLY);
    filesize = lseek(sfd, 0, SEEK_END);

    src = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, sfd, 0);

    /* DESTINATION */
    dfd = open(outpath, O_RDWR | O_CREAT, 0660);

    ftruncate(dfd, filesize);

    dest = mmap(NULL, filesize, PROT_READ | PROT_WRITE, MAP_SHARED, dfd, 0);

    /* COPY */
    memcpy(dest, src, filesize);
    munmap(src, filesize);
    munmap(dest, filesize);
    close(sfd);
    close(dfd);
    printf("copied %s to %s\n", inpath, outpath);
    return;
}

/* recursedir uses a static char buffer, the leadingpath variable is used for recursion purposes*/
void recursedir(char *leadingpath)
{

    if (leadingpath == NULL) {
	list.count = 0;
	strncpy(inpath, inputdir, strlen(inputdir));
	strncpy(outpath, outputdir, strlen(outputdir));
    } else {
	int insize = strlen(inpath);
	int outsize = strlen(outpath);
	inpath[insize] = '/';
	outpath[outsize] = '/';
	strwrite(inpath, leadingpath, insize + 1, 4095 - insize);
	strwrite(outpath, leadingpath, outsize + 1, 4095 - outsize);
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
		addfilename(de->d_name, 1);
		handlefiles(inpath, outpath);
		*strrchr(inpath, '/') = '\0';
		*strrchr(outpath, '/') = '\0';
	    } else {
		addfilename(de->d_name, 0);
		copyfile();
		*strrchr(inpath, '/') = '\0';
		*strrchr(outpath, '/') = '\0';
	    }
	}
    }
    if (leadingpath != NULL) {
	*strrchr(inpath, '/') = '\0';
	*strrchr(outpath, '/') = '\0';
    }
    closedir(dr);
    return;
}
