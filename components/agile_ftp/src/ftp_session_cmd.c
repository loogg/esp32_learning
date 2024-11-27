#include <dirent.h>
#include "ftp_session_cmd.h"

#define RT_EOK   0
#define RT_ERROR 1

static int ftp_create_dir(const char *path)
{
    int result = RT_EOK;

    DIR *dir = opendir(path);
    if(dir == NULL)
    {
        if(mkdir(path, 0x777) != 0)
            result = -RT_ERROR;
    }
    else
        closedir(dir);

    return result;
}

static char* ftp_normalize_path(char* fullpath)
{
    char *dst0, *dst, *src;

    src = fullpath;
    dst = fullpath;

    dst0 = dst;
    while (1)
    {
        char c = *src;

        if (c == '.')
        {
            if (!src[1]) src ++; /* '.' and ends */
            else if (src[1] == '/')
            {
                /* './' case */
                src += 2;

                while ((*src == '/') && (*src != '\0')) src ++;
                continue;
            }
            else if (src[1] == '.')
            {
                if (!src[2])
                {
                    /* '..' and ends case */
                    src += 2;
                    goto up_one;
                }
                else if (src[2] == '/')
                {
                    /* '../' case */
                    src += 3;

                    while ((*src == '/') && (*src != '\0')) src ++;
                    goto up_one;
                }
            }
        }

        /* copy up the next '/' and erase all '/' */
        while ((c = *src++) != '\0' && c != '/') *dst ++ = c;

        if (c == '/')
        {
            *dst ++ = '/';
            while (c == '/') c = *src++;

            src --;
        }
        else if (!c) break;

        continue;

up_one:
        dst --;
        if (dst < dst0) return NULL;
        while (dst0 < dst && dst[-1] != '/') dst --;
    }

    *dst = '\0';

    /* remove '/' in the end of path if exist */
    dst --;
    if ((dst != fullpath) && (*dst == '/')) *dst = '\0';

    return fullpath;
}

static int port_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    if (session->port_pasv_fd >= 0)
    {
        close(session->port_pasv_fd);
        session->port_pasv_fd = -1;
    }

    char *reply = NULL;
    int portcom[6];
    char iptmp[100];
    int index = 0;
    char *ptr = cmd_param;
    while (ptr != NULL)
    {
        if (*ptr == ',')
            ptr++;
        portcom[index] = atoi(ptr);
        if ((portcom[index] < 0) || (portcom[index] > 255))
            break;
        index++;
        if (index == 6)
            break;
        ptr = strchr(ptr, ',');
    }
    if (index < 6)
    {
        reply = "504 invalid parameter.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    snprintf(iptmp, sizeof(iptmp), "%d.%d.%d.%d", portcom[0], portcom[1], portcom[2], portcom[3]);

    int rc = -RT_ERROR;
    do
    {
        session->port_pasv_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (session->port_pasv_fd < 0)
            break;
        struct timeval tv;
        tv.tv_sec = 20;
        tv.tv_usec = 0;
        if (setsockopt(session->port_pasv_fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&tv, sizeof(struct timeval)) < 0)
            break;
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(portcom[4] * 256 + portcom[5]);
        addr.sin_addr.s_addr = inet_addr(iptmp);
        if (connect(session->port_pasv_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            break;

        rc = RT_EOK;
    } while (0);

    if (rc != RT_EOK)
    {
        reply = "425 Can't open data connection.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        if (session->port_pasv_fd >= 0)
        {
            close(session->port_pasv_fd);
            session->port_pasv_fd = -1;
        }
        return RT_EOK;
    }

    reply = "200 Port Command Successful.\r\n";
    send(session->fd, reply, strlen(reply), 0);
    return RT_EOK;
}

static int pwd_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = malloc(1024);
    if(reply == NULL)
        return -RT_ERROR;

    snprintf(reply, 1024, "257 \"%s\" is current directory.\r\n", session->currentdir);
    send(session->fd, reply, strlen(reply), 0);
    free(reply);
    return RT_EOK;
}

static int type_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    // Ignore it
    char *reply = NULL;
    if (strcmp(cmd_param, "I") == 0)
        reply = "200 Type set to binary.\r\n";
    else
        reply = "200 Type set to ascii.\r\n";

    send(session->fd, reply, strlen(reply), 0);
    return RT_EOK;
}

