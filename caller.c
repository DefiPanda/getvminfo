
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "getpinfo.h" /* used by both kernel module and user program */

int fp;
char the_file[256] = "/sys/kernel/debug/";
char call_buf[MAX_CALL];  /* assumes no bufferline is longer */
char resp_buf[MAX_RESP];  /* assumes no bufferline is longer */

void do_syscall(char *call_string);

void main (int argc, char* argv[])
{
  int i;
  int rc = 0;
  pid_t my_pid;

  /* Open the file */

  strcat(the_file, dir_name);
  strcat(the_file, "/");
  strcat(the_file, file_name);

  if ((fp = open (the_file, O_RDWR)) == -1) {
      fprintf (stderr, "error opening %s\n", the_file);
      exit (-1);
  }

  if(argc != 2) {
     printf("Please enter exactly two arguments.\n");
     exit (-1);
  }

  do_syscall(argv[1]);

  //file descriptor 
  int fd;
  //status for file write
  int result;
  //open our file
  fd = open(FILEPATH, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
  //handling file open error
  if (fd == -1) {
    perror("Error opening file for writing");
    exit(EXIT_FAILURE);
  }
  //Stretch the file size to the size of the (mmapped) array of ints
  result = lseek(fd, MAX_RESP-1, SEEK_SET);
  //handling lseek() fail error
  if (result == -1) {
    close(fd);
    perror("Error calling lseek() to 'stretch' the file");
    exit(EXIT_FAILURE);
  }
  //write an empty string to the end of the file
  result = write(fd, "", 1);
  //handling file writing error
  if (result != 1) {
    close(fd);
    perror("Error writing last byte of the file");
    exit(EXIT_FAILURE);
  }
  //file mmaping
  if (mmap(0, MAX_RESP, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0) == MAP_FAILED) {
    close(fd);
    perror("Error mmapping the file");
    exit(EXIT_FAILURE);
  }
  //close the file
  close(fd);

  fprintf(stdout, "Module getpinfo returns %s", resp_buf);

  close (fp);
} /* end main() */

void do_syscall(char *call_string)
{
  int rc;

  strcpy(call_buf, call_string);

  rc = write(fp, call_buf, strlen(call_buf) + 1);
  if (rc == -1) {
     fprintf (stderr, "error writing %s\n", the_file);
     fflush(stderr);
     exit (-1);
  }

  rc = read(fp, resp_buf, sizeof(resp_buf));
  if (rc == -1) {
     fprintf (stderr, "error reading %s\n", the_file);
     fflush(stderr);
     exit (-1);
  }
}

