#include <nautilus/nautilus.h>
#include <nautilus/cpu.h>
#include <nautilus/naut_assert.h>
#include <nautilus/irq.h>
#include <nautilus/idle.h>
#include <nautilus/paging.h>
#include <nautilus/thread.h>
#include <nautilus/percpu.h>
#include <nautilus/atomic.h>
#include <nautilus/queue.h>
#include <nautilus/list.h>
#include <nautilus/errno.h>

#include <lib/liballoc.h>

#ifndef NAUT_CONFIG_DEBUG_THREADS
#undef  DEBUG_PRINT
#define DEBUG_PRINT(fmt, args...)
#endif
#define SCHED_PRINT(fmt, args...) printk("SCHED: " fmt, ##args)
#define SCHED_DEBUG(fmt, args...) DEBUG_PRINT("SCHED: " fmt, ##args)

static unsigned long next_tid = 0;
static nk_thread_queue_t * global_thread_list;

extern addr_t boot_stack_start;
extern void nk_thread_switch(nk_thread_t*);
extern void nk_thread_entry(void *);
static struct nk_tls tls_keys[TLS_MAX_KEYS];


/****** SEE BELOW FOR EXTERNAL THREAD INTERFACE ********/


nk_thread_queue_t*
nk_thread_queue_create (void) 
{
    nk_thread_queue_t * q = NULL;

    q = nk_queue_create();

    if (!q) {
        ERROR_PRINT("Could not allocate thread queue\n");
        return NULL;
    }
    return q;
}



/* NOTE: this does not delete the threads in the queue, just
 * their entries in the queue
 */
void 
nk_thread_queue_destroy (nk_thread_queue_t * q)
{
    // free any remaining entries
    SCHED_DEBUG("Destroying thread queue\n");
    nk_queue_destroy(q, 1);
}


inline void 
nk_enqueue_thread_on_runq (nk_thread_t * t, int cpu)
{
    nk_thread_queue_t * q = NULL;
    struct sys_info * sys = per_cpu_get(system);

    if (cpu == CPU_ANY || 
        cpu < CPU_ANY  || 
        cpu >= sys->num_cpus || 
        cpu == my_cpu_id()) {

        q = per_cpu_get(run_q);

    } else {
        q = sys->cpus[cpu]->run_q;
    }

    /* bail if the run queue hasn't been created yet */
    if (!q) {
        ERROR_PRINT("Attempt (by cpu %d) to enqueue thread (tid=%d) on non-existant run queue (cpu=%u)\n", my_cpu_id(), t->tid, cpu);
        return;
    }

    t->cur_run_q = q;
    t->status    = NK_THR_RUNNING;

    nk_enqueue_entry_atomic(q, &(t->runq_node));
}


static inline void 
enqueue_thread_on_waitq (nk_thread_t * waiter, nk_thread_queue_t * waitq)
{
    /* are we already waiting on another queue? */
    if (waiter->status == NK_THR_WAITING) {
        panic("Attempt to put thread on more than one wait queue\n");
        //return;
    } else {
        waiter->status = NK_THR_WAITING;
    }

    nk_enqueue_entry_atomic(waitq, &(waiter->wait_node));
}


static inline nk_thread_t*
dequeue_thread_from_waitq (nk_thread_t * waiter, nk_thread_queue_t * waitq)
{
    nk_thread_t * t        = NULL;
    nk_queue_entry_t * elm = nk_dequeue_entry_atomic(waitq, &(waiter->wait_node));

    t = container_of(elm, nk_thread_t, wait_node);

    if (t) {
        t->status = NK_THR_SUSPENDED;
    }

    return t;
}


inline nk_thread_t*
nk_dequeue_thread_from_runq (nk_thread_t * t)
{
    nk_thread_queue_t * q = t->cur_run_q;
    nk_queue_entry_t * elm = NULL;
    nk_thread_t * ret = NULL;

    /* bail if the run queue doesn't exist */
    if (!q) {
        ERROR_PRINT("Attempt to dequeue thread not on run queue (cpu=%u)\n", my_cpu_id());
        return NULL;
    }

    elm = nk_dequeue_entry_atomic(q, &(t->runq_node));
    ret = container_of(elm, nk_thread_t, runq_node);

    t->status    = NK_THR_SUSPENDED;
    t->cur_run_q = NULL;

    return ret;
}


static inline void
enqueue_thread_on_tlist (nk_thread_t * t)
{
    nk_thread_queue_t * q = global_thread_list;
    nk_enqueue_entry_atomic(q, &(t->thr_list_node));
}


