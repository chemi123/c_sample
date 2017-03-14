#include <stdio.h>
#include <stdlib.h>

int main() {
  char *figure1;
  char **figure2;

  char buf1[50] = "abcde";
  char buf2[50] = "fghij";

  char *str = "hoge";

  figure1 = buf1;
  figure2 = &figure1;

  printf("figure1 = %p\n", figure1);
  printf("figure2 = %p\n", figure2);
  printf("buf1 = %p\n", buf1);
  printf("buf2 = %p\n", buf2);
  printf("*figure2 = %p <- figure1のアドレスを保持\n", *figure2);
  printf("**figure2 = %c <- figure1の保持するアドレスが持つ変数\n", **figure2);

  *figure2 = buf2;
  printf("figure1 = %s\n\n", figure1);

  printf("str = %s\n", str);
  printf("*str = %p\n", *str);
  printf("*(str+1) = %p\n", *(str+1));
  printf("*(str+2) = %p\n", *(str+2));
  printf("*(str+3) = %p\n", *(str+3));

  exit(0);
}
