#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define INIT_BUFSIZE 1024

char *
my_getcwd(void) {
  char *buf, *tmp;
  size_t size = INIT_BUFSIZE;

  int count = 1;
  buf = malloc(size);
  for (;;) {
    errno = 0;
    if (getcwd(buf, size)) {
      //printf("could allocate buf size at %d times\n", count);
      return buf;
    }
    if (errno != ERANGE) {
      break;
    }
    count++;
    size *= 2;
    tmp = realloc(buf, size);
    if (!tmp) {
      break;
    }
    buf = tmp;
  }
  free(buf);
  return NULL;
}

int main(int argc, char *argv[]) {
  char *buf = my_getcwd();
  printf("%s\n", buf);
  free(buf);
  exit(0);
}
