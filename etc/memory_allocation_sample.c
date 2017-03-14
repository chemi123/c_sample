#include <stdio.h>
#include <stdlib.h>

#define ALOCATION_SIZE 1024

/*
reallocの挙動を確認するためのプログラム。
reallocは一度確保した領域を指定したサイズで確保し直す関数。
元々確保していた領域の先頭のメモリから指定したサイズ分確保できたらそのまま確保し、
できなかったら他のメモリ領域を探してきて確保する。
*/

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "put one argument\n");
    exit(1);
  }

  int allocation_times = atoi(argv[1]);
  char *buf1, *buf2, *buf3;
  buf1 = (char *)malloc(ALOCATION_SIZE);
  if (!buf1) {
    fprintf(stderr, "could not allocate memory for buf1\n");
    exit(1);
  }

  buf2 = (char *)realloc(buf1, ALOCATION_SIZE*allocation_times);
  if (!buf2) {
    fprintf(stderr, "could not allocate memory for buf2\n");
    exit(1);
  }

  printf("%p\n", buf1);
  printf("%p\n", buf2);

  if (buf1 == buf2) {
    printf("could allocate same place of memory\n");
    free(buf1);
  } else {
    printf("allocated different place of memory\n");
    //free(buf1); reallocで違う領域を確保した場合は元々確保されていた領域はメモリが解放されるみたいなのでダブルフリーとなって落ちる
    free(buf2);
  }

  exit(0);
}
