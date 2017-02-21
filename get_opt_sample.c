#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fputs("no pattern\n", stderr);
    exit(1);
  }

  int opt;
  while ((opt = getopt(argc, argv, "abcde:f:")) != -1) {
    switch (opt) {
      case 'a':
        printf("optind = %d\n", optind);
        break;
      case 'b':
        printf("optind = %d\n", optind);
        break;
      case 'c':
        printf("optind = %d\n", optind);
        break;
      case 'd':
        printf("optind = %d\n", optind);
        break;
      case 'e':
        printf("optind = %d\n", optind);
        printf("optarg = %s\n", optarg);
        break;
      case 'f':
        printf("optind = %d\n", optind);
        printf("optarg = %s\n", optarg);
        break;
      case '?':
      default:
        fprintf(stderr, "Not valid option\n");
        exit(1);
        break;
    }
  }

  printf("optind = %d\n", optind);
  printf("%s\n", argv[0]);
  argv++;
  printf("%s\n", argv[0]);

  return 0;
}
