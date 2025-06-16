#include <stdio.h>
#include "util.h"

const char *test[6] = {
  "GET / HTTP/1.1\r\n"
  "Host: localhost:8080\r\n"
  "User-Agent: curl/8.14.0\r\n"
  "Accept: */*\r\n"
  "\r\n",
  "GET / HTTP/1.1\r\n"
  "Host: localhost:8080\r\n"
  "Connection: keep-alive\r\n"
  "sec-ch-ua: \"Google Chrome\";v=\"137\", \"Chromium\";v=\"137\", \"Not/A)Brand\";v=\"24\"\r\n"
  "sec-ch-ua-mobile: ?1\r\n"
  "sec-ch-ua-platform: \"Android\"\r\n"
  "DNT: 1\r\n"
  "Upgrade-Insecure-Requests: 1\r\n"
  "User-Agent: Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Mobile Safari/537.36\r\n"
  "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
  "Sec-Fetch-Site: cross-site\r\n"
  "Sec-Fetch-Mode: navigate\r\n"
  "Sec-Fetch-User: ?1\r\n"
  "Sec-Fetch-Dest: document\r\n"
  "Accept-Encoding: gzip, deflate, br, zstd\r\n"
  "Accept-Language: id-ID,id;q=0.9,en-US;q=0.8,en;q=0.7,fr;q=0.6\r\n"
  "\r\n",
  "GET /favicon.ico HTTP/1.1\r\n"
  "Host: localhost:8080\r\n"
  "Connection: keep-alive\r\n"
  "sec-ch-ua-platform: \"Android\"\r\n"
  "User-Agent: Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Mobile Safari/537.36\r\n"
  "sec-ch-ua: \"Google Chrome\";v=\"137\", \"Chromium\";v=\"137\", \"Not/A)Brand\";v=\"24\"\r\n"
  "DNT: 1\r\n"
  "sec-ch-ua-mobile: ?1\r\n"
  "Accept: image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8\r\n"
  "Sec-Fetch-Site: same-origin\r\n"
  "Sec-Fetch-Mode: no-cors\r\n"
  "Sec-Fetch-Dest: image\r\n"
  "Referer: https://localhost:8080/\r\n"
  "Accept-Encoding: gzip, deflate, br, zstd\r\n"
  "Accept-Language: id-ID,id;q=0.9,en-US;q=0.8,en;q=0.7,fr;q=0.6\r\n"
  "\r\n",
  "GET /image/background.svg HTTP/1.1\r\n"
  "Host: localhost:8080\r\n"
  "Connection: keep-alive\r\n"
  "sec-ch-ua-platform: \"Android\"\r\n"
  "User-Agent: Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Mobile Safari/537.36\r\n"
  "sec-ch-ua: \"Google Chrome\";v=\"137\", \"Chromium\";v=\"137\", \"Not/A)Brand\";v=\"24\"\r\n"
  "DNT: 1\r\n"
  "sec-ch-ua-mobile: ?1\r\n"
  "Accept: image/avif,image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8\r\n"
  "Sec-Fetch-Site: same-origin\r\n"
  "Sec-Fetch-Mode: no-cors\r\n"
  "Sec-Fetch-Dest: image\r\n"
  "Referer: https://localhost:8080/\r\n"
  "Accept-Encoding: gzip, deflate, br, zstd\r\n"
  "Accept-Language: id-ID,id;q=0.9,en-US;q=0.8,en;q=0.7,fr;q=0.6\r\n"
  "\r\n",
  "GET /font/ic.ttf HTTP/1.1\r\n"
  "Host: localhost:8080\r\n"
  "Connection: keep-alive\r\n"
  "sec-ch-ua-platform: \"Android\"\r\n"
  "User-Agent: Mozilla/5.0 (Linux; Android 10; K) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/137.0.0.0 Mobile Safari/537.36\r\n"
  "sec-ch-ua: \"Google Chrome\";v=\"137\", \"Chromium\";v=\"137\", \"Not/A)Brand\";v=\"24\"\r\n"
  "DNT: 1\r\n",
  "sec-ch-ua-mobile: ?1\r\n"
  "Accept: font/*,*/*;q=0.8\r\n"
  "Sec-Fetch-Site: same-origin\r\n"
  "Sec-Fetch-Mode: no-cors\r\n"
  "Sec-Fetch-Dest: font\r\n"
  "Referer: https://localhost:8080/\r\n"
  "Accept-Encoding: gzip, deflate, br, zstd\r\n"
  "Accept-Language: id-ID,id;q=0.9,en-US;q=0.8,en;q=0.7,fr;q=0.6\r\n"
  "\r\n"
};

int util_test() {
  httpRequest req;
  char buffer[512], cp[2048];
  if (prepare_utils()) return 1;
  for (size_t i = 3, j; i < 4; ++i) {
    printf("\n\n");
    printf("%s",  test[i]);
    printf("\n\n");
    strcpy(buffer, test[i]);
    const text *ts = request_parser(&req, buffer);
    memset(cp, 0, 2048);
    char *cpc = cp;
    for(j = 0; j < 20; ++j) {
      if (ts[j].l == 0) break;
      memcpy(cpc, ts[j].t, ts[j].l);
      cpc += ts[j].l;
    }
    printf("%s", cp);
    freed_responseText(ts);
    printf("\n\n");
  }
  free_utils();
  return 0;
}