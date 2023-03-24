#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

void
main(int argc, char **argv)
{
  char *buf;
  struct dirent *p;
  int i, cnt, dir, nbytes;
  off_t base = 0;

  if (argc != 2)
    {
      fprintf(stderr, "Usage: test-dirent <path>\n");
      exit(1);
    }

  buf = calloc(1, 8192);
  if (!buf)
    {
      fprintf(stderr, "error: out of virtual memory\n");
      exit(1);
    }

  fprintf(stdout, "test-dirent: opening `%s'...\n", argv[1]);
  dir = open(argv[1], O_RDONLY);
  if (dir == -1)
    {
      fprintf(stderr, "error: cannot open directory `%s'\n", argv[1]);
      exit(1);
    }

  nbytes = getdirentries(dir, buf, 8192, &base);
  fprintf(stdout, "%d = getdirentries(%d, %p, %d, %d)\n",
	  nbytes, dir, buf, 8192, base);

  /* emit results */
  for (i=0, cnt=0, p=(struct dirent *)buf;
       cnt < nbytes && p->d_reclen > 0;
       i++, cnt += p->d_reclen, p=(struct dirent *)(buf+cnt))
    {
      fprintf(stdout, "rec #%2d:  d_ino: %7d,  d_reclen: %2d,  d_name: %s\n",
	      i, p->d_ino, p->d_reclen, p->d_name);
    }

  if (cnt != nbytes)
    fprintf(stderr, "warn: cnt != nbytes, cnt == %d, nbytes == %d",
	    cnt, nbytes);
}
