#ifndef __FTP_SESSION_CMD_H
#define __FTP_SESSION_CMD_H

#include "ftp_session.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ftp_session_cmd
{
    char *cmd;
    int (*cmd_fn)(struct ftp_session *session, char *cmd, char *cmd_param);
};

int ftp_session_cmd_process(struct ftp_session *session, char *cmd, char *cmd_param);

#ifdef __cplusplus
}
#endif

#endif
