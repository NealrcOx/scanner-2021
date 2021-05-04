/*
sanner.c
this programming is the scanner's bady
*/
#include<stdio.h>

int main(int argc, char * argv[])
{
  printf("%d\n", argc);
  printf("%c\n",*(*(argv + 1) + 1));
  return 0;
}
