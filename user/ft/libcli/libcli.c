#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#endif

#define CRYPTECH_NO_CRYPT
#define CRYPTECH_NO_MEMORY_H
//#define CRYPTECH_NO_FDOPEN
//#define CRYPTECH_NO_SELECT
#define CRYPTECH_NO_REGEXP

#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#ifndef CRYPTECH_NO_MEMORY_H
#include <memory.h>
#endif /* CRYPTECH_NO_MEMORY_H */
#include <string.h>
#include <strings.h>
#include <unistd.h>
#if !defined(WIN32) && !defined(CRYPTECH_NO_REGEXP)
#include <regex.h>
#endif
#include "libcli.h"

// vim:sw=4 tw=120 et

#ifdef __GNUC__
# define UNUSED(d) d __attribute__ ((unused))
#else
# define UNUSED(d) d
#endif

#define MATCH_REGEX     1
#define MATCH_INVERT    2

#ifdef CRYPTECH_NO_REGEXP
/*
 * Dummy definitions to allow compilation on Cryptech STM32 MCU
 */
int regex_dummy() {return 0;}
#define regfree(...) regex_dummy()
#define regexec(...) regex_dummy()
#define regcomp(...) regex_dummy()
#define regex_t int
#define REG_NOSUB       0
#define REG_EXTENDED    0
#define REG_ICASE       0
#endif /* CRYPTECH_NO_REGEXP */

struct unp {
    char *username;
    char *password;
    struct unp *next;
};

struct cli_filter_cmds
{
    const char *cmd;
    const char *help;
};

static struct cli_filter_cmds filter_cmds[] =
{
    /* cryptech: removed all filters, was using dynamic memory and seemed an unneccessarily big attack surface */
    { NULL, NULL}
};

