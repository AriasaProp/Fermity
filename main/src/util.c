#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>


/*
  HTTP Response Code Lists
1xx informational response – the request was received, continuing process
100 Continue
101 Switching Protocol
103 Early Hints - get response before get full message

2xx successful – the request was successfully received, understood, and accepted
200 OK
201 Created - generate new resource in server
202 Accepted - request may processing
204 No Content
205 Reset Content - reset document view
206 Partial Content - deliver part of content
207 Multi-Status - followed by xml that has some response code
208 Already Reported
226 IM Used

3xx redirection – further action needs to be taken in order to complete the request
300 Multiple Choices
301 Moved Permanently - redirect to given uri
302 Found - like 301
304 Not Modified
306 Switch Proxy - 
308 Permanent Redirect

4xx client error – the request contains bad syntax or cannot be fulfilled
400 Bad Request
401 Unauthorized
402 Payment Required
403 Forbidden
404 Not Found
405 Method Not Allowed
406 Not Acceptable
407 Proxy Authentication Required
408 Request Timeout
409 Conflict
410 Gone - resources is lost (there is exists before but removed)
411 Length Required
412 Precondition Failed
413 Payload Too Large
414 URI Too Long
415 Unsupported Media Type
416 Range Not Satisfiable - The client has asked for a portion of the file (byte serving), but the server cannot supply that portion. 
417 Expectation Failed - The server cannot meet the requirements of the Expect request-header field
418 I'm a teapot (RFC 2324, RFC 7168)
421 Misdirected Request
422 Unprocessable Content
423 Locked (WebDAV; RFC 4918)
424 Failed Dependency (WebDAV; RFC 4918)
425 Too Early (RFC 8470)
426 Upgrade Required
428 Precondition Required (RFC 6585)
429 Too Many Requests (RFC 6585)
431 Request Header Fields Too Large (RFC 6585)
451 Unavailable For Legal Reasons (RFC 7725) - server operator has received a legal demand to deny access.

5xx server error – the server failed to fulfil an apparently valid request
500 Internal Server Error - 
501 Not Implemented
502 Bad Gateway - as gateway
503 Service Unavailable - server may not handle cause overloaded service
504 Gateway Timeout - server act like gateway/proxy and doesn't receive any response
505 HTTP Version Not Supported - not support http version in request
506 Variant Also Negotiates
507 Insufficient Storage - tell about server storage problem
508 Loop Detected - (see 208)
510 Not Extended
511 Network Authentication Required

*/


static text mmap_head, mmap_content;
static text heads_template[MAX_SPLIT] = {0};
static text contents_template[MAX_SPLIT] = {0};

int prepare_utils() {
  struct stat bstat;
  int fd;
  
  if ((fd = open("./pack/rth.t0", O_RDONLY | S_IRUSR)) < 0)
    return -1;
  if (fstat(fd, &bstat) == -1) {
    close(fd);
    return -1;
  }
  mmap_head.l = bstat.st_size;
  mmap_head.tt = TEXT_TYPE_CONST;
  if (!(mmap_head.t = (const char*)mmap(NULL, mmap_head.l, PROT_READ, MAP_PRIVATE, fd, 0))) {
    close(fd);
    return -1;
  }
  close(fd);
  if ((fd = open("./pack/rtc.t0", O_RDONLY | S_IRUSR)) < 0)
    return -1;
  if (fstat(fd, &bstat) == -1) {
    close(fd);
    return -1;
  }
  mmap_content.l = bstat.st_size;
  mmap_content.tt = TEXT_TYPE_CONST;
  if (!(mmap_content.t = (const char*)mmap(NULL, mmap_content.l, PROT_READ, MAP_PRIVATE, fd, 0))) {
    munmap((void*)mmap_head.t, mmap_head.l);
    close(fd);
    return -1;
  }
  close(fd);
  
  const char *last_strstr = mmap_head.t;
  for (size_t i = 0; i < MAX_SPLIT; ++i) {
    heads_template[i].tt = TEXT_TYPE_CONST;
    heads_template[i].t = last_strstr;
    last_strstr = strstr(last_strstr, "\%s");
    if (last_strstr) {
      heads_template[i].l = (size_t)(last_strstr - heads_template[i].t);
      last_strstr += 2;
    } else {
      heads_template[i].l = strlen(heads_template[i].t);
      break;
    }
  }
  last_strstr = mmap_content.t;
  for (size_t i = 0; i < MAX_SPLIT; ++i) {
    contents_template[i].tt = TEXT_TYPE_CONST;
    contents_template[i].t = last_strstr;
    last_strstr = strstr(last_strstr, "\%s");
    if (last_strstr) {
      contents_template[i].l = (size_t)(last_strstr - contents_template[i].t);
      last_strstr += 2;
    } else {
      contents_template[i].l = strlen(contents_template[i].t);
      break;
    }
  }
  
  return 0;
}

void free_utils() {
  munmap((void*)mmap_head.t, mmap_head.l);
  munmap((void*)mmap_content.t, mmap_content.l);
}

