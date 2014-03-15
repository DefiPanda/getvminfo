
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

