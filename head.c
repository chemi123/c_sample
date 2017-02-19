#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#define _DEFAULT_NLINES 10

// 簡単なheadコマンド
// 学習用に作ったため敢えて使い方を限定している

static void do_head(const char *path, long nlines);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s n [file file...]\n", argv[0]);
    exit(1);
  }

  int has_option = 0;
  int opt;
  long nlines = _DEFAULT_NLINES;
  while ((opt = getopt(argc, argv, "n:")) != -1) {
    switch (opt) {
      case 'n':
        nlines = atol(optarg);
        has_option = 1;
        break;
      case '?':
      default:
        fprintf(stderr, "Not valid option\n");
        exit(1);
        break;
    }
  }

  int i;
  // もう面倒臭いからマジックナンバー使った
  if (!has_option) {
    i = 1;
  } else {
    i = 3;
  }

  for (; i < argc; i++) {
    do_head(argv[i], nlines);
  }

  exit(0);
}

static void
do_head(const char *path, long nlines) {
  FILE *f = fopen(path, "r");
  if (!f) {
    perror(path);
    exit(1);
  }
  int c;

  if (nlines <= 0) return;
  while ((c = fgetc(f)) != EOF) {
    if (putchar(c) < 0) exit(1);
    if (c == '\n') {
      if ((--nlines) == 0) {
        fclose(f);
        return;
      }
    }
  }
}