static int loadFile(text *t, const char *f) {
  int fd = open(f, O_RDONLY | S_IRUSR);
  if (fd < 0) return -1;
  struct stat bstat;
  if (fstat(fd, &bstat) == -1) {
    close(fd);
    return -1;
  }
  t->l = bstat.st_size;
  t->tt = TEXT_TYPE_NEEDFREE;
  if (!(t->t = (const char*)mmap(NULL,t->l, PROT_READ, MAP_PRIVATE, fd, 0))) {
    close(fd);
    t->tt = TEXT_TYPE_NULL;
    t->l = 0;
    return -1;
  }
  close(fd);
  return 0;
}

const text *request_parser(httpRequest *req, char *b) {
  const char *buff = b, *a1, *a2, *a3;
  size_t i, j, k;
  if (sscanf(buff, "%s %s %s\r\n", req->method, req->uri, req->http_ver) < 3)
    goto error_response;
  buff = strstr(buff, "\r\n") + 2;
  memset (req->headers, 0, sizeof(header_data) * MAX_HEADER_DATA);
  for (i = 0; *buff && (i < MAX_HEADER_DATA); ++i) {
    a1 = strstr(buff, ": ");
    if (!a1) break;
    a2 = strstr(a1, "\r\n");
    if (!a2) break;
    strncpy(req->headers[i].name, buff, (size_t)(a1 - buff));
    a1 += 2;
    strncpy(req->headers[i].value, a1, (size_t)(a2 - a1));
    buff = a2 + 2;
  }
  for (i = 0; *buff && (i < MAX_HEADER_DATA); ++i) {
    if (!strcmp_textual(req->headers[i].name, "accept")) {
      if (strstr(req->headers[i].value, "image/svg+xml")) {
        req->flags |= SUPPORT_SVG;
        break;
      }
    }
  }
  text *ret = (text*)calloc(MAX_SPLIT, sizeof(text));
#define START i = 0, j = 0, k = 0
#define SET_LEN k = i++;
#define SET_NULL ret[i++].l = 0;
#define SET_HEAD(X) ret[i++] = heads_template[X]
#define SET_CONTENT(X) ret[i++] = contents_template[X], j += contents_template[X].l
#define SET_CONTENT_FILE(X) if (loadFile(&ret[i++], X)) goto error_file; else j += ret[i-1].l

  if (!strcmp_textual(req->uri, "/")) { // Compare with 2 bytes for "/" including null terminator
    START;
    SET_HEAD(0); // http
    SET_HEAD(4); // OK
    SET_HEAD(1); // content type
    SET_HEAD(8); // text html + some header
    SET_HEAD(2); // content length
    SET_LEN; // length
    SET_HEAD(3); // end
    SET_CONTENT(0);
    SET_NULL;// TITLE
    SET_CONTENT(1);
    SET_NULL;// home link
    SET_CONTENT(2);
    SET_CONTENT(10);
    SET_CONTENT(11);
    SET_CONTENT(3);
    SET_CONTENT(12);
    SET_CONTENT(13);
    SET_CONTENT(4);
    
    sprintf(b, "%zu", j);
    ret[k].t = b;
    ret[k].l = strlen(ret[k].t);
    
  } else if (!strcmp_textual(req->uri, "/home") ||
    !strcmp_textual(req->uri, "/index") ||
    !strcmp_textual(req->uri, "/dashboard")) {
     // redirect
    START;
    SET_HEAD(0); // http
    SET_HEAD(6); // redirect permanent
    SET_HEAD(9); // redirect location
    SET_HEAD(3); // end
    
  } else if (!strcmp_textual(req->uri, "/account")) {
    START;
    SET_HEAD(0); // http
    SET_HEAD(4); // OK
    SET_HEAD(1); // content type
    SET_HEAD(8); // text html + some header
    SET_HEAD(2); // content length
    SET_LEN; // length
    SET_HEAD(3); // end
    SET_CONTENT(0);
    SET_CONTENT(6);// TITLE account
    SET_CONTENT(1);
    SET_CONTENT(9);// home link added
    SET_CONTENT(2);
    SET_CONTENT(11);
    SET_CONTENT(3);
    SET_CONTENT(12);
    SET_CONTENT(13);
    SET_CONTENT(4);
    
    sprintf(b, "%zu", j);
    ret[k].t = b;
    ret[k].l = strlen(ret[k].t);
    
  } else if (!strcmp_textual(req->uri, "/preferences")) {
    START;
    SET_HEAD(0); // http
    SET_HEAD(4); // OK
    SET_HEAD(1); // content type
    SET_HEAD(8); // text html + some header
    SET_HEAD(2); // content length
    SET_LEN; // length
    SET_HEAD(3); // end
    SET_CONTENT(0);
    SET_CONTENT(8);// TITLE preferences
    SET_CONTENT(1);
    SET_CONTENT(9);// home link added
    SET_CONTENT(2);
    SET_CONTENT(10);
    SET_CONTENT(3);
    SET_CONTENT(12);
    SET_CONTENT(13);
    SET_CONTENT(4);
    
    sprintf(b, "%zu", j);
    ret[k].t = b;
    ret[k].l = strlen(ret[k].t);
    
  } else if (!strcmp_textual(req->uri, "/aboutme")) {
    START;
    SET_HEAD(0); // http
    SET_HEAD(4); // OK
    SET_HEAD(1); // content type
    SET_HEAD(8); // text html + some header
    SET_HEAD(2); // content length
    SET_LEN; // length
    SET_HEAD(3); // end
    SET_CONTENT(0);
    SET_CONTENT(5);// TITLE aboutme
    SET_CONTENT(1);
    SET_CONTENT(9);// home link added
    SET_CONTENT(2);
    SET_CONTENT(10);
    SET_CONTENT(11);
    SET_CONTENT(3);
    SET_CONTENT(13);
    SET_CONTENT(4);
    
    sprintf(b, "%zu", j);
    ret[k].t = b;
    ret[k].l = strlen(ret[k].t);
    
  } else if (!strcmp_textual(req->uri, "/updates")) {
    START;
    SET_HEAD(0); // http
    SET_HEAD(4); // OK
    SET_HEAD(1); // content type
    SET_HEAD(8); // text html + some header
    SET_HEAD(2); // content length
    SET_LEN; // length
    SET_HEAD(3); // end
    SET_CONTENT(0);
    SET_CONTENT(7);// TITLE updates
    SET_CONTENT(1);
    SET_CONTENT(9);// home link added
    SET_CONTENT(2);
    SET_CONTENT(10);
    SET_CONTENT(11);
    SET_CONTENT(3);
    SET_CONTENT(12);
    SET_CONTENT(4);
    
    sprintf(b, "%zu", j);
    ret[k].t = b;
    ret[k].l = strlen(ret[k].t);
    
  } else if (!strcmp_textual(req->uri, "/image*")) {
    START;
    SET_HEAD(0); // http
    SET_HEAD(4); // OK
    SET_HEAD(1); // content type
    SET_HEAD(7); // svg image
    SET_HEAD(2); // content length
    SET_LEN; // length
    SET_HEAD(12); // accept range
    SET_HEAD(3); // end
    sprintf(b, "./assets%s.%s", req->uri + 6, (req->flags & SUPPORT_SVG) ? "svg":" png");
    SET_CONTENT_FILE(b);
    
    sprintf(b, "%zu", j);
    ret[k].t = b;
    ret[k].l = strlen(ret[k].t);
    
  } else if (!strcmp_textual(req->uri, "/font*")) {
    START;
    SET_HEAD(0); // http
    SET_HEAD(4); // OK
    SET_HEAD(1); // content type
    SET_HEAD(11); // ttf
    SET_HEAD(2); // content length
    SET_LEN; // length
    SET_HEAD(12); // accept range
    SET_HEAD(3); // end
    sprintf(b, "./assets%s", req->uri + 5);
    printf("%s", req->uri + 6);
    SET_CONTENT_FILE(b);
    
    sprintf(b, "%zu", j);
    ret[k].t = b;
    ret[k].l = strlen(ret[k].t);
  } else
    goto error_response;
  goto return_response;
error_file:
  START;
  SET_HEAD(0); // http
  SET_HEAD(5); // Not Found
  SET_HEAD(3); // end
  goto return_response;
error_response:
  START;
  SET_HEAD(0); // http
  SET_HEAD(13); // Bad Request
  SET_HEAD(1); // content type
  SET_HEAD(8); // text html + some header
  SET_HEAD(2); // content length
  SET_LEN; // length
  SET_HEAD(3); // end
  SET_CONTENT(0);
  SET_CONTENT(14);// TITLE Error
  SET_CONTENT(1);
  SET_CONTENT(9);// home link added
  SET_CONTENT(2);
  SET_CONTENT(10);
  SET_CONTENT(11);
  SET_CONTENT(3);
  SET_CONTENT(12);
  SET_CONTENT(13);
  SET_CONTENT(4);
  sprintf(b, "%zu", j);
  ret[k].t = b;
  ret[k].l = strlen(ret[k].t);
return_response:
  return ret;
#undef START
#undef SET_LEN
#undef SET_NULL
#undef SET_HEAD
#undef SET_CONTENT
#undef SET_CONTENT_FILE
}

void freed_responseText(const text *s) {
  for (size_t i = 0; i < MAX_SPLIT; ++i) {
    if (s[i].tt == TEXT_TYPE_NEEDFREE)
      munmap((void*)s[i].t, s[i].l);
  }
  free ((void*)s);
}

// compare 2 string event Upper lowercase
int strcmp_textual(const char *a, const char *b) {
  const char *A = a, *B = b;
  int res = 0;
  do {
    if (*B == '*') break;
    res = tolower(*A) - tolower(*B);
  } while (!res && *(A++) && *(B++));
  return res;
}