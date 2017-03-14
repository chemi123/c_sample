#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>

static void
exit_process() {
  fprintf(stderr, "Exit normally.\n");
  exit(0);
}

static void
wait_process(int second) {
  int cnt = 0;
  char mark[5] = {'|', '/', '-', '\\'};

  for (cnt = 0; cnt < second; cnt++) {
    fprintf(stdout, "%c\r", mark[(cnt+1)%4]);
    fflush(stdout);
    sleep(1);
  }
}

static int
sample_func(void) {
  int rc = 0;
  struct sigaction act;

  memset(&act, 0, sizeof(act));
  act.sa_handler = exit_process;
  act.sa_flags = SA_RESETHAND;

  rc = sigaction(SIGINT, &act, NULL);
  if (rc < 0) {
    fprintf(stderr, "Error: sigaction() %s\n", strerror(errno));
    return -1;
  }

  wait_process(30);

  return 0;
}

int main(int argc, char *argv[]) {
  int rc = 0;

  if (argc != 1) {
    fprintf(stderr, "Usage: %s\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  sample_func();

  exit(0);
}
