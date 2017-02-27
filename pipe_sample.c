#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
  int fd[2];
  pid_t pid;
  char buf[32];
  int status;
  if (pipe(fd) < 0) {
    perror("pipe()");
    exit(1);
  }

  if ((pid = fork()) < 0) {
    perror("pipe()");
    exit(1);
  }

  if (pid == 0) {
     // パイプから読み込む
    if(read(fd[0], buf, sizeof(buf)) < 0) {
      perror("read()");
      return -1;
    }
    printf("%s, child process received\n", buf);
 
    // パイプへ出力
    if(write(fd[1], "hello parent", 12) < 0) {
      perror("write()");
      return -1;
    }
 
    // パイプをクローズ
    close(fd[0]); // 入力
    close(fd[1]); // 出力 
  } else {
    // パイプへ出力
    if(write(fd[1], "hello child", 11) < 0) {
        perror("write()");
        return -1;
    }
 
    sleep(1); // スリープで実行順序を制御
 
    // パイプから読み込む
    if(read(fd[0], buf, sizeof(buf)) < 0) {
        perror("read()");
        return -1;
    }
    printf("%s, parent process received\n", buf);
 
    close(fd[0]);
    close(fd[1]);
 
    // 子プロセスの終了を待つ
    waitpid(pid, &status, 0);
    printf ("child process (PID = %d) finished\n", pid);
  }

  exit(0);
}
