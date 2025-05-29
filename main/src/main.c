#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>

#include "log.h"

#define ERROR_MESSAGE_CONSOLE_LENGTH 256
#define BUFFER_SIZE 30720
char error_message_console[ERROR_MESSAGE_CONSOLE_LENGTH];

// variables
enum method_t {
  INVALID_METHOD, GET, POST, PUT, PATCH, DELETE, HEAD, OPTIONS, TRACE, CONNECT;
};

static int pipe_con[2] = {0}; // 0 => read pipe, 1 => write_pipe
static inline void signal_handler(int);
static void writeBytesToFD(int, const char*);
static int readBytesFromFD(int, char*);

int main (int args, char **argv) {
  char buffer[BUFFER_SIZE + 1] = {0};
  size_t i, j;
  int server_sock, client_sock;
  struct sockaddr_in server_addr;

  if (args < 3) {
    strcpy(error_message_console, "not enough arguments, must be fermity <host> <port>");
    goto failure;
  }
  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  {
    //host
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int status = getaddrinfo(argv[1], NULL, &hints, &res);
    if (status != 0) {
      sprintf(error_message_console, "host '%s': %s", argv[1], gai_strerror(status));
      goto failure;
    }
    server_addr.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    freeaddrinfo(res);
    // port
    int port = atoi(argv[2]);
    if (port < 0 || port > 65535) {
      strcpy(error_message_console, "wrong port format");
      goto failure;
    }
    server_addr.sin_port = htons(port);
  }
  // create server_sock
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock < 0) {
    strcpy(error_message_console, "socket create error");
    goto failure;
  }
  // bind server
  if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in)) < 0) {
    strcpy(error_message_console, "fail to bind");
    goto failure_sock;
  }
  // listen max stack queue request
  if (listen(server_sock, SOMAXCONN) < 0) {
    strcpy(error_message_console, "fail to listen");
    goto failure_sock;
  }
  // create pipe
  if (pipe(pipe_con) < 0) {
    strcpy(error_message_console, "failed to create pipe");
    goto failure_sock;
  }
  signal(SIGINT, signal_handler);
  printf("Server start. Press Ctrl+C to stop.\n");
  int max_fd_input = 2;
  fd_set master_fdset, readable_fdset;
  FD_ZERO(&master_fdset);
  FD_SET(pipe_con[0], &master_fdset);
  FD_SET(server_sock, &master_fdset);
