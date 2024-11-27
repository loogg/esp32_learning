#ifndef __FTP_H
#define __FTP_H

#include <stdint.h>

int ftp_get_max_session_num(void);
int ftp_set_max_session_num(int num);
const char *ftp_get_session_username(void);
int ftp_set_session_username(const char *username);
const char *ftp_get_session_password(void);
int ftp_set_session_password(const char *password);
const char *ftp_get_session_welcome_msg(void);
int ftp_set_session_welcome_msg(const char *welcome_msg);
int ftp_session_force_quit(void);

int ftp_force_restart(void);
int ftp_get_port(void);
int ftp_set_port(int port);
int ftp_init(uint32_t stack_size, uint8_t priority);

#endif
