#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s FILENAME", argv[0]);
    exit(1);
  }

  FILE *file_stream = fopen(argv[1], "r");
  if (!file_stream) {
    perror(argv[1]);
    exit(1);
  }

  char buf[1024];
  while (fgets(buf, sizeof buf, file_stream) != NULL) {
    fputs(buf, stdout);
  }

  fclose(file_stream);

  exit(0);
}
