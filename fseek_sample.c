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

  // fdの確認
  //printf("%d\n", fileno(file_stream));

  char buf[1024];
  while (fgets(buf, sizeof buf, file_stream) != NULL) {
    fputs(buf, stdout);
    // ここで無限ループ発生の原因となる
    //fseek(file_stream, 50, SEEK_SET);
  }

  fclose(file_stream);

  exit(0);
}
