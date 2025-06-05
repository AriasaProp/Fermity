#ifndef _UTIL_INCLUDED_
#define _UTIL_INCLUDED_

typedef enum {
  METHOD_NONE = 0, 
  METHOD_GET, METHOD_PUT, 
  METHOD_POST, METHOD_HEAD,
  METHOD_PATCH, METHOD_TRACE,
  METHOD_DELETE,
  METHOD_OPTIONS, METHOD_CONNECT
} METHOD;

typedef struct {
  METHOD method;
  char uri[1024];
} httpRequest;

httpRequest request_parser(char *);

#endif // _UTIL_INCLUDED_