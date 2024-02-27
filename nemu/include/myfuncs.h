#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//------------------------------------------
//表达式求值

uint32_t expr_eval(char* expr)
{
  int len = strlen(expr); uint32_t res = 0;
  for(int i = len - 1, base = 1; i > 1; i--, base *= 16)
  {
    res += ((expr[i] >= '0' && expr[i] <= '9')?(expr[i] - '0'):(expr[i] - 'a' + 10)) * base;
  }
  return res;
}