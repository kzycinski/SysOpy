#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <limits.h>


char static_array[STATIC_ARRAY_SIZE][STATIC_BLOCK_SIZE];

//Creating structure with array, if is_static == 0 then memory is allocated dynamically
struct array_struct *create_struct(int blocks, int block_size, int is_static){
    if(blocks <=0 || block_size <= 0) return NULL;

    struct array_struct *arr_struct = malloc(sizeof(arr_struct));

    arr_struct -> blocks = blocks;
    arr_struct -> block_size = block_size;
    arr_struct -> is_static = is_static;

    //allocating array
    if(is_static)
        arr_struct -> array = (char **) static_array;
    else{
        char **array = malloc(blocks * sizeof(char *));
        arr_struct -> array = array;
    }

    return arr_struct;
}

//deleting array, firstly deleting all blocks then whole structure
void delete_array(struct array_struct *arr_struct){
    for (int i = 0 ; i < arr_struct -> blocks ; i++){
        delete_block(arr_struct, i);
    }

//if array is not allocated satically then free array memory
    if(!arr_struct->is_static) free(arr_struct->array);
    free(arr_struct);
}

//deleting single block
void delete_block(struct array_struct *arr_struct, int index){
    if (index < 0 || index >= arr_struct -> blocks) return;
    if (arr_struct -> array[index] == NULL) return;
    if (arr_struct -> is_static) {
        arr_struct->array[index] = ""; //if array is static then instead of deleting I just make the block empty
        return;
    }
    else {
        free(arr_struct->array[index]);
    }
    return;
}

//adding block on exact index
void add_block(struct array_struct *arr_struct, int index, char *block){
    if (index < 0 || index > arr_struct -> blocks) return;
    if (arr_struct -> array[index] != NULL) return;
    if (strlen(block) >= arr_struct -> block_size) return;
    if (arr_struct -> is_static)
        arr_struct -> array[index] = block;
    else
        arr_struct -> array[index] = strcpy((malloc(arr_struct -> block_size * sizeof(char))), block);
}

//calculate ASCII sum of block
int calculate_block(char *block){
    int tmp = 0;
    for (int i = 0 ; i < strlen(block) ; i++){
        tmp += (int)block[i];
    }
    return tmp;
}

//Finding block with closest sum to one with given index
char *find_block(struct array_struct *arr_struct, int index) {
    if (index < 0 || index >= arr_struct -> blocks) return NULL;
    if (arr_struct -> array[index] == NULL) return NULL;

    char* closest = NULL;
    int min_difference = INT_MAX;
    int sum = calculate_block(arr_struct->array[index]);

    for(int i = 0; i < arr_struct->blocks; i++){
        if(i==index) continue;
        char* block = arr_struct->array[i];
        if(block != NULL){
            int tmp = abs(calculate_block(block) - sum);
            if(min_difference > tmp){
                min_difference = tmp;
                closest = block;
            }
        }
    }
    return closest;
}