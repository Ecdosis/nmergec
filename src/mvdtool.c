#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mvdtool.h"
#include "command.h"
#include "mvd/chunk_state.h"
#define EXAMPLE_ADD "nmerge -c add -g \"MS 5 versions\" -s \"P\" -l \"third "\
"correction layer\" -t p.txt -m work.mvd"
#define EXAMPLE_ARCHIVE "nmerge -c archive -m work.mvd -a myArchiveFolder"
#define EXAMPLE_COMPARE "nmerge -c compare -m work.mvd -v 4 -w 7"
#define EXAMPLE_DELETE "nmerge -c delete -m work.mvd -v 7"
#define EXAMPLE_CREATE "nmerge -c create -m work.mvd -d \"my new MVD\" -e UTF-8"
#define EXAMPLE_DESCRIPTION "nmerge -c description -m work.mvd -d "\
"\"my new mvd description\""
#define EXAMPLE_TREE "nmerge -c tree -m work.mvd -d existing.mvd"
#define EXAMPLE_EXPORT "nmerge -c export -m work.mvd -x work.xml"
#define EXAMPLE_FIND "nmerge -c find -m work.mvd -f \"bananas are nice\""
#define EXAMPLE_IMPORT "nmerge -c import -m work.mvd -x work.xml"
#define EXAMPLE_LIST "nmerge -c list -m work.mvd"
#define EXAMPLE_READ "nmerge -c read -m work.mvd -v 4"
#define EXAMPLE_UNARCHIVE "nmerge -c unarchive -m work.mvd -a myArchiveFolder"
#define EXAMPLE_UPDATE "nmerge -c update -m work.mvd -v 3 -t 5.txt"
#define EXAMPLE_VARIANTS "nmerge -c variants -m work.mvd -v 3 -o 1124 -k 23"
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
	chunk_state uniqueState;
	/** id of default folder */
	int folderId;
    /** whether to merge shared versions */
    int mergeSharedVersions;
    /** do only direct alignment */
    int directAlignOnly;
    const char *UTF8_BOM;
};
static struct mvdtool_struct mt;
/**
 * Reset all the static variables to sensible defaults
 */
static void clear_to_defaults()
{
    mt.encoding = "UTF-8";
    mt.version = 1;
    mt.uniqueState = deleted;
    mt.UTF8_BOM = "\357\273\277";
    mt.ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    mt.BREAK_AFTER = "> ,\n\r\t";
    mt.BREAK_BEFORE = "<";
    mt.groupName = "Base";
    mt.longName = "version 1";
    mt.shortName = "1";
	mt.mvdFile = "untitled.mvd";
	mt.folderId = 1;
	mt.xmlFile = "untitled.xml";
    mt.out = stdout;
}
/**
 * Read in the arguments
 * @param argc number of arguments plus 1
 * @param agv the argument array
 * @return 1 if the arguments were valid
 */
