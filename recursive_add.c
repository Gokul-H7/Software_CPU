#include <stdio.h>

int add(int x){
    if (x == 1){
        return 1;
    }
    
    return x + add(x - 1);
}

int main (void){
    int x = 6;
    printf("%d\n", add(x));
}

