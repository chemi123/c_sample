#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main() {
  int sock;

  // address family, socket type, protcol
  // AF_INET: IPv4
  // SOCK_STREAM: TCP
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr, "socket failed\n");
    exit(1);
  }

  printf("socket fd: %d\n", sock);
  exit(0);
}
