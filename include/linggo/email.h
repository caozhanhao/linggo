#ifndef LINGGO_EMAIL_HPP
#define LINGGO_EMAIL_HPP
#pragma once

#include <stdlib.h>
#include <stdint.h>

  struct email_upload_status
  {
    uint32_t bytes_read;
    const char* *const payload_text;
  };
  struct Email
  {
    char* from_addr;
    char* from_name;
    char* to_addr;
    char* to_name;
    char* subject;
    char* body;
  };
  
  struct EmailProvider
  {
    char* username;
    char* passwd;
    char* server;
  };
#endif