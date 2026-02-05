#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

struct DynamicString {
  char *ptr;
  int cap;
  int len;
};

extern "C" void __attribute__((noinline)) DynamicString_destroy(DynamicString *str) {
  free(str->ptr);
  memset(str, 0, sizeof(DynamicString));
}

extern "C" void __attribute__((noinline)) DynamicString_init(DynamicString *str, int init_cap) {
  DynamicString_destroy(str);

  str->cap = init_cap;
  str->len = 0;
  str->ptr = (char *)malloc(str->cap + 1);
  str->ptr[str->len] = '\0';
}

extern "C" void __attribute__((noinline)) DynamicString_append(DynamicString *str, char ch) {
  if (str->len == str->cap) {
    str->cap *= 2;
    str->ptr = (char *)realloc(str->ptr, str->cap + 1);
  }
  str->ptr[str->len++] = ch;
  str->ptr[str->len] = '\0';
}

int main(int argc, char **argv) {
  if (argc < 2) {
    puts("Usage: profileme myfile.txt");
    return EXIT_FAILURE;
  }

  auto fp = fopen(argv[1], "r");
  DynamicString line = {};
  DynamicString_init(&line, 10);
  int count = 1;
  while (true) {
    char ch = 0;
    if (fread(&ch, 1, 1, fp) != 1) {
      break;
    }
    if (ch == '\n') {
      printf("[%03d] %s\n", count++, line.ptr);
      DynamicString_init(&line, 10);
    } else {
      DynamicString_append(&line, ch);
    }
  }
  fclose(fp);
  DynamicString_destroy(&line);
  return EXIT_SUCCESS;
}