static inline nk_thread_t*
dequeue_thread_from_tlist (nk_thread_t * t)
{
    nk_queue_entry_t * elm = NULL;
    nk_thread_t * ret = NULL;

    nk_thread_queue_t * q = global_thread_list;
    elm = nk_dequeue_entry_atomic(q, &(t->thr_list_node));
    ret = container_of(elm, nk_thread_t, thr_list_node);

    return ret;
}


/*
 * thread_detach
 *
 * detaches a child from its parent
 *
 * assumes interrupts are off
 *
 * @t: the thread to detach
 *
 */
static int
thread_detach (nk_thread_t * t)
{
    ASSERT(t->refcount > 0);

    /* remove me from my parent's child list */
    list_del(&(t->child_node));

    if (--t->refcount == 0) {
        nk_thread_destroy(t);
    }

    return 0;
}




static void 
tls_exit (void) 
{
    nk_thread_t * t = get_cur_thread();
    unsigned i, j;
    uint8_t called = 0;

    for (i = 0; i < MIN_DESTRUCT_ITER; i++) {
        for (j = 0 ; j < TLS_MAX_KEYS; j++) {
            void * val = (void*)t->tls[j]; 
            if (val && tls_keys[j].destructor) {
                called = 1;
                t->tls[j] = NULL;
                tls_keys[j].destructor(val);
            }

            if (!called) {
                break;
            }
        }
    }
}






/*
 * get_runnable_thread
 *
 * get the next thread in the specified thread's CPU
 *
 */
static nk_thread_t *
get_runnable_thread (uint32_t cpu)
{
    nk_thread_t * runnable   = NULL;
    nk_thread_queue_t * runq = NULL;
    nk_queue_entry_t * elm   = NULL;
    struct sys_info * sys = per_cpu_get(system);

    if (cpu >= sys->num_cpus || !sys->cpus[cpu]) {
        ERROR_PRINT("Attempt to get thread on invalid CPU (%u)\n", cpu);
        return NULL;
    }

    runq = sys->cpus[cpu]->run_q;

    if (!runq) {
        ERROR_PRINT("Attempt to get thread from invalid run queue on CPU %u\n", cpu);
        return NULL;
    }

    elm = nk_dequeue_first_atomic(runq);
    if (!elm) {
        return NULL;
    }
    return container_of(elm, nk_thread_t, runq_node);
}


static nk_thread_t * 
get_runnable_thread_myq (void) 
{
    cpu_id_t id = my_cpu_id();
    return get_runnable_thread(id);
}



static int
thread_init (nk_thread_t * t, 
             void * stack, 
             uint8_t is_detached, 
             int cpu, 
             nk_thread_t * parent)
{

    if (!t) {
        ERROR_PRINT("Given NULL thread pointer...\n");
        return -EINVAL;
    }

    t->stack      = stack;
    t->rsp        = (uint64_t)stack + t->stack_size;
    t->tid        = atomic_inc(next_tid) + 1;
    t->refcount   = is_detached ? 1 : 2; // thread references itself as well
    t->parent     = parent;
    t->bound_cpu  = cpu;
    t->fpu_state_offset = offsetof(struct nk_thread, fpu_state);

    INIT_LIST_HEAD(&(t->children));

    /* I go on my parent's child list */
    if (parent) {
        list_add_tail(&(t->child_node), &(parent->children));
    }

    t->waitq = nk_thread_queue_create();
    if (!t->waitq) {
        ERROR_PRINT("Could not create thread's wait queue\n");
        return -EINVAL;
    }

    return 0;
}


static void
thread_cleanup (void)
{
    SCHED_DEBUG("Thread (%d) exiting on core %d\n", get_cur_thread()->tid, my_cpu_id());
    nk_thread_exit(0);
}


/*
 * utility function for setting up
 * a thread's stack 
 */
static inline void
thread_push (nk_thread_t * t, uint64_t x)
{
    t->rsp -= 8;
    *(uint64_t*)(t->rsp) = x;
}


