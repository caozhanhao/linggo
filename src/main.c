#include "linggo/server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char* argv[])
{
  char resource_path[512] = {0};
  srand(time(NULL));
  if (argc != 2)
  {
    printf("Warning: failed to read config path from command line args.\nPlease input the path of config.json:");
    scanf("%512s", resource_path);
  }
  else
    strcpy(resource_path, argv[1]);

  enum LINGGO_CODE ret = linggo_server_init(resource_path);
  if(ret != LINGGO_OK)
  {
    printf("Failed to initialize LingGo server: %s.", linggo_strerror(ret));
    return -1;
  }
  linggo_server_start();
  return 0;
}
