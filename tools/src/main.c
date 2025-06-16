#include <stdio.h>
#include <stdlib.h>

static int modifyPackHead(FILE *, FILE *);
static int modifyPackHtml(FILE *, FILE *);

int main(int argc, char *argv[]) {
  FILE *infile = NULL;
  FILE *outfile = NULL;
  int c, result = EXIT_SUCCESS;
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <type> <input_file> <output_file>\n", argv[0]);
    result = EXIT_FAILURE;
    goto end;
  }
  infile = fopen(argv[2], "rb");
  if (infile == NULL) {
    perror("Error opening input file"); 
    result = EXIT_FAILURE;
    goto end;
  }
  outfile = fopen(argv[3], "wb");
  if (outfile == NULL) {
    perror("Error opening output file");
    result = EXIT_FAILURE;
    goto close_infile;
  }
  if (!strcmp(argv[1], "head")) {
    printf("Converting %s of '%s' to '%s' ...\n", argv[1], argv[2], argv[3]);
    if (modifyPackHead(infile, outfile)) {
      result = EXIT_FAILURE;
      goto close_outfile;
    }
    printf("Conversion complete!\n");
  } else if (!strcmp(argv[1], "html")) {
    printf("Converting %s of '%s' to '%s' ...\n", argv[1], argv[2], argv[3]);
    if (modifyPackHtml(infile, outfile)) {
      result = EXIT_FAILURE;
      goto close_outfile;
    }
    printf("Conversion complete!\n");
  } else {
    fprintf(stderr, "Unsupport %s as type", argv[1]);
    result = EXIT_FAILURE;
  }
close_outfile:
  fclose(outfile);
close_infile:
  fclose(infile);
end:
  return result;
}

static int modifyPackHead(FILE *in, FILE *out) {
  int c; 
  while ((c = fgetc(in)) != EOF) {
    if (c == '\n') {
      if (fputs("\r\n", out) == EOF) {
        perror("Error writing \\r to output file");
        return 1;
      }
    } else {
      if (fputc(c, out) == EOF) {
        perror("Error writing character to output file");
        return 1;
      }
    }
  }
  return 0;
}
static int modifyPackHtml(FILE *in, FILE *out) {
  int c, d = 0; 
  while ((c = fgetc(in)) != EOF) {
    if ((c != '\n') && ((d != ' ') || (c != ' '))) {
      if (fputc(c, out) == EOF) {
        perror("Error writing character to output file");
        return 1;
      }
    }
    d = c;
  }
  return 0;
}