static void
thread_setup_init_stack (nk_thread_t * t, nk_thread_fun_t fun, void * arg)
{

#define RSP_STACK_OFFSET   8
#define GPR_RDI_OFFSET     48
#define GPR_RAX_OFFSET     8
#define GPR_SAVE_SIZE      120
#define STACK_SAVE_SIZE    64
#define THREAD_SETUP_SIZE  (STACK_SAVE_SIZE + GPR_SAVE_SIZE)

    /*
     * if this is a thread fork, this part is taken care of
     * in _thread_fork(). There is no function!
     */
    if (fun) {
        thread_push(t, (uint64_t)&thread_cleanup);
        thread_push(t, (uint64_t)fun);
    }

    thread_push(t, (uint64_t)KERNEL_SS);                 // SS
    thread_push(t, (uint64_t)(t->rsp+RSP_STACK_OFFSET)); // rsp
    thread_push(t, (uint64_t)0UL);                       // rflags
    thread_push(t, (uint64_t)KERNEL_CS);
    thread_push(t, (uint64_t)&nk_thread_entry);
    thread_push(t, 0);                                   // intr no

    /*
     * if we have a function, it needs an input argument 
     * so we overwrite its RDI
     */  
    if (fun) {
	*(uint64_t*)(t->rsp-GPR_RDI_OFFSET) = (uint64_t)arg; 
    }

    /* 
     * if this is a thread fork, we return the child's thread_id_t
     * via RAX - note that _fork_return will not restore RAX
     */
    if (!fun) {
        *(uint64_t*)(t->rsp-GPR_RAX_OFFSET) = (uint64_t)t;
    }

    t->rsp -= GPR_SAVE_SIZE;                             // account for the GPRS;
}




/****** EXTERNAL THREAD INTERFACE ******/



/* 
 * nk_thread_create
 *
 * creates a thread. 
 * its stack wil not be initialized with any intial data, 
 * and it will go on the thread list, but it wont be runnable
 *
 * @fun: the function to run
 * @input: the argument to the thread
 * @output: where the thread should store its output
 * @is_detached: true if this thread won't be attached to a parent (it will
 *               die immediately when it exits)
 * @stack_size: size of the thread's stack. 0 => let us decide
 * @tid: opaque user object for the thread to be set (this is the output)
 * @cpu: cpu on which to bind the thread. CPU_ANY means any CPU
 *
 * return: on error returns -EINVAL, returns 0 on success
 *
 */
int
nk_thread_create (nk_thread_fun_t fun, 
                  void * input,
                  void ** output,
                  uint8_t is_detached,
                  nk_stack_size_t stack_size,
                  nk_thread_id_t * tid,
                  int cpu)
{
    nk_thread_t * t = NULL;
    void * stack    = NULL;

    if (cpu >= per_cpu_get(system)->num_cpus && cpu != CPU_ANY) {
        ERROR_PRINT("thread create received invalid CPU id (%u)\n", cpu);
        return -EINVAL;
    }

    t = malloc(sizeof(nk_thread_t));
    if (!t) {
        ERROR_PRINT("Could not allocate thread struct\n");
        return -EINVAL;
    }
    memset(t, 0, sizeof(nk_thread_t));

    if (stack_size) {
        stack = (void*)malloc(stack_size);
        t->stack_size = stack_size;
    } else {
        stack = (void*)malloc(PAGE_SIZE_4KB);
        t->stack_size =  PAGE_SIZE_4KB;
    }

    if (!stack) {
        ERROR_PRINT("Could not allocate thread stack\n");
        goto out_err;
    }

    if (thread_init(t, stack, is_detached, cpu, get_cur_thread()) < 0) {
        ERROR_PRINT("Could not initialize thread\n");
        goto out_err1;
    }

    t->status = NK_THR_INIT;
    
    enqueue_thread_on_tlist(t);

    if (tid) {
        *tid = (nk_thread_id_t)t;
    }

    SCHED_DEBUG("Thread create creating new thread with t=%p, tid=%lu\n", t, t->tid);

    return 0;

out_err1:
    free(stack);
out_err:
    free(t);
    return -1;
}


/* 
 * nk_thread_start
 *
 * creates a thread and puts it on the specified cpu's run 
 * queue
 *
 * @fun: the function to run
 * @input: the argument to the thread
 * @output: where the thread should store its output
 * @is_detached: true if this thread won't be attached to a parent (it will
 *               die immediately when it exits)
 * @stack_size: size of the thread's stack. 0 => let us decide
 * @tid: the opaque pointer passed to the user (output variable)
 * @cpu: cpu on which to bind the thread. CPU_ANY means any CPU 
 *
 *
 * on error, returns -EINVAL, otherwise 0
 */
