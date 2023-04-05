#ifndef _PROC_MANAGER_H
#define _PROC_MANAGER_H
#include "code.h"

typedef struct proc_t{
    char *name;
    int addr;
    code_seq proc_seq; 
    struct proc_t *next; 
} proc_t; 

extern proc_t *create_process(code_seq proc_seq, const char *name, int num_params); 
extern code_seq proc_manager_get_code(); 
extern void proc_manager_store_proc(code_seq proc_seq, const char *name, int num_params); 
extern int proc_manager_get_addr(const char *name); 

#endif
