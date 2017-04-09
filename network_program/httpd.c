#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <getopt.h>

#define SERVER_NAME "LittleHTTP"
#define SERVER_VERSION "1.0"
#define LINE_BUF_SIZE 4096
#define MAX_REQUEST_BODY_LENGTH (1024 * 1024)
#define BLOCK_BUF_SIZE 1024
#define TIME_BUF_SIZE 64
#define HTTP_MINOR_VERSION 0
#define MAX_BACKLOG 5

struct HTTPHeaderField {
  char *name;
  char *value;
  struct HTTPHeaderField *next;
};

struct HTTPRequest {
  int protocol_minor_version;
  char *method;
  char *path;
  struct HTTPHeaderField *header;
  char *body;
  long length;
};

struct FileInfo {
  char *path;
  long size;
  int ok;
};

static void setup_environment(char *root, char *user, char *group);
typedef void (*sighandler_t)(int); // int型の引数をとり、返り値を持たない関数へのポインタsighandler_t
static void install_signal_handler(void);
static void trap_signal(int sig, sighandler_t handler);
static void signal_exit(int sig);
static void wait_child(int sig);
static void become_daemon(void);
static int listen_socket(char *port);
static void server_main(int server, char *docroot);
static void service(FILE *in, FILE *out, char *docroot);
static struct HTTPRequest* read_request(FILE *in);
static void read_request_line(struct HTTPRequest *req, FILE *in);
static struct HTTPHeaderField* read_header_field(FILE *in);
static void upcase(char *str);
static void free_request(struct HTTPRequest *req);
static long content_length(struct HTTPRequest *req);
static char* lookup_header_field_value(struct HTTPRequest *req, char *name);
static void respond_to(struct HTTPRequest *req, FILE *out, char *docroot);
static void do_file_response(struct HTTPRequest *req, FILE *out, char *docroot);
static void method_not_allowed(struct HTTPRequest *req, FILE *out);
static void not_implemented(struct HTTPRequest *req, FILE *out);
static void not_found(struct HTTPRequest *req, FILE *out);
static void output_common_header_fields(struct HTTPRequest *req, FILE *out, char *status);
static struct FileInfo* get_fileinfo(char *docroot, char *urlpath);
static char* build_fspath(char *docroot, char *urlpath);
static void free_fileinfo(struct FileInfo *info);
static char* guess_content_type(struct FileInfo *info);
static void* xmalloc(size_t size);
static void log_exit(char *fmt, ...); // エラー時にログを出力してexit(1)

#define USAGE "Usage: %s [--port=n] [--chroot --user=u --group=g] [--debug] <docroot>\n"

static int debug_mode = 0;

static struct option longopts[] = {
  {"debug", no_argument, &debug_mode, 1},
  {"chroot", no_argument, NULL, 'c'},
  {"user", required_argument, NULL, 'u'},
  {"group", required_argument, NULL, 'g'},
  {"port", required_argument, NULL, 'p'},
  {"help", required_argument, NULL, 'h'},
  {0, 0, 0, 0}
};