static int syst_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = "215 RT-Thread RTOS\r\n";
    send(session->fd, reply, strlen(reply), 0);
    return RT_EOK;
}

static int quit_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = "221 Bye!\r\n";
    send(session->fd, reply, strlen(reply), 0);

    return -RT_ERROR;
}

static int list_statbuf_get(struct dirent *dirent, struct stat *s, char *buf, int bufsz)
{
    int ret = 0;
    struct tm ftm;
    struct tm ntm;
    time_t now_time;

    // file type
    memset(buf, '-', 10);

    if (S_ISDIR(s->st_mode))
        buf[0] = 'd';

    if (s->st_mode & S_IRUSR)
        buf[1] = 'r';
    if (s->st_mode & S_IWUSR)
        buf[2] = 'w';
    if (s->st_mode & S_IXUSR)
        buf[3] = 'x';

    if (s->st_mode & S_IRGRP)
        buf[4] = 'r';
    if (s->st_mode & S_IWGRP)
        buf[5] = 'w';
    if (s->st_mode & S_IXGRP)
        buf[6] = 'x';

    if (s->st_mode & S_IROTH)
        buf[7] = 'r';
    if (s->st_mode & S_IWOTH)
        buf[8] = 'w';
    if (s->st_mode & S_IXOTH)
        buf[9] = 'x';

    ret += 10;
    buf[ret++] = ' ';

    // user info
    ret += snprintf(buf + ret, bufsz - ret, "%d %s %s", s->st_nlink, "admin", "admin");
    buf[ret++] = ' ';

    // file size
    ret += snprintf(buf + ret, bufsz - ret, "%ld", s->st_size);
    buf[ret++] = ' ';

    // file date
    gmtime_r(&s->st_mtime, &ftm);
    now_time = time(NULL);
    gmtime_r(&now_time, &ntm);

    if (ftm.tm_year == ntm.tm_year) {
        ret += strftime(buf + ret, bufsz - ret, "%b %d %H:%M", &ftm);
    } else {
        ret += strftime(buf + ret, bufsz - ret, "%b %d %Y", &ftm);
    }

    buf[ret++] = ' ';

    // file name
    ret += snprintf(buf + ret, bufsz - ret, "%s\r\n", dirent->d_name);

    return ret;
}

static int list_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = NULL;
    if (session->port_pasv_fd < 0)
    {
        reply = "502 Not Implemented.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    DIR *dir = opendir(session->currentdir);
    if(dir == NULL)
    {
        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 directory \"%s\" can't open.\r\n", session->currentdir);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        return RT_EOK;
    }

    reply = "150 Opening Binary mode connection for file list.\r\n";
    send(session->fd, reply, strlen(reply), 0);

    struct dirent *dirent = NULL;
    char tmp[516];
    struct stat s;
    do {
        dirent = readdir(dir);
        if (dirent == NULL)
            break;
        snprintf(tmp, sizeof(tmp), "%s/%s", session->currentdir, dirent->d_name);
        memset(&s, 0, sizeof(struct stat));
        if (stat(tmp, &s) != 0)
            continue;

        int stat_len = list_statbuf_get(dirent, &s, tmp, sizeof(tmp));
        if (stat_len <= 0)
            continue;

        send(session->port_pasv_fd, tmp, stat_len, 0);
    } while (dirent != NULL);

    closedir(dir);

    close(session->port_pasv_fd);
    session->port_pasv_fd = -1;

    reply = "226 Transfert Complete.\r\n";
    send(session->fd, reply, strlen(reply), 0);
    return RT_EOK;
}

static int nlist_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = NULL;
    if (session->port_pasv_fd < 0)
    {
        reply = "502 Not Implemented.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    DIR *dir = opendir(session->currentdir);
    if (dir == NULL)
    {
        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 directory \"%s\" can't open.\r\n", session->currentdir);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        return RT_EOK;
    }

    reply = "150 Opening Binary mode connection for file list.\r\n";
    send(session->fd, reply, strlen(reply), 0);

    struct dirent *dirent = NULL;
    char tmp[260];
    do
    {
        dirent = readdir(dir);
        if (dirent == NULL)
            break;
        snprintf(tmp, sizeof(tmp), "%s\r\n", dirent->d_name);
        send(session->port_pasv_fd, tmp, strlen(tmp), 0);
    } while (dirent != NULL);

    closedir(dir);

    close(session->port_pasv_fd);
    session->port_pasv_fd = -1;

    reply = "226 Transfert Complete.\r\n";
    send(session->fd, reply, strlen(reply), 0);
    return RT_EOK;
}