int
nk_thread_start (nk_thread_fun_t fun, 
                 void * input,
                 void ** output,
                 uint8_t is_detached,
                 nk_stack_size_t stack_size,
                 nk_thread_id_t * tid,
                 int cpu)
{
    nk_thread_id_t newtid   = NULL;
    nk_thread_t * newthread = NULL;

    if (nk_thread_create(fun, input, output, is_detached, stack_size, &newtid, cpu) < 0) {
        ERROR_PRINT("Could not create thread\n");
        return -1;
    }

    newthread = (nk_thread_t*)newtid;

    *tid = newtid;

    thread_setup_init_stack(newthread, fun, input);

    newthread->status = NK_THR_RUNNING;

    nk_enqueue_thread_on_runq(newthread, cpu);
    if (cpu == CPU_ANY) {
        SCHED_DEBUG("Started thread (%p, tid=%u) on [ANY CPU]\n", newthread, newthread->tid); 
    } else {
        SCHED_DEBUG("Started thread (%p, tid=%u) on cpu %u\n", newthread, newthread->tid, cpu); 
    }

    return 0;
}


/*
 * wake_waiters
 *
 * wake all threads that are waiting on me
 *
 */
void 
nk_wake_waiters (void)
{
    nk_thread_t * me  = get_cur_thread();
    nk_queue_entry_t * tmp = NULL;
    nk_queue_entry_t * elm = NULL;

    uint8_t flags = spin_lock_irq_save(&(me->waitq->lock));
    if (list_empty_careful(&(me->waitq->queue))) {
        goto out;
    }

    list_for_each_entry_safe(elm, tmp, &(me->waitq->queue), node) {

        nk_thread_t * t = container_of(elm, nk_thread_t, wait_node);

        if (!t) {
            continue;
        }

        nk_enqueue_thread_on_runq(t, t->bound_cpu);
        nk_dequeue_entry(&(t->wait_node));

    }

out:
    spin_unlock_irq_restore(&(me->waitq->lock), flags);
}


/*
 * nk_thread_exit
 *
 * exit from this thread
 *
 * @retval: the value to return to the parent
 *
 * If there is someone waiting on this thread, this 
 * function will wake them up. This will also call
 * any destructors for thread local storage
 *
 */
void
nk_thread_exit (void * retval) 
{
    nk_thread_t * me = get_cur_thread();

    if (irqs_enabled()) {
        cli();
    }

    /* clear any thread local storage that may have been allocated */
    tls_exit();

    /* wait for my children to finish */
    nk_join_all_children(NULL);

    /* wake up everyone who is waiting on me */
    nk_wake_waiters();

    me->output      = retval;
    me->status      = NK_THR_EXITED;

    me->refcount--;

    SCHED_DEBUG("Thread %p (tid=%u) exiting, joining with children\n", me, me->tid);

    nk_schedule();

    /* we should never get here! */
    panic("Should never get here!\n");
}


/*
 * nk_thread_destroy
 *
 * destroys a thread and reclaims its memory (its stack page mostly)
 * interrupts should be off
 *
 * @t: the thread to destroy
 *
 */
void 
nk_thread_destroy (nk_thread_id_t t)
{


    nk_thread_t * thethread = (nk_thread_t*)t;

    SCHED_DEBUG("Destroying thread (%p, tid=%lu)\n", (void*)thethread, thethread->tid);

    ASSERT(!irqs_enabled());

    nk_dequeue_thread_from_runq(thethread);
    dequeue_thread_from_tlist(thethread);

    /* remove it from any wait queues */
    nk_dequeue_entry(&(thethread->wait_node));

    /* remove its own wait queue 
     * (waiters should already have been notified */
    nk_thread_queue_destroy(thethread->waitq);

    free(thethread->stack);
    free(thethread);
}


/*
 * nk_join
 *
 * join (wait) on the given thread
 *
 * t: the thread to wait on
 * retval: where the waited-on thread should 
 *         put its output
 *
 * returns  -EINVAL on error, 0 on success
 *
 */
int
nk_join (nk_thread_id_t t, void ** retval)
{
    nk_thread_t *thethread = (nk_thread_t*)t;
    int flags;

    //ASSERT(irqs_enabled());
    ASSERT(thethread->parent == get_cur_thread());


    flags = irq_disable_save();
    if (thethread->status == NK_THR_EXITED) {
        if (thethread->output) {
            *retval = thethread->output;
        }
        return 0;
    } else {
        while (thethread->status != NK_THR_EXITED) {
            nk_wait(t);
        }
    }

    *retval = thethread->output;

    thread_detach(thethread);

    irq_enable_restore(flags);
    return 0;
}


/* 
 * nk_join_all_children
 *
 * Join all threads that the current thread
 * has either forked or spawned
 *
 * @func: this function will be called with each 
 *        output value generated by this threads
 *        children
 *
 *  returns -EINVAL on error, 0 on success
 *
 */
