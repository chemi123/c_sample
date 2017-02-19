#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  int i;

  if (argc < 2) {
    fprintf(stderr, "%s: file name not given\n", argv[0]);
    exit(1);
  }

  for (i = 1; i < argc; i++) {
    FILE *file_stream;
    file_stream = fopen(argv[i], "r");
    if (!file_stream) {
      perror(argv[i]);
      exit(1);
    }

    int c;
    while ((c = fgetc(file_stream)) != EOF) {
      //if (fputc(c, stdout) < 0) {
      if (putchar(c) < 0) {
        exit(1);
      }
    }

    fclose(file_stream);
  }

  exit(0);
}