static int build_full_path(char *buf, int bufsz, const char *path)
{
    if(path[0] == '/')
        snprintf(buf, bufsz, "%s", path);
    else
    {
        strcat(buf, "/");
        int remain_len = bufsz - strlen(buf) - 1;
        strncat(buf, path, remain_len);
    }

    if(ftp_normalize_path(buf) == NULL)
        return -RT_ERROR;

    return RT_EOK;
}

static int cwd_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    if(build_full_path(session->currentdir, sizeof(session->currentdir), cmd_param) != RT_EOK)
        return -RT_ERROR;

    char *reply = NULL;
    DIR *dir = opendir(session->currentdir);
    if (dir == NULL)
    {
        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 directory \"%s\" can't open.\r\n", session->currentdir);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        return RT_EOK;
    }

    closedir(dir);

    reply = malloc(1024);
    if(reply == NULL)
        return -RT_ERROR;

    snprintf(reply, 1024, "250 Changed to directory \"%s\"\r\n", session->currentdir);
    send(session->fd, reply, strlen(reply), 0);
    free(reply);
    return RT_EOK;
}

static int cdup_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    if(build_full_path(session->currentdir, sizeof(session->currentdir), "..") != RT_EOK)
        return -RT_ERROR;

    char *reply = NULL;
    DIR *dir = opendir(session->currentdir);
    if (dir == NULL)
    {
        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 directory \"%s\" can't open.\r\n", session->currentdir);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        return RT_EOK;
    }

    closedir(dir);

    reply = malloc(1024);
    if(reply == NULL)
        return -RT_ERROR;

    snprintf(reply, 1024, "250 Changed to directory \"%s\"\r\n", session->currentdir);
    send(session->fd, reply, strlen(reply), 0);
    free(reply);
    return RT_EOK;
}

static int mkd_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = NULL;
    if(session->is_anonymous)
    {
        reply = "550 Permission denied.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    char path[260];
    snprintf(path, sizeof(path), "%s", session->currentdir);
    if(build_full_path(path, sizeof(path), cmd_param) != RT_EOK)
        return -RT_ERROR;

    reply = malloc(1024);
    if(reply == NULL)
        return -RT_ERROR;

    if(ftp_create_dir(path) != RT_EOK)
        snprintf(reply, 1024, "550 directory \"%s\" create error.\r\n", path);
    else
        snprintf(reply, 1024, "257 directory \"%s\" successfully created.\r\n", path);

    send(session->fd, reply, strlen(reply), 0);
    free(reply);
    return RT_EOK;
}

static int rmd_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = NULL;
    if(session->is_anonymous)
    {
        reply = "550 Permission denied.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    char path[260];
    snprintf(path, sizeof(path), "%s", session->currentdir);
    if(build_full_path(path, sizeof(path), cmd_param) != RT_EOK)
        return -RT_ERROR;

    reply = malloc(1024);
    if(reply == NULL)
        return -RT_ERROR;

    if(unlink(path) != 0)
        snprintf(reply, 1024, "550 directory \"%s\" delete error.\r\n", path);
    else
        snprintf(reply, 1024, "257 directory \"%s\" successfully deleted.\r\n", path);

    send(session->fd, reply, strlen(reply), 0);
    free(reply);
    return RT_EOK;
}

static int dele_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = NULL;
    if(session->is_anonymous)
    {
        reply = "550 Permission denied.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    char path[260];
    snprintf(path, sizeof(path), "%s", session->currentdir);
    if(build_full_path(path, sizeof(path), cmd_param) != RT_EOK)
        return -RT_ERROR;

    reply = malloc(1024);
    if(reply == NULL)
        return -RT_ERROR;

    if(unlink(path) != 0)
        snprintf(reply, 1024, "550 file \"%s\" delete error.\r\n", path);
    else
        snprintf(reply, 1024, "250 file \"%s\" successfully deleted.\r\n", path);

    send(session->fd, reply, strlen(reply), 0);
    free(reply);
    return RT_EOK;
}