int main(int argc, char *argv[]) {
  int server;
  char *port = NULL;
  char *docroot;
  int do_chroot = 0;
  char *user = NULL;
  char *group = NULL;
  int opt;

  while ((opt = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
    switch (opt) {
      case 0:
        break;
      case 'c':
        do_chroot = 1;
        break;
      case 'u':
        user = optarg;
        break;
      case 'g':
        group = optarg;
        break;
      case 'p':
        port = optarg;
        break;
      case 'h':
        fprintf(stdout, USAGE, argv[0]);
        exit(0);
        break;
      case '?':
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
        break;
    }
  }

  if (optind != argc - 1) {
    fprintf(stderr, USAGE, argv[0]);
    exit(1);
  }

  docroot = argv[optind];
  if (do_chroot) {
    setup_environment(docroot, user, group);
    docroot = "";
  }

  install_signal_handler();
  server = listen_socket(port);
  if (!debug_mode) {
    openlog(SERVER_NAME, LOG_PID|LOG_NDELAY, LOG_DAEMON);
    become_daemon();
  }

  server_main(server, docroot);
  exit(0);
}

static void
setup_environment(char *root, char *user, char *group)
{
  struct passwd *pw;
  struct group *gr;
  
  if (!user || !group) {
    fprintf(stderr, "user both of --user and --group\n");
    exit(1);
  }

  gr = getgrnam(group);
  if (!gr) {
    fprintf(stderr, "no such group: %s", group);
    exit(1);
  }

  if (setgid(gr->gr_gid) < 0) {
    perror("setgid(2)");
    exit(1);
  }

  if (initgroups(user, gr->gr_gid) < 0) {
    perror("initgroups(2)");
    exit(1);
  }

  pw = getpwnam(user);
  if (!pw) {
    fprintf(stderr, "no such user: %s", user);
  }

  chroot(root);
  if (setuid(pw->pw_uid) < 0) {
    perror("setuid(2)");
    exit(1);
  }
}

static void
install_signal_handler(void)
{
  trap_signal(SIGPIPE, signal_exit); // ソケット接続が何らかの原因で突然切断された場合に送られるシグナル、ひとまずexitしておく
  trap_signal(SIGCHLD, wait_child);  // 子プロセスがゾンビプロセスになった場合に送られてくるシグナル
}

static void
trap_signal(int sig, sighandler_t handler)
{
  struct sigaction act;

  act.sa_handler = handler;
  /*
  sigemptyset: シグナルマスクを全て除外に設定する
  シグナルマスク: 送信されても無視されるシグナルのこと
  つまりsigemptysetを使った後は全てのシグナルを除外することとなる
  */
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;

  // sigで指定されたシグナルに対してactのhandlerに格納されたハンドラを実行するよう設定
  if (sigaction(sig, &act, NULL) < 0) {
    log_exit("exit by signal %d", sig);
  }
}

static void
signal_exit(int sig)
{
  log_exit("exit by signal %d", sig);
}

static void
wait_child(int sig)
{
  wait(NULL);
}

// デーモンになる作業
// ここでの作業は実はdaemon(3)というAPIに置き換えられるが勉強がてら書いてみる
static void
become_daemon(void)
{
  int n;

  if (chdir("/") < 0) {
    log_exit("chdir(2) failed: %s", strerror(errno));
  }

  // 標準入出力、標準エラー出力を/dev/nullと繋げる
  // プログラムがうっかり標準入出力や標準エラー出力を使用した際に出力しないようにするため
  // ログに落とせばいい
  freopen("/dev/null", "r", stdin);
  freopen("/dev/null", "w", stdout);
  freopen("/dev/null", "w", stderr);

  n = fork();
  if (n < 0) {
    log_exit("fork(2) failed: %s", strerror(errno));
  }

  if (n != 0) {
    _exit(0);  // 親プロセスは終了する
  }

  // 新しいセッションを作成
  // ttyやptsを切り離してセッションリーダー及びグループリーダー化する
  if (setsid() < 0) {
    log_exit("setsid(2) failed: %s", strerror(errno));
  }
}

//struct addrinfo {
//  int              ai_flags;
//  int              ai_family;
//  int              ai_socktype;
//  int              ai_protocol;
//  socklen_t        ai_addrlen;
//  struct sockaddr *ai_addr;
//  char            *ai_canonname;
//  struct addrinfo *ai_next;
//};
//
// getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
// 上記の関数はホスト名・サービス名からIPアドレス、ポート番号を得る時に使う
// 接続対象nodeのアドレスの候補をresに書き込む
// resにはstruct addrinfoのリンクリストが書き込まれる。
// getaddrinfoで作られたaddrinfoのリンクリストはmallocでメモリが割り当てられているためfreeadrinfoで明示的に解放しなければならない
static int listen_socket(char *port)
{
  struct addrinfo hints, *res, *ai;
  int err;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET; // IPv4
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((err = getaddrinfo(NULL, port, &hints, &res)) != 0) {
    log_exit(gai_strerror(err));
  }

  for (ai = res; ai; ai = ai->ai_next) {
    int server_sock_fd;
    
    server_sock_fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (server_sock_fd < 0) {
      continue;
    }

    if (bind(server_sock_fd, ai->ai_addr, ai->ai_addrlen) < 0) {
      close(server_sock_fd);
      continue;
    }

    if (listen(server_sock_fd, MAX_BACKLOG) < 0) {
      close(server_sock_fd);
      continue;
    }

    freeaddrinfo(res);
    return server_sock_fd;
  }

  log_exit("failed to listen socket");
  return -1; /* not reach, but need to put this because this function is supposed to return value int */
}

//struct sockaddr_storage {
//  __uint8_t   ss_len;     /* address length */
//  sa_family_t ss_family;  /* [XSI] address family */
//  char            __ss_pad1[_SS_PAD1SIZE];
//  __int64_t   __ss_align; /* force structure storage alignment */
//  char            __ss_pad2[_SS_PAD2SIZE];
//};
static void
server_main(int server, char *docroot)
{
  for (;;) {
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof addr;
    int sock;
    int pid;

    sock = accept(server, (struct sockaddr*)&addr, &addrlen);
    if (sock < 0) {
      log_exit("accept(2) failed: %s");
    }

    pid = fork();
    if (pid < 0) {
      exit(3);
    }
    
    if (pid == 0) {
      FILE *inf = fdopen(sock, "r");
      FILE *outf = fdopen(sock, "w");

      service(inf, outf, docroot);
      exit(0);
    }
    close(sock);
  }
}

static void
service(FILE *in, FILE *out, char *docroot)
{
  struct HTTPRequest *req;

  req = read_request(in);
  respond_to(req, out, docroot);
  free_request(req);
}

static struct HTTPRequest*
read_request(FILE *in)
{
  struct HTTPRequest *req;
  struct HTTPHeaderField *h;

  req = xmalloc(sizeof(struct HTTPRequest));
  read_request_line(req, in);
  req->header = NULL;
  while ((h = read_header_field(in))) {
    h->next = req->header;
    req->header = h;
  }
  req->length = content_length(req);
  if (req->length != 0) {
    if (req->length > MAX_REQUEST_BODY_LENGTH) {
      log_exit("request body is too long");
    }
      
    req->body = xmalloc(req->length);
    if (fread(req->body, req->length, 1, in) < 1) {
      log_exit("failed to read request body");
    } else {
      req->body = NULL;
    }
  }

  return req;
}

static void
read_request_line(struct HTTPRequest *req, FILE *in)
{
  char buf[LINE_BUF_SIZE];
  char *path, *p;

  if (!fgets(buf, LINE_BUF_SIZE, in)) {
    log_exit("no request line");
  }
  /*
   リクエスト例(in):
   GET /index.html HTTP/1.0
      ^
      |
      p
   pはまずstrchrで' 'を指定しているのでここを指す
  */
  p = strchr(buf,  ' ');
  if (!p) {
    log_exit("parse error on request line (1): %s", buf);
  }

  // 空白を'\0'で埋めてから先に一つ進む
  *p++ = '\0';

  // 例のリクエストの場合だとGETの分だけメモリ確保してbufの内容をコピー(GETが入る)
  req->method = xmalloc(p - buf);
  strcpy(req->method, buf);
  upcase(req->method);

  /*
    /index.html HTTP/1.0
    ^          ^
    |          |
    path       p
  */
  path = p;
  p = strchr(path, ' ');
  if (!p) log_exit("parse error on request line (2): %s", buf);
  *p++ = '\0';
  req->path = xmalloc(p - path);
  strcpy(req->path, path);

  if (strncasecmp(p, "HTTP/1.", strlen("HTTP/1.")) != 0) {
    log_exit("parse error on request line (3): %s", buf);
  }

  p += strlen("HTTP/1.");
  req->protocol_minor_version = atoi(p);
}

//  名前から分かるがhttpリクエストのヘッダ部分を1行読み込む関数
//  以下ヘッダ例
//
//  "Connection: close\r\n"
//  "Accept: */*\r\n"
//  "Host: hogehoge.com\r\n"
//  "\r\n" <- 改行のみの行で終了を表す
//
static struct HTTPHeaderField*
read_header_field(FILE *in)
{
  struct HTTPHeaderField *h;
  char buf[LINE_BUF_SIZE];
  char *p;

  //  リクエストヘッダ(1行を読み込む)
  if (!fgets(buf, LINE_BUF_SIZE, in)) {
    log_exit("failed to read request header field: %s", strerror(errno));
  }

  // 改行のみの行であればヘッダが終わりとみなしてNULLをreturn
  // "\r\n"がhttpの仕様であるが、"\n"にも対応
  if ((buf[0] == '\n') || (strcmp(buf, "\r\n") == 0)) {
    return NULL;
  }

  p = strchr(buf, ':');
  if (!p) log_exit("parse error on request header field: %s", buf);
  *p++ = '\0';
  h = xmalloc(sizeof(struct HTTPHeaderField));
  h->name = xmalloc(p - buf);
  //strncpy(h->name, buf, sizeof(buf));
  strcpy(h->name, buf);

  p += strspn(p, " \t");
  h->value = xmalloc(strlen(p) + 1);
  strcpy(h->value, p);

  return h;
}

static void
upcase(char *str)
{
  char *p;

  for (p = str; *p; p++) {
    *p = (char)toupper((int)*p);
  }
}

static void
free_request(struct HTTPRequest *req)
{
  struct HTTPHeaderField *h, *head;

  head = req->header;
  while (head) {
    h = head;
    head = head->next;
    free(h->name);
    free(h->value);
    free(h);
  }

  free(req->method);
  free(req->path);
  free(req->body);
  free(req);
}

static long
content_length(struct HTTPRequest *req)
{
  char *val;
  long len;

  val = lookup_header_field_value(req, "Content-Length");
  if (!val) return 0;
  len = atol(val);
  if (len < 0) {
    log_exit("negative Content-Length value");
  }

  return len;
}

static char*
lookup_header_field_value(struct HTTPRequest *req, char *name)
{
  struct HTTPHeaderField *h;

  for (h = req->header; h; h = h->next) {
    if (strcasecmp(h->name, name) == 0) {
      return h->value;
    }
  }

  return NULL;
}

static void
respond_to(struct HTTPRequest *req, FILE *out, char *docroot)
{
  if (strcmp(req->method, "GET") == 0) {
    do_file_response(req, out, docroot);
  } else if (strcmp(req->method, "HEAD") == 0) {
    do_file_response(req, out, docroot);
  } else if (strcmp(req->method, "POST") == 0) {
    method_not_allowed(req, out);
  } else {
    not_implemented(req, out);
  }
}

static void
do_file_response(struct HTTPRequest *req, FILE *out, char *docroot)
{
  struct FileInfo *info;
  
  info = get_fileinfo(docroot, req->path);
  if (!info->ok) {
    free_fileinfo(info);
    not_found(req, out);
    return;
  }

  output_common_header_fields(req, out, "200 OK");
  fprintf(out, "Content-Length: %ld\r\n", info->size);
  fprintf(out, "Content-Type: %s\r\n", guess_content_type(info));
  fprintf(out, "\r\n");

  if (strcmp(req->method, "HEAD") != 0) {
    int fd;
    char buf[BLOCK_BUF_SIZE];
    ssize_t n;

    fd = open(info->path, O_RDONLY);
    if (fd < 0) {
      log_exit("failed to open %s: %s", info->path, strerror(errno));
    }
    
    for (;;) {
      n = read(fd, buf, BLOCK_BUF_SIZE);
      if (n < 0) {
        log_exit("failed to read %s: %s", info->path, strerror(errno));
      } else if (n == 0) {
        break;
      }

      if (fwrite(buf, 1, n, out) < n) {
        log_exit("failed to write to socket: %s", strerror(errno));
      }
    }

    close(fd);
  }

  fflush(out);
  free_fileinfo(info);
}

static void
method_not_allowed(struct HTTPRequest *req, FILE *out)
{
  output_common_header_fields(req, out, "405 Method Not Allowed");
  fprintf(out, "Content-Type: text/html\r\n");
  fprintf(out, "\r\n");
  fprintf(out, "<html>\r\n");
  fprintf(out, "<header>\r\n");
  fprintf(out, "<title>405 Method Not Allowed</title>\r\n");
  fprintf(out, "<header>\r\n");
  fprintf(out, "<body>\r\n");
  fprintf(out, "<p>The request method %s is not allowed</p>\r\n", req->method);
  fprintf(out, "</body>\r\n");
  fprintf(out, "</html>\r\n");
  fflush(out);
  log_exit("post method is not allowed for now");
}

static void
not_implemented(struct HTTPRequest *req, FILE *out)
{
  output_common_header_fields(req, out, "501 Not Implemented");
  fprintf(out, "Content-Type: text/html\r\n");
  fprintf(out, "\r\n");
  fprintf(out, "<html>\r\n");
  fprintf(out, "<header>\r\n");
  fprintf(out, "<title>501 Not Implemented</title>\r\n");
  fprintf(out, "<header>\r\n");
  fprintf(out, "<body>\r\n");
  fprintf(out, "<p>The request method %s is not implemented</p>\r\n", req->method);
  fprintf(out, "</body>\r\n");
  fprintf(out, "</html>\r\n");
  fflush(out);
}

static void
not_found(struct HTTPRequest *req, FILE *out)
{
  output_common_header_fields(req, out, "404 Not Found");
  fprintf(out, "Content-Type: text/html\r\n");
  fprintf(out, "\r\n");
  if (strcmp(req->method, "HEAD") != 0) {
    fprintf(out, "<html>\r\n");
    fprintf(out, "<header><title>Not Found</title><header>\r\n");
    fprintf(out, "<body><p>File not found</p></body>\r\n");
    fprintf(out, "</html>\r\n");
  }
  fflush(out);
}

static void
output_common_header_fields(struct HTTPRequest *req,
                            FILE *out, char *status)
{
  time_t t;
  struct tm *tm;
  char buf[TIME_BUF_SIZE];

  t = time(NULL);
  tm = gmtime(&t);
  if (!tm) {
    log_exit("gmtime() failed: %s", strerror(errno));
  }
  strftime(buf, TIME_BUF_SIZE, "%a, %d, %b %Y %H:%M:%S GMT", tm);
  fprintf(out, "HTTP/1.%d %s\r\n", HTTP_MINOR_VERSION, status);
  fprintf(out, "Date: %s\r\n", buf);
  fprintf(out, "Server: %s/%s\r\n", SERVER_NAME, SERVER_VERSION);
  fprintf(out, "Connection: close\r\n");
}

/*struct stat {
    dev_t     st_dev;     // ファイルがあるデバイスの ID 
    ino_t     st_ino;     // inode 番号
    mode_t    st_mode;    // アクセス保護
    nlink_t   st_nlink;   // ハードリンクの数
    uid_t     st_uid;     // 所有者のユーザー ID
    gid_t     st_gid;     // 所有者のグループ ID
    dev_t     st_rdev;    // デバイス ID (特殊ファイルの場合)
    off_t     st_size;    // 全体のサイズ (バイト単位)
    blksize_t st_blksize; // ファイルシステム I/O でのブロックサイズ
    blkcnt_t  st_blocks;  // 割り当てられた 512B のブロック数
};*/
static struct
FileInfo* get_fileinfo(char *docroot, char *urlpath)
{
  struct FileInfo *info;
  struct stat st;

  info = xmalloc(sizeof(struct FileInfo));
  info->path = build_fspath(docroot, urlpath);
  info->ok = 0;
  // lstat: stat(この例ではst)にpathで指定されたファイル情報を格納する
  if (lstat(info->path, &st) < 0) return info;
  // S_ISREG: 通常のファイルか？
  if (!S_ISREG(st.st_mode)) return info;
  info->ok = 1;
  info->size = st.st_size;
  return info;
}

static char*
build_fspath(char *docroot, char *urlpath)
{
  char *path;
  
  path = xmalloc(strlen(docroot) + 1 + strlen(urlpath) + 1);
  // 第一引数のpathに第二引数を格納
  sprintf(path, "%s/%s", docroot, urlpath);
  return path;
}

static void
free_fileinfo(struct FileInfo *info)
{
  free(info->path); 
  free(info); 
}

static char*
guess_content_type(struct FileInfo *info)
{
    return "text/plain";
}

static void*
xmalloc(size_t size)
{
  void *p;
  p = malloc(size);
  if (!p) log_exit("failed to allocate memory");
  return p;
}

static void
log_exit(char *fmt, ...)
{
  va_list ap; // 可変長引数を扱う変数
  va_start(ap, fmt);

  if (debug_mode) {  
    vfprintf(stderr, fmt, ap); // vが先頭につく関数は可変長引数変数(今回の場合ap)を渡せる
    fputc('\n', stderr);
  } else {
    vsyslog(LOG_ERR, fmt, ap);
  }

  va_end(ap);
  exit(1);
}
