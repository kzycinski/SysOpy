#ifndef SYSOPY_LIBRARY_H
#define SYSOPY_LIBRARY_H

struct array_struct{
    char **array;
    int blocks;
    int block_size;
    int is_static;
};

#define STATIC_ARRAY_SIZE 100000
#define STATIC_BLOCK_SIZE 1000

struct array_struct *create_struct(int blocks, int block_size, int is_static);
void delete_array(struct array_struct *arr_struct);
void add_block(struct array_struct *arr_struct, int index, int size, char *block);
void delete_block(struct array_struct *arr_struct, int index);
char *find_closest_block_sum(struct array_struct *arr_struct, int index);


#endif //SYSOPY_LIBRARY_H