#include <stdio.h>
#include <stdlib.h>

int main(){
    int i = 1024;
    while(1){
        int *x = calloc(i, sizeof(int));
        i*=2;
    }
}
