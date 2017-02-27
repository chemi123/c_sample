#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
  int fds[2];
  pid_t pid;
  int status;

  if (pipe(fds) < 0) {
    perror("pipe()");
    exit(1);
  }

  if ((pid = fork()) < 0) {
    perror("fork()");
    exit(1);
  }

  if (pid == 0) {
    close(fds[0]);

    if (write(fds[1], "This is a message from child process", 40) < 0) {
      perror("write()");
      exit(1);
    }

    close(fds[1]);
  } else {
    char buf[1024];
    close(fds[1]);
    sleep(1);

    if (read(fds[0], buf, sizeof(buf)) < 0) {
      perror("read()");
      exit(1);
    }

    printf("%s\n", buf);
    close(fds[0]);
  }
  
  exit(0);
}
