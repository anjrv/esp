#ifndef COMMANDS_H_
#define COMMANDS_H_

void set_error(char* err, char* command);
char* get_error(int num_args);
char* command_ping(int num_args);
char* command_mac(int num_args);
char* command_id(int num_args);
char* command_version(int num_args);
char* command_store(int num_args, char** vars, dict* d);
char* command_query(int num_args, char** vars, dict* d);
char* command_push(int num_args, char** vars, stack *pt);
char* command_pop(int num_args, stack *pt);
char* command_add(int num_args, char** vars, stack *pt);

#endif
