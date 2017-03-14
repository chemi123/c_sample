#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
  int server_sock_fd, client_sock_fd;
  int server_len, client_len;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;

  server_sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock_fd < 0) {
    perror("socket: ");
    exit(1);
  }
  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
  server_address.sin_port = 9734;

  server_len = sizeof(server_address);
  if (bind(server_sock_fd, (struct sockaddr *)&server_address, server_len) < 0) {
    perror("bind: ");
    exit(1);
  }

  if (listen(server_sock_fd, 5) < 0) {
    perror("listen: ");
    exit(1);
  }
  
  while(1) {
    //char ch;

    printf("server waiting\n");
    
    client_sock_fd = accept(server_sock_fd, (struct sockaddr *)&client_address, &client_len);

    //read(client_sock_fd, &ch, 1);
    //ch++;
    write(client_sock_fd, "abcde", 5);
    close(client_sock_fd);
  }

  close(server_sock_fd);

  exit(0);
}
