#include <stdio.h>

long long x = 0x100000000LL;
long long y = 0x1ffffffffLL;
long long z = 0x010000000LL;
long long w = 0x01fffffffLL;

void
main(void)
{
  fprintf(stdout, "x+1 = 0x%016Lx\n", x+1);
  fprintf(stdout, "x-1 = 0x%016Lx\n", x-1);
  fprintf(stdout, "y+1 = 0x%016Lx\n", y+1);
  fprintf(stdout, "x+y = 0x%016Lx\n", x+y);
  fprintf(stdout, "z*w = 0x%016Lx\n", z*w);
  exit(0);
}
 
