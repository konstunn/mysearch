
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define handle_error(msg) \
	   do { perror(msg); exit(EXIT_FAILURE); } while (0)

// $ mysearch -n 10 -inputfile test.txt -outputfile result.txt -s needtext

// search_time variable

// define thread argument structure

int main(int argc, char **argv)
{
	int n = 1;
	// char *s, *inputfile, *outputfile;
	
	// TODO: parse command line options, claim if error
	
	if (argc < 3 || argc > 4) {
		fprintf(stderr, "%s file offset [length]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// open file
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1)
		handle_error("open");

	/* Get file status to obtain file size */
	struct stat sb;
	if (fstat(fd, &sb) == -1)	
	   handle_error("fstat");

    off_t offset = atoi(argv[2]);
	/* offset for mmap() must be page aligned */
	off_t pa_offset = offset & ~(sysconf(_SC_PAGE_SIZE) - 1);
					  
	if (offset >= sb.st_size) {
		fprintf(stderr, "offset is past end of file\n");
		exit(EXIT_FAILURE);
	}

	size_t length;

	// TODO: this relates to parsing command line options
	// - move this up
	if (argc == 4) {
		// TODO: replace atoi() with strtol() to detect errors
		length = atoi(argv[3]); 
		if (offset + length > sb.st_size)
			/* Can't display bytes past end of file */
			length = sb.st_size - offset; 
	} else		
		/* No length arg ==> display to end of file */
		length = sb.st_size - offset;

	// map inputfile into memory 
	char *addr = (char*) mmap(NULL, length + offset - pa_offset, PROT_READ,
			MAP_PRIVATE, fd, pa_offset);

	if (addr == MAP_FAILED)
		handle_error("mmap");

	// print mapped file to stdout
	ssize_t sz = write(STDOUT_FILENO, addr + offset - pa_offset, length);

	if (sz != length) { 
		if (sz == -1)
			handle_error("write");
		fprintf(stderr, "partial write");
	}

	struct line {
		char *addr;
		int num;
		struct line *next;
	} **headline;

	headline = (struct line**) malloc(sizeof(struct line*));

	// TODO: count lines?
	struct line * pline = *headline;
	char *pt = addr;
	for (int i = 0, lines = 1; i < length; ++i) {
		if (addr[i] == '\n') {

			pline->addr = pt;	

			pline->num = lines++;

		}
	}

	//		- create n threads and give a piece of data to each one
	//			to search string in that piece

	return 0;
}