static int read_args( int argc, char **argv ) 
{
    int sane = 1;
    memset( &mt, 0, sizeof(struct mvdtool_struct) );
    clear_to_defaults();
    if ( argc > 1 )
    {
        int i=1;
        while ( i<argc )
        {
            if ( argv[i][0] =='-' && strlen(argv[i])>1 )
            {
                switch ( argv[i][1] )
                {
                    case 'p':
                        mt.partial = 1;
                        i++;
                        continue;
                    case '?':
                        if ( mt.op != ACOMMAND )
                        {
                            fprintf(stderr,
                                "mvdtool: only one of -c and -? is allowed");
                            sane = 0;
                        }
                        else
                            mt.op = DETAILED_USAGE;
                        i++;
                        continue;
                    case 'D':
                        mt.directAlignOnly = 1;
                        i++;
                        continue;
                    case 'n':
                        mt.mergeSharedVersions = 1;
                        i++;
                        continue;
                }
                if ( i<argc-1 )
                {
                    switch ( argv[i][1] )
                    {
                        case 'a':
                            mt.archiveName = argv[++i];
                            break;
                        case 'b':
                            mt.backup = atoi(argv[++i]);
                            break;
                        case 'c':
                            if ( mt.op == USAGE )
                            {
                                fprintf(stderr,
                                    "mvdtool: only one of -c and -? is allowed\n");
                                sane = 0;
                            }
                            else
                                mt.op = command_value(argv[++i]);
                            break;
                        case 'd':
                            mt.description = argv[++i];
                            break;
                        case 'e':
                            mt.encoding = argv[++i];
                            break;
                        case 'f':
                            mt.findString = argv[++i];
                            break;
                        case 'g':
                            mt.groupName = argv[++i];
                            break;
                        case 'h':
                            mt.op = HELP;
                            mt.helpCommand = argv[++i];
                            break;
                        case 'k':
                            mt.variantLen = atoi(argv[++i]);
                            break;
                        case 'l':
                            mt.longName = argv[++i];
                            break;
                        case 'm':
                            mt.mvdFile = argv[++i];
                            break;
                        case 'o':
                            mt.fromOffset = atoi(argv[++i]);
                            break;
                        case 's':
                            mt.shortName = argv[++i];
                            break;
                        case 't':
                            mt.textFile = argv[++i];
                            break;
                        case 'u':
                            mt.uniqueState = chunk_state_value(argv[++i]);
                            break;
                        case 'v':
                            mt.version = atoi(argv[++i]);
                            break;
                        case 'w':
                            mt.with = atoi(argv[++i]);
                            break;
                        case 'x':
                            mt.xmlFile = argv[++i];
                            break;
                        case 'z':
                            mt.folderId = atoi(argv[++i]);
                            break;
                        default:
                            fprintf(stderr, "mvdtool: unknown option %c\n",
                                argv[i][1] );
                            break;
                    }
                }
                else
                    fprintf(stderr,"mvdtool: wrong number of argument\n");
            }
            else 
            {
                fprintf(stderr,"mvdtool: unexpected commandline token %s\n",
                    argv[i]);
                sane = 0;
            }
            if ( !sane )
                break;
            i++;
        }
    }
    else
        sane = 0;
    if ( mt.op == ACOMMAND )
        mt.op = USAGE;
    return sane;
}
/**
 * Print more detailed help
 */
static void detailed_usage()
{
    fprintf(stdout,
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
    "-z folderId - id of folder to store mvd (default 1)\n"
    "-? - print this message\n"
    );
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
    "     [-t textfile] [-v version] [-w with] [-x XMLfile] \n");
}
/**
 * Print an example for the given helpCommand
 * @return 1 if it could be printed else 0
 */
static int print_example() 
{
    int sane = 1;
    if ( mt.helpCommand == ACOMMAND )
    {
        fprintf(stderr,"mvdtool: please specify a help command!\n");
        sane = 0;
    }
    command hComm = command_value(mt.helpCommand);
    switch ( hComm )
    {
        case ADD:
            fprintf(mt.out, "%s\n", EXAMPLE_ADD );
            break;
        case ARCHIVE:
            fprintf(mt.out, "%s\n", EXAMPLE_ARCHIVE);
            break;
        case COMPARE:
            fprintf(mt.out, "%s\n", EXAMPLE_COMPARE);
            break;
        case DELETE:
            fprintf(mt.out, "%s\n", EXAMPLE_DELETE);
            break;
        case CREATE:
            fprintf(mt.out, "%s\n", EXAMPLE_CREATE);
            break;
        case DESCRIPTION:
            fprintf(mt.out, "%s\n", EXAMPLE_DESCRIPTION);
            break;
        case TREE:
            fprintf(mt.out, "%s\n", EXAMPLE_TREE );
            break;
        case EXPORT:
            fprintf(mt.out, "%s\n", EXAMPLE_EXPORT );
            break;
        case FIND:
            fprintf(mt.out, "%s\n", EXAMPLE_FIND);
            break;
        case IMPORT:
            fprintf(mt.out, "%s\n", EXAMPLE_IMPORT);
            break;
        case LIST:
            fprintf(mt.out, "%s\n", EXAMPLE_LIST);
            break;
        case READ:
            fprintf(mt.out, "%s\n", EXAMPLE_READ);
            break;
        case UNARCHIVE:
            fprintf(mt.out, "%s\n", EXAMPLE_UNARCHIVE);
            break;
        case UPDATE:
            fprintf(mt.out, "%s\n", EXAMPLE_UPDATE);
            break;
        case VARIANTS:
            fprintf(mt.out, "%s\n", EXAMPLE_VARIANTS);					
            break;
        case HELP: 
            print_example();
            break;
        case DETAILED_USAGE: 
            detailed_usage();
            break;
        case USAGE:
            usage();
            break;
        default:
            fprintf(stderr, "mvdtool: command %s not catered for\n",
                mt.helpCommand );
            sane = 0;
            break;
    }
    return sane;
}
/**
 * Add the specified version to the MVD. Don't replace an existing 
 * version, so we reset the supplied version parameter to the number 
 * of versions+1
 */
