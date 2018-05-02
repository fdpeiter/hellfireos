//
// Created by Felipe Peiter on 4/29/18.
//
#include <hellfire.h>

struct tcb_entry* aptask;

static void ap_queue_next()
{
    aptask = hf_queue_remhead(krnl_ap_queue);
    if (!aptask) {
        hf_yield();
        panic(PANIC_NO_TASKS_AP);
        continue;
    }
    if (hf_queue_addtail(krnl_ap_queue, aptask))
        panic(PANIC_CANT_PLACE_AP);
}

void ap_sched(void)
{
    int32_t rc;

    for(;;){
        ap_queue_next();
        if(aptask->apjobs == 0) {
            // TODO: Verificar se deve matar no começo ou no fim
            hf_kill(aptask->id);
            continue;
        }
        aptask->apjobs--;

        krnl_task = &krnl_tcb[krnl_current_task];

        rc = _context_save(krnl_task->task_context);
        if (rc)
            continue;
        aptask->state = TASK_RUNNING;
        // Copied from scheduler.c
        if (krnl_task->state == TASK_RUNNING)
            krnl_task->state = TASK_READY;
        if (krnl_task->pstack[0] != STACK_MAGIC)
            panic(PANIC_STACK_OVERFLOW);
        _context_restore(krnl->task_context, 1);
    }
}

// Copied from sched_test2.c
void task(void){
    int32_t jobs, id;

    id = hf_selfid();
    for(;;){
        jobs = hf_jobs(id);
        printf("\n%s (%d)[%d][%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id));
        while (jobs == hf_jobs(id));
    }
}

// Copied and adapted from sched_lottery
void randomAP(void)
{
    int32_t id, delay = 0;
    id = hf_selfid();
    for(;;){
        hf_spawn(task, 0, 3, 0, "random_ap_task" + delay, 1024);
        // Corrigido o erro que o r iniciava em 0
        delay = (1 + (random() % 9)) * 50;
        printf("\n%s (%d)[%d][%d]", hf_selfname(), id, hf_jobs(id), hf_dlm(id));
        delay_ms(delay);
    }
}

void app_main(void){
    int32_t i;
    hf_spawn(ap_sched, 1, 1, 1, "poolingServer", 1024);
    hf_spawn(task, 1, 1, 2, "task 1", 1024);
    hf_spawn(task, 8, 1, 2, "task 2", 1024);
    hf_spawn(task, 5, 2, 4, "task 3", 1024);
    hf_spawn(randomAP, 0, 0, 0, "random", 1024);
}

