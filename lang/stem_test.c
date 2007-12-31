
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <api.h>

#define cat_(x, y) x ## y
#define cat(x, y) cat_(x, y)
#define str_(x) #x
#define str(x) str_(x)

extern struct SN_env * cat(LANG, _create_env(void));

void cat(LANG, _stem(struct SN_env *z));

int main(int argc, char **argv)
{
        struct SN_env *z = NULL;

        if (argc < 2){
                printf("test_" str(LANG) " token\n");
                exit(1);
        }
        
        printf("Language: " str(LANG) "\n");
        
        z = cat(LANG, _create_env());
        
        SN_set_current(z, strlen(argv[1]), argv[1]);

        cat(LANG, _stem(z));
        z->p[z->l] = 0;

        printf("Stems '%s' to '%s'\n", argv[1], z->p);

        return 0;
}
