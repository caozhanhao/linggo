#include "linggo/server.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char* argv[])
{
  srand(time(NULL));
  if (argc != 2)
  {
    printf("Usage: %s <config path>\n", argv[0]);
    return -1;
  }

  linggo_server_init(argv[1]);
  linggo_server_start();
  return 0;
}
