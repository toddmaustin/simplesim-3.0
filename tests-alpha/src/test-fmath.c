#include <stdio.h>
#include <stdlib.h>
#include <math.h>

float foo1=5.3, foo2=6.4;
double bar1=5.3, bar2=6.4;
int q = 4;
float x = 12;
double y = 29;
double z;
long long l = 6;
long long *lp = &l;

int
main(void)
{
  printf("q=%d (int)x=%d (int)y=%d\n", q, (int)x, (int)y);
  z = pow(x, 2);
  printf("z=%d\n", (int)z);
  z = pow(y, 2);
  printf("z=%d\n", (int)z);
  z = 13.21;
  printf("z=%d\n", (int)z);
  z = (double)13;
  printf("z=%d\n", (int)z);
  printf("l=%d\n", (int)l);
  l = l * 6;
  printf("l=%d\n", (int)l);
  *lp = *lp * 6;
  printf("*lp=%d\n", (int)*lp);
  z = pow(x, 2);
  printf("z=%f\n", z);

  printf("q=%d x=%f (int)x=%d y=%f (int)y=%d\n", q, x, (int)x, y, (int)y);

  x = foo1+foo2;
  y = 2*bar1+2*bar2;

  q = q*4;

  printf("q = %d  x = %f  y = %f\n", q, x, y);

  return 0;
}
