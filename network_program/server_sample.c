#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
  int server_socket_fd, client_socket_fd;
  int server_len, client_len;
  struct sockaddr_in server_address;
  struct sockaddr_in client_address;

  // socketの作成
  server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_socket_fd < 0) {
    perror("socket: ");
    exit(1);
  }

  server_address.sin_family= AF_INET;
  //server_address.sin_addr.s_addr = inet_addr("127.0.0.1"); // ループバックアドレス 
  server_address.sin_addr.s_addr = INADDR_ANY; // このサーバが持つIPなんでも
  server_address.sin_port = 9734;
  server_len = sizeof(server_address);
  if (bind(server_socket_fd, (struct sockaddr *)&server_address, server_len) < 0) {
    perror("bind: ");
    exit(1);
  }

  if (listen(server_socket_fd, 5)) {
    perror("listen: ");
    exit(1);
  }

  printf("server waiting\n");
  while (1) {

    client_socket_fd = accept(server_socket_fd, (struct sockaddr *)&server_address, &client_len);
    if (client_socket_fd < 0) {
      perror("accept: ");
      break;
    }

    // アクセス元のログ表示
    printf("accepted connection from %s, port=%d\n",
              inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    if (write(client_socket_fd, "abcde", 5) < 0) {
      perror("write: ");
      break;
    }

    close(client_socket_fd);
  }

  close(server_socket_fd);
}
