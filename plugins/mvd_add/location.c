#include <stdlib.h>
#include <unicode/uchar.h>
#include "plugin_log.h"
#include "bitset.h"
#include "link_node.h"
#include "pair.h"
#include "dyn_array.h"
#include "card.h"
#include "location.h"

/**
 * Set the global position or vindex of this location
 * @param l the location instance
 * @param version the version to follow
 * @param start the first card in the deck
 * @return 1 if it worked else 0
 */
int location_set( location *l, int version, card *start )
{
    int res = 0;
    int offset = 0;
    l->version = version;
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        bitset_set( bs, l->version );
        card *temp = card_first(start,bs);
        while ( temp != NULL )
        {
            if ( temp != l->current )
            {
                offset += card_len( temp );
                temp = card_next( temp, bs );
            }
            else
            {
                l->vindex = offset+l->pos;
                res = 1;
                break;
            }
        }
        bitset_dispose( bs );
    }
    return res;
}
/**
 * Update current and pos by following the version up until vindex
 * @param l the location instance
 * @param version the version to follow
 * @param start the first card in the deck
 * @return 1 if it worked else 0
 */
int location_update( location *l, int version, card *start )
{
    int res = 0;
    int offset = 0;
    l->version = version;
    bitset *bs = bitset_create();
    if ( bs != NULL )
    {
        bitset_set( bs, l->version );
        card *temp = card_first(start,bs);
        while ( temp != NULL )
        {
            if ( offset+card_len(temp) < l->vindex )
            {
                offset += card_len( temp );
                temp = card_next( temp, bs );
            }
            else
            {
                l->current = temp;
                l->pos = l->vindex-offset;
                res = 1;
                break;
            }
        }
        bitset_dispose( bs );
    }
    return res;
}