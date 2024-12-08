#include "linggo/server.h"

int main(int argc, char *argv[])
{
  linggo_server_init("../res/config/config.json");
  linggo_server_start();
  return 0;
}