
void free_string_array(char** arr);
void free_string_array_leave_strings(char** arr);
char** split(const char* str, const char* delim);
char *read_all(int sock, char *buffer, size_t length);
char *read_until(int sock, const char *delim, int include_delim);
