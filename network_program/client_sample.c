#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
  int socket_fd;
  int len;
  struct sockaddr_in address;

  socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_fd < 0) {
    perror("socket: ");
    exit(1);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = 9734;
  len = sizeof(address);
  if (connect(socket_fd, (struct sockaddr *)&address, len) < 0) {
    perror("socket: ");
    exit(1);
  }

  char buf[5];
  if (read(socket_fd, buf, 5) < 0) {
    perror("read: ");
    exit(1);
  }
  printf("char from server = %s\n",buf);
  close(socket_fd);

  exit(0);
}
