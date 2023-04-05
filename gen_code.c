/* $Id: gen_code_stubs.c,v 1.4 2023/03/23 20:01:03 leavens Exp leavens $ */
#include "utilities.h"
#include "gen_code.h"
#include "symtab.h"
#include "scope_check.h"
#include "id_attrs.h"
#include "label.h"
#include "id_use.h"
#include "proc_manager.h"

void gen_code_initialize(){
    symtab_initialize(); 
    symtab_enter_scope(); 
}

// Generate code for the given AST
code_seq gen_code_program(AST *prog){
    gen_code_procDecls(prog->data.program.pds); 
    code_seq prog_seq = proc_manager_get_code(); 
    
    if (!ast_list_is_empty(prog->data.program.pds)){
         code_seq jump = code_seq_singleton(code_jmp(code_seq_size(prog_seq) + 1)); 
         prog_seq = code_seq_concat(jump, prog_seq); 
    }
    // Prevent generating code for program processes twice 
    prog->data.program.pds = NULL; 

    prog_seq = code_seq_concat(prog_seq, code_inc(3)); 
    prog_seq = code_seq_concat(prog_seq, gen_code_block(prog)); 
    prog_seq = code_seq_add_to_end(prog_seq, code_hlt());

    code_seq_fix_labels(prog_seq);
    return prog_seq;
}

// generate code for blk
code_seq gen_code_block(AST *blk){
    gen_code_procDecls(blk->data.program.pds); 
    code_seq blk_seq = gen_code_constDecls(blk->data.program.cds);
    blk_seq = code_seq_concat(blk_seq, gen_code_varDecls(blk->data.program.vds)); 
    blk_seq = code_seq_concat(blk_seq, gen_code_stmt(blk->data.program.stmt));  
    return blk_seq;
}

// generate code for the declarations in cds
code_seq gen_code_constDecls(AST_list cds){
    code_seq head = code_seq_empty(); 

    while (!ast_list_is_empty(cds)) {
        if (code_seq_is_empty(head)){
            head = gen_code_constDecl(ast_list_singleton(cds)); 
        }
        else {
            code_seq_add_to_end(head, gen_code_constDecl(ast_list_singleton(cds))); 
        }
	    cds = ast_list_rest(cds);
    }
    return head;
}

// generate code for the const declaration cd
code_seq gen_code_constDecl(AST *cd){
    code_seq const_decl = code_lit(cd->data.const_decl.num_val); 
    return const_decl;
}

// generate code for the declarations in vds
code_seq gen_code_varDecls(AST_list vds){
    code_seq head = code_seq_empty(); 

    while (!ast_list_is_empty(vds)) {
        if (code_seq_is_empty(head)){
            head = gen_code_varDecl(ast_list_singleton(vds)); 
        }
        else {
            code_seq_add_to_end(head, gen_code_varDecl(ast_list_singleton(vds))); 
        }
	    vds = ast_list_rest(vds);
    }
    return head;
}

// generate code for the var declaration vd
code_seq gen_code_varDecl(AST *vd){
    return code_inc(1);
}

// generate code for the declarations in pds
void gen_code_procDecls(AST_list pds){
    while (!ast_list_is_empty(pds)){
        gen_code_procDecl(pds);  
        pds = ast_list_rest(pds); 
    }
}

// generate code for the procedure declaration pd
void gen_code_procDecl(AST *pd){
    // Create attributes and insert into symbol table
    id_attrs *attrs = id_attrs_proc_create(pd->data.proc_decl.block->file_loc, pd->data.proc_decl.lab); 
    symtab_insert(pd->data.proc_decl.name, attrs); 
    
    // Generate and save procedure
    code_seq proc_decl_seq = gen_code_block(pd->data.proc_decl.block); 
    int num_params = ast_list_size(pd->data.proc_decl.block->data.program.cds) + ast_list_size(pd->data.proc_decl.block->data.program.vds); 
    proc_manager_store_proc(proc_decl_seq, pd->data.proc_decl.name, num_params); 

    // Correct the procedure label for future call instructions
    pd->data.proc_decl.lab->addr = proc_manager_get_addr(pd->data.proc_decl.name); 
    pd->data.proc_decl.lab->is_set = true; 
}

