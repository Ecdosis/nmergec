#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef MVD_TEST
#include <string.h>
#include <stdio.h>
#endif
#define LINE_END "\n"

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char decoding_table[] = {
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,62,0,0,0,63,
52,53,54,55,56,57,58,59,60,61,0,0,0,0,0,0,
0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,
15,16,17,18,19,20,21,22,23,24,25,0,0,0,0,0,
0,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
41,42,43,44,45,46,47,48,49,50,51,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
static int mod_table[] = {0, 2, 1};

/**
 * Encode to base64. Output not NULL-terminated.
 * @param data input data
 * @param input_len the length of the input data
 * @param output the output buffer of the correct size
 * @param output_len length of output buffer
 */
void b64_encode( const unsigned char *data, size_t input_len, 
    char *output, size_t output_len ) 
{
    int i,j;
    int cr_len = strlen(LINE_END);
    for (i = 0, j = 0; i < input_len;) 
    {
        uint32_t octet_a = i < input_len ? data[i++] : 0;
        uint32_t octet_b = i < input_len ? data[i++] : 0;
        uint32_t octet_c = i < input_len ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        output[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
        // put these line-ends in for formatting 
        if ( i % 57 == 0 )
        {
            int k;
            const char *cr = LINE_END;
            for ( k=0;k<cr_len;k++ )
                output[j++] = cr[k];
        }
    }
    for (i = 0; i < mod_table[input_len % 3]; i++)
        output[output_len - 1 - i] = '=';
}
/**
 * Compute the correct buffer length for encode 
 * @param input_len the length of the input to encode
 * @return the length of the output buffer for encode
 */
size_t b64_encode_buflen( size_t input_len )
{
    size_t base_len = (size_t) (4.0 * ceil((double) input_len / 3.0));
    // add this for the inserted LINE_ENDs
    return base_len + (input_len/57)*strlen(LINE_END);
}
/**
 * Compute the correct output length for decode 
 * @param input_len the length of the input to decode
 * @return the length of the output buffer for decode
 */
size_t b64_decode_buflen( size_t input_len )
{
    return input_len / 4 * 3; 
}
/**
 * Decode base64. Output not NULL-terminated.
 * @param data the b64 input data
 * @param input_len the length of the input data
 * @param output output char array must be of correct length
 * @param output_len the length of the output buffer
 */
void b64_decode(const char *data, size_t input_len, 
    unsigned char *output, size_t output_len )
{
    int i,j;
    if (data[input_len - 1] == '=') 
        (output_len)--;
    if (data[input_len - 2] == '=') 
        (output_len)--;
    for (i = 0, j = 0; i < input_len;) 
    {
        if ( data[i]<=32 )
        {   
            i++;
            continue;
        }
        uint32_t sextet_a = data[i] == '=' ? 0 & i++:decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++:decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++:decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++:decoding_table[data[i++]];
        uint32_t triple = (sextet_a << 3 * 6)
                        + (sextet_b << 2 * 6)
                        + (sextet_c << 1 * 6)
                        + (sextet_d << 0 * 6);
        if (j < output_len) output[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < output_len) output[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < output_len) output[j++] = (triple >> 0 * 8) & 0xFF;
    }
}
#ifdef MVD_TEST
int test_b64( int *passed, int *failed )
{
    // encode/decode every possible 8-bit character except NULL
    const unsigned char *src = "\1\2\3\4\5\6\7\10\11\12\13\14\15\16\17\20"
    "\21\22\23\24\25\26\27\30\31\32\33\34\35\36\37\40"
    "\41\42\43\44\45\46\47\50\51\52\53\54\55\56\57\60"
    "\61\62\63\64\65\66\67\70\71\72\73\74\75\76\77\100"
    "\101\102\103\104\105\106\107\110\111\112\113\114\115\116\117\120"
    "\121\122\123\124\125\126\127\130\131\132\133\134\135\136\137\140"
    "\141\142\143\144\145\146\147\150\151\152\153\154\155\156\157\160"
    "\161\162\163\164\165\166\167\170\171\172\173\174\175\176\177\200"
    "\201\202\203\204\205\206\207\210\211\212\213\214\215\216\217\220"
    "\221\222\223\224\225\226\227\230\231\232\233\234\235\236\237\240"
    "\241\242\243\244\245\246\247\250\251\252\253\254\255\256\257\260"
    "\261\262\263\264\265\266\267\270\271\272\273\274\275\276\277\300"
    "\301\302\303\304\305\306\307\310\311\312\313\314\315\316\317\320"
    "\321\322\323\324\325\326\327\330\331\332\333\334\335\336\337\340"
    "\341\342\343\344\345\346\347\350\351\352\353\354\355\356\357\360"
    "\361\362\363\364\365\366\367\370\371\372\373\374\375\376\377";
    size_t src_len = strlen(src);
    size_t encoded_len = b64_encode_buflen( src_len );
    char * encoded = malloc( encoded_len );
    if ( encoded != NULL )
    {
        b64_encode(src,src_len,encoded,encoded_len);
        size_t decode_len = b64_decode_buflen( encoded_len );
        unsigned char *decoded = malloc( decode_len );
        if ( decoded != NULL )
        {
            b64_decode(encoded,encoded_len,decoded,decode_len);
            if ( decode_len < src_len )
            {
                fprintf(stderr,"original len=%d final len=%d\n",
                    (int)src_len,(int)decode_len);
                *failed += 1;
            }
            else
            {
                int i;
                for ( i=0;i<src_len;i++ )
                {
                    if ( src[i] != decoded[i] )
                    {
                        fprintf(stderr,"mismatch at %d (%c vs %c)\n",i,
                            src[i],decoded[i]);
                        *failed += 1;
                        break;
                    }
                }
                if ( i == src_len )
                    *passed += 1;
            }
            free( decoded );
        }
        free( encoded );
    }
}
#endif