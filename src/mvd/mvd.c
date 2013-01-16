#include "mvd/MVD.h"
#include "dyn_array.h"
#include "bitset.h"
struct MVD_struct
{
    dyn_array *groups;		// id = position in table+1
	dyn_array *versions;	// id = position in table+1
	dyn_array *pairs;
	char *description;
	int headerSize,groupTableSize,versionTableSize,pairsTableSize,
	dataTableSize,versionSetSize;
	int bestScore;
	long startTime;
	bitset partialVersions;
	char *encoding;
};
int mvd_datasize( MVD *mvd )
{
    return 1;
}
int mvd_serialise( MVD *mvd, char *data )
{
    return 1;
}