int 
nk_join_all_children (int (*func)(void * res)) 
{
    nk_thread_t * elm = NULL;
    nk_thread_t * tmp = NULL;
    nk_thread_t * me         = get_cur_thread();
    void * res               = NULL;
    int ret                  = 0;

    list_for_each_entry_safe(elm, tmp, &(me->children), child_node) {

        if (nk_join(elm, &res) < 0) {
            ERROR_PRINT("Could not join child thread (t=%p)\n", elm);
            ret = -1;
            continue;
        }

        if (func) {
            if (func(res) < 0) {
                ERROR_PRINT("Could not invoke destructo for child thread (t=%p)\n", elm);
                ret = -1;
                continue;
            }
        }

    }

    return ret;
}


/* 
 * nk_wait
 *
 * Go to sleep on a thread's wait queue. 
 *
 * @t : the thread to wait on
 *
 */
void
nk_wait (nk_thread_id_t t)
{
    nk_thread_t * cur    = get_cur_thread();
    nk_thread_t * waiton = (nk_thread_t*)t;

    nk_thread_queue_t * wq = waiton->waitq;

    /* make sure we're not putting ourselves on our 
     * own waitq */
    ASSERT(!irqs_enabled());
    ASSERT(wq != cur->waitq);

    enqueue_thread_on_waitq(cur, wq);
    nk_schedule();
}


/*
 * nk_yield
 *
 * schedule some other thread
 *
 */
void 
nk_yield (void)
{
    uint8_t flags   = irq_disable_save();
    nk_thread_t * t = get_cur_thread();

    nk_enqueue_thread_on_runq(t, t->bound_cpu);
    nk_schedule();
    irq_enable_restore(flags);
}


/* 
 * nk_set_thread_fork_output
 *
 * @result: the output to set
 *
 */
void
nk_set_thread_fork_output (void * result)
{
    nk_thread_t* t = get_cur_thread();
    t->output = result;
}


/*
 * nk_thread_queue_sleep
 *
 * Goes to sleep on the given queue
 *
 * @q: the thread queue to sleep on
 * 
 */
int 
nk_thread_queue_sleep (nk_thread_queue_t * q)
{
    uint8_t flags = irq_disable_save();
    nk_thread_t * t = get_cur_thread();
    enqueue_thread_on_waitq(t, q);
    nk_schedule();
    irq_enable_restore(flags);
    return 0;
}


/* 
 * nk_thread_queue_wake_one
 *
 * wake one thread waiting on this queue
 *
 * @q: the queue to use
 *
 * returns -EINVAL on error, 0 on success
 *
 */
int 
nk_thread_queue_wake_one (nk_thread_queue_t * q)
{
    nk_queue_entry_t * elm = NULL;
    nk_thread_t * t = NULL;

    SCHED_DEBUG("Thread queue wake one (q=%p)\n", (void*)q);

    if (!q) {
        ERROR_PRINT("Attempt to wake up thread on non-existant thread queue\n");
        return -EINVAL;
    }

    elm = nk_dequeue_first_atomic(q);

    /* no one is sleeping on this queue */
    if (!elm) {
        SCHED_DEBUG("No waiters on wait queue\n");
        return 0;
    }

    t = container_of(elm, nk_thread_t, wait_node);

    if (!t) {
        ERROR_PRINT("Could not get thread from queue element\n");
        return -EINVAL;
    }

    ASSERT(t->status == NK_THR_WAITING);

    nk_enqueue_thread_on_runq(t, t->bound_cpu);

    return 0;
}


/* 
 * nk_thread_queue_wake_all
 *
 * wake all threads waiting on this queue
 *
 * @q: the queue to use
 *
 * returns -EINVAL on error, 0 on success
 *
 */
int
nk_thread_queue_wake_all (nk_thread_queue_t * q)
{
    nk_queue_entry_t * elm = NULL;
    nk_thread_t * t = NULL;

    SCHED_DEBUG("Waking all waiters on thread queue (q=%p)\n", (void*)q);

    if (!q) {
        ERROR_PRINT("Attempt to wake up thread on non-existant thread queue\n");
        return -EINVAL;
    }

    while ((elm = nk_dequeue_first_atomic(q))) {
        t = container_of(elm, nk_thread_t, wait_node);
        if (!t) {
            continue;
        }

        ASSERT(t->status == NK_THR_WAITING);

        nk_enqueue_thread_on_runq(t, t->bound_cpu);
    }

    return 0;
}


/* 
 * nk_tls_key_create
 *
 * create thread local storage
 *
 * @key: where to stash the created key
 * @destructor: function pointer to be called when the thread
 *              exits
 *
 * returns -EAGAIN on error, 0 on success
 *
 */
