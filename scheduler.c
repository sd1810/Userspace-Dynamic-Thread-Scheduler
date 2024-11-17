/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * scheduler.c
 */

#undef _FORTIFY_SOURCE
#define STACK_SIZE 1048576

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <setjmp.h>
#include "system.h"
#include "scheduler.h"

struct thread {
    jmp_buf ctx;
    struct {
        char* memory;
        char* memory_;
    } stack;
    struct {
        void* arg;
        scheduler_fnc_t fnc;
    } code;
    enum state {INIT, RUNNING, SLEEPING, TERMINATED} state;
    struct thread* link;
};

struct thread* head;
struct thread* currentThread;
jmp_buf main_context;

void handler(int signalNum)
{
    signal(SIGALRM, handler);
    alarm(1);
    printf("Running signal %d\n", signalNum);
    scheduler_yield();

}

static struct thread* candidate()
{
    struct thread* itr=currentThread;
    while(itr != NULL)
    {
        itr=itr->link;
        if(itr != NULL && (itr->state==INIT || itr->state==SLEEPING)) {
            return itr;
        }      
    }
    currentThread = head;
    if(currentThread->state == TERMINATED)
        return NULL;
    return currentThread;
}

static void schedule()
{
    currentThread = candidate();
    
    if(currentThread == NULL) 
    {
        return;
    }
    else if(currentThread->state == INIT) 
    {
        
        currentThread->state = RUNNING;
        __asm__ volatile ("mov %[rs], %%rsp \n" : : [rs] "r" (currentThread->stack.memory + STACK_SIZE) :);
        currentThread->code.fnc(currentThread->code.arg);
        currentThread->state = TERMINATED;
        longjmp(main_context,1);
    }
    else 
    {
        currentThread->state = RUNNING;
        longjmp(currentThread->ctx,1);
    }

    /*Implement this*/
    

}

static void destroy()
{
    struct thread *t, *next;
    t = head;

    while(t)
    {
        next = t;
        t = t->link;
        free(next->stack.memory_);
        free(next);
    }
}

int scheduler_create(scheduler_fnc_t fnc, void* arg)
{
    struct thread *t;
    t = malloc(sizeof(*t));
    t->state = INIT;
    t->code.fnc = fnc;
    t->code.arg = arg;

    t->stack.memory_ = malloc(STACK_SIZE + page_size());
    t->stack.memory = memory_align(t->stack.memory_, page_size());

    t->link = head;
    head = t;

    return 0;
}

void scheduler_execute()
{
    signal(SIGALRM, handler);
    alarm(1);
    setjmp(main_context);
    schedule();
    destroy();
}

void scheduler_yield()
{
    if(!setjmp(currentThread->ctx))
    {
        currentThread->state = SLEEPING;
        longjmp(main_context, 1);
    }
}