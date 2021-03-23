#include <stdio.h>
#include <stdlib.h>
#include "pcre.h"

#define OVECCOUNT 30 /* should be a multiple of 3 */
#define EBUFLEN 128
#define BUFLEN 1024

int main()
{
        pcre *reCM, *reUN, *reTC, *reCDMA;
        const char *error;
        int erroffset;
        int ovector[OVECCOUNT];
        int rcCM, rcUN, rcTC, rcCDMA, i;

        /*
            yidong:134.135.136.137.138.139.150.151.152.157.158.159.187.188,147
            liandong:130.131.132.155.156.185.186
            dianxin:133.153.180.189
            CDMA :133,153
         */
        char src[22];
        char pattern_CM[] = "^1(3[4-9]|5[012789]|8[78])\\d{8}$";
        char pattern_UN[] = "^1(3[0-2]|5[56]|8[56])\\d{8}$";
        char pattern_TC[] = "^18[09]\\d{8}$";
        char pattern_CDMA[] = "^1[35]3\\d{8}$";

        fprintf(stderr, "please input your telephone number \n");
        scanf("%s", src);
        fprintf(stderr, "String : %s\n", src);
        fprintf(stderr, "Pattern_CM: \"%s\"\n", pattern_CM);
        fprintf(stderr, "Pattern_UN: \"%s\"\n", pattern_UN);
        fprintf(stderr, "Pattern_TC: \"%s\"\n", pattern_TC);
        fprintf(stderr, "Pattern_CDMA: \"%s\"\n", pattern_CDMA);

        reCM = pcre_compile(pattern_CM, 0, &error, &erroffset, NULL);
        reUN = pcre_compile(pattern_UN, 0, &error, &erroffset, NULL);
        reTC = pcre_compile(pattern_TC, 0, &error, &erroffset, NULL);
        reCDMA = pcre_compile(pattern_CDMA, 0, &error, &erroffset, NULL);

        if (reCM==NULL && reUN==NULL && reTC==NULL && reCDMA==NULL) {
                fprintf(stderr, "PCRE compilation telephone failed at offset %d: %s\n", erroffset, error);
                return 1;
        }

        rcCM = pcre_exec(reCM, NULL, src, strlen(src), 0, 0, ovector, OVECCOUNT);
        rcUN = pcre_exec(reUN, NULL, src, strlen(src), 0, 0, ovector, OVECCOUNT);
        rcTC = pcre_exec(reTC, NULL, src, strlen(src), 0, 0, ovector, OVECCOUNT);
        rcCDMA = pcre_exec(reCDMA, NULL, src, strlen(src), 0, 0, ovector, OVECCOUNT);

        if (rcCM<0 && rcUN<0 && rcTC<0 && rcCDMA<0) {
                if (rcCM==PCRE_ERROR_NOMATCH && rcUN==PCRE_ERROR_NOMATCH &&
                                rcTC==PCRE_ERROR_NOMATCH && rcTC==PCRE_ERROR_NOMATCH) {
                        fprintf(stderr, "Sorry, no match ...\n");
                }
                else {
                        fprintf(stderr, "Matching error %d\n", rcCM);
                        fprintf(stderr, "Matching error %d\n", rcUN);
                        fprintf(stderr, "Matching error %d\n", rcTC);
                        fprintf(stderr, "Matching error %d\n", rcCDMA);
                }
                free(reCM);
                free(reUN);
                free(reTC);
                free(reCDMA);
                return 1;
        }
        fprintf(stderr, "\nOK, has matched ...\n\n");
        if (rcCM > 0) {
                fprintf(stderr, "Pattern_CM: \"%s\"\n", pattern_CM);
                fprintf(stderr, "String : %s\n", src);
        }
        if (rcUN > 0) {
                fprintf(stderr, "Pattern_UN: \"%s\"\n", pattern_UN);
                fprintf(stderr, "String : %s\n", src);
        }
        if (rcTC > 0) {
                fprintf(stderr, "Pattern_TC: \"%s\"\n", pattern_TC);
                fprintf(stderr, "String : %s\n", src);
        }
        if (rcCDMA > 0) {
                fprintf(stderr, "Pattern_CDMA: \"%s\"\n", pattern_CDMA);
                fprintf(stderr, "String : %s\n", src);
        }
        free(reCM);
        free(reUN);
        free(reTC);
        free(reCDMA);
        return 0;
}