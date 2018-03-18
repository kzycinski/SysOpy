#ifndef LAB2_SYSLIBRARY_H
#define LAB2_SYSLIBRARY_H

void system_copy(char *source_file, char *target_file, int records, int record_length);
void system_sort(char *source_file, int records, int record_length);
void generate(char *file_name, int records, int record_length);
void library_copy(char *source_file, char *target_file, int records, int record_length);
void library_sort(char *source_file, int records, int record_length);

#endif //LAB2_SYSLIBRARY_H
