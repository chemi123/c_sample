#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

// 子プロセスから親プロセスに出力する

int main() {
  int fds[2];
  pid_t pid;
  int status;

  // fd[0]: 読み込み専用
  // fd[1]: 書きみ込み専用
  if (pipe(fds) < 0) {
    perror("pipe()");
    exit(1);
  }

  if ((pid = fork()) < 0) {
    perror("fork()");
    exit(1);
  }

  if (pid == 0) {
    // 子プロセスの読み込み側は不要なので閉じる
    close(fds[0]);

    // (i) ここで書き込み側のfdにメッセージを書き込む->(ii)で読み込む
    if (write(fds[1], "This is a message from child process", 40) < 0) {
      perror("write()");
      exit(1);
    }

    close(fds[1]);
  } else {
    char buf[1024];
    // 親プロセスの書き込み側は不要なので閉じる
    close(fds[1]);

    // (ii) ここで(i)で書き込まれたメッセージを読み込み側のfdから読む
    if (read(fds[0], buf, sizeof(buf)) < 0) {
      perror("read()");
      exit(1);
    }

    printf("%s\n", buf);
    close(fds[0]);
  }
  
  exit(0);
}
