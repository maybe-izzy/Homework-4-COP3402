#include <string.h>
#include <stdlib.h>
#include "proc_manager.h"
#include "code.h"

proc_t *proc_list = NULL; 
int current_addr = 1; 

extern proc_t *create_process(code_seq proc_seq, const char *name, int num_params){
    proc_t *new_proc = (proc_t*) malloc(sizeof(proc_t)); 
    new_proc->addr = current_addr; 
    new_proc->name = (char*) malloc(sizeof(name + 1)); 
    strcpy(new_proc->name, name); 
    if (num_params > 0){
        proc_seq = code_seq_concat(proc_seq, code_inc(-1 * num_params)); 
    }
    proc_seq = code_seq_add_to_end(proc_seq, code_rtn()); 
    new_proc->proc_seq = proc_seq;
    new_proc->next = NULL;
    current_addr += code_seq_size(proc_seq); 
    return new_proc;  
}

extern code_seq proc_manager_get_code(){
    code_seq complete_proc_code = code_seq_empty(); 

    while (proc_list != NULL){
        complete_proc_code = code_seq_concat(complete_proc_code, proc_list->proc_seq); 
        proc_list = proc_list->next;
    }
    return complete_proc_code; 
}

extern int proc_manager_get_addr(const char *name){
    proc_t *temp = proc_list; 
    while (temp != NULL){
        if (strcmp(temp->name, name) == 0){ 
            break; 
        }
        temp = temp->next; 
    }
    return temp->addr; 
}

extern void proc_manager_store_proc(code_seq proc_seq, const char *name, int num_params){
    proc_t *new_proc = create_process(proc_seq, name, num_params); 
    if (proc_list == NULL){
        proc_list = new_proc; 
    }
    else {
        proc_t *temp = proc_list; 
        while (temp->next != NULL){
            temp = temp->next; 
        }
        temp->next = new_proc; 
    }
}
