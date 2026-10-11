#ifndef VOLTSHIFT_POWER_PLAN_H
#define VOLTSHIFT_POWER_PLAN_H
typedef struct {
    int available, name_available;
    unsigned long error, name_error;
    char guid[40];
    char name[1024];
} PowerPlan;
int power_plan_read(PowerPlan *plan);
#endif
