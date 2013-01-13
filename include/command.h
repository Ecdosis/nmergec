/* 
 * File:   commands.h
 * Author: desmond
 *
 * Created on January 11, 2013, 1:01 PM
 */

#ifndef COMMANDS_H
#define	COMMANDS_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum { ACOMMAND=0, ADD, ARCHIVE, COMPARE, CREATE,
	DELETE, DESCRIPTION, DETAILED_USAGE, EXPORT, FIND, HELP,
	IMPORT, LIST, READ, TREE, UNARCHIVE, UPDATE,
	USAGE, VARIANTS } command;
    command command_value( const char *value );
#ifdef MVD_TEST
    int test_command( int *passed, int *failed );
#endif
#ifdef	__cplusplus
}
#endif

#endif	/* COMMANDS_H */

