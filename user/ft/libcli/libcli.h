#ifndef __LIBCLI_H__
#define __LIBCLI_H__

// vim:sw=4 tw=120 et

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#define CLI_OK                  0
#define CLI_ERROR               -1
#define CLI_QUIT                -2
#define CLI_ERROR_ARG           -3

#define MAX_HISTORY             5  /* testing, used to be 256 */
#define HISTORY_CMD_LEN         128

#define PRIVILEGE_UNPRIVILEGED  0
#define PRIVILEGE_PRIVILEGED    15
#define MODE_ANY                -1
#define MODE_EXEC               0
#define MODE_CONFIG             1

#define LIBCLI_HAS_ENABLE       1

#define PRINT_PLAIN             0
#define PRINT_FILTERED          0x01
#define PRINT_BUFFERED          0x02

#define CLI_MAX_LINE_LENGTH     64
#define CLI_MAX_LINE_WORDS      16
#define CLI_MAX_CMD_NAME_LEN    32

#define CLI_LOOP_CTRL_CONTINUE 1
#define CLI_LOOP_CTRL_BREAK 2

enum cli_states {
    CLI_STATE_LOGIN,
    CLI_STATE_PASSWORD,
    CLI_STATE_NORMAL,
    CLI_STATE_ENABLE_PASSWORD,
    CLI_STATE_ENABLE
};

struct cli_def {
    int completion_callback;
    struct cli_command *commands;
    int (*auth_callback)(const char *, const char *);
    int (*regular_callback)(struct cli_def *cli);
    int (*enable_callback)(const char *);
    char *banner;
    struct unp *users;
    char *enable_password;
    char history[MAX_HISTORY][HISTORY_CMD_LEN];
    char showprompt;
    char *promptchar;
    char *hostname;
    char *modestring;
    int privilege;
    int mode;
    enum cli_states state;
    struct cli_filter *filters;
    void (*print_callback)(struct cli_def *cli, const char *string);
    FILE *client;
    /* internal buffers */
    void *conn;
    void *service;
    struct timeval timeout_tm;
    time_t idle_timeout;
    int (*idle_timeout_callback)(struct cli_def *);
    time_t last_action;
    int telnet_protocol;
    void *user_context;
    int (*read_callback)(struct cli_def *cli, void *buf, const size_t count);
    int (*write_callback)(struct cli_def *cli, const void *buf, const size_t count);
};

struct cli_filter {
    int (*filter)(struct cli_def *cli, const char *string, void *data);
    void *data;
    struct cli_filter *next;
};

struct cli_command {
    char *command;
    int (*callback)(struct cli_def *, const char *, char **, int);
    unsigned int unique_len;
    char *help;
    int privilege;
    int mode;
    struct cli_command *next;
    struct cli_command *children;
    struct cli_command *parent;
};

struct cli_loop_ctx {
    char cmd[CLI_MAX_LINE_LENGTH];
    char username[64];
    int l, restore_cmd_l;
    int cursor, insertmode;
    int lastchar, is_telnet_option, skip, esc;
    signed int in_history;
    int sockfd;
};

int cli_init(struct cli_def *cli);
int cli_done(struct cli_def *cli);
void cli_register_command2(struct cli_def *cli, struct cli_command *cmd, struct cli_command *parent);
int cli_unregister_command(struct cli_def *cli, const char *command);
int cli_run_command(struct cli_def *cli, const char *command);
int cli_loop(struct cli_def *cli, int sockfd);
int cli_file(struct cli_def *cli, FILE *fh, int privilege, int mode);
void cli_set_auth_callback(struct cli_def *cli, int (*auth_callback)(const char *, const char *));
void cli_set_enable_callback(struct cli_def *cli, int (*enable_callback)(const char *));
void cli_allow_enable(struct cli_def *cli, const char *password);
void cli_deny_user(struct cli_def *cli, const char *username);
void cli_set_banner(struct cli_def *cli, char *banner);
void cli_set_hostname(struct cli_def *cli, char *hostname);
void cli_set_promptchar(struct cli_def *cli, char *promptchar);
void cli_set_modestring(struct cli_def *cli, char *modestring);
int cli_set_privilege(struct cli_def *cli, int privilege);
int cli_set_configmode(struct cli_def *cli, int mode, const char *config_desc);
void cli_reprompt(struct cli_def *cli);
void cli_regular(struct cli_def *cli, int (*callback)(struct cli_def *cli));
void cli_regular_interval(struct cli_def *cli, int seconds);
void cli_print(struct cli_def *cli, const char *format, ...) __attribute__((format (printf, 2, 3)));
void cli_bufprint(struct cli_def *cli, const char *format, ...) __attribute__((format (printf, 2, 3)));
void cli_vabufprint(struct cli_def *cli, const char *format, va_list ap);
void cli_error(struct cli_def *cli, const char *format, ...) __attribute__((format (printf, 2, 3)));
void cli_print_callback(struct cli_def *cli, void (*callback)(struct cli_def *, const char *));
void cli_free_history(struct cli_def *cli);
void cli_set_idle_timeout(struct cli_def *cli, unsigned int seconds);
void cli_set_idle_timeout_callback(struct cli_def *cli, unsigned int seconds, int (*callback)(struct cli_def *));
void cli_read_callback(struct cli_def *cli, int (*callback)(struct cli_def *cli, void *buf, size_t count));
void cli_write_callback(struct cli_def *cli, int (*callback)(struct cli_def *cli, const void *buf, size_t count));

void cli_loop_start_new_command(struct cli_def *cli, struct cli_loop_ctx *ctx);
void cli_loop_show_prompt(struct cli_def *cli, struct cli_loop_ctx *ctx);
int cli_loop_read_next_char(struct cli_def *cli, struct cli_loop_ctx *ctx, unsigned char *c);
int cli_loop_process_char(struct cli_def *cli, struct cli_loop_ctx *ctx, unsigned char c);
int cli_loop_process_cmd(struct cli_def *cli, struct cli_loop_ctx *ctx);

// Enable or disable telnet protocol negotiation.
// Note that this is enabled by default and must be changed before cli_loop() is run.
void cli_telnet_protocol(struct cli_def *cli, int telnet_protocol);

// Set/get user context
void cli_set_context(struct cli_def *cli, void *context);
void *cli_get_context(struct cli_def *cli);

#ifdef __cplusplus
}
#endif

#endif
