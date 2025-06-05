#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "util.h"

#define MAX_ERROR_MSG 512
#define MAX_BUFFER 4096
#define NUM_WORK 8
#define NUM_CLIENT 100

static pthread_mutex_t pmtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pcond = PTHREAD_COND_INITIALIZER;
static volatile int alive = 1, work_active = 0;
static int clients[NUM_CLIENT] = {0};
static SSL_CTX *sslctx;

static void handling_signal(int);
static void *serving_client(void *);

int main(int argc, char *argv[]) {
  unsigned long ssle;
  char error_msg[MAX_ERROR_MSG] = {0};
  int server_sock, t, t1, maxfd;
  struct sockaddr_in addr;
  SSL *ssl;
  fd_set fd_master;
  pthread_t workers[NUM_WORK];
  (void) signal(SIGINT, handling_signal);

  if (argc < 3) {
    strcpy(error_msg, "arguments to few, mush be <host> <port>");
    goto main_end;
  }
  {
    // define address
    // host and port validate
    memset(&addr, 0, sizeof(addr));
    struct addrinfo hints,
    *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    t = getaddrinfo(argv[1], argv[2], &hints, &result);
    if (t != 0) {
      sprintf(error_msg, "getaddrinfo: %s", gai_strerror(t));
      goto main_end;
    }
    addr = *(struct sockaddr_in*)result->ai_addr;
    freeaddrinfo (result);
  }
  // init ssl
  if (!OPENSSL_init_ssl (OPENSSL_INIT_LOAD_SSL_STRINGS, 0)) {
    strcpy(error_msg, "fail to initialize OpenSSL SSL: %s");
    goto main_end;
  }
  SSL_library_init();
  OpenSSL_add_ssl_algorithms();
  // init ssl ctx
  if (!(sslctx = SSL_CTX_new(TLS_server_method ()))) {
    strcpy(error_msg, "failed to create ssl context");
    goto main_ssl;
  }
  if (
    (SSL_CTX_use_certificate_file (sslctx, "ssl-certs/crt.pem", SSL_FILETYPE_PEM) <= 0) ||
    (SSL_CTX_use_PrivateKey_file (sslctx, "ssl-certs/key.pem", SSL_FILETYPE_PEM) <= 0) ||
    (SSL_CTX_check_private_key(sslctx) <= 0)
  ) goto main_ssl_ctx;

  if((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    strcpy(error_msg, "create socket fail");
    goto main_ssl_ctx;
  }
  FD_ZERO(&fd_master);
  FD_SET(server_sock, &fd_master);
  maxfd = server_sock + 1;
  if(bind(server_sock, (struct sockaddr*)&addr, sizeof(addr)) || 
    listen(server_sock, SOMAXCONN)) {
    strcpy(error_msg, "socket failure (bind/listen)");
    goto main_sock;
  }
  // make thread pool
  {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for (t = 0; t < NUM_WORK; ++t) {
      if (pthread_create(workers + t, &attr, serving_client, NULL)) {
        strcpy(error_msg, "Insufficient resources to create thread");
        pthread_mutex_lock(&pmtx);
        goto kill_worker;
      }
    }
    pthread_attr_destroy(&attr);
  }
  printf("Server is started. Ctrl + C to stop serving ... \n");

main_select:
  pthread_mutex_lock(&pmtx);
  if (!alive) goto kill_worker;
  pthread_mutex_unlock(&pmtx);
  if (select(maxfd, &fd_master, NULL, NULL, NULL) <= 0) {
    printf("Unexpected! select got error, %s.\n", strerror(errno));
  } else {
    pthread_mutex_lock(&pmtx);
    for (t = 0; t < NUM_CLIENT; ++t)
      if (clients[t] <= 0) break;
    clients[t] = accept(server_sock, NULL, NULL);
    pthread_mutex_unlock(&pmtx);
    pthread_cond_signal(&pcond);
  }
  goto main_select;

kill_worker:
  while(work_active)
    pthread_cond_wait(&pcond, &pmtx);
  pthread_mutex_unlock(&pmtx);
main_sock:
  close(server_sock);
main_ssl_ctx:
  SSL_CTX_free(sslctx);
main_ssl:
  EVP_cleanup();
main_end:
  printf("Server stoped\n");
  ssle = ERR_get_error();
  if (*error_msg || ssle) {
    if (*error_msg) fprintf("\n\033[31mError: %s\033[0m\n", error_msg);
    if (errno) fprintf("\033[31mSystem Error: %s\033[0m\n", strerror(errno));
    if (ssle) fprintf("\033[31mSSL Error: %s\033[0m\n", ERR_error_string(ssle, error_msg));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
static void handling_signal(int s) {
  ((void)s);
  pthread_mutex_lock(&pmtx);
  alive = 0;
  pthread_mutex_unlock(&pmtx);
  pthread_cond_broadcast(&pcond);
}
static void *serving_client(void *arg) {
  pthread_mutex_lock(&pmtx);
  ++work_active;
  pthread_mutex_unlock(&pmtx);
  (void)arg;
  int client_sock, t;
  char buffer[MAX_BUFFER], err_client[MAX_ERROR_MSG] = {0};
  SSL *ssl;
get_job:
  pthread_mutex_lock(&pmtx);
  while(*clients <= 0 && alive) pthread_cond_wait(&pcond, &pmtx);
  if (!alive) goto serv_end;
  client_sock = *clients;
  // it's last item become 0?
  memmove(clients, clients + 1, NUM_CLIENT - 1);
  pthread_mutex_unlock(&pmtx);
  if (!(ssl = SSL_new(sslctx))) {
    strcpy(err_client, "ssl failed to create ssl");
    goto client_end;
  }
  SSL_set_fd(ssl, client_sock);
  if (SSL_accept(ssl) <= 0) {
    strcpy(err_client, "ssl failed to perform handshake");
    goto client_stop;
  }
  if (((t = SSL_read(ssl, buffer, MAX_BUFFER)) <= 0) ||
      (strlen(buffer) < 14)) {
    strcpy(err_client, "ssl failed to read");
    goto client_close;
  }
  httpRequest req = request_parser(buffer);
  printf("URI: %s\n", req.uri);
  static const char *send = "HTTP/1.1 200 OK\r\n"
  "Content-Type: text/html\r\n"
  "Content-Length: 89\r\n\r\n"
  "<!DOCTYPE html><html lang='en'>"
  "<head></head>"
  "<body><h1>Full of page</h1></body>"
  "</html>\r\n\r\n";
  SSL_write(ssl, send, strlen(send));
client_close:
  SSL_shutdown(ssl);
client_stop:
  SSL_free(ssl);
client_end:
  close(client_sock);
  if (err_client[0]) {
    printf("\n\033[31mClient Error %s\033[0m", err_client);
    printf("\n\033[31mClient SSL Error %s\033[0m\n", ERR_error_string(ERR_get_error(), err_client));
    err_client[0] = 0;
  }
  goto get_job;
serv_end:
  --work_active;
  pthread_mutex_unlock(&pmtx);
  pthread_cond_signal(&pcond);
  return NULL;
}
