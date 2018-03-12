//
// Created by kryst on 12.03.2018.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"

char static_array[STATIC_ARRAY_SIZE][STATIC_BLOCK_SIZE];

int main(){
    struct array_struct *arrstruct = create_struct(1000,10,1);
    add_block(arrstruct, 0, "xD");
    add_block(arrstruct, 1, "Ala");
    add_block(arrstruct, 2, "abc");
    add_block(arrstruct, 3, "Alb");
    add_block(arrstruct, 4, "xD");
    add_block(arrstruct, 5, "xxxdddd");

    printf("%s\n", findBlock(arrstruct,1));
    delete_block(arrstruct,3);
    printf("%s\n", findBlock(arrstruct,1));
    delete_array(arrstruct);
    printf("%s\n", findBlock(arrstruct,1));
    return 0;
}
