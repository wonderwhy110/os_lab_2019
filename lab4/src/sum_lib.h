#ifndef SUM_LIB_H
#define SUM_LIB_H

struct SumArgs {
  int *array;
  int begin;
  int end;
};

long long Sum(const struct SumArgs *args);

#endif
