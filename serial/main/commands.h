#ifndef COMMANDS_H_
#define COMMANDS_H_

void set_error(char* err, char* command);
char* get_error();
char* command_ping();
char* command_mac();
char* command_id();
char* command_version();
char* command_store(int num_args, char** vars, dict* d);
char* command_query(int num_args, char** vars, dict* d);
char* command_push(int num_args, char** vars, stack *pt);
char* command_pop(stack *pt);
char* command_add(int num_args, char** vars, stack *pt);

#endif
