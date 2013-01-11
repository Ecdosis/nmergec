#include <stdio.h>
#include "mvdtool.h"
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