// generate code for the statement
code_seq gen_code_stmt(AST *stmt){
    if (stmt != NULL){
        switch(stmt->type_tag){
        case skip_ast: 
            return gen_code_skipStmt(stmt); 
            break; 
        case write_ast: 
            return gen_code_writeStmt(stmt); 
            break;
        case assign_ast: 
            return gen_code_assignStmt(stmt); 
            break; 
        case begin_ast: 
            return gen_code_beginStmt(stmt); 
            break;
        case if_ast: 
            return gen_code_ifStmt(stmt); 
            break; 
        case while_ast: 
            return gen_code_whileStmt(stmt); 
            break; 
        case call_ast: 
            return gen_code_callStmt(stmt);
            break;
        default: 
            // handle error
            return code_seq_empty(); 
        }
    }
    return code_seq_empty();
}

// generate code for the statement
code_seq gen_code_assignStmt(AST *stmt){
    code_seq assign_seq = code_compute_fp(stmt->data.assign_stmt.ident->data.ident.idu->levelsOutward); 
    assign_seq = code_seq_concat(assign_seq, gen_code_expr(stmt->data.assign_stmt.exp)); 
    int offset = stmt->data.assign_stmt.ident->data.ident.idu->attrs->loc_offset; 
    assign_seq = code_seq_concat(assign_seq, code_sto(offset)); 
    return assign_seq; 
}

// generate code for the statement
code_seq gen_code_callStmt(AST *stmt){
    id_use *idu = symtab_lookup(stmt->data.call_stmt.ident->data.ident.name); 
    code_seq call_seq = code_cal(idu->attrs->lab); 
    return call_seq; 
}

// generate code for the statement
code_seq gen_code_beginStmt(AST *stmt){
    AST_list head = stmt->data.begin_stmt.stmts; 
    code_seq begin_stmt_seq = code_seq_empty(); 

    while (!ast_list_is_empty(head)){
        begin_stmt_seq = code_seq_concat(begin_stmt_seq, gen_code_stmt(head)); 
        head = ast_list_rest(head); 
    }
    return begin_stmt_seq; 
}

// generate code for the statement
code_seq gen_code_ifStmt(AST *stmt){
    code_seq if_stmt_seq = gen_code_cond(stmt->data.if_stmt.cond); 
    if_stmt_seq = code_seq_concat(if_stmt_seq, code_jpc(2));
    
    code_seq then_stmt_seq = gen_code_stmt(stmt->data.if_stmt.thenstmt); 
    if_stmt_seq = code_seq_concat(if_stmt_seq, code_jmp(code_seq_size(then_stmt_seq)+ 2)); 
    if_stmt_seq = code_seq_concat(if_stmt_seq, then_stmt_seq); 

    code_seq else_stmt_seq = gen_code_stmt(stmt->data.if_stmt.elsestmt); 
    if_stmt_seq = code_seq_concat(if_stmt_seq, code_jmp(code_seq_size(else_stmt_seq)+ 1)); 
    if_stmt_seq = code_seq_concat(if_stmt_seq, else_stmt_seq); 

    return if_stmt_seq; 
}

// generate code for the statement
code_seq gen_code_whileStmt(AST *stmt){
    code_seq while_stmt_seq = code_seq_empty(); 

    code_seq C = gen_code_cond(stmt->data.while_stmt.cond); 
    while_stmt_seq = code_seq_concat(while_stmt_seq, C); 
    while_stmt_seq = code_seq_concat(while_stmt_seq, code_jpc(2)); 

    code_seq S = gen_code_stmt(stmt->data.while_stmt.stmt); 
    while_stmt_seq = code_seq_concat(while_stmt_seq, code_jmp(code_seq_size(S) + 2)); 
    while_stmt_seq = code_seq_concat(while_stmt_seq, S); 
    while_stmt_seq = code_seq_concat(while_stmt_seq, code_jmp(-1 * code_seq_size(C))); 
    return while_stmt_seq;
}

