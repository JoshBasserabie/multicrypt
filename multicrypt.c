#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define TEST_STRING "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define SECURITY_FACTOR 1009
#define KEY_COORDINATE -1

int find_generator(int modulus);
int power(unsigned int x, unsigned int y, unsigned int p);
int evaluatePolynomial(int *polynomialCoefficients, int polynomialDegree, int coordinate, int modulus);
int neville_algo(int *x, int *y, int n, int t);
int field_division(int a, int b);
int generator_helper(int modulus, int currCandidate);

int log_table[SECURITY_FACTOR];
int antilog_table[SECURITY_FACTOR];

union charint {
    char c;
    int m;
};

int main(int argc, char **argv) {
    union charint buff;
    int encryptFlag = 1;
    int encryptKey = 7;
    int decryptKey = 103;
    int modulus = 143;
    int TEST_STRING_LENGTH = strlen(TEST_STRING);
    if(argc != 2 && argc != 3) {
        printf("Usage: multicrypt [-d] <filename>\n");
        printf("-d    decrypt\n");
        exit(1);
    }
    int generator = find_generator(SECURITY_FACTOR);
    //https://crypto.stackexchange.com/questions/12956/multiplicative-inverse-in-operatornamegf28/12962#12962
    for(int x, i = 0; i < SECURITY_FACTOR; i++) {
        x = power(generator, i, SECURITY_FACTOR);
        log_table[x] = i;
        antilog_table[i] = x;
    }
    if(argc == 3) {
        if(strcmp(argv[1], "-d")) {
            printf("Usage: multicrypt [-d] <filename>\n");
            printf("-d    decrypt\n");
            exit(1);
        }
        int keyNum;
        encryptFlag = 0;
        printf("How many decrypt keys will you be entering?\n");
        scanf("%d", &keyNum);
        int *indices = malloc(keyNum * sizeof(int));
        int *keys = malloc(keyNum * sizeof(int));
        int j;
        for(int i = 0; i < keyNum; i++) {
            printf("Enter key: ");
            scanf("%d:%d", &(indices[i]), &(keys[i]));
        }
        decryptKey = neville_algo(indices, keys, keyNum, KEY_COORDINATE);
        argv[1] = argv[2];
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
        printf("How many people would you like to give keys to?\n");
        int keyNum;
        scanf("%d", &keyNum);
        printf("How many of these keys should be required to open the file?\n");
        int minKeys;
        scanf("%d", &minKeys);
        printf("Here are your keys:\n");
        int *polynomialCoefficients = malloc(minKeys * sizeof(int));
        srand(time(0));
        int alternatingSum = 0;
        for(int i = 1; i < minKeys; i++) {
            polynomialCoefficients[i] = rand() % SECURITY_FACTOR;
            alternatingSum += pow(-1, i) * polynomialCoefficients[i];
        }
        alternatingSum %= SECURITY_FACTOR;
        if(alternatingSum < 0) {
            alternatingSum += SECURITY_FACTOR;
        }
        polynomialCoefficients[0] = (decryptKey - alternatingSum) % SECURITY_FACTOR;
        if(polynomialCoefficients[0] < 0) {
            polynomialCoefficients[0] += SECURITY_FACTOR;
        }
        for(int i = 0; i < keyNum; i++) {
            printf("%d:%d\n", i, evaluatePolynomial(polynomialCoefficients, minKeys - 1, i, SECURITY_FACTOR));
        }
    } else {
        printf("Successfully decrypted file.\n");
    }
    // Look up openPGP, AES, RSA (search for C RSA libraries), ECC, Shamir's secret sharing (consider speed)
}

int find_generator(int modulus) {
    return generator_helper(modulus, 2);
}

int generator_helper(int modulus, int currCandidate) {
    int x = 0;
    for(int i = 2; i < modulus - 1; i++) {
        x = power(currCandidate, i, modulus);
        if(x == 1) {
            return generator_helper(modulus, currCandidate + 1);
        }
    }
    return currCandidate;
}

//https://www.geeksforgeeks.org/modular-exponentiation-power-in-modular-arithmetic/
int power(unsigned int x, unsigned int y, unsigned int p) 
{ 
    unsigned int res = 1;      // Initialize result 
  
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

int evaluatePolynomial(int *polynomialCoefficients, int polynomialDegree, int coordinate, int modulus) {
    int sum = 0;
    for(int i = 0; i <= polynomialDegree; i++) {
        sum += (polynomialCoefficients[i] * power(coordinate, i, modulus));
        sum %= modulus;
    }
    return sum;
}

//http://practicalcryptography.com/miscellaneous/machine-learning/fitting-polynomial-set-points/
/************************************************
#### IMPORTANT -- contents of y are destroyed
Inputs: x   -   x values, array[0..N-1].
        y   -   y values, array[0..N-1].
        n   -   number of x or y values
        t   -   interpolation point
Return: the interpolated value of y at x=t
************************************************/
int neville_algo(int *x, int *y, int n, int t) {
    for(int m = 1; m < n; m++){
        for(int i = 0; i < n-m; i++){
            int numerator = ((t-x[i+m])*y[i] + (x[i]-t)*y[i+1]) % SECURITY_FACTOR;
            if(numerator < 0) {
                numerator += SECURITY_FACTOR;
            }
            int denominator = (x[i]-x[i+m]) % SECURITY_FACTOR;
            if(denominator < 0) {
                denominator += SECURITY_FACTOR;
            }
            y[i] = field_division(numerator, denominator);
        }
    }
    return y[0];
}

//https://crypto.stackexchange.com/questions/12956/multiplicative-inverse-in-operatornamegf28/12962#12962
int field_division(int a, int b) {
    if (a == 0 || b == 0) return 0;
    int x = log_table[a];
    int y = log_table[b];
    int log_div = (x - y);
    if(log_div < 0) {
        log_div += SECURITY_FACTOR - 1;
    }
    return antilog_table[log_div];
}
