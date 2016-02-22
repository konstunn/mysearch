
#include <stdio.h>
#include <stdlib.h>
#include <err.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

// $ mysearch -n 10 -inputfile test.txt -outputfile result.txt -s needtext

// search_time variable

// define thread argument structure
struct thread_arg
{
	char **ofmap;
	char **lines_base;
	char **lines;
	int lines_cnt;
};

void* thread_func(void *p)
{
	struct thread_arg *ta = (struct thread_arg*) ta;
	const int N = ta->lines_cnt;

	for (int i = 0; i < N; ++i) {

	}
	return NULL;
}

int main(int argc, char **argv)
{
	int n = 1;
	char *s, *inputfilename, *outputfilename;
	
	// TODO: parse command line options, claim on error

	// open input file
	FILE* inputfile = fopen(inputfilename, "r");
	if (inputfile == NULL)
		err(EXIT_FAILURE, "fopen(inputfile) failed");

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

	// open output file 
	mode_t mode = S_IRWXU | S_IRGRP | S_IROTH;
	int ofd = creat(outputfilename, mode);
	if (ofd < 0)
		err(EXIT_FAILURE, "open(outputfile) failed");

	// get output file size (it should be zero)
	struct stat st;
	if (fstat(ofd, & st) < 0)
		err(EXIT_FAILURE, "fstat() failed");

	int file_len = st.st_size;

	if (file_len == 0) {
		// extend output file 
		file_len = 4096; // 4 kilobytes
		if (ftruncate(ofd, file_len) < 0) 
			err(EXIT_FAILURE, "ftruncate() failed");
	}

	// map output file into memory
	char **addr = (char**) malloc(sizeof(char*));
	*addr = (char*) mmap(NULL, file_len, PROT_READ | PROT_WRITE, MAP_SHARED, ofd, 0); 	
	if (*addr == MAP_FAILED)
		err(EXIT_FAILURE, "mmap() failed");
	
	// TODO: create mutex
	
	/*	pthread_create
		create n threads and give a piece of data to each one
			to search string in that piece */
	
	pthread_t *tid = (pthread_t*) malloc(sizeof(pthread_t) * n);	

	struct thread_arg *ta = (struct thread_arg*) malloc(n * sizeof(struct thread_arg));

	for (int i = 0; i < n; ++i) 
	{
		ta[i].lines_base = lines;
		ta[i].ofmap = addr;
		ta[i].lines = & lines[i*lines_per_thread];
		ta[i].lines_cnt = lines_per_thread;

		if (i == n-1)
			ta[i].lines_cnt += xtra_lines;

		pthread_create(& tid[i], NULL, thread_func, & ta[i]);
	}

	// TODO: join with all threads
	for (int i = 0; i < n; ++i)
	{
		pthread_join(tid[i], NULL);
	}

	// TODO: use strlen(), mremap() and ftruncate() 
	//		to shrink the file to fit the data

	// TODO: msync(), munmap() ?

	// free memory
	for (int i = 0; i < lines_cap; ++i)
		free(lines[i]);

	free(lines);
	free(tid);
	free(addr);
	free(ta);

	return 0;
}
