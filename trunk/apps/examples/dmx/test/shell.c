 /*
 * Copyright (c) 2003, Adam Dunkels.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the uIP TCP/IP stack.
 *
 * $Id: shell.c 366 2009-02-24 08:42:03Z tk $
 *
 */

#include <uip.h> // We need to get connection stats etc.
#include "shell.h"
#include <string.h>

struct ptentry {
  char *commandstr;
  void (* pfunc)(char *str);
};

#define SHELL_PROMPT "uIP 1.0> "

/*---------------------------------------------------------------------------*/
static void
parse(register char *str, struct ptentry *t)
{
  struct ptentry *p;
  for(p = t; p->commandstr != NULL; ++p) {
    if(strncmp(p->commandstr, str, strlen(p->commandstr)) == 0) {
      break;
    }
  }

  p->pfunc(str);
}
/*---------------------------------------------------------------------------*/
static void
inttostr(register char *str, unsigned int i)
{
  str[0] = '0' + i / 100;
  if(str[0] == '0') {
    str[0] = ' ';
  }
  str[1] = '0' + (i / 10) % 10;
  if(str[0] == ' ' && str[1] == '0') {
    str[1] = ' ';
  }
  str[2] = '0' + i % 10;
  str[3] = ' ';
  str[4] = 0;
}
/*---------------------------------------------------------------------------*/
static void
help(char *str)
{
  shell_output("Available commands:", "");
  shell_output("stats   - show network statistics", "");
  shell_output("conn    - show TCP connections", "");
  shell_output("help, ? - show help", "");
  shell_output("exit    - exit shell", "");
}
/*---------------------------------------------------------------------------*/
static void
stats(char *str)
{
  char line_buffer[80];
  shell_output("Network statistics", "");
  sprintf(line_buffer,"IP stats: rx:%d tx:%d dropped:%d",uip_stat.ip.recv,uip_stat.ip.sent,uip_stat.ip.drop);
  shell_output(line_buffer, "");
  sprintf(line_buffer,"IP err: vhl:%d hblen:%d lblen:%d frag:%d chk:%d proto:%d",\
	uip_stat.ip.vhlerr,uip_stat.ip.hblenerr,uip_stat.ip.lblenerr,uip_stat.ip.fragerr,uip_stat.ip.chkerr,uip_stat.ip.protoerr);
  shell_output(line_buffer, "");
  sprintf(line_buffer,"TCP stats: rx:%d tx:%d dropped:%d",uip_stat.tcp.recv,uip_stat.tcp.sent,uip_stat.tcp.drop);
  shell_output(line_buffer, "");
  sprintf(line_buffer,"TCP err: chk:%d ack:%d rst:%d rexmit:%d syndrop:%d synrst:%d",\
	uip_stat.tcp.chkerr,uip_stat.tcp.ackerr,uip_stat.tcp.rst,uip_stat.tcp.rexmit,uip_stat.tcp.syndrop,uip_stat.tcp.synrst);
  shell_output(line_buffer, "");
  sprintf(line_buffer,"UDP rx:%d tx:%d dropped:%d chkerr:%d",uip_stat.udp.recv,uip_stat.udp.sent,uip_stat.udp.drop,uip_stat.udp.chkerr);
  shell_output(line_buffer, "");
  sprintf(line_buffer,"ICMP rx:%d tx:%d dropped:%d typeerr:%d",uip_stat.icmp.recv,uip_stat.icmp.sent,uip_stat.icmp.drop,uip_stat.icmp.typeerr);
  shell_output(line_buffer, "");
}
/*---------------------------------------------------------------------------*/
static void
unknown(char *str)
{
  if(strlen(str) > 0) {
    shell_output("Unknown command: ", str);
  }
}
/*---------------------------------------------------------------------------*/
static struct ptentry parsetab[] =
  {{"stats", stats},
   {"conn", help},
   {"help", help},
   {"exit", shell_quit},
   {"?", help},

   /* Default action */
   {NULL, unknown}};
/*---------------------------------------------------------------------------*/
void
shell_init(void)
{
}
/*---------------------------------------------------------------------------*/
void
shell_start(void)
{
  shell_output("uIP command shell", "");
  shell_output("Type '?' and return for help", "");
  shell_prompt(SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
void
shell_input(char *cmd)
{
  parse(cmd, parsetab);
  int f;
  for(f=0;f<strlen(cmd);f++)
	*cmd++='\0';
  shell_prompt(SHELL_PROMPT);
}
/*---------------------------------------------------------------------------*/
