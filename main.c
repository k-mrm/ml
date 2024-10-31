#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ml.h"

static const char *
readfile(char *path)
{
  FILE *file = fopen(path, "r");
  size_t sz;
  char *src;

  if(!file)
    return NULL;
  fseek(file, 0, SEEK_END);
  sz = ftell(file);
  fseek(file, 0, SEEK_SET);

  src = malloc(sz+1);
  if (!src)
    return NULL;
  memset(src, 0, sz+1);
  if(fread(src, 1, sz, file) < sz)
    error("Error reading file");
  fclose(file);

  return src;
}

int
main(int argc, char **argv)
{
  const char *src;

  if (argc != 2)
    return -1;
  src = readfile(argv[1]);
  if (!src)
    return -1;

  return exec(parse(lex(src)));
}
