
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <getopt.h>

// thread argument structure
struct thread_arg {
	int ofd;
	char *str;
	char **lines_base;
	char **lines;
	int lines_cnt;
};

struct log_info {
	int linenum;	
	int occurs;
	struct timespec tp; 
};

struct timespec g_tp;

void* thread_func(void *p) 
{
	struct thread_arg *ta = (struct thread_arg*) p;

	const char *needle = ta->str;
	const char **haystack = (const char **) ta->lines;
	const char **base = (const char **) ta->lines_base;
	const int ofd = ta->ofd;
	const int N = ta->lines_cnt;

	struct log_info *li = (struct log_info*) malloc(N * sizeof(struct log_info));
	memset(li, 0, N * sizeof(struct log_info));

	// search in each line
	for (int i = 0; i < N; ++i) {
		const char *s = haystack[i];

		li[i].linenum = & haystack[i] - base + 1;	// store line number

		// find all occurences 
		while ((s = strstr(s, needle)) != NULL) {
			s++;				// move away from current occurence
			li[i].occurs ++;	// count occurences 

			// get search time 
			clock_gettime(CLOCK_PROCESS_CPUTIME_ID, & li[i].tp);

			//if (li[i].tp.tv_nsec == g_tp.tv_nsec && li[i].tp.tv_sec == g_tp.tv_sec)
			//	warnx("no time left, wtf?");

			li[i].tp.tv_nsec -= g_tp.tv_nsec;
			li[i].tp.tv_sec -= g_tp.tv_sec;
		}
	}
	
	// log everything to output file 
	// fully with stdio thread safe funtions
	FILE* of = fdopen(ofd, "a");
	if (of == NULL)
		err(EXIT_FAILURE, "fdopen() failed");
	
	flockfile(of);

	// for each line
	for (int i = 0; i < N; ++i) {
		if (li[i].occurs > 0)
			fprintf(of, "line # %d: %d occurences, time %ld us\n", 
				li[i].linenum, li[i].occurs, li[i].tp.tv_nsec / 1000); 
	}
	fflush(of);

	funlockfile(of);

	free(li);

	return NULL;
}

void usage() {
	fprintf(stderr, "Usage:\n\t%s -n <number_of_threads> ", program_invocation_short_name);
	fprintf(stderr, "-inputfile <path> ");
	fprintf(stderr, "-outputfile <path> -s <string_to_search>\n");
	exit(EXIT_SUCCESS);	
}

int main(int argc, char **argv)
{
	int n = 1;
	char inputfilename[BUFSIZ];
	char outputfilename[BUFSIZ];
	char searchstr[BUFSIZ];

	// TODO: add the following options?
	//	- "dryrun"
	//	- "dump"
	const struct option longopts[] = {
		{ "inputfile",	required_argument,	0,	0 },
		{ "outputfile",	required_argument,	0,	0 },
		{ 0,			0,					0,	0 }		
	};

	int longindex;
	int opt;

	// $ mysearch -n 10 -inputfile test.txt -outputfile result.txt -s needtext
	
	if (argc < 9)
		usage();
	
	opterr = 1;	
	
	// get command line options
	while ((opt = getopt_long_only(argc, argv, ":n:s:", longopts, & longindex)) != -1) {
		switch (opt)
		{
			case 0:		
				switch (longindex) 
				{
					case 0:
						strcpy(inputfilename, optarg);
						break;
					case 1:
						strcpy(outputfilename, optarg);
				} 
				break;

			case 'n': 
				n = strtol(optarg, NULL, 10);
				if (n == 0) 
					errx(EXIT_FAILURE, "invalid integer value %s", optarg);
				break;

			case 's':
				strcpy(searchstr, optarg);
				break;

			case '?':
				errx(EXIT_FAILURE, "unknown option -%c", optopt);
				break;
				
			case ':':
				errx(EXIT_FAILURE, "missing option argument -%c", optopt);
				break;
		}
	}

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, & g_tp);

	// open input file
	FILE* inputfile = fopen(inputfilename, "r");
	if (inputfile == NULL)
		err(EXIT_FAILURE, "unable to open '%s'", inputfilename);

	int lines_cap = 10;
	int lines_cnt = 0;
	char b[BUFSIZ];
	
	char **lines = (char**) malloc(lines_cap * sizeof(char*)); 

	// read all lines from input file
	for (int i = 0; fgets(b, BUFSIZ, inputfile) != NULL; ++i)  
	{
		lines_cnt ++;
		if (i == lines_cap) 
		{
			lines_cap *= 2;
			lines = (char**) realloc(lines, lines_cap * sizeof(char*));
		}
		lines[i] = (char*) malloc(strlen(b)+1 * sizeof(char)); 
		strcpy(lines[i], b);
	}

	// shrink to fit
	lines = (char**) realloc(lines, lines_cnt * sizeof(char*));	

	// divide lines among threads	
	int lines_per_thread = lines_cnt / n;	
	
	if (lines_per_thread < 1)
		// little cheating
		n = lines_cnt; 

	int xtra_lines = lines_cnt % n;

	// create output file 
	mode_t mode = S_IRWXU | S_IRGRP | S_IROTH;
	int ofd = creat(outputfilename, mode);
	if (ofd < 0)
		err(EXIT_FAILURE, "open(outputfile) failed");


	/*	create n threads and give a piece of data to each one
			to search string in that piece */
	
	pthread_t *tid = (pthread_t*) malloc(sizeof(pthread_t) * n);	

	struct thread_arg *ta = (struct thread_arg*) malloc(n * sizeof(struct thread_arg));

	for (int i = 0; i < n; ++i) 
	{
		ta[i].lines_base = lines;
		ta[i].ofd = ofd;
		ta[i].lines = & lines[i*lines_per_thread];
		ta[i].lines_cnt = lines_per_thread;
		ta[i].str = searchstr;

		if (i == n-1)
			ta[i].lines_cnt += xtra_lines;

		pthread_create(& tid[i], NULL, thread_func, & ta[i]);
	}


	// TODO: join with all threads
	for (int i = 0; i < n; ++i)
		pthread_join(tid[i], NULL);


	// free memory
	for (int i = 0; i < lines_cnt; ++i)
		free(lines[i]);

	free(lines);
	free(tid);
	free(ta);

	return 0;
}
