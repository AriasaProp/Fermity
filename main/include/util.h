#ifndef _UTIL_INCLUDED_
#define _UTIL_INCLUDED_

#define MAX_HEADER_DATA 100
#define MAX_SPLIT 100

#include <unistd.h>

typedef struct {
  char name[256];
  char value[256];
} header_data;

typedef enum : int {
  SUPPORT_SVG = 1 >> 0,
} flag_support;

typedef enum  {
  TEXT_TYPE_NULL = 0,
  TEXT_TYPE_NEEDFREE,
  TEXT_TYPE_CONST,
} textType;

typedef struct {
  flag_support flags;
  char method[8];
  char uri[512];
  char http_ver[16];
  header_data headers[MAX_HEADER_DATA];
} httpRequest;

typedef struct {
  textType tt;
  size_t l;
  const char *t;
} text;

int prepare_utils();
void free_utils();

const text *request_parser(httpRequest*, char*);
void freed_responseText(const text *);
int strcmp_textual(const char *, const char *);

#endif // _UTIL_INCLUDED_