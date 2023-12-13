#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>
#include "threads/synch.h"
#include "fixed_point.h"
/* States in a thread's life cycle. */
enum thread_status
{
   THREAD_RUNNING, /* Running thread. */
   THREAD_READY,   /* Not running but ready to run. */
   THREAD_BLOCKED, /* Waiting for an event to trigger. */
   THREAD_DYING    /* About to be destroyed. */
};

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t)-1) /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0      /* Lowest priority. */
#define PRI_DEFAULT 31 /* Default priority. */
#define PRI_MAX 63     /* Highest priority. */

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */

struct thread
{
   /* Owned by thread.c. */
   tid_t tid;                 /* Thread identifier. */
   enum thread_status status; /* Thread state. */
   char name[16];             /* Name (for debugging purposes). */
   uint8_t *stack;            /* Saved stack pointer. */
   int priority;              /* Priority. */
   struct list_elem allelem;  /* List element for all threads list. */
   int64_t ticks_blocked;
   /* Shared between thread.c and synch.c. */
   struct list_elem elem; /* List element. */

   // 线程一共可以有三个关于优先级的属性，真正优先级，基础优先级和持有锁的最高优先级，这是为了在之后的工作中能够快速检测到线程能够达到的
   // 最高优先级，从而使一个线程快速的完成自己的任务。
   // 此任务的最高标准就是优先级，一定要让线程和锁在任何时候保持自己能够达到的最高优先级，这样可以使整体完成的速度更快。
   int base_priority;         // 基础优先级
   int locks_priority;        // 线程持有的锁中最高优先级
   struct list locks;         // 记录线程持有的锁
   struct lock *lock_waiting; // 记录线程等待的锁

   fixed_t recent_cpu;
   int nice;
   
#ifdef USERPROG
   /* Owned by userprog/process.c. */
   uint32_t *pagedir; /* Page directory. */
#endif
   unsigned magic; /* Detects stack overflow. */
   int exit_code;
};

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;
void thread_init(void);
void thread_start(void);

void thread_tick(void);
void thread_print_stats(void);

typedef void thread_func(void *aux);
tid_t thread_create(const char *name, int priority, thread_func *, void *);

void thread_block(void);
void thread_unblock(struct thread *);
bool thread_cmp_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);
bool thread_less_priority(const struct list_elem *a, const struct list_elem *b, void *aux UNUSED);
struct thread *thread_current(void);
tid_t thread_tid(void);
const char *thread_name(void);

void thread_exit(void) NO_RETURN;
void thread_yield(void);
void thread_sleep(int64_t ticks);
void thread_wakeup();
static bool sleep_ticks_less(const struct list_elem *a_, const struct list_elem *b_, void *aux UNUSED);
/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func(struct thread *t, void *aux);
void thread_foreach(thread_action_func *, void *);

void thread_set_priority(int);

// priority schedule
void thread_priority_donate_nest(struct thread *t);
void thread_update_priority(struct thread *t);
void lock_update_priority(struct lock *l);

// mlfqs
void mlfqs_inc_recent_cpu(void);
void mlfqs_update_load_avg_and_recent_cpu(void);
void mlfqs_update_priority(struct thread *t);

#endif /* threads/thread.h */