int
nk_tls_key_create (nk_tls_key_t * key, void (*destructor)(void*))
{
    int i;

    for (i = 0; i < TLS_MAX_KEYS; i++) {
        unsigned sn = tls_keys[i].seq_num;

        if (TLS_KEY_AVAIL(sn) && TLS_KEY_USABLE(sn) &&
            atomic_cmpswap(tls_keys[i].seq_num, sn, sn+1) == sn) {

            tls_keys[i].destructor = destructor;
            *key = i;

            /* we're done */
            return 0;
        }
    }

    return -EAGAIN;
}


/*
 * nk_tls_key_delete
 *
 * @key: the key to delete
 *
 * returns -EINVAL on error, 0 on success
 *
 */
int 
nk_tls_key_delete (nk_tls_key_t key)
{
    if (likely(key < TLS_MAX_KEYS)) {
        unsigned sn = tls_keys[key].seq_num;

        if (likely(!TLS_KEY_AVAIL(sn)) &&
            atomic_cmpswap(tls_keys[key].seq_num, sn, sn+1) == sn) {
            return 0;
        }
    }

    return -EINVAL;
}


/* 
 * nk_tls_get
 *
 * get the value stored for this key for this
 * thread
 *
 * @key: the key to act as index for value lookup
 *
 * returns NULL on error, the value otherwise
 *
 */
void*
nk_tls_get (nk_tls_key_t key) 
{
    nk_thread_t * t;

    if (unlikely(key >= TLS_MAX_KEYS)) {
        return NULL;
    }

    t = get_cur_thread();
    return (void*)t->tls[key];
}


/* 
 * nk_tls_set
 *
 * @key: the key to use for index lookup
 * @val: the new value to set at this key
 *
 * returns -EINVAL on error, 0 on success
 *
 */
int 
nk_tls_set (nk_tls_key_t key, const void * val)
{
    nk_thread_t * t;
    unsigned sn;

    if (key >= TLS_MAX_KEYS || 
        TLS_KEY_AVAIL((sn = tls_keys[key].seq_num))) {
        return -EINVAL;
    }

    t = get_cur_thread();
    t->tls[key] = val;
    return 0;
}


/*
 * nk_get_tid
 *
 * get the opaque thread id for the currently
 * running thread
 *
 */
nk_thread_id_t
nk_get_tid (void) 
{
    nk_thread_t * t = (nk_thread_t*)get_cur_thread();
    return (nk_thread_id_t)t;
}


/* 
 * nk_get_parent_tid
 *
 * get this thread's parent tid
 *
 */
nk_thread_id_t
nk_get_parent_tid (void) 
{
    nk_thread_t * t = (nk_thread_t*)get_cur_thread();
    if (t && t->parent) {
        return (nk_thread_id_t)t->parent;
    }
    return NULL;
}





/********** END EXTERNAL INTERFACE **************/





// push the child stack down by this much just in case
// we only have one caller frame to mangle
// the launcher function needs to put a new return address
// prior to the current stack frame, at least.
// should be at least 16
#define LAUNCHPAD 16
#define STACK_CLONE_DEPTH 2
#define STACK_SIZE_MIN    (4096 * 16)
#define LAUNCHER_STACK_SIZE STACK_SIZE_MIN

/* 
 * note that this isn't called directly. It is vectored
 * into from an assembly stub
 *
 * new tid is returned both to parent and child
 */
