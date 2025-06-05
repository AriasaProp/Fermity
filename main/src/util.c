#include "util.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


httpRequest request_parser(char *buff) {
  httpRequest req;
  
  req.method = METHOD_NONE;
  if (*buff == 'P') {
    ++buff;
    if (!strcmp(buff, "UT ")) {
      req.method = METHOD_PUT;
      buff += 3;
    } else if (!strcmp(buff, "OST ")) {
      req.method = METHOD_POST;
      buff += 4;
    } else if (!strcmp(buff, "ATCH ")) {
      req.method = METHOD_PATCH;
      buff += 5;
    }
  } else {
    if (!strcmp(buff, "GET ")) {
      req.method = METHOD_GET;
      buff += 4;
    } else if (!strcmp(buff, "HEAD ")) {
      req.method = METHOD_HEAD;
      buff += 5;
    } else if (!strcmp(buff, "TRACE ")) {
      req.method = METHOD_TRACE;
      buff += 6;
    } else if (!strcmp(buff, "DELETE ")) {
      req.method = METHOD_DELETE;
      buff += 7;
    } else if (!strcmp(buff, "OPTIONS ")) {
      req.method = METHOD_OPTIONS;
      buff += 8;
    } else if (!strcmp(buff, "CONNECT ")) {
      req.method = METHOD_CONNECT;
      buff += 8;
    }
  }
  if (req.method != METHOD_NONE) sscanf(buff, "%s", req.uri);
  return req;
}