static int isspace(int c)
{
    /* isspace() not provided with gcc-arm-none-eabi 4.9.3 */
    return (c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v');
}

static ssize_t _write(struct cli_def *cli, struct cli_loop_ctx *ctx, const void *buf, size_t count)
{
    size_t written = 0;
    ssize_t thisTime = 0;

    if (! cli->write_callback && ! ctx->sockfd)
        return -1;

    while (count != written)
    {
        if (cli->write_callback)
	    thisTime = cli->write_callback(cli, (char*)buf + written, count - written);
	else
	    thisTime = write(ctx->sockfd, (char*)buf + written, count - written);
        if (thisTime == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }
        written += thisTime;
    }
    return written;
}

/* cryptech: made this function use a static buffer instead of dynamically allocated memory
   for command "show counters" this function is called on command "counters", so the string
   is built backwards while there still are parent nodes.
 */
char *cli_command_name(struct cli_def *cli, struct cli_command *command)
{
    static char buf[CLI_MAX_CMD_NAME_LEN];
    buf[CLI_MAX_CMD_NAME_LEN - 1] = 0;
    int idx = CLI_MAX_CMD_NAME_LEN - 1;

    while (command)
    {
      /* XXX what to do if there is no room left in buf? */
      if (idx > (int) strlen(command->command) + 1)
        {
	    if (buf[idx])
	    {
	        /* this is not the first command, need to add a space */
		buf[--idx] = ' ';
	    }
	    idx -= strlen(command->command);
	    memcpy(buf + idx, command->command, strlen(command->command));
        }
        command = command->parent;
    }

    return buf + idx;
}

void cli_set_auth_callback(struct cli_def *cli, int (*auth_callback)(const char *, const char *))
{
    cli->auth_callback = auth_callback;
}

void cli_set_enable_callback(struct cli_def *cli, int (*enable_callback)(const char *))
{
    cli->enable_callback = enable_callback;
}


/* cryptech: removed unused function: cli_allow_user */
/* cryptech: removed unused function: cli_allow_enable */
/* cryptech: removed unused function: cli_deny_user */

void cli_set_banner(struct cli_def *cli, char *banner)
{
    cli->banner = banner;
}

void cli_set_hostname(struct cli_def *cli, char *hostname)
{
    cli->hostname = hostname;
}

void cli_set_promptchar(struct cli_def *cli, char *promptchar)
{
    cli->promptchar = promptchar;
}

static int cli_build_shortest(struct cli_def *cli, struct cli_command *commands)
{
    struct cli_command *c, *p;
    char *cp, *pp;
    unsigned len;

    for (c = commands; c; c = c->next)
    {
        c->unique_len = strlen(c->command);
        if ((c->mode != MODE_ANY && c->mode != cli->mode) || c->privilege > cli->privilege)
            continue;

        c->unique_len = 1;
        for (p = commands; p; p = p->next)
        {
            if (c == p)
                continue;

            if ((p->mode != MODE_ANY && p->mode != cli->mode) || p->privilege > cli->privilege)
                continue;

            cp = c->command;
            pp = p->command;
            len = 1;

            while (*cp && *pp && *cp++ == *pp++)
                len++;

            if (len > c->unique_len)
                c->unique_len = len;
        }

        if (c->children)
            cli_build_shortest(cli, c->children);
    }

    return CLI_OK;
}

int cli_set_privilege(struct cli_def *cli, int priv)
{
    int old = cli->privilege;
    cli->privilege = priv;

    if (priv != old)
    {
        static char priv_prompt[] = "# ", nopriv_prompt[] = "> ";
        cli_set_promptchar(cli, priv == PRIVILEGE_PRIVILEGED ? priv_prompt : nopriv_prompt);
        cli_build_shortest(cli, cli->commands);
    }

    return old;
}

void cli_set_modestring(struct cli_def *cli, char *modestring)
{
    cli->modestring = modestring;
}

int cli_set_configmode(struct cli_def *cli, int mode, const char *config_desc)
{
    int old = cli->mode;
    static char string[64];
    cli->mode = mode;

    if (mode != old)
    {
        if (!cli->mode)
        {
            // Not config mode
            cli_set_modestring(cli, NULL);
        }
        else if (config_desc && *config_desc)
        {
            snprintf(string, sizeof(string), "(config-%s)", config_desc);
            cli_set_modestring(cli, string);
        }
        else
        {
	    snprintf(string, sizeof(string), "(config)");
            cli_set_modestring(cli, string);
        }

        cli_build_shortest(cli, cli->commands);
    }

    return old;
}

void cli_register_command2(struct cli_def *cli, struct cli_command *cmd, struct cli_command *parent)
{
    struct cli_command *p;

    if (parent)
    {
        cmd->parent = parent;
        if (!parent->children)
        {
            parent->children = cmd;
        }
        else
        {
            for (p = parent->children; p && p->next; p = p->next);
            if (p) p->next = cmd;
        }
    }
    else
    {
        if (!cli->commands)
        {
            cli->commands = cmd;
        }
        else
        {
	    for (p = cli->commands; p && p->next; p = p->next);
            if (p) p->next = cmd;
        }
    }
}

/* cryptech: removed unused function cli_free_command */

int cli_unregister_command(struct cli_def *cli, const char *command)
{
    struct cli_command *c, *p = NULL;

    if (!command) return -1;
    if (!cli->commands) return CLI_OK;

    for (c = cli->commands; c; c = c->next)
    {
        if (strcmp(c->command, command) == 0)
        {
            if (p)
                p->next = c->next;
            else
                cli->commands = c->next;

            return CLI_OK;
        }
        p = c;
    }

    return CLI_OK;
}

int cli_show_help(struct cli_def *cli, struct cli_command *c)
{
    struct cli_command *p;

    for (p = c; p; p = p->next)
    {
        if (p->command && p->callback && cli->privilege >= p->privilege &&
            (p->mode == cli->mode || p->mode == MODE_ANY))
        {
            cli_error(cli, "  %-35s %s", cli_command_name(cli, p), (p->help != NULL ? p->help : ""));
        }

        if (p->children)
            cli_show_help(cli, p->children);
    }

    return CLI_OK;
}

int cli_int_enable(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    if (cli->privilege == PRIVILEGE_PRIVILEGED)
        return CLI_OK;

    if (!cli->enable_password && !cli->enable_callback)
    {
        /* no password required, set privilege immediately */
        cli_set_privilege(cli, PRIVILEGE_PRIVILEGED);
        cli_set_configmode(cli, MODE_EXEC, NULL);
    }
    else
    {
        /* require password entry */
        cli->state = CLI_STATE_ENABLE_PASSWORD;
    }

    return CLI_OK;
}

int cli_int_disable(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    cli_set_privilege(cli, PRIVILEGE_UNPRIVILEGED);
    cli_set_configmode(cli, MODE_EXEC, NULL);
    return CLI_OK;
}

int cli_int_help(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    cli_error(cli, "\nCommands available:");
    cli_show_help(cli, cli->commands);
    return CLI_OK;
}

int cli_int_history(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    int i;

    cli_error(cli, "\nCommand history:");
    for (i = 0; i < MAX_HISTORY; i++)
    {
        if (cli->history[i][0])
            cli_error(cli, "%3d. %s", i, cli->history[i]);
    }

    return CLI_OK;
}

int cli_int_quit(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    cli_set_privilege(cli, PRIVILEGE_UNPRIVILEGED);
    cli_set_configmode(cli, MODE_EXEC, NULL);
    return CLI_QUIT;
}

int cli_int_exit(struct cli_def *cli, const char *command, char *argv[], int argc)
{
    if (cli->mode == MODE_EXEC)
        return cli_int_quit(cli, command, argv, argc);

    if (cli->mode > MODE_CONFIG)
        cli_set_configmode(cli, MODE_CONFIG, NULL);
    else
        cli_set_configmode(cli, MODE_EXEC, NULL);

    cli->service = NULL;
    return CLI_OK;
}

int cli_int_idle_timeout(struct cli_def *cli)
{
    cli_print(cli, "Idle timeout");
    return CLI_QUIT;
}

int cli_int_configure_terminal(struct cli_def *cli, UNUSED(const char *command), UNUSED(char *argv[]), UNUSED(int argc))
{
    cli_set_configmode(cli, MODE_CONFIG, NULL);
    return CLI_OK;
}

int cli_init(struct cli_def *cli)
{
    static struct cli_command cmd_int_help_s = {(char *) "help", cli_int_help, 0,
						(char *) "Show available commands",
						PRIVILEGE_UNPRIVILEGED, MODE_ANY, NULL, NULL, NULL};
    static struct cli_command cmd_int_quit_s = {(char *) "quit", cli_int_quit, 0,
						(char *) "Disconnect",
						PRIVILEGE_UNPRIVILEGED, MODE_ANY, NULL, NULL, NULL};
    static struct cli_command cmd_int_logout_s = {(char *) "logout", cli_int_quit, 0,
						  (char *) "Disconnect",
						  PRIVILEGE_UNPRIVILEGED, MODE_ANY, NULL, NULL, NULL};
    static struct cli_command cmd_int_exit_s = {(char *) "exit", cli_int_exit, 0,
						(char *) "Exit from current mode",
						PRIVILEGE_UNPRIVILEGED, MODE_ANY, NULL, NULL, NULL};
    static struct cli_command cmd_int_history_s = {(char *) "history", cli_int_history, 0,
						   (char *) "Show a list of previously run commands",
						   PRIVILEGE_UNPRIVILEGED, MODE_ANY, NULL, NULL, NULL};
    static struct cli_command cmd_int_enable_s = {(char *) "enable", cli_int_enable, 0,
						  (char *) "Turn on privileged commands",
						  PRIVILEGE_UNPRIVILEGED, MODE_EXEC, NULL, NULL, NULL};
    static struct cli_command cmd_int_disable_s = {(char *) "disable", cli_int_disable, 0,
						   (char *) "Turn off privileged commands",
						   PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL, NULL, NULL};
    static struct cli_command cmd_int_configure_s = {(char *) "configure", NULL, 0,
						     (char *) "Enter configuration mode",
						     PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL, NULL, NULL};
    static struct cli_command cmd_int_configure_terminal_s = {(char *) "terminal", cli_int_configure_terminal, 0,
							      (char *) "Configure from the terminal",
							      PRIVILEGE_PRIVILEGED, MODE_EXEC, NULL, NULL, NULL};

    cli->telnet_protocol = 1;

    cli_register_command2(cli, &cmd_int_help_s, NULL);
    cli_register_command2(cli, &cmd_int_quit_s, NULL);
    cli_register_command2(cli, &cmd_int_logout_s, NULL);
    cli_register_command2(cli, &cmd_int_exit_s, NULL);
    cli_register_command2(cli, &cmd_int_history_s, NULL);
    cli_register_command2(cli, &cmd_int_enable_s, NULL);
    cli_register_command2(cli, &cmd_int_disable_s, NULL);

    cli_register_command2(cli, &cmd_int_configure_s, NULL);
    cli_register_command2(cli, &cmd_int_configure_terminal_s, &cmd_int_configure_s);

    cli->privilege = cli->mode = -1;
    cli_set_privilege(cli, PRIVILEGE_UNPRIVILEGED);
    cli_set_configmode(cli, MODE_EXEC, 0);

    return 1;
}

/* cryptech: removed unused function cli_unregister_all */

int cli_done(struct cli_def *cli)
{
    return CLI_OK;
}

static int cli_add_history(struct cli_def *cli, const char *cmd)
{
    int i;
    for (i = 0; i < MAX_HISTORY; i++)
    {
        if (!cli->history[i][0])
        {
            if (i == 0 || strcasecmp(cli->history[i-1], cmd))
	    {
	      strncpy(cli->history[i], cmd, HISTORY_CMD_LEN - 1);
	      cli->history[i][HISTORY_CMD_LEN - 1] = 0;
	    }
            return CLI_OK;
        }
    }
    // No space found, drop one off the beginning of the list
    for (i = 0; i < MAX_HISTORY-1; i++)
        memcpy(cli->history[i], cli->history[i+1], HISTORY_CMD_LEN);
    strncpy(cli->history[MAX_HISTORY-1], cmd, HISTORY_CMD_LEN - 1);
    cli->history[MAX_HISTORY-1][HISTORY_CMD_LEN - 1] = 0;
    return CLI_OK;
}

/* cryptech: removed unused cli_free_history */

static int cli_parse_line(const char *line, char *words[], int max_words)
{
    int nwords = 0;
    const char *p = line;
    const char *word_start = 0;
    int inquote = 0;
    static char buf[CLI_MAX_LINE_LENGTH];
    char *ptr = buf;

    while (*p)
    {
      if (!isspace((unsigned char) *p))
        {
            word_start = p;
            break;
        }
        p++;
    }

    memset(buf, 0, sizeof(buf));

    while (nwords < max_words - 1)
    {
      if (!*p || *p == inquote || (word_start && !inquote && (isspace((unsigned char) *p) || *p == '|')))
        {
            if (word_start)
              {
                  int len = p - word_start;

		  if (len > 1)
		    {
		      if ((ptr + len + 1) > buf + sizeof(buf) - 1) break;

		      memcpy(ptr, word_start, len);
		      words[nwords++] = ptr;
		      ptr += len;
		      ptr++; /* NULL terminate through memset above */
		    }
              }

            if (!*p)
                break;

            if (inquote)
                p++; /* skip over trailing quote */

            inquote = 0;
            word_start = 0;
        }
        else if (*p == '"' || *p == '\'')
        {
            inquote = *p++;
            word_start = p;
        }
        else
        {
            if (!word_start)
            {
                if (*p == '|')
                {
		    if ((ptr + 1 + 1) > buf + sizeof(buf) - 1) break;

		    *ptr = '|';
		    words[nwords++] = ptr;
		    ptr += strlen("|");
		    ptr++; /* NULL terminate through memset above */
                }
                else if (!isspace((unsigned char) *p))
                    word_start = p;
            }

            p++;
        }
    }

    return nwords;
}

/* cryptech: removed unused function join_words */

static int cli_find_command(struct cli_def *cli, struct cli_command *commands, int num_words, char *words[],
                            int start_word, int filters[])
{
    struct cli_command *c, *again_config = NULL, *again_any = NULL;
    int c_words = num_words;

    if (filters[0])
        c_words = filters[0];

    // Deal with ? for help
    if (!words[start_word])
        return CLI_ERROR;

    if (words[start_word][strlen(words[start_word]) - 1] == '?')
    {
        int l = strlen(words[start_word])-1;

        if (commands->parent && commands->parent->callback)
            cli_error(cli, "%-20s %s", cli_command_name(cli, commands->parent),
                      (commands->parent->help != NULL ? commands->parent->help : ""));

        for (c = commands; c; c = c->next)
        {
            if (strncasecmp(c->command, words[start_word], l) == 0
                && (c->callback || c->children)
                && cli->privilege >= c->privilege
                && (c->mode == cli->mode || c->mode == MODE_ANY))
                    cli_error(cli, "  %-20s %s", c->command, (c->help != NULL ? c->help : ""));
        }

        return CLI_OK;
    }

    for (c = commands; c; c = c->next)
    {
        if (cli->privilege < c->privilege)
            continue;

        if (strncasecmp(c->command, words[start_word], c->unique_len))
            continue;

        if (strncasecmp(c->command, words[start_word], strlen(words[start_word])))
            continue;

        AGAIN:
        if (c->mode == cli->mode || (c->mode == MODE_ANY && again_any != NULL))
        {
            int rc = CLI_OK;

            // Found a word!
            if (!c->children)
            {
                // Last word
                if (!c->callback)
                {
                    cli_error(cli, "No callback for \"%s\"", cli_command_name(cli, c));
                    return CLI_ERROR;
                }
            }
            else
            {
                if (start_word == c_words - 1)
                {
                    if (c->callback)
                        goto CORRECT_CHECKS;

                    cli_error(cli, "Incomplete command");
                    return CLI_ERROR;
                }
                rc = cli_find_command(cli, c->children, num_words, words, start_word + 1, filters);
                if (rc == CLI_ERROR_ARG)
                {
                    if (c->callback)
                    {
                        rc = CLI_OK;
                        goto CORRECT_CHECKS;
                    }
                    else
                    {
                        cli_error(cli, "Invalid %s \"%s\"", commands->parent ? "argument" : "command",
                                  words[start_word]);
                    }
                }
                return rc;
            }

	    CORRECT_CHECKS:

            if (!c->callback)
            {
                cli_error(cli, "Internal server error processing \"%s\"", cli_command_name(cli, c));
                return CLI_ERROR;
            }

	    /* cryptech: removed filter checking here */

            if (rc == CLI_OK)
                rc = c->callback(cli, cli_command_name(cli, c), words + start_word + 1, c_words - start_word - 1);

	    /* cryptech: removed filter cleanup here */

            return rc;
        }
        else if (cli->mode > MODE_CONFIG && c->mode == MODE_CONFIG)
        {
            // command matched but from another mode,
            // remember it if we fail to find correct command
            again_config = c;
        }
        else if (c->mode == MODE_ANY)
        {
            // command matched but for any mode,
            // remember it if we fail to find correct command
            again_any = c;
        }
    }

    // drop out of config submode if we have matched command on MODE_CONFIG
    if (again_config)
    {
        c = again_config;
        cli_set_configmode(cli, MODE_CONFIG, NULL);
        goto AGAIN;
    }
    if (again_any)
    {
        c = again_any;
        goto AGAIN;
    }

    if (start_word == 0)
        cli_error(cli, "Invalid %s \"%s\"", commands->parent ? "argument" : "command", words[start_word]);

    return CLI_ERROR_ARG;
}

int cli_run_command(struct cli_def *cli, const char *command)
{
    int r;
    unsigned int num_words, i, f;
    char *words[CLI_MAX_LINE_WORDS] = {0};
    int filters[CLI_MAX_LINE_WORDS] = {0};

    if (!command) return CLI_ERROR;
    while (isspace((int) *command)) {
        command++;
    }

    if (!*command) return CLI_OK;

    num_words = cli_parse_line(command, words, CLI_MAX_LINE_WORDS);
    for (i = f = 0; i < num_words && f < CLI_MAX_LINE_WORDS - 1; i++)
    {
        if (words[i][0] == '|')
        filters[f++] = i;
    }

    filters[f] = 0;

    if (num_words)
        r = cli_find_command(cli, cli->commands, num_words, words, 0, filters);
    else
        r = CLI_ERROR;

    if (r == CLI_QUIT)
        return r;

    return CLI_OK;
}

static int cli_get_completions(struct cli_def *cli, const char *command, char **completions, int max_completions)
{
    struct cli_command *c;
    struct cli_command *n;
    int num_words, i, k=0;
    char *words[CLI_MAX_LINE_WORDS] = {0};
    int filter = 0;

    if (!command) return 0;
    while (isspace((unsigned char) *command))
        command++;

    num_words = cli_parse_line(command, words, sizeof(words)/sizeof(words[0]));
    if (!command[0] || command[strlen(command)-1] == ' ')
        num_words++;

    if (!num_words)
        goto out;

    for (i = 0; i < num_words; i++)
    {
        if (words[i] && words[i][0] == '|')
            filter = i;
    }

    if (filter) // complete filters
    {
        unsigned len = 0;

        if (filter < num_words - 1) // filter already completed
            goto out;

        if (filter == num_words - 1)
            len = strlen(words[num_words-1]);

        for (i = 0; filter_cmds[i].cmd && k < max_completions; i++)
        {
            if (!len || (len < strlen(filter_cmds[i].cmd) && !strncmp(filter_cmds[i].cmd, words[num_words - 1], len)))
                completions[k++] = (char *)filter_cmds[i].cmd;
        }

        completions[k] = NULL;
        goto out;
    }

    for (c = cli->commands, i = 0; c && i < num_words && k < max_completions; c = n)
    {
        n = c->next;

        if (cli->privilege < c->privilege)
            continue;

        if (c->mode != cli->mode && c->mode != MODE_ANY)
            continue;

        if (words[i] && strncasecmp(c->command, words[i], strlen(words[i])))
            continue;

        if (i < num_words - 1)
        {
            if (strlen(words[i]) < c->unique_len)
                continue;

            n = c->children;
            i++;
            continue;
        }

        completions[k++] = c->command;
    }

out:

    return k;
}

static void cli_clear_line(struct cli_def *cli, struct cli_loop_ctx *ctx)
{
    int i;
    if (ctx->cursor < ctx->l)
    {
        for (i = 0; i < (ctx->l - ctx->cursor); i++)
	  _write(cli, ctx, " ", 1);
    }
    for (i = 0; i < ctx->l; i++)
        ctx->cmd[i] = '\b';
    _write(cli, ctx, ctx->cmd, i);
    for (i = 0; i < ctx->l; i++)
        ctx->cmd[i] = ' ';
    _write(cli, ctx, ctx->cmd, i);
    for (i = 0; i < ctx->l; i++)
        ctx->cmd[i] = '\b';
    _write(cli, ctx, ctx->cmd, i);
    memset(ctx->cmd, 0, i);
    ctx->l = ctx->cursor = 0;
}

void cli_reprompt(struct cli_def *cli)
{
    if (!cli) return;
    cli->showprompt = 1;
}

static int pass_matches(const char *pass, const char *try)
{
    /* cryptech: removed DES mode here */
    return !strcmp(pass, try);
}

#define CTRL(c) (c - '@')

static int show_prompt(struct cli_def *cli, struct cli_loop_ctx *ctx)
{
    int len = 0;

    if (cli->hostname)
        len += _write(cli, ctx, cli->hostname, strlen(cli->hostname));

    if (cli->modestring)
        len += _write(cli, ctx, cli->modestring, strlen(cli->modestring));

    return len + _write(cli, ctx, cli->promptchar, strlen(cli->promptchar));
}

int cli_loop(struct cli_def *cli, int sockfd)
{
    unsigned char c;
    int n = 0;
    struct cli_loop_ctx ctx;

    memset(&ctx, 0, sizeof(ctx));
    ctx.insertmode = 1;
    ctx.sockfd = sockfd;

    cli_build_shortest(cli, cli->commands);
    cli->state = CLI_STATE_LOGIN;

    if (cli->telnet_protocol)
    {
        static const char *negotiate =
            "\xFF\xFB\x03"
            "\xFF\xFB\x01"
            "\xFF\xFD\x03"
            "\xFF\xFD\x01";
        _write(cli, &ctx, negotiate, strlen(negotiate));
    }

#ifndef CRYPTECH_NO_FDOPEN
#ifdef WIN32
    /*
     * OMG, HACK
     */
    if (!(cli->client = fdopen(_open_osfhandle(sockfd, 0), "w+")))
        return CLI_ERROR;
    cli->client->_file = sockfd;
#else
    if (!(cli->client = fdopen(sockfd, "w+")))
        return CLI_ERROR;
#endif
#endif /* CRYPTECH_NO_FDOPEN */

    setbuf(cli->client, NULL);
    if (cli->banner)
        cli_error(cli, "%s", cli->banner);

    /* start off in unprivileged mode */
    cli_set_privilege(cli, PRIVILEGE_UNPRIVILEGED);
    cli_set_configmode(cli, MODE_EXEC, NULL);

    /* no auth required? */
    if (!cli->users && !cli->auth_callback)
        cli->state = CLI_STATE_NORMAL;

    while (1)
    {
	cli_loop_start_new_command(cli, &ctx);

        while (1)
        {
	    cli_loop_show_prompt(cli, &ctx);

	    n = cli_loop_read_next_char(cli, &ctx, &c);
	    if (n == CLI_LOOP_CTRL_BREAK)
	      break;
	    if (n == CLI_LOOP_CTRL_CONTINUE)
	      continue;

	    n = cli_loop_process_char(cli, &ctx, c);
	    if (n == CLI_LOOP_CTRL_BREAK)
	      break;
	    if (n == CLI_LOOP_CTRL_CONTINUE)
	      continue;
        }


        if (ctx.l < 0) break;

	n = cli_loop_process_cmd(cli, &ctx);
	if (n == CLI_LOOP_CTRL_BREAK)
	  break;
    }

    fclose(cli->client);
    cli->client = 0;
    return CLI_OK;
}

void cli_loop_start_new_command(struct cli_def *cli, struct cli_loop_ctx *ctx)
{
    ctx->in_history = 0;
    ctx->lastchar = 0;

    cli->showprompt = 1;

    if (ctx->restore_cmd_l > 0)
    {
	ctx->l = ctx->cursor = ctx->restore_cmd_l;
	ctx->cmd[ctx->l] = 0;
	ctx->restore_cmd_l = 0;
    }
    else
    {
	memset(ctx->cmd, 0, CLI_MAX_LINE_LENGTH);
	ctx->l = 0;
	ctx->cursor = 0;
    }

}

int cli_loop_read_next_char(struct cli_def *cli, struct cli_loop_ctx *ctx, unsigned char *c)
{
    int n;
#ifndef CRYPTECH_NO_SELECT
    struct timeval tm;
    int sr;
    fd_set r;

    FD_ZERO(&r);
    FD_SET(ctx->sockfd, &r);
    memcpy(&tm, &cli->timeout_tm, sizeof(tm));

    if ((sr = select(ctx->sockfd + 1, &r, NULL, NULL, &tm)) < 0)
    {
	/* select error */
	if (errno == EINTR)
	    return CLI_LOOP_CTRL_CONTINUE;

	perror("select");
	ctx->l = -1;
	return CLI_LOOP_CTRL_BREAK;
    }

    if (sr == 0)
    {
	/* timeout every second */
	if (cli->regular_callback && cli->regular_callback(cli) != CLI_OK)
	{
	    ctx->l = -1;
	    return CLI_LOOP_CTRL_BREAK;
	}

	if (cli->idle_timeout)
	{
	    if (time(NULL) - cli->last_action >= cli->idle_timeout)
	    {
		if (cli->idle_timeout_callback)
		{
		    // Call the callback and continue on if successful
		    if (cli->idle_timeout_callback(cli) == CLI_OK)
		    {
			// Reset the idle timeout counter
			time(&cli->last_action);
			return CLI_LOOP_CTRL_CONTINUE;
		    }
		}
		// Otherwise, break out of the main loop
		ctx->l = -1;
		return CLI_LOOP_CTRL_BREAK;
	    }
	}

	memcpy(&tm, &cli->timeout_tm, sizeof(tm));
	return CLI_LOOP_CTRL_CONTINUE;
    }
#endif /* CRYPTECH_NO_SELECT */

    if (cli->read_callback)
    {
        if ((n = cli->read_callback(cli, c, (size_t) 1)) < 0)
	{
	    perror("read_callback");
	    ctx->l = -1;
	    return CLI_LOOP_CTRL_BREAK;
	}
	if (n == 0)
	    return CLI_LOOP_CTRL_CONTINUE;
    }
    else
    {
	if ((n = read(ctx->sockfd, c, 1)) < 0)
	{
	    if (errno == EINTR)
		return CLI_LOOP_CTRL_CONTINUE;

	    perror("read");
	    ctx->l = -1;
	    return CLI_LOOP_CTRL_BREAK;
	}
    }

    if (n == 0)
    {
	ctx->l = -1;
	return CLI_LOOP_CTRL_CONTINUE;
    }

    return 0;
}


void cli_loop_show_prompt(struct cli_def *cli, struct cli_loop_ctx *ctx)
{
    if (cli->showprompt)
    {
	if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
	    _write(cli, ctx, "\r\n", 2);

	switch (cli->state)
	{
	case CLI_STATE_LOGIN:
	    _write(cli, ctx, "Username: ", strlen("Username: "));
	    break;

	case CLI_STATE_PASSWORD:
	    _write(cli, ctx, "Password: ", strlen("Password: "));
	    break;

	case CLI_STATE_NORMAL:
	case CLI_STATE_ENABLE:
	    show_prompt(cli, ctx);
	    _write(cli, ctx, ctx->cmd, ctx->l);
	    if (ctx->cursor < ctx->l)
	    {
		int n = ctx->l - ctx->cursor;
		while (n--)
		    _write(cli, ctx, "\b", 1);
	    }
	    break;

	case CLI_STATE_ENABLE_PASSWORD:
	    _write(cli, ctx, "Password: ", strlen("Password: "));
	    break;

	}

	cli->showprompt = 0;
    }
}


/*
 * This function should be called once for every character received from the user.
 *
 * It will return CLI_LOOP_CTRL_BREAK if the command is now ready to be processed (or the
 * session should be terminated, and CLI_LOOP_CTRL_CONTINUE if it should be called again
 * for the next character received.
 */
int cli_loop_process_char(struct cli_def *cli, struct cli_loop_ctx *ctx, unsigned char c)
{
    if (ctx->skip)
    {
	ctx->skip--;
	return CLI_LOOP_CTRL_CONTINUE;
    }

    if (c == 255 && !ctx->is_telnet_option)
    {
	ctx->is_telnet_option++;
	return CLI_LOOP_CTRL_CONTINUE;
    }

    if (ctx->is_telnet_option)
    {
	if (c >= 251 && c <= 254)
	{
	    ctx->is_telnet_option = c;
	    return CLI_LOOP_CTRL_CONTINUE;
	}

	if (c != 255)
	{
	    ctx->is_telnet_option = 0;
	    return CLI_LOOP_CTRL_CONTINUE;
	}

	ctx->is_telnet_option = 0;
    }

    /* handle ANSI arrows */
    if (ctx->esc)
    {
	if (ctx->esc == '[')
	{
	    /* remap to readline control codes */
	    switch (c)
	    {
	    case 'A': /* Up */
		c = CTRL('P');
		break;

	    case 'B': /* Down */
		c = CTRL('N');
		break;

	    case 'C': /* Right */
		c = CTRL('F');
		break;

	    case 'D': /* Left */
		c = CTRL('B');
		break;

	    default:
		c = 0;
	    }

	    ctx->esc = 0;
	}
	else
	{
	    ctx->esc = (c == '[') ? c : 0;
	    return CLI_LOOP_CTRL_CONTINUE;
	}
    }

    if (c == 0) return CLI_LOOP_CTRL_CONTINUE;
    if (c == '\n') return CLI_LOOP_CTRL_CONTINUE;

    if (c == '\r')
    {
	if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
	    _write(cli, ctx, "\r\n", 2);
	return CLI_LOOP_CTRL_BREAK;
    }

    if (c == 27)
    {
	ctx->esc = 1;
	return CLI_LOOP_CTRL_CONTINUE;
    }

    if (c == CTRL('C'))
    {
	_write(cli, ctx, "\a", 1);
	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* back word, backspace/delete */
    if (c == CTRL('W') || c == CTRL('H') || c == 0x7f)
    {
	int back = 0;

	if (c == CTRL('W')) /* word */
	{
	    int nc = ctx->cursor;

	    if (ctx->l == 0 || ctx->cursor == 0)
		return CLI_LOOP_CTRL_CONTINUE;

	    while (nc && ctx->cmd[nc - 1] == ' ')
	    {
		nc--;
		back++;
	    }

	    while (nc && ctx->cmd[nc - 1] != ' ')
	    {
		nc--;
		back++;
	    }
	}
	else /* char */
	{
	    if (ctx->l == 0 || ctx->cursor == 0)
	    {
		_write(cli, ctx, "\a", 1);
		return CLI_LOOP_CTRL_CONTINUE;
	    }

	    back = 1;
	}

	if (back)
	{
	    while (back--)
	    {
		if (ctx->l == ctx->cursor)
		{
		    ctx->cmd[--ctx->cursor] = 0;
		    if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
			_write(cli, ctx, "\b \b", 3);
		}
		else
		{
		    int i;
		    ctx->cursor--;
		    if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
		    {
			for (i = ctx->cursor; i <= ctx->l; i++) ctx->cmd[i] = ctx->cmd[i+1];
			_write(cli, ctx, "\b", 1);
			_write(cli, ctx, ctx->cmd + ctx->cursor, strlen(ctx->cmd + ctx->cursor));
			_write(cli, ctx, " ", 1);
			for (i = 0; i <= (int)strlen(ctx->cmd + ctx->cursor); i++)
			    _write(cli, ctx, "\b", 1);
		    }
		}
		ctx->l--;
	    }

	    return CLI_LOOP_CTRL_CONTINUE;
	}
    }

    /* redraw */
    if (c == CTRL('L'))
    {
	int i;
	int cursorback = ctx->l - ctx->cursor;

	if (cli->state == CLI_STATE_PASSWORD || cli->state == CLI_STATE_ENABLE_PASSWORD)
	    return CLI_LOOP_CTRL_CONTINUE;

	_write(cli, ctx, "\r\n", 2);
	show_prompt(cli, ctx);
	_write(cli, ctx, ctx->cmd, ctx->l);

	for (i = 0; i < cursorback; i++)
	    _write(cli, ctx, "\b", 1);

	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* clear line */
    if (c == CTRL('U'))
    {
	if (cli->state == CLI_STATE_PASSWORD || cli->state == CLI_STATE_ENABLE_PASSWORD)
	    memset(ctx->cmd, 0, ctx->l);
	else
	    cli_clear_line(cli, ctx);

	ctx->l = ctx->cursor = 0;
	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* kill to EOL */
    if (c == CTRL('K'))
    {
	if (ctx->cursor == ctx->l)
	    return CLI_LOOP_CTRL_CONTINUE;

	if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
	{
	    int c;
	    for (c = ctx->cursor; c < ctx->l; c++)
		_write(cli, ctx, " ", 1);

	    for (c = ctx->cursor; c < ctx->l; c++)
		_write(cli, ctx, "\b", 1);
	}

	memset(ctx->cmd + ctx->cursor, 0, ctx->l - ctx->cursor);
	ctx->l = ctx->cursor;
	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* EOT */
    if (c == CTRL('D'))
    {
	if (cli->state == CLI_STATE_PASSWORD || cli->state == CLI_STATE_ENABLE_PASSWORD)
	    return CLI_LOOP_CTRL_BREAK;

	if (ctx->l)
	    return CLI_LOOP_CTRL_CONTINUE;

	ctx->l = -1;
	return CLI_LOOP_CTRL_BREAK;
    }

    /* disable */
    if (c == CTRL('Z'))
    {
	if (cli->mode != MODE_EXEC)
	{
	    cli_clear_line(cli, ctx);
	    cli_set_configmode(cli, MODE_EXEC, NULL);
	    cli->showprompt = 1;
	}

	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* TAB completion */
    if (c == CTRL('I'))
    {
	char *completions[CLI_MAX_LINE_WORDS];
	int num_completions = 0;

	if (cli->state == CLI_STATE_LOGIN || cli->state == CLI_STATE_PASSWORD || cli->state == CLI_STATE_ENABLE_PASSWORD)
	    return CLI_LOOP_CTRL_CONTINUE;

	if (ctx->cursor != ctx->l) return CLI_LOOP_CTRL_CONTINUE;

	num_completions = cli_get_completions(cli, ctx->cmd, completions, CLI_MAX_LINE_WORDS);
	if (num_completions == 0)
	{
	    _write(cli, ctx, "\a", 1);
	}
	else if (num_completions == 1)
	{
	    // Single completion
	    for (; ctx->l > 0; ctx->l--, ctx->cursor--)
	    {
		if (ctx->cmd[ctx->l-1] == ' ' || ctx->cmd[ctx->l-1] == '|')
		    break;
		_write(cli, ctx, "\b", 1);
	    }
	    strcpy((ctx->cmd + ctx->l), completions[0]);
	    ctx->l += strlen(completions[0]);
	    ctx->cmd[ctx->l++] = ' ';
	    ctx->cursor = ctx->l;
	    _write(cli, ctx, completions[0], strlen(completions[0]));
	    _write(cli, ctx, " ", 1);
	}
	else if (ctx->lastchar == CTRL('I'))
	{
	    // double tab
	    int i;
	    _write(cli, ctx, "\r\n", 2);
	    for (i = 0; i < num_completions; i++)
	    {
		_write(cli, ctx, completions[i], strlen(completions[i]));
		if (i % 4 == 3)
		    _write(cli, ctx, "\r\n", 2);
		else
		    _write(cli, ctx, "     ", 1);
	    }
	    if (i % 4 != 3) _write(cli, ctx, "\r\n", 2);
	    cli->showprompt = 1;
	}
	else
	{
	    // More than one completion
	    ctx->lastchar = c;
	    _write(cli, ctx, "\a", 1);
	}
	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* history */
    if (c == CTRL('P') || c == CTRL('N'))
    {
	int history_found = 0;

	if (cli->state == CLI_STATE_LOGIN || cli->state == CLI_STATE_PASSWORD || cli->state == CLI_STATE_ENABLE_PASSWORD)
	    return CLI_LOOP_CTRL_CONTINUE;

	if (c == CTRL('P')) // Up
	{
	    ctx->in_history--;
	    if (ctx->in_history < 0)
	    {
		for (ctx->in_history = MAX_HISTORY-1; ctx->in_history >= 0; ctx->in_history--)
		{
		    if (cli->history[ctx->in_history][0])
		    {
			history_found = 1;
			break;
		    }
		}
	    }
	    else
	    {
		if (cli->history[ctx->in_history]) history_found = 1;
	    }
	}
	else // Down
	{
	    ctx->in_history++;
	    if (ctx->in_history >= MAX_HISTORY || !cli->history[ctx->in_history])
	    {
		int i = 0;
		for (i = 0; i < MAX_HISTORY; i++)
		{
		    if (cli->history[i])
		    {
			ctx->in_history = i;
			history_found = 1;
			break;
		    }
		}
	    }
	    else
	    {
		if (cli->history[ctx->in_history]) history_found = 1;
	    }
	}
	if (history_found && cli->history[ctx->in_history])
	{
	    // Show history item
	    cli_clear_line(cli, ctx);
	    memset(ctx->cmd, 0, CLI_MAX_LINE_LENGTH);
	    strncpy(ctx->cmd, cli->history[ctx->in_history], CLI_MAX_LINE_LENGTH - 1);
	    /* cryptech: not sure if needed, but ensure we don't disclose memory after buf */
	    ctx->cmd[CLI_MAX_LINE_LENGTH - 1] = 0;
	    ctx->l = ctx->cursor = strlen(ctx->cmd);
	    _write(cli, ctx, ctx->cmd, ctx->l);
	}

	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* left/right cursor motion */
    if (c == CTRL('B') || c == CTRL('F'))
    {
	if (c == CTRL('B')) /* Left */
	{
	    if (ctx->cursor)
	    {
		if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
		    _write(cli, ctx, "\b", 1);

		ctx->cursor--;
	    }
	}
	else /* Right */
	{
	    if (ctx->cursor < ctx->l)
	    {
		if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
		    _write(cli, ctx, &ctx->cmd[ctx->cursor], 1);

		ctx->cursor++;
	    }
	}

	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* start of line */
    if (c == CTRL('A'))
    {
	if (ctx->cursor)
	{
	    if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
	    {
		_write(cli, ctx, "\r", 1);
		show_prompt(cli, ctx);
	    }

	    ctx->cursor = 0;
	}

	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* end of line */
    if (c == CTRL('E'))
    {
	if (ctx->cursor < ctx->l)
	{
	    if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
		_write(cli, ctx, &ctx->cmd[ctx->cursor], ctx->l - ctx->cursor);

	    ctx->cursor = ctx->l;
	}

	return CLI_LOOP_CTRL_CONTINUE;
    }

    /* normal character typed */
    if (ctx->cursor == ctx->l)
    {
	/* append to end of line */
	ctx->cmd[ctx->cursor] = c;
	if (ctx->l < CLI_MAX_LINE_LENGTH - 1)
	{
	    ctx->l++;
	    ctx->cursor++;
	}
	else
	{
	    _write(cli, ctx, "\a", 1);
	    return CLI_LOOP_CTRL_CONTINUE;
	}
    }
    else
    {
	// Middle of text
	if (ctx->insertmode)
	{
	    int i;
	    // Move everything one character to the right
	    if (ctx->l >= CLI_MAX_LINE_LENGTH - 2) ctx->l--;
	    for (i = ctx->l; i >= ctx->cursor; i--)
		ctx->cmd[i + 1] = ctx->cmd[i];
	    // Write what we've just added
	    ctx->cmd[ctx->cursor] = c;

	    _write(cli, ctx, &ctx->cmd[ctx->cursor], ctx->l - ctx->cursor + 1);
	    for (i = 0; i < (ctx->l - ctx->cursor + 1); i++)
		_write(cli, ctx, "\b", 1);
	    ctx->l++;
	}
	else
	{
	    ctx->cmd[ctx->cursor] = c;
	}
	ctx->cursor++;
    }

    if (cli->state != CLI_STATE_PASSWORD && cli->state != CLI_STATE_ENABLE_PASSWORD)
    {
	if (c == '?' && ctx->cursor == ctx->l)
	{
	    _write(cli, ctx, "\r\n", 2);
	    ctx->restore_cmd_l = ctx->l -1;
	    return CLI_LOOP_CTRL_BREAK;
	}
	_write(cli, ctx, &c, 1);
    }

    ctx->restore_cmd_l = 0;
    ctx->lastchar = c;

    return 0;
}



int cli_loop_process_cmd(struct cli_def *cli, struct cli_loop_ctx *ctx)
{
    if (cli->state == CLI_STATE_LOGIN)
    {
	if (ctx->l == 0) return 0;

	/* require login */
	if (strlen(ctx->cmd) > sizeof(ctx->username))
	    return CLI_LOOP_CTRL_BREAK;
	strncpy(ctx->username, ctx->cmd, sizeof(ctx->username) - 1);
	cli->state = CLI_STATE_PASSWORD;
	cli->showprompt = 1;
    }
    else if (cli->state == CLI_STATE_PASSWORD)
    {
	/* require password */
	int allowed = 0;

	if (cli->auth_callback)
	{
	    if (cli->auth_callback(ctx->username, ctx->cmd) == CLI_OK)
		allowed++;
	}

	if (!allowed)
	{
	    struct unp *u;
	    for (u = cli->users; u; u = u->next)
	    {
		if (!strcmp(u->username, ctx->username) && pass_matches(u->password, ctx->cmd))
		{
		    allowed++;
		    break;
		}
	    }
	}
	memset(ctx->cmd, 0, sizeof(ctx->cmd));  // XXX verify this sizeof

	if (allowed)
	{
	    cli_error(cli, " ");
	    cli->state = CLI_STATE_NORMAL;
	}
	else
	{
	    cli_error(cli, "\n\nAccess denied");
	    cli->state = CLI_STATE_LOGIN;
	}

	cli->showprompt = 1;
    }
    else if (cli->state == CLI_STATE_ENABLE_PASSWORD)
    {
	int allowed = 0;
	if (cli->enable_password)
	{
	    /* check stored static enable password */
	    if (pass_matches(cli->enable_password, ctx->cmd))
		allowed++;
	}

	if (!allowed && cli->enable_callback)
	{
	    /* check callback */
	    if (cli->enable_callback(ctx->cmd))
		allowed++;
	}

	if (allowed)
	{
	    cli->state = CLI_STATE_ENABLE;
	    cli_set_privilege(cli, PRIVILEGE_PRIVILEGED);
	}
	else
	{
	    cli_error(cli, "\n\nAccess denied");
	    cli->state = CLI_STATE_NORMAL;
	}
    }
    else
    {
	if (ctx->l == 0) return 0;
	/* XXX also don't add to history if command equals the last history entry? */
	if (ctx->cmd[ctx->l - 1] != '?' && strcasecmp(ctx->cmd, "history") != 0)
	    cli_add_history(cli, ctx->cmd);

	if (cli_run_command(cli, ctx->cmd) == CLI_QUIT)
	    return CLI_LOOP_CTRL_BREAK;
    }

    return 0;
}


/* cryptech: removed unused file mode */

static void _print(struct cli_def *cli, int print_mode, const char *format, va_list ap)
{
    static char buf[1024];
    va_list aq;
    int n;
    char *p;

    if (!cli) return; // sanity check

    while (1)
    {
        va_copy(aq, ap);
        if ((n = vsnprintf(buf, sizeof(buf), format, ap)) == -1)
            return;

        if ((unsigned)n >= sizeof(buf))
        {
	  strncpy(buf, "_print buffer would have overflown", sizeof(buf));
            return;
        }
        break;
    }

    p = buf;
    do
    {
        char *next = strchr(p, '\n');

        if (next)
            *next++ = 0;
        else if (print_mode & PRINT_BUFFERED)
            break;

        if (cli->print_callback)
           cli->print_callback(cli, p);
        else if (cli->client)
            fprintf(cli->client, "%s\r\n", p);

        p = next;
    } while (p);
}

void cli_bufprint(struct cli_def *cli, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    _print(cli, PRINT_BUFFERED|PRINT_FILTERED, format, ap);
    va_end(ap);
}

void cli_vabufprint(struct cli_def *cli, const char *format, va_list ap)
{
    _print(cli, PRINT_BUFFERED, format, ap);
}

void cli_print(struct cli_def *cli, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    _print(cli, PRINT_FILTERED, format, ap);
    va_end(ap);
}

void cli_error(struct cli_def *cli, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    _print(cli, PRINT_PLAIN, format, ap);
    va_end(ap);
}

struct cli_match_filter_state
{
    int flags;
    union {
        char *string;
        regex_t re;
    } match;
};

/* cryptech: removed unused filter functions */

void cli_print_callback(struct cli_def *cli, void (*callback)(struct cli_def *, const char *))
{
    cli->print_callback = callback;
}

void cli_set_idle_timeout(struct cli_def *cli, unsigned int seconds)
{
    if (seconds < 1)
        seconds = 0;
    cli->idle_timeout = seconds;
}

void cli_set_idle_timeout_callback(struct cli_def *cli, unsigned int seconds, int (*callback)(struct cli_def *))
{
    cli_set_idle_timeout(cli, seconds);
    cli->idle_timeout_callback = callback;
}

void cli_telnet_protocol(struct cli_def *cli, int telnet_protocol) {
    cli->telnet_protocol = !!telnet_protocol;
}

void cli_set_context(struct cli_def *cli, void *context) {
    cli->user_context = context;
}

void *cli_get_context(struct cli_def *cli) {
    return cli->user_context;
}

void cli_read_callback(struct cli_def *cli, int (*callback)(struct cli_def *, void *, size_t))
{
    cli->read_callback = callback;
}

void cli_write_callback(struct cli_def *cli, int (*callback)(struct cli_def *, const void *, size_t))
{
    cli->write_callback = callback;
}
