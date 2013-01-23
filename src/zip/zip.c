#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "zip/zlib.h"
#include "char_buf.h"
#define CHUNK 16384
#define min(a,b) (a<b)?a:b
static char out[CHUNK];
/**
 * Compress some data using ZIP
 * @param src the source data
 * @param src_len its length
 * @param dst VAR param destination buffer caller to deallocate
 * @param buf a dynamic output buffer
 */
int zip_deflate( unsigned char *src, int src_len, char_buf *buf )
{
	int ret=0;
	int left;
    int flush;
	z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, (15+16), 
        8, Z_DEFAULT_STRATEGY );
    if (ret == Z_OK)
    {
		left = src_len;
		do 
		{
		    int amount = min(CHUNK,left);
            flush = (amount<CHUNK)? Z_FINISH : Z_NO_FLUSH;
            strm.next_in = &src[src_len-left];
			strm.avail_in = amount;
			do 
			{
				strm.avail_out = CHUNK;
		        strm.next_out = out;
		        ret = deflate(&strm, flush);
		        if ( ret == Z_STREAM_ERROR )
		        {
					(void)deflateEnd(&strm);
                		return 0;
				}
				else
				{
					ret = char_buf_write(buf,out,CHUNK-strm.avail_out);
					if ( !ret )
						return 0;
				}
		    } while (strm.avail_out == 0);
		    assert(strm.avail_in == 0);     /* all input will be used */
			left -= amount;
			if ( left == 0 )
				break;
		} while (flush != Z_FINISH);
		assert(ret == Z_STREAM_END);        /* stream will be complete */
		/* clean up and return */
		(void)deflateEnd(&strm);
		ret = 1;
	}
	return ret;
}
/**
 * Decompress some data using ZIP
 * @param src the source data
 * @param src_len its length
 * @param dst VAR param destination buffer caller to deallocate
 * @param buf a dynamic output buffer
 */
int zip_inflate( unsigned char *src, int src_len, char_buf *buf )
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm,16+MAX_WBITS);
    if (ret != Z_OK)
        return 0;

    /* decompress until deflate stream ends or end of file */
    int left = src_len;
    do {
        int amount = min(CHUNK,left);
		strm.avail_in = amount;
        strm.next_in = &src[src_len-left];

        do {
            strm.avail_out = CHUNK;
            strm.next_out = out;
            ret = inflate(&strm, Z_NO_FLUSH);
            assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return 0;
            }
            have = CHUNK - strm.avail_out;
            if ( !char_buf_write(buf,out,have) )
			{
                (void)inflateEnd(&strm);
                return 0;
            }
        } while (strm.avail_out == 0);
        left -= amount;
        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);

    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? 1 : 0;
}
#ifdef MVD_TEST
static const char *src = 
"Sed ut perspiciatis unde omnis iste natus error sit "
"voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque "
"ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae "
"dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit "
"aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos "
"qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui "
"dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia"
"non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam "
"quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem "
"ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi "
"consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate "
"velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum "
"fugiat quo voluptas nulla pariatur?";
int test_zip( int *passed, int *failed )
{
    int res = 0;
    char_buf *cb = char_buf_create( 1024 );
    if ( cb != NULL )
    {
        int slen = strlen(src);
        int blen = slen*100;
        unsigned char *bbuf = calloc( blen+1, 1 );
        if ( bbuf != NULL )
        {
            int i,pos;
            // fill bbuf with copies of test text
            for ( i=0,pos=0;i<100;i++ )
            {
                memcpy( &bbuf[pos], src, slen );
                pos += slen;
            }
            //  jumble up the sample text
            for ( i=0;i<blen;i++ )
            {
                int d = rand()%blen;
                if ( i != d )
                {
                    unsigned char u = bbuf[d];
                    bbuf[d] = bbuf[i];
                    bbuf[i] = u;
                }
            }
            // now compress it
            res = zip_deflate( bbuf, pos, cb );
            if ( res )
            {
                int len;
                char_buf *text = char_buf_create( 1024 );
                if ( text != NULL )
                {
                    int ilen;
                    // now decompress
                    unsigned char *buf = char_buf_get(cb,&len);
                    res = zip_inflate( buf,len,text);
                    unsigned char *t = char_buf_get(text,&ilen);
                    // check that the decompressed text matches
                    if ( blen == ilen )
                    {
                        for ( i=0;i<ilen;i++ )
                        {
                            if ( bbuf[i] != t[i] )
                            {
                                fprintf(stderr,"zip: mismatch at %d\n",i);
                                break;
                            }
                        }
                        if ( i == ilen )
                        {
                            *passed += 1;
                            res = 1;
                        }
                    }
                    char_buf_dispose( text );
                }
            }
            free( bbuf );
        }
        char_buf_dispose( cb );
    }
    if ( !res )
        *failed += 1;
    return res;
}
#endif