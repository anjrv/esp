#ifndef TASKS_H_
#define TASKS_H_

#define MAIN_PRIORITY 0
#define LOW_PRIORITY 1
#define HIGH_PRIORITY 4
#define WAIT_QUEUE ((TickType_t)(10 / portTICK_PERIOD_MS))
#define DELAY ((TickType_t)(50 / portTICK_PERIOD_MS))

void initialize_tasks();
void get_result(char *id);
void display_factors();
int prepare_factor(int value, char *id);
int prepare_append(int value, char *id, char *dataset);

#endif