select_fd:
  readable_fdset = master_fdset;
  if (select(master_fdset.fd_count + 1, &readable_fdset, NULL, NULL) < 0) {
    strcpy(error_message_console, "unexpected error on select continue");
    goto select_fd;
  }
  for (i = 0; i < readable_fdset.fd_count; ++i) {
    int sck = readable_fdset.fd_array[i];
    if (sck == server_sock) {
      client_sock = accept(server_sock, 0, 0);
      FD_SET(client_sock, &master_fdset);
    } else if (sck == pipe_con[0]) {
      if (read(pipe_con[0], &client_sock, sizeof(int)) != sizeof(int)) {
        printf("pipe read error! continue proccess!\n");
        continue;
      }
      switch (client_sock) {
        case SIGINT:
          printf("\n detect SIGINT (Ctrl+C)\n");
          goto cleanup_fd;
        case SIGTERM:
          printf("\nSIGTERM detected. Shutting down server gracefully...\n");
          goto cleanup_fd;
        case SIGHUP:
          printf("\nSIGHUP detected. Server might reload configuration (not implemented yet).\n");
          goto cleanup_fd;
        case SIGQUIT:
          printf("\nSIGQUIT (Ctrl+\\) detected. Terminating with core dump (if configured).\n");
          // keep_running = 0; // Anda mungkin ingin menghentikan server juga, atau membiarkan default action (core dump)
          // signal(SIGQUIT, SIG_DFL); // Reset to default handler to allow core dump
          // raise(SIGQUIT); // Re-raise the signal
          goto cleanup_fd;
        case SIGPIPE:
          printf("\nSIGPIPE detected. Broken pipe while writing. Ignoring.\n");
          // Sangat umum untuk mengabaikan SIGPIPE di aplikasi jaringan
          // Ini mencegah crash ketika klien menutup koneksi saat server masih mencoba menulis
          goto cleanup_fd;
        case SIGCHLD:
          //printf("\nSIGCHLD detected. Child process status changed.\n");
          // Ini penting untuk mencegah zombie processes, tapi implementasi waitpid
          // perlu dilakukan di sini, atau Anda bisa menggunakan signal(SIGCHLD, SIG_IGN);
          // forked processes should handle SIGCHLD or use wait/waitpid
          goto cleanup_fd;
        case SIGTSTP:
          printf("\nSIGTSTP (Ctrl+Z) detected. Server paused (job control).\n");
          // Anda mungkin ingin menunda pemrosesan hingga SIGCONT diterima
          goto cleanup_fd;
        case SIGCONT:
          printf("\nSIGCONT detected. Server resumed.\n");
          goto cleanup_fd;
        case SIGSEGV:
          fprintf(stderr, "\nSIGSEGV (Segmentation Fault) detected. Crashing...\n");
          // Ini adalah kesalahan fatal, Anda mungkin ingin log sebelum crash
          // signal(SIGSEGV, SIG_DFL); // Kembali ke default handler untuk crash
          // raise(SIGSEGV); // Re-raise the signal
          goto cleanup_fd;
        case SIGABRT:
          fprintf(stderr, "\nSIGABRT (Abort) detected. Aborting...\n");
          // Sama seperti SIGSEGV, ini adalah kesalahan fatal
          // signal(SIGABRT, SIG_DFL);
          // raise(SIGABRT);
          goto cleanup_fd;
        // Tambahkan sinyal lain yang ingin Anda tangani
        default:
          printf("\nReceived unhandled signal: %d\n", client_sock);
          break;
      }
    } else {
      FD_CLR(client_sock, &master_fdset);
      // receive request
      int readbytes = read(client_sock, buffer, BUFFER_SIZE);
      if (readbytes < 0) {
        printf("Failed to receive bytes from client socket connection\n");
      } else if (readbytes > 14) {
        char m[7], uri[1024], ver[100];
        sscanf(buffer, "%s %s %s\r\n", m, uri, ver);
        // get methode
        enum method_t mt = INVALID_METHOD;
        if (m[0] == 'P') {
          if (strcmp(m, "PUT") == 0)
            mt = PUT;
          else if (strcmp(m, "POST") == 0)
            mt = POST;
          else if (strcmp(m, "PATCH") == 0)
            mt = PATCH;
        } else {
          if (strcmp(m, "GET") == 0)
            mt = DELETE;
          else if (strcmp(m, "HEAD") == 0)
            mt = HEAD;
          else if (strcmp(m, "TRACE") == 0)
            mt = TRACE;
          else if (strcmp(m, "DELETE") == 0)
            mt = DELETE;
          else if (strcmp(m, "CONNECT") == 0)
            mt = CONNECT;
          else if (strcmp(m, "OPTIONS") == 0)
            mt = OPTIONS;
        }
        size_t ls = strcspn(buffer, "\n") + 1;
        memmove(buffer, buffer + ls, BUFFER_SIZE - ls);
        printf("%s", buffer);
        // response request
        if (mt == GET && (strcmp(uri, "/") == 0)) {
          writeBytesToFD(client_sock, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 18\r\n\r\n"
            "<h1>Home Page</h1>\r\n\r\n"
          );
        } else {
          writeBytesToFD(client_sock, 
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html\r\n"
            "Content-Length: 21\r\n\r\n"
            "<h1>Unkonwn Page</h1>\r\n\r\n"
          );
        }
      }
      close(client_sock);
    }
  }
  goto select_fd;
cleanup_fd:
  close(pipe_con[1]);
  for (i = 0; i < master_fdset.fd_count; ++i)
    close(master_fdset.fd_array[i]);
  FD_ZERO(&master_fdset);
  return EXIT_SUCCESS;
failure_sock:
  close(server_sock);
failure:
  printf("Error \x1b[31m%s\x1b[0m", error_message_console);
  return EXIT_FAILURE;
}
static inline void signal_handler(int signum) {
  while(write(pipe_con[1], &signum, sizeof(int)) != sizeof(int))
    printf("Failed to write on pipe, write again!");
}
static void writeBytesToFD(int fd, const char *buff) {
  int l = strlen(buff);
  const char *b = buff;
  for (int i = l + 1, j = 0; i; ) {
    j = write(fd, b, i);
    if (j < 0) {
      printf("Failed to send bytes to client socket connection\n");
    } else {
      i -= j;
      b += j;
    }
  }
}
static int readBytesFromFD(int fd, char *b) {
  int read, read_total = 0;
  size_t buff_len = BUFFER_SIZE;
  do {
    read = read(fd, b, buff_len);
    b += read;
    buff_len -= read;
    read_total += read;
  } while (read > 0);
  return (read < 0) ? read : read_total;
}