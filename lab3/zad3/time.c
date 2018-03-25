#include <stdlib.h>

int main(){
    long double x =101010;
    long double y = 101.32131;
    while(1){
        x *= 10;
        y *= 10;
        x += 1;
        y += 1;
        x /= 10;
        y /= 10;
    }
}
