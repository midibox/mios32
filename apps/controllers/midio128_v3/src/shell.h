// $Id$
/*
 * Telnet shell
 *
 * ==========================================================================
 *
 *  Copyright (C) 2010 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _SHELL_H
#define _SHELL_H

/**
 * Initialize the shell.
 *
 * Called when the shell front-end process starts. This function may
 * be used to start listening for signals.
 */
void shell_init(void);

/**
 * Start the shell back-end.
 *
 * Called by the front-end when a new shell is started.
 */
void shell_start(void);

/**
 * Process a shell command.
 *
 * This function will be called by the shell GUI / telnet server whan
 * a command has been entered that should be processed by the shell
 * back-end.
 *
 * \param command The command to be processed.
 */
void shell_input(char *command);

/**
 * Quit the shell.
 *
 */
void shell_quit(char *);


/**
 * Print a string to the shell window.
 *
 * This function is implemented by the shell GUI / telnet server and
 * can be called by the shell back-end to output a string in the
 * shell window. The string is automatically appended with a linebreak.
 *
 * \param str1 The first half of the string to be output.
 * \param str2 The second half of the string to be output.
 */
void shell_output(char *str1, char *str2);

/**
 * Print a prompt to the shell window.
 *
 * This function can be used by the shell back-end to print out a
 * prompt to the shell window.
 *
 * \param prompt The prompt to be printed.
 *
 */
void shell_prompt(char *prompt);

#endif /* _SHELL_H */
