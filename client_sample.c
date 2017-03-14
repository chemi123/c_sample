#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
  int sock_fd;
  int len;
  struct sockaddr_in address;
  int result;
  //char ch = 'A';

  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    perror("socket: ");
    exit(1);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr("127.0.0.1");
  address.sin_port = 9734;
  len = sizeof(address);

  result = connect(sock_fd, (struct sockaddr *)&address, len);

  if (result < 0) {
    perror("connect: ");
    exit(1);
  }

  //write(sock_fd, &ch, 1);
  char buf[5];
  read(sock_fd, buf, 5);
  //read(sock_fd, &ch, 1);
  printf("char from server = %s\n", buf);
  close(sock_fd);

  exit(0);
}
