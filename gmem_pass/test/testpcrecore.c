#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pcre.h"

#define OVECCOUNT 36 /* should be a multiple of 3 */
#define EBUFLEN 128
#define BUFLEN 1024

int main() {
    pcre *re;
    const char *error;
    int erroffset;
    int ovector[OVECCOUNT];
    int rc, i;
    FILE *fp;

    fp = fopen("telephone.sql", "r");
    freopen("result.txt", "w", stdout);
    char pattern[] = "1[35]\\d{5}, '.+', '.+'";

    re = pcre_compile(pattern, 0, &error, &erroffset, NULL);
    if (re == NULL) {
        printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
        return 1;
    }
    char src[100];
    while (fgets(src, 128, fp) != 0) {
        rc = pcre_exec(re, NULL, src, strlen(src), 0, 0, ovector, OVECCOUNT);
        if (rc < 0) {
            if (rc == PCRE_ERROR_NOMATCH)
                printf("Sorry, no match ...\n");
            else
                printf("Matching error %d\n", rc);
        }

        for (i = 0; i < rc; i++) {
            char *substring_start = src + ovector[2 * i];
            int substring_length = ovector[2 * i + 1] - ovector[2 * i];
            printf("%2d: %.*s\n", i, substring_length, substring_start);
        }
    }
    free(re);
    return 0;
}