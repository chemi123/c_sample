#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <regex.h>

static void do_grep(regex_t *pat, FILE *f, int ignore_case);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs("no pattern\n", stderr);
    exit(1);
  }

  int has_option = 0;
  int reg_icase = 0;
  int ignore_case = 0;
  int opt;
  while ((opt = getopt(argc, argv, "iv")) != -1) {
    switch (opt) {
      case 'i':
        reg_icase = 1;
        has_option = 1;
        break;
      case 'v':
        has_option = 1;
        ignore_case = 1;
        break;
      case '?':
      default:
        fprintf(stderr, "Not valid option\n");
        exit(1);
        break;
    }
  }

  regex_t pat;
  int i;
  int err;
  if (reg_icase) {
    err = regcomp(&pat, argv[2], REG_EXTENDED | REG_NOSUB | REG_NEWLINE | REG_ICASE);
  } else {
    if (has_option) {
      err = regcomp(&pat, argv[2], REG_EXTENDED | REG_NOSUB | REG_NEWLINE);
    } else {
      err = regcomp(&pat, argv[1], REG_EXTENDED | REG_NOSUB | REG_NEWLINE);
    }
  }

  if (err != 0) {
    char buf[1024];
    regerror(err, &pat, buf, sizeof buf);
    puts(buf);
    exit(1);
  }

  int is_pipe = 0;
  if (has_option) {
    if (argc > 3) {
      is_pipe = 1;
    }
  } else {
    if (argc > 2) {
      is_pipe = 1;
    }
  }

  if (!is_pipe) {
    do_grep(&pat, stdin, ignore_case);
  } else {
    int i;
    int file_pos;
    if (has_option) {
      file_pos = 3;
    } else {
      file_pos = 2;
    }

    for (i = file_pos; i < argc; i++) {
      FILE *src = fopen(argv[i], "r");

      if (!src){
        perror(argv[i]);
	exit(1);
      }

      do_grep(&pat, src, ignore_case);
      fclose(src);
    }
  }

  regfree(&pat); 
  return 0;
}

// あんまり書き方が綺麗じゃない
static void
do_grep(regex_t *pat, FILE *src, int ignore_case) {
  char buf[4096];
  while (fgets(buf, sizeof buf, src) != NULL) {
    if (ignore_case) {
      if (regexec(pat, buf, 0, NULL, 0)) {
        fputs(buf, stdout);
      }
    } else {
      if (regexec(pat, buf, 0, NULL, 0) == 0) {
        fputs(buf, stdout);
      }
    }
  }
}