// generate code for the statement
code_seq gen_code_readStmt(AST *stmt){
    int levels_out = stmt->data.read_stmt.ident->data.ident.idu->levelsOutward; 
    int offset = stmt->data.read_stmt.ident->data.ident.idu->attrs->loc_offset; 

    code_seq read_seq = code_compute_fp(levels_out); 
    read_seq = code_seq_concat(read_seq, code_chi()); 
    read_seq = code_seq_concat(read_seq, code_sto(offset)); 
    return read_seq; 
}

// generate code for the statement
code_seq gen_code_writeStmt(AST *stmt){
    code_seq expr = gen_code_expr(stmt->data.write_stmt.exp);  
    code_seq write = code_cho(); 
    code_seq write_stmt = code_seq_add_to_end(expr, write); 

    return write_stmt;
}

// generate code for the statement
code_seq gen_code_skipStmt(AST *stmt){
    return code_nop();
}

// generate code for the condition
code_seq gen_code_cond(AST *cond){
    switch (cond->type_tag){
        case odd_cond_ast: 
            return gen_code_odd_cond(cond); 
            break; 
        case bin_cond_ast: 
            return gen_code_bin_cond(cond); 
            break;
        default: 
            // Handle errors here
             return code_seq_empty();
    }
}

// generate code for the condition
code_seq gen_code_odd_cond(AST *cond){
    code_seq odd_seq = gen_code_expr(cond->data.odd_cond.exp); 
    odd_seq = code_seq_concat(odd_seq, code_lit(2)); 
    odd_seq = code_seq_concat(odd_seq, code_mod()); 
    return odd_seq; 
}

// generate code for the condition
code_seq gen_code_bin_cond(AST *cond){
    code_seq bin_cond_seq = gen_code_expr(cond->data.bin_cond.leftexp); 
    bin_cond_seq = code_seq_concat(bin_cond_seq, gen_code_expr(cond->data.bin_cond.rightexp)); 
    

    code_seq evaluate = code_seq_empty(); 
    switch (cond->data.bin_cond.relop){
        case eqop:
            evaluate = code_eql(); 
            break; 
        case neqop: 
            evaluate = code_neq(); 
            break; 
        case ltop: 
            evaluate = code_lss(); 
            break; 
        case leqop: 
            evaluate = code_leq(); 
            break; 
        case gtop: 
            evaluate = code_gtr(); 
            break;
        case geqop: 
            evaluate = code_geq(); 
            break; 
        default: 
            // Error handle here
    }
    
    bin_cond_seq = code_seq_concat(bin_cond_seq, evaluate); 

    return bin_cond_seq; 
}

// generate code for the expresion
code_seq gen_code_expr(AST *exp){
    switch (exp->type_tag){
        case bin_expr_ast: 
            return gen_code_bin_expr(exp); 
            break; 
        case ident_ast: 
            return gen_code_ident_expr(exp); 
            break; 
        case number_ast:  
            return gen_code_number_expr(exp); 
            break; 
        default: 
            // error handle here
    }   
    return code_seq_empty();
}

// generate code for the expression (exp)
code_seq gen_code_bin_expr(AST *exp){
    code_seq left_seq = gen_code_expr(exp->data.bin_expr.leftexp); 
    code_seq right_exp = gen_code_expr(exp->data.bin_expr.rightexp); 

    code_seq evaluate = code_seq_empty(); 
    switch (exp->data.bin_expr.arith_op){
        case addop:
            evaluate = code_add(); 
            break; 
        case subop: 
            evaluate = code_sub(); 
            break; 
        case multop: 
            evaluate = code_mul(); 
            break; 
        case divop: 
            evaluate = code_div(); 
            break; 
        default: 
            // Error handle here
    }

    code_seq ret = code_seq_concat(left_seq, right_exp); 
    ret = code_seq_concat(ret, evaluate); 
    return ret; 
}

// generate code for the ident expression (ident)
code_seq gen_code_ident_expr(AST *ident){
    code_seq ident_expr = code_compute_fp(ident->data.ident.idu->levelsOutward); 
    int offset = ident->data.ident.idu->attrs->loc_offset;  
    ident_expr = code_seq_concat(ident_expr, code_lod(offset)); 
    return ident_expr; 
}

// generate code for the number expression (num)
code_seq gen_code_number_expr(AST *num){
    return code_lit(num->data.number.value); 
}