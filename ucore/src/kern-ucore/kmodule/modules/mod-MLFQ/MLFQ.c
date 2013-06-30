#include <types.h>
#include <mod.h>
#include <kio.h>
#include <sched.h>
#include <list.h>
#include <sched_RR.h>
#include <sched_MLFQ.h>
#include <proc.h>

extern struct run_queue *rq;
extern struct sched_class *sched_class;
extern struct sched_class RR_sched_class;
static struct sched_class *RR_class;

extern inline bool mod_init_save(void);
extern inline void mod_init_restore(bool flag);

static void MLFQ_init(struct run_queue *rq)
{
}

static void MLFQ_enqueue(struct run_queue *rq, struct proc_struct *proc)
{
	struct run_queue *nrq = rq;
	if (proc->rq != NULL && proc->time_slice == 0) { 
		nrq = le2rq(list_next(&(proc->rq->rq_link)), rq_link);
		if (nrq == rq) {
			nrq = proc->rq;
		}
	}
	RR_class->enqueue(nrq, proc);
}

static void MLFQ_dequeue(struct run_queue *rq, struct proc_struct *proc)
{
	RR_class->dequeue(proc->rq, proc);
}

static struct proc_struct *MLFQ_pick_next(struct run_queue *rq)
{
	struct proc_struct *next;
	list_entry_t *list = &(rq->rq_link), *le = list;
	do {
		if ((next = RR_class->pick_next(le2rq(le, rq_link))) != NULL) {
			break;
		}
		le = list_next(le);
	} while (le != list);
	return next;
}

static void MLFQ_proc_tick(struct run_queue *rq, struct proc_struct *proc)
{
	RR_class->proc_tick(proc->rq, proc);
}   


struct sched_class MLFQ_class = {
	.name = "MLFQ_scheduler",
	.init = MLFQ_init,
	.enqueue = MLFQ_enqueue,
	.dequeue = MLFQ_dequeue,
	.pick_next = MLFQ_pick_next,
	.proc_tick = MLFQ_proc_tick,
};


static __init int MLFQ_mod_init() {
    bool intr_flag;
    intr_flag = mod_init_save();

    kprintf("MLFQ_init!\n");

    sched_class=&MLFQ_class;
    kprintf("sched_class = %s\n",sched_class->name); 
    kprintf("proc_num = %d\n",rq->proc_num);

    RR_class = &RR_sched_class;
    list_entry_t *list = &(rq->rq_link), *le = list;
    le = list_next(le);
    do {
	RR_class->init(le2rq(le, rq_link));
	le = list_next(le);
    } while (le != list);

    mod_init_restore(intr_flag);
    return 0;
}


static __exit void MLFQ_mod_exit() {

    bool intr_flag;
    intr_flag = mod_init_save();
    kprintf("MLFQ_exit!\n");

    kprintf("No.1 run queue: %d\n",rq->proc_num);
    list_entry_t *list = &(rq->rq_link), *le = list;
    le = list_next(le);
    int rq_num = 1;
    do {
	struct run_queue *temp = le2rq(le, rq_link);
	rq_num++;
	kprintf("No.%d run queue: %d\n",rq_num,temp->proc_num);
	while(temp->proc_num>0)
	{
		list_entry_t *temp_le = list_next(&(temp->run_list));
		if (temp_le != &(temp->run_list)) {
			 struct proc_struct *temp_proc = le2proc(temp_le, run_link);
			 RR_class->dequeue(temp, temp_proc);
			 RR_class->enqueue(rq, temp_proc);
		}
	}
	le = list_next(le);
    } while (le != list);
    sched_class=&RR_sched_class;

    kprintf("sched_class = %s\n",sched_class->name);
    kprintf("proc_num = %d\n",rq->proc_num);
    mod_init_restore(intr_flag);

}

module_init(MLFQ_mod_init);
module_exit(MLFQ_mod_exit);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
    .name = "mod-MLFQ",
    .init = MLFQ_mod_init,
    .exit = MLFQ_mod_exit,
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";
