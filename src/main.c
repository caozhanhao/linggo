#include "linggo/server.h"

int main(int argc, char *argv[])
{
  linggo_init("../res/config/config.json");
  linggo_server_start();
  return 0;
}