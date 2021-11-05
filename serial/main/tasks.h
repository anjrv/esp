#ifndef TASKS_H_
#define TASKS_H_

#define MAIN_PRIORITY 5 
#define LOW_PRIORITY 1
#define HIGH_PRIORITY 3 
#define WAIT_QUEUE ((TickType_t)(10 / portTICK_PERIOD_MS))
#define DELAY ((TickType_t)(50 / portTICK_PERIOD_MS))

#define ACTIVE 'A'
#define PENDING 'P'
#define COMPLETE0 'C'
#define COMPLETE1 'X'

void initialize_tasks();
void get_result(char *id);
void display_factors();
int prepare_factor(int value, char *id);
int prepare_append(int value, char *id, char *dataset);
int prepare_stat(int value, char *id, char *dataset);
int get_task(char *id);
int change_task(char *id, char state, char *results);

#endif