static void do_add_version()
{
    printf("mvdtool: executing add version\n");
}
/**
 * Write out all the versions as separate files
 */
static void do_archive()
{
    printf("mvdtool: executing archive\n");
}
static void do_compare()
{
    printf("mvdtool: executing compare\n");
}
static void do_delete_version()
{
    printf("mvdtool: executing delete version\n");
}
static void do_create()
{
    printf("mvdtool: executing create\n");
}
static void do_description()
{
    printf("mvdtool: executing description\n");
}
static void do_export_to_XML()
{
    printf("mvdtool: executing export to XML\n");
}
static void do_find()
{
    printf("mvdtool: executing find\n");
}
static void do_import_from_XML()
{
    printf("mvdtool: executing import from XML\n");
}
static void do_list_versions()
{
    printf("mvdtool: executing list versions\n");
}
static void do_read_version()
{
    printf("mvdtool: executing read version\n");
}
/**
 * Read the versions of an archive back in to create an MVD
 * in one step.
 */
static void do_unarchive()
{
    printf("mvdtool: executing unarchive\n");
}
static void do_update_MVD()
{
    printf("mvdtool: executing update MVD\n");
}
static void do_find_variants()
{
    printf("mvdtool: executing find variants\n");
}
/**
 * Create a phylogenetic tree using the fastME method.
 */
static void do_tree()
{
    printf("mvdtool: executing tree\n");
}
/**
 * Execute the preset arguments
 */