static int size_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = NULL;
    char path[260];
    snprintf(path, sizeof(path), "%s", session->currentdir);
    if(build_full_path(path, sizeof(path), cmd_param) != RT_EOK)
        return -RT_ERROR;

    struct stat s;
    memset(&s, 0, sizeof(struct stat));
    if(stat(path, &s) != 0)
    {
        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 \"%s\" : not a regular file\r\n", path);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        return RT_EOK;
    }

    if(!S_ISREG(s.st_mode))
    {
        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 \"%s\" : not a regular file\r\n", path);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        return RT_EOK;
    }

    reply = malloc(1024);
    if(reply == NULL)
        return -RT_ERROR;

    snprintf(reply, 1024, "213 %ld\r\n", s.st_size);
    send(session->fd, reply, strlen(reply), 0);
    free(reply);
    return RT_EOK;
}

static int rest_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = NULL;

    int offset = atoi(cmd_param);
    if(offset < 0)
    {
        reply = "504 invalid parameter.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        session->offset = 0;
        return RT_EOK;
    }

    reply = "350 Send RETR or STOR to start transfert.\r\n";
    send(session->fd, reply, strlen(reply), 0);
    session->offset = offset;
    return RT_EOK;
}

static int retr_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    char *reply = NULL;
    if (session->port_pasv_fd < 0)
    {
        reply = "502 Not Implemented.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        session->offset = 0;
        return RT_EOK;
    }

    char path[260];
    snprintf(path, sizeof(path), "%s", session->currentdir);
    if(build_full_path(path, sizeof(path), cmd_param) != RT_EOK)
        return -RT_ERROR;

    int fd = open(path, O_RDONLY);
    if(fd < 0)
    {
        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 \"%s\" : not a regular file\r\n", path);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        session->offset = 0;
        return RT_EOK;
    }

    int rc = -RT_ERROR;
    int file_size = 0;
    do
    {
        file_size = lseek(fd, 0, SEEK_END);
        lseek(fd, 0, SEEK_SET);
        if(file_size <= 0)
            break;

        rc = RT_EOK;
    }while(0);

    if(rc != RT_EOK)
    {
        close(fd);

        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 \"%s\" : not a regular file\r\n", path);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        session->offset = 0;
        return RT_EOK;
    }

    reply = malloc(4096);
    if(reply == NULL)
    {
        close(fd);
        return -RT_ERROR;
    }

    if((session->offset > 0) && (session->offset < file_size))
    {
        lseek(fd, session->offset, SEEK_SET);
        snprintf(reply, 4096, "150 Opening binary mode data connection for \"%s\" (%d/%d bytes).\r\n",
                 path, file_size - session->offset, file_size);
    }
    else
    {
        snprintf(reply, 4096, "150 Opening binary mode data connection for \"%s\" (%d bytes).\r\n",
                 path, file_size);
    }
    send(session->fd, reply, strlen(reply), 0);

    int recv_bytes = 0;
    int result = RT_EOK;
    while((recv_bytes = read(fd, reply, 4096)) > 0)
    {
        if(send(session->port_pasv_fd, reply, recv_bytes, 0) != recv_bytes)
        {
            result = -RT_ERROR;
            break;
        }
    }

    free(reply);
    close(fd);
    close(session->port_pasv_fd);
    session->port_pasv_fd = -1;

    if(result != RT_EOK)
        return -RT_ERROR;

    reply = "226 Finished.\r\n";
    send(session->fd, reply, strlen(reply), 0);
    session->offset = 0;
    return RT_EOK;
}

