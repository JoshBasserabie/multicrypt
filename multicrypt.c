#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

#define TEST_STRING "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

int power(int x, unsigned int y, int p);

int main(int argc, char **argv) {
    union charint {
        char c;
        int m;
    };
    union charint buff;
    int encryptFlag = 1;
    int encryptKey = 7;
    int decryptKey = 103;
    int modulus = 143;
    int TEST_STRING_LENGTH = strlen(TEST_STRING);
    if(argc != 2 && argc != 4) {
        printf("Error: Please provide a single filename to be encrypted.\n");
        exit(1);
    }
    if(argc == 4) {
        if(strstr(argv[1], "d") != NULL) {
            encryptFlag = 0;
        } else {
            printf("Error: Please provide a single filename to be encrypted.\n");
            exit(1);
        }
        decryptKey = atoi(argv[2]);
        argv[1] = argv[3];
        modulus = 143; // should be inputted?
    } else {
        //generate keys
        encryptKey = 7;
        modulus = 143;
    }
    FILE *fpIn;
    FILE *fpOut;
    if((fpIn = fopen(argv[1], "r")) == NULL) {
        printf("Error: Could not open this file.\n");
        exit(1);
    }
    if((fpOut = fopen("multicrypt_TEMP", "w+")) == NULL) {
        printf("Error: Could not open this file.\n");
        exit(1);
    }
    struct stat *filestats = malloc(sizeof(struct stat));
    if(filestats == NULL) {
        printf("Error: Could not allocate memory for struct stat.\n");
        exit(1);
    }
    stat(argv[1], filestats);
    if(!S_ISREG(filestats->st_mode)) {
        printf("Can only support regular files currently.\n");
        exit(1);
    }
    buff.m = 0;
    if(encryptFlag) {
        for(int i = 0; i < TEST_STRING_LENGTH; i++) {
            buff.c = TEST_STRING[i];
            buff.m = power(buff.m, encryptKey, modulus);
            fputc(buff.c, fpOut);
        }
    } else {
        char *decryptCheckBuff = malloc((TEST_STRING_LENGTH + 1) * sizeof(char));
        decryptCheckBuff[TEST_STRING_LENGTH] = '\0';
        for(int i = 0; i < TEST_STRING_LENGTH; i++) {
            fscanf(fpIn, "%c", &(buff.c));
            buff.m = power(buff.m, decryptKey, modulus);
            decryptCheckBuff[i] = buff.c;
        }
        if(strcmp(decryptCheckBuff, TEST_STRING)) {
            printf("Decryption failed.\n");
            fclose(fpIn);
            fclose(fpOut);
            remove("multicrypt_TEMP");
            exit(1);
        }
    }
    buff.m = 0;
    while(fscanf(fpIn, "%c", &(buff.c)) != EOF) {
        if(encryptFlag) {
            buff.m = power(buff.m, encryptKey, modulus);
        } else {
            buff.m = power(buff.m, decryptKey, modulus);
        }
        fputc(buff.c, fpOut);
    }
    rename("multicrypt_TEMP", argv[1]);
    fclose(fpIn);
    fclose(fpOut);
    if (encryptFlag) {
        printf("Successfully encrypted file. Your decrypt key is %d\n", decryptKey);
    } else {
        printf("Successfully decrypted file.\n");
    }
    // Look up openPGP, AES, RSA (search for C RSA libraries), ECC, Shamir's secret sharing (consider speed)
}

int power(int x, unsigned int y, int p) 
{ 
    int res = 1;      // Initialize result 
  
    x = x % p;  // Update x if it is more than or  
                // equal to p 
  
    while (y > 0) 
    { 
        // If y is odd, multiply x with result 
        if (y & 1) {
            res = (res*x) % p; 
        }
  
        // y must be even now 
        y >>= 1; // y = y/2 
        x = (x * x) % p;   
    } 
    return res; 
} 

// p = 100003, q = 100417
// n = 10042001251
// totient = 10041800832
// e = 13
// d = 5407123525
