#include <stdio.h>
#include "bitset.h"
#include "unicode/uchar.h"
#include "link_node.h"
#include "version.h"
#include "pair.h"
#include "dyn_array.h"
#include "mvd.h"
#include "plugin.h"

/**
* Compare two versions u and v. If it is in u but not in v 
* then turn that pair and any subsequent pairs with the 
* same characteristic into a match. We also generate merged 
* Matches for the gaps between the state matches (added or 
* deleted). This way we can link up the merged matches in 
* the GUI.
* @param u the first version to compare
* @param v the second version to compare
* @param state the state of text belonging only to u
* @return an array of chunks for special display
*/
public Chunk[] compare( short u, short v, ChunkState state ) 
   throws MVDException
{
   ArrayList<Chunk> chunks = new ArrayList<Chunk>();
   short backup = versions.get(u-1).getBackup();
   Chunk current = new Chunk( encoding, backup );
   current.setVersion( u );
   TransposeState oldTS = null;
   TransposeState ts = new TransposeState();
   ChunkStateSet cs = new ChunkStateSet( backup );
   ChunkStateSet oldCS = null;
   Pair p = null;
   Chunk.chunkId = 0;
   TransposeState.transposeId = Integer.MAX_VALUE;
   int i = next( 0, u );
   while ( i < pairs.size() )
   {
       p = pairs.get( i );
       oldTS = ts;
       oldCS = cs;
       ts = ts.next( p, u, v );
       // transposed is not deleted, inserted or merged
       if ( !ts.isTransposed() )
           cs = cs.next( p, state, v );
       if ( ts != oldTS || cs != oldCS )
       {
           // then we have to write out current
           ChunkStateSet cs1 = current.getStates();
           if ( current.getLength()>0 )
           {
               if ( cs1.isMerged() )
                   current.setId( ++Chunk.chunkId );
               chunks.add( current );
           }
           // set up a new current chunk
           ChunkState[] newStates;
           if ( ts.getId() != 0 )
           {
               newStates = new ChunkState[1];
               newStates[0] = ts.getChunkState();
           }
           else
               newStates = cs.getStates();
           current = new Chunk( encoding, ts.getId(), 
               newStates, p.getData(), backup );
           current.setVersion( u );
       }
       else
           current.addData( p.getData() );
       if ( i < pairs.size()-1 )
           i = next( i+1, u );
       else
           break;
   }
   // add any lingering chunks
   if ( current.getStates().isMerged() )
       current.setId( ++Chunk.chunkId );
   if ( chunks.isEmpty() || current != chunks.get(chunks.size()-1) )
       chunks.add( current );
   Chunk[] result = new Chunk[chunks.size()];
   chunks.toArray( result );
   return result;
}
/**
 * Do the work of this plugin
 * @param mvd the mvd to compare versions in
 * @param options a string containing the plugin's options
 * @param output buffer for errors, caller to dispose
 * @param data data passed directly in 
 * @param data_len length of data
 * @return 1 if the process completed successfully
 */
int process( MVD **mvd, char *options, unsigned char *output, 
    unsigned char *data, size_t data_len )
{
    int res = 1;
    *output = NULL;
    if ( *mvd == NULL )
        *mvd = mvd_create( 1 );
    if ( *mvd != NULL )
    {
        plugin_log *log = plugin_log_create();
        if ( log != NULL )
        {
            if ( *output != NULL )
                plugin_log_add(log,*output);
            if ( !mvd_is_clean(*mvd) )
                res = mvd_clean( *mvd );
            if ( res )
            {
                hashmap *map = parse_options( options );
                if ( map != NULL )
                {
                    res = adder_set_options( add, map );
                    if ( res && data != NULL && data_len > 0 )
                        res = add_mvd_text( add, *mvd, data, data_len, 
                            log );
                    else
                        plugin_log_add(log,"mvd_add: length was 0\n");
                    hashmap_dispose( map, free );
                }
                else
                    plugin_log_add(log,"mvd_add: invalid options %s", options);
            }
        }
    }
    return res;
}
/**
 * Do we change the MVD?
 * @return 1 if we do else 0
 */
int changes()
{
    return 0;
}
/**
 * Print a help message to stdout explaining what the parameters are
 */
char *help()
{
    return "help\n";
}
/**
 * Print a description of what we do
 */
char *description()
{
    return "compare two versions within an mvd\n";
}
/**
 * Report the plugin's version and author to stdout
 */
char *plug_version()
{
    printf( "version 0.1 (c) 2013 Desmond Schmidt\n");
}
/**
 * Report the plugin's name
 * @return a string being it's canonical name
 */
char *name()
{
    return "compare";
}
/**
 * Test the plugin
 * @param p VAR param update number of passed tests
 * @param f VAR param update number of failed tests
 * @return 1 if all tests passed else 0
 */
int test(int *p,int *f)
{
    return 1;
}
