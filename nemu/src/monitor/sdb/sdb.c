/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>
#include <myfuncs.h>
#define SDB_DEBUG

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  printf(ANSI_FMT("exited successfully\n", ANSI_FG_YELLOW));
  set_nemu_state(2, 0, 0);
  return -1;
}

static int cmd_help(char *args);

static int cmd_si(char *args)
{
  if(args == NULL)
    cpu_exec(1);
  else 
    cpu_exec(atoi(args));
  return 0;
}

static int cmd_i(char *args)
{
  if(args[0] == 'r')
    isa_reg_display();
  else if(args[0] == 'w')
    printf(ANSI_FMT("unfinished function", ANSI_FG_RED));
  else
    printf(ANSI_FMT("unknown command", ANSI_FG_RED));
  return 0;
}

// 扫描内存
static int cmd_x(char *args)
{
  int n = atoi(strtok(args, " "));// 内存个数
  paddr_t val = expr_eval(strtok(NULL, " "));
  #ifdef SDB_DEBUG
  printf("input n is %d and val is %#x\n", n, val);
  #endif
  uint8_t* s = guest_to_host(val);
  printf(ANSI_FMT("scan %d bytes starting from %#x\n", ANSI_FG_YELLOW), n, val);
  for(int i = 0; i < n; i++, s++, val++)
  {
    printf(ANSI_FMT("%#x : %#x\n", ANSI_FG_MAGENTA), val, *s);
  }
  return 0;
}

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si", "si N,Run by Step, and execute once by default", cmd_si},
  { "info", "info r or info w , print information of register or watchpoints", cmd_i},
  { "x", "x N EXPR , scan N memory blocks from EXPR", cmd_x}
  /* TODO: Add more commands */

};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;
  
  printf(ANSI_BG_WHITE);

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }

  printf(ANSI_NONE);

  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