void do_command()
{
    switch ( mt.op )
    {
        case ADD:
            do_add_version();
            break;
        case ARCHIVE:
            do_archive();
            break;
        case COMPARE:
            do_compare();
            break;
        case DELETE:
            do_delete_version();
            break;
        case CREATE:
            do_create();
            break;
        case DESCRIPTION:
            do_description();
            break;
        case EXPORT:
            do_export_to_XML();
            break;
        case FIND:
            do_find();
            break;
        case HELP:
            print_example();
            break;
        case IMPORT:
            do_import_from_XML();
            break;
        case LIST:
            do_list_versions();
            break;
        case READ:
            do_read_version();
            break;
        case UNARCHIVE:
            do_unarchive();
            break;
        case UPDATE:
            do_update_MVD();
            break;
        case USAGE:
            usage();
            break;
        case VARIANTS:
            do_find_variants();
            break;
        case DETAILED_USAGE:
            detailed_usage();
            break;
        case TREE:
            do_tree();
            break;
    }
}
#ifndef MVD_TEST
int main( int argc, char **argv )
{
    if ( read_args(argc,argv) )
        do_command();
    else
        usage();
}
#else
static int test_args( const char *example, int *passed, int *failed, int true )
{
    int i,state = 0;
    int sane = 1;
    char *copy = strdup(example);
    if ( copy != NULL )
    {
        int len = strlen(copy);
        for ( i=0;i<len;i++ )
        {
            switch ( state )
            {
                case 0:
                    if ( copy[i]==' ' )
                    {
                        copy[i] = 0;
                        state = 1;
                    }
                    else if ( copy[i]=='"' )
                    {
                        copy[i] = 0;
                        state = 2;
                    }
                    break;
                case 1:
                    if ( copy[i]=='"' )
                    {
                        copy[i] = 0;
                        state = 2;
                    }
                    else if ( copy[i] != ' ' )
                        state = 0;
                    else
                        copy[i] = 0;
                    break;
                case 2:
                    if ( copy[i]=='"' )
                    {
                        state = 0;
                        copy[i] = 0;
                    }
                    break;
            }
        }
        // count remaining sections
        state = 0;
        int argc = 0;
        for ( i=0;i<len;i++ )
        {
            switch ( state )
            {
                case 0:// initial: neither
                    if ( copy[i] != 0 )
                        state = 1;
                    break;
                case 1:// characters
                    if ( copy[i] == 0 )
                    {
                        argc++;
                        state = 2;
                    }
                    break;
                case 2:// blanks
                    if ( copy[i] != 0 )
                        state = 1;
                    break;
            }
        }
        if ( state == 1 )
            argc++;
        char **argv = calloc( argc, sizeof(char*) );
        if ( argv != NULL )
        {
            // assign pointers
            state = 0;
            int j = 0;
            for ( i=0;i<len;i++ )
            {
                switch ( state )
                {
                    case 0:
                        if ( copy[i] != 0 )
                        {
                            if ( j == argc )
                            {
                                fprintf(stderr,"mvdtool: invalid index %d\n",j);
                                exit(0);
                            }
                            argv[j++] = &copy[i];
                            state = 1;
                        }
                        break;
                    case 1:
                        if ( copy[i] == 0 )
                        {
                            state = 2;
                        }
                        break;
                    case 2:
                        if ( copy[i] != 0 )
                        {
                            if ( j == argc )
                            {
                                fprintf(stderr,"mvdtool: invalid index %d\n",j);
                                exit(0);
                            }
                            argv[j++] = &copy[i];
                            state = 1;
                        }
                        break;
                }
            }
            sane = read_args( argc, argv );
            if ( (sane && true)||(!sane&&!true) )
                *passed += 1;
            else
            {
                fprintf(stderr,"mvdtool: command %s failed\n",example);
                *failed += 1;
            }
            free( argv );
        }
        free( copy );
    }
    return sane;
}
/**
 * Test the mvdtool class
 * @param passed VAR param number of passed tests
 * @param failed VAR param number of failed tests
 * @return 1 if all was OK
 */
int test_mvdtool( int *passed, int *failed )
{
    test_args(EXAMPLE_ADD,passed,failed,1);
    test_args(EXAMPLE_ARCHIVE,passed,failed,1);
    test_args(EXAMPLE_COMPARE,passed,failed,1);
    test_args(EXAMPLE_DELETE,passed,failed,1);
    test_args(EXAMPLE_CREATE,passed,failed,1);
    test_args(EXAMPLE_DESCRIPTION,passed,failed,1);
    test_args(EXAMPLE_TREE,passed,failed,1);
    test_args(EXAMPLE_EXPORT,passed,failed,1);
    test_args(EXAMPLE_FIND,passed,failed,1);
    test_args(EXAMPLE_IMPORT,passed,failed,1);
    test_args(EXAMPLE_LIST,passed,failed,1);
    test_args(EXAMPLE_READ,passed,failed,1);
    test_args(EXAMPLE_UNARCHIVE,passed,failed,1);
    test_args(EXAMPLE_UPDATE,passed,failed,1);
    test_args(EXAMPLE_VARIANTS,passed,failed,1);
    test_args("-t text.txt -m my.mvd",passed,failed,0);
    test_args("foo bar -M banana -m 27",passed,failed,0);
    // add some more tests here when things go wrong later
    return (*failed)==0;
}
#endif