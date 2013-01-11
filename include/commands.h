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

typedef enum { ADD, ARCHIVE, COMPARE, CREATE,
	DELETE, DESCRIPTION, EXPORT, FIND, HELP,
	IMPORT, LIST, READ, UNARCHIVE, UPDATE,
	USAGE, VARIANTS, TREE } Commands;

#ifdef	__cplusplus
}
#endif

#endif	/* COMMANDS_H */

