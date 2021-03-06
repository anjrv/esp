#ifndef COMMANDS_H_
#define COMMANDS_H_

void set_error(char *err, char *command);
void get_error();
void command_ping();
void command_mac();
void command_id();
void command_version();
void command_store(int num_args, char **vars, dict *d);
void command_query(int num_args, char **vars, dict *d);
void command_push(int num_args, char **vars, stack *pt);
void command_pop(stack *pt);
void command_add(int num_args, char **vars, stack *pt, dict *d);
void command_ps();
void command_result(int num_args, char **vars);
void command_factor(int num_args, char **vars, int counter, stack *pt, dict *d);
void command_bt_connect(int num_args, char **vars);
void command_bt_status();
void command_bt_close();
void command_data_create(int num_args, char **vars);
void command_data_destroy(int num_args, char **vars);
void command_data_info(int num_args, char **vars);
void command_data_append(int num_args, char **vars, int counter);
void command_data_raw(int num_args, char **vars);
void command_data_stat(int num_args, char **vars, int counter);
void command_net_locate();
void command_net_table();
void command_net_reset();
void command_net_status();

#endif
