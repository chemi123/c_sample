#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <errno.h>

int main() {
  int sock;

  sock = socket(3000, 4000, 5000);
  if (sock < 0) {
    perror("socket");
    printf("errno: %d\n", errno);
    exit(1);
  }
  exit(0);
}