static int stor_cmd_receive(int socket, uint8_t *buf, int bufsz, int timeout)
{
    if((socket < 0) || (buf == NULL) || (bufsz <= 0) || (timeout <= 0))
        return -RT_ERROR;

    int len = 0;
    int rc = 0;
    fd_set rset;
    struct timeval tv;

    FD_ZERO(&rset);
    FD_SET(socket, &rset);
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    while(bufsz > 0)
    {
        rc = select(socket + 1, &rset, NULL, NULL, &tv);
        if(rc <= 0) {
            break;
        }


        rc = recv(socket, buf + len, bufsz, MSG_DONTWAIT);
        if (rc <= 0) {
            if (rc < 0) {
                if (errno == ENOTCONN) {
                    rc = 0;
                }
            }

            break;
        }

        len += rc;
        bufsz -= rc;

        tv.tv_sec = 3;
        tv.tv_usec = 0;
        FD_ZERO(&rset);
        FD_SET(socket, &rset);
    }

    if(rc >= 0)
        rc = len;

    return rc;
}

static int stor_cmd_fn(struct ftp_session *session, char *cmd, char *cmd_param)
{
    session->offset = 0;

    char *reply = NULL;
    if(session->is_anonymous)
    {
        reply = "550 Permission denied.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    if (session->port_pasv_fd < 0)
    {
        reply = "502 Not Implemented.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    char path[260];
    snprintf(path, sizeof(path), "%s", session->currentdir);
    if(build_full_path(path, sizeof(path), cmd_param) != RT_EOK)
        return -RT_ERROR;

    int fd = open(path, O_CREAT | O_RDWR | O_TRUNC);
    if(fd < 0)
    {
        reply = malloc(1024);
        if(reply == NULL)
            return -RT_ERROR;

        snprintf(reply, 1024, "550 Cannot open \"%s\" for writing.\r\n", path);
        send(session->fd, reply, strlen(reply), 0);
        free(reply);
        return RT_EOK;
    }

    reply = malloc(4096);
    if(reply == NULL)
    {
        close(fd);
        return -RT_ERROR;
    }

    snprintf(reply, 4096, "150 Opening binary mode data connection for \"%s\".\r\n", path);
    send(session->fd, reply, strlen(reply), 0);

    int result = RT_EOK;
    int timeout = 3000;
    while(1)
    {
        int recv_bytes = stor_cmd_receive(session->port_pasv_fd, (uint8_t *)reply, 4096, timeout);
        if(recv_bytes < 0)
        {
            result = -RT_ERROR;
            break;
        }
        if(recv_bytes == 0)
            break;
        if(write(fd, reply, recv_bytes) != recv_bytes)
        {
            result = -RT_ERROR;
            break;
        }
        fsync(fd);

        timeout = 3000;
    }

    free(reply);
    close(fd);
    close(session->port_pasv_fd);
    session->port_pasv_fd = -1;

    if(result != RT_EOK)
        return -RT_ERROR;

    reply = "226 Finished.\r\n";
    send(session->fd, reply, strlen(reply), 0);
    return RT_EOK;
}

static struct ftp_session_cmd session_cmds[] =
{
    {"PORT", port_cmd_fn},
    {"PWD", pwd_cmd_fn},
    {"XPWD", pwd_cmd_fn},
    {"TYPE", type_cmd_fn},
    {"SYST", syst_cmd_fn},
    {"QUIT", quit_cmd_fn},
    {"LIST", list_cmd_fn},
    {"NLST", nlist_cmd_fn},
    {"CWD", cwd_cmd_fn},
    {"CDUP", cdup_cmd_fn},
    {"MKD", mkd_cmd_fn},
    {"RMD", rmd_cmd_fn},
    {"DELE", dele_cmd_fn},
    {"SIZE", size_cmd_fn},
    {"REST", rest_cmd_fn},
    {"RETR", retr_cmd_fn},
    {"STOR", stor_cmd_fn}
};

int ftp_session_cmd_process(struct ftp_session *session, char *cmd, char *cmd_param)
{
    int array_cnt = sizeof(session_cmds) / sizeof(session_cmds[0]);
    struct ftp_session_cmd *session_cmd = NULL;

    for (int i = 0; i < array_cnt; i++)
    {
        if(strstr(cmd, session_cmds[i].cmd) == cmd)
        {
            session_cmd = &session_cmds[i];
            break;
        }
    }

    if(session_cmd == NULL)
    {
        char *reply = "502 Not Implemented.\r\n";
        send(session->fd, reply, strlen(reply), 0);
        return RT_EOK;
    }

    int result = session_cmd->cmd_fn(session, cmd, cmd_param);

    return result;
}
