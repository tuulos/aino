
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "rrice.h"
#include "dub.h"

#define BUFSIZE 1024 * 1024

int main(int argc, char **argv)
{
        char buf[BUFSIZE];
        memset(buf, 0, BUFSIZE);
        int num[] = {19179, 30456, 0, 0, 0, 0, 32123, 39999};

        s32 *ko = NULL;
        uint ko_len = 0;

        dub_init();
        
        uint val = atoi(argv[1]);
       
        uint i;
        uint offs = 0;
        //rice_write(buf, &offs, val, 1);
        rice_encode(buf, &offs, num, sizeof(num) / sizeof(uint));

        printf("%u bits used\n", offs);
        offs = 0;

        ko_len = rice_decode(&ko, &ko_len, buf, &offs);
        printf("KO %u\n", ko_len);

        for (i = 0; i < ko_len; i++)
                printf("%d ", ko[i]);
        printf("\n");

        
        /*
        elias_gamma_write(buf, &offs, val);
        for (i = 0; i < 1e8; i++){
                offs = 0;
                elias_gamma_read(buf, &offs);
        }
        */
        
        return 0;
}
