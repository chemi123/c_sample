#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
  int n;          /* nはint型変数 */
  int *ip;        /* ipはint型のポインタ変数 */
  double x;       /* xはdouble型変数 */
  double *dp;     /* dpはdouble型のポインタ変数 */
 
  /* それぞれの変数のメモリサイズを表示 */
  printf("size of int       : %lu\n", sizeof(int));
  printf("size of (int *)   : %lu\n", sizeof(int *));
  printf("size of double    : %lu\n", sizeof(double));
  printf("size of (double *): %lu\n", sizeof(double *));
  printf("size of (char *): %lu\n", sizeof(char *));
 
  return 0;
  exit(0);
}