nk_thread_id_t 
__thread_fork (void)
{
    nk_thread_id_t  tid;
    nk_thread_t * t;
    nk_stack_size_t size, alloc_size;
    uint64_t     rbp1_offset_from_ret0_addr;
    void         *child_stack;

    void *rbp0      = __builtin_frame_address(0);                   // current rbp, *rbp0 = rbp1
    void *rbp1      = __builtin_frame_address(1);                   // caller rbp, *rbp1 = rbp2  (forker's frame)
    void *rbp_tos   = __builtin_frame_address(STACK_CLONE_DEPTH);   // should scan backward to avoid having this be zero or crazy
    void *ret0_addr = rbp0 + 8;

    // we're being called with a stack not as deep as STACK_CLONE_DEPTH...
    // fail back to a single frame...
    if (rbp_tos == 0 || rbp_tos < rbp1) { 
        rbp_tos = rbp1;
    }

    // from last byte of tos_rbp to the last byte of the stack on return from this function (return address of wrapper
    // the "launch pad" is added so that in the case where there is no stack frame above the caller
    // we still have the space to fake one.
    size = (rbp_tos + 8) - ret0_addr + LAUNCHPAD;   

    rbp1_offset_from_ret0_addr = rbp1 - ret0_addr;

    alloc_size = (size > STACK_SIZE_MIN) ? size : STACK_SIZE_MIN;    // at least enough to grow to STACK_SIZE_MIN

    if (nk_thread_create(NULL,        // no function pointer, we'll set rip explicity in just a sec...
                         NULL,        // no input args, it's not a function
                         NULL,        // no output args
                         0,           // this thread's parent will wait on it
                         TSTACK_2MB,  // stack size
                         &tid,        // give me a thread id
                         CPU_ANY)     // not bound to any particular CPU
            < 0) {
        ERROR_PRINT("Could not fork thread\n");
        return 0;
    }

    t = (nk_thread_t*)tid;

    child_stack = t->stack;

    // this is at the top of the stack, just in case something goes wrong
    thread_push(t, (uint64_t)&thread_cleanup);

    // Copy stack frames of caller and up to stack max
    // this should copy from 1st byte of my_rbp to last byte of tos_rbp
    // notice that leaves ret
    memcpy(child_stack + alloc_size - size, ret0_addr, size - LAUNCHPAD);
    t->rsp = (uint64_t)(child_stack + alloc_size - size);

    void **rbp2_ptr = (void**)(t->rsp + rbp1_offset_from_ret0_addr);
    void **ret2_ptr = rbp2_ptr+1;

    // rbp2 we don't care about
    *rbp2_ptr = 0x0ULL;

    // fix up the return address to point to our thread cleanup function
    *ret2_ptr = &thread_cleanup;

    // now we need to setup the interrupt stack etc.
    // we provide null for thread func to indicate this is a fork
    thread_setup_init_stack(t, NULL, NULL); 

    // put it on the run queue
    nk_enqueue_thread_on_runq(t, CPU_ANY);

    // return child's tid to parent
    return tid;
}


nk_thread_t*
nk_need_resched (void) 
{
    nk_thread_t * p;
    nk_thread_t * c;

    ASSERT(!irqs_enabled());

    c = get_cur_thread();
    p = get_runnable_thread_myq();

    if (p) {
        nk_enqueue_thread_on_runq(c, c->bound_cpu);
    }

    return p;
}


/* 
 * schedule
 *
 * pick a thread to run
 *
 */
void
nk_schedule (void) 
{
    nk_thread_t * runme = NULL;

    ASSERT(!irqs_enabled());

    runme = get_runnable_thread_myq();

    if (!runme) {
        cpu_id_t id = my_cpu_id();
        ERROR_PRINT("Nothing to run (cpu=%d)\n", id);
        return;
    }

    // KCH TEMPORARY before we switch, make sure we're not
    // stomping around past our given stack...
    nk_thread_t * me  = get_cur_thread();

    // we've overrun our stack...
    if (me->rsp <= (uint64_t)(me->stack)) {
        panic("This thread (%p, tid=%u) has run off the end of its stack! (start=%p, rsp=%p, start size=%lx)\n", 
                (void*)me,
                me->tid,
                me->stack,
                (void*)me->rsp,
                me->stack_size);
    }


    nk_thread_switch(runme);
}


/*
 * sched_init_ap
 *
 * scheduler init routine for APs once they
 * have booted up
 *
 */
int
nk_sched_init_ap (void)
{
    nk_thread_t * me = NULL;
    void * my_stack = NULL;
    cpu_id_t id = my_cpu_id();
    struct cpu * my_cpu = get_cpu();
    uint8_t flags;

    flags = irq_disable_save();

    SCHED_DEBUG("Initializing CPU %u\n", id);

    my_cpu->run_q = nk_thread_queue_create();

    if (!my_cpu->run_q) {
        ERROR_PRINT("Could not create run queue for CPU %u)\n", id);
        goto out_err;
    }

    me = malloc(sizeof(nk_thread_t));
    if (!me) {
        ERROR_PRINT("Could not allocate thread for CPU (%u)\n", id);
        goto out_err1;
    }

    my_stack = malloc(PAGE_SIZE);
    if (!my_stack) {
        ERROR_PRINT("Couldn't allocate new stack for CPU (%u)\n", id);
        goto out_err2;
    }

    /* we have no parent thread... */
    if (thread_init(me, my_stack, 1, id, NULL) != 0) {
        ERROR_PRINT("Could not init start thread on core %u\n", id);
        goto out_err3;
    }

    // set my current thread
    put_cur_thread(me);

    // start another idle thread
    SCHED_DEBUG("Starting idle thread for cpu %d\n", id);
    nk_thread_start(idle, NULL, NULL, 0, TSTACK_DEFAULT, NULL, id);

    irq_enable_restore(flags);
    return 0;

out_err3:
    free(me->stack);
out_err2:
    free(me);
out_err1:
    nk_thread_queue_destroy(my_cpu->run_q);
out_err:
    sti();
    return -1;
}


