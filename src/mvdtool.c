#include <stdio.h>
#include "mvdtool.h"
#include "command.h"
#include "mvd/chunk_state.h"
#ifdef MVD_DEBUG
#include "memwatch.h"
#endif
struct mvdtool_struct
{
    /** version id of backup version */
	int backup;
	/** user issued command */
	command op;
	/** description of MVD */
	char *description;
	/** encoding of text file to be merged */
	char *encoding;
	/** text to be found */
	char *findString;
	/** group name of new version */
	char *groupName;
	/** length of variant in base version */
	int variantLen;
	/** long name of new version */
	char *longName;
	/** name of mvd file */
	char *mvdFile;
	/** name of archive */
	char *archiveName;
	/** from offset in base version */
	int fromOffset;
	/** specified version is partial */
	int partial;
	/** short name of new version */
	char *shortName;
	/** name of text file for merging */
	char *textFile;
	/** id of specified version */
	short version;	
	/** version to compare with version */
	short with;	
	/** file name for database connection */
	char *dbConn;
	/** name of xml file for export only */
	char *xmlFile;
	/** command to print example of */
	char *helpCommand;
	/** String for defining default sigla */
	const char *ALPHABET;
	/** default break before and after byte arrays */
	const char *BREAK_BEFORE;
	const char *BREAK_AFTER;
	/** Stream to report results to */
	FILE *out;
	/** state to label version text not found in with text 
	 * during compare */
	ChunkState uniqueState;
	/** id of default folder */
	int folderId;
    /** whether to merge shared versions */
    int mergeSharedVersions = 0;
    /** do only direct alignment */
    int directAlignOnly = 0;
    const char *UTF8_BOM;
};
static struct mvdtool_struct mt;
/**
 * Reset all the static variables to sensible defaults
 */
static void clear_to_defaults( struct mvdtool_struct *mts )
{
   mts->encoding = "UTF-8";
   mts->version = 1;
   mts->uniqueState = deleted;
   mts->UTF8_BOM = "\357\273\277";
   mts->ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   mts->BREAK_AFTER = "> ,\n\r\t";
   mts->BREAK_BEFORE = "<";
}
/**
 * Read in the arguments
 * @param args an array of Strings from the command line
 * @return 1 if the arguments were valid
 */
static void read_args( int argc, char **argv ) 
{
    int sane = 1;
    clear_to_defaults( &mt );
    if ( argc > 1 )
    {
        int i;
        for ( i=1;i<argc;i++ )
        {
            if ( argv[i][0] =='-' && strlen(argv[i]>1) )
            {
                switch ( argv[i][1] )
                {
                    case 'c':
                        if ( mt.op == USAGE )
                        {
                            fprintf(stderr,"Only one of -c and -? is allowed\n");
                            command = command_value(argv[i+1]);
                        }
                        break;
                    case 'p':
                        mt.partial = 1;
                        break;
               
                    case '?':
                        if ( mt.op != ACOMMAND )
                        {
                            fprintf(stderr,"Only one of -c and -? is allowed");
                            sane = 0;
                        }
                        else
                            mt.op = USAGE;
                        break;
           
                    case 'D':
                        mt.directAlignOnly = 1;
                        break;
                    case 'a':
                        mt.archiveName = argv[i+1];
                        break;
                    case 'b':
                        mt.backup = atoi(argv[i+1]);
                        break;
                    case 'd':
                        mt.description = argv[i+1];
                        break;
                    case 'e':
                        mt.encoding = argv[i+1];
                        break;
                    case 'f':
                        mt.findString = argv[i+1];
                        break;
                    case 'g':
                        mt.groupName = argv[i+1];
                        break;
                    case 'h':
                        mt.op = HELP;
                        mt.helpCommand = argv[i+1];
                        break;
                    case 'k':
                        mt.variantLen = atoi(argv[i+1]);
                        break;
                    case 'l':
                        mt.longName = argv[i+1];
                        break;
                    case 'm':
                        mt.mvdFile = argv[i+1];
                        break;
                    case 'o':
                        mt.fromOffset = atoit(argv[i+1]);
                        break;
                    case 's':
                        mt.shortName = argv[i+1];
                        break;
                    case 't':
                        mt.textFile = argv[i+1];
                        break;
                    case 'u':
                        mt.uniqueState = chunk_state_value(argv[i+1]);
                        break;
               else if ( key.equals("v") )
                   version = Short.parseShort(value);
               else if ( key.equals("w") )
                   with = Short.parseShort(value);
               else if ( key.equals("x") )
                   xmlFile = value;
               else if ( key.equals("y") )
                   dbConn = value;
               else if ( key.equals("z") )
                   folderId = Integer.parseInt(value);
               else if ( key.equals("n") )
                   mergeSharedVersions = true;
               else
                   throw new MVDToolException( "Unknown option "+key );
           }
            if ( !sane )
                break;
       }
   }
   if ( command == null )
       command = Commands.USAGE;
}
/**
 * Tell the user about how to use this program
 */
static void usage()
{
   fprintf(stdout,
    "usage: nmerge [-c command] [-a archive] [-b backup]  [-d description]\n"
    "     [-e encoding] [-f string] [-g group] [-h command] [-k length]\n"
    "     [-l longname] [-m MVD] [-o offset] [-p] [-s shortname]\n"
    "     [-t textfile] [-v version] [-w with] [-x XMLfile] [-y] dbconn [-?] \n\n"
    "-a archive - folder to use with archive and unarchive commands\n"
    "-b backup - the version number of a backup (for partial versions)\n"
    "-c command - operation to perform. One of:\n"
    "     add - add the specified version to the MVD\n"
    "     archive - save MVD in a folder as a set of separate versions\n"
    "     compare - compare specified version 'with' another version\n"
    "     create - create a new empty MVD\n"
    "     description - print or change the MVD's description string\n"
    "     delete - delete specified version from the MVD\n"
    "     export - export the MVD as XML\n"
    "     find - find specified text in all versions or in specified version\n"
    "     import - convert XML file to MVD\n"
    "     list - list versions and groups\n"
    "     read - print specified version to standard out\n"
    "     tree - compute phylogenetic tree\n"
    "     update - replace specified version with contents of textfile\n"
    "     unarchive - convert an MVD archive into an MVD\n"
    "     variants - find variants of specified version, offset and length\n"
    "-d description - specified when setting/changing the MVD description\n"
    "-D - direct align only (no transpositions)\n"
    "-e encoding - the encoding of the version's text e.g. UTF-8\n"
    "-f string - to be found (used with command find)\n"
    "-g group - name of group for new version\n"
    "-h command - print example for command\n"
    "-k length - find variants of this length in the base version's text\n"
    "-l longname - the long name/description of the new version (quoted)\n"
    "-m MVD - the MVD file to create/update\n"
    "-n - apply update to all versions sharing the same text\n"
    "-o offset - in given version to look for variants\n"
    "-p - specified version is partial\n"
    "-s shortname - short name or siglum of specified version\n"
    "-t textfile - the text file to add to/update in the MVD\n"
    "-u unique - name of state to label text found in the main -v version,\n"
    "   not in -w version during compare - e.g. 'added' or 'deleted'(default)\n"
    "-v version - number of version for command (starting from 1)\n"
    "-w with - another version to compare with version\n"
    "-x XML - the XML file to export or import\n"
    "-y dbconn - name of database connection file\n"
    "-z folderId - id of folder to store mvd (default 1)\n"
    "-? - print this message\n"
   );
}
