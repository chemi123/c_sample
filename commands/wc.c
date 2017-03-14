#include <stdio.h>
#include <stdlib.h>

static void do_wc(const char *path);

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s FILENAME", argv[0]);
    exit(1);
  }

  do_wc(argv[1]);

  exit(0);
}

static void
do_wc(const char *path) {
  FILE *file_stream = fopen(path, "r");
  if (!file_stream) {
    perror(path);
    exit(1);
  }
  
  int c;
  int cl = 0;
  while ((c = fgetc(file_stream)) != EOF) {
    if (c == '\n') cl++;
  }

  printf("%d\n", cl);
}