/* 
 * sched_init
 *
 * entry point into the scheduler at bootup
 *
 */
int
nk_sched_init (void) 
{
    struct nk_sched_state * sched = NULL;
    struct cpu * my_cpu = per_cpu_get(system)->cpus[0];
    nk_thread_t * main = NULL;
    int flags;

    flags = irq_disable_save();

    SCHED_PRINT("Initializing scheduler\n");

    sched = malloc(sizeof(struct nk_sched_state));
    if (!sched) {
        ERROR_PRINT("Could not allocate scheduler state\n");
        goto out_err0;
    }
    memset(sched, 0, sizeof(struct nk_sched_state));

    my_cpu->run_q = nk_thread_queue_create();
    if (!my_cpu->run_q) {
        ERROR_PRINT("Could not create run queue\n");
        goto out_err1;
    }

    sched->thread_list = nk_thread_queue_create();
    if (!sched->thread_list) {
        ERROR_PRINT("Could not create thread list\n");
        goto out_err2;
    }
    global_thread_list = sched->thread_list;


    // first we need to add our current thread as the current thread
    main  = malloc(sizeof(nk_thread_t));
    if (!main) {
        ERROR_PRINT("Could not allocate main thread\n");
        goto out_err3;
    }


    /* 
     * some important notes:
     * 1. The actual stack size is really set in the linker
     *    script (nautilus.ld). It defaults to 2MB
     * 2. boot_stack_start is the *bottom* of the stack.
     * 3. We're setting rsp to the wrong value here in thread_init, but that's
     *    okay because it will be overwritten in thread_lowlevel.S 
     *    when we switch from it to the first spawned thread
     *
     * 4. TODO: currently, the boot thread is bound to CPU 0. I will change this
     *
     */
    main->stack_size = TSTACK_2MB;
    thread_init(main, (void*)&boot_stack_start, 1, my_cpu_id(), NULL);
    main->waitq = nk_thread_queue_create();
    if (!main->waitq) {
        ERROR_PRINT("Could not create main thread's wait queue\n");
        goto out_err4;
    }

    put_cur_thread(main);

    enqueue_thread_on_tlist(main);

    irq_enable_restore(flags);

    return 0;

out_err4:
    free(main);
out_err3:
    free(sched->thread_list);
out_err2:
    free(my_cpu->run_q);
out_err1: 
    free(sched);
out_err0:
    sti();
    return -EINVAL;
}




static void 
tls_dummy (void * in, void ** out)
{
    unsigned i;
    nk_tls_key_t * keys = NULL;

    printk("Beginning test of thread local storage...\n");
    keys = malloc(sizeof(nk_tls_key_t)*TLS_MAX_KEYS);
    if (!keys) {
        ERROR_PRINT("could not allocate keys\n");
        return;
    }

    for (i = 0; i < TLS_MAX_KEYS; i++) {
        if (nk_tls_key_create(&keys[i], NULL) != 0) {
            ERROR_PRINT("Could not create TLS key (%u)\n", i);
            return;
        }

        if (nk_tls_set(keys[i], (const void *)(i + 100L)) != 0) {
            ERROR_PRINT("Could not set TLS key (%u)\n", i);
            return;
        }

    }

    for (i = 0; i < TLS_MAX_KEYS; i++) {
        if (nk_tls_get(keys[i]) != (void*)(i + 100L)) {
            ERROR_PRINT("Mismatched TLS val! Got %p, should be %p\n", nk_tls_get(keys[i]), (void*)(i+100L));
            return;
        }

        if (nk_tls_key_delete(keys[i]) != 0) {
            ERROR_PRINT("Could not delete TLS key %u\n", i);
            return;
        }
    }

    if (nk_tls_key_create(&keys[0], NULL) != 0) {
        ERROR_PRINT("2nd key create failed\n");
        return;
    }
    
    if (nk_tls_key_delete(keys[0]) != 0) {
        ERROR_PRINT("2nd key delete failed\n");
        return;
    }

    printk("Thread local storage test succeeded\n");
}


void 
nk_tls_test (void)
{
    nk_thread_start(tls_dummy, NULL, NULL, 1, TSTACK_DEFAULT, NULL, 1);
}



