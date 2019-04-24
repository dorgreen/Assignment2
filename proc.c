#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
    struct spinlock lock;
    struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;

extern void forkret(void);

extern void trapret(void);

static void wakeup1(void *chan);

int close_thread(struct thread *t);

void
pinit(void) {
    initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
    return mycpu() - cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *
mycpu(void) {
    int apicid, i;

    if (readeflags() & FL_IF)
        panic("mycpu called with interrupts enabled\n");

    apicid = lapicid();
    // APIC IDs are not guaranteed to be contiguous. Maybe we should have
    // a reverse map, or reserve a register to store &cpus[i].
    for (i = 0; i < ncpu; ++i) {
        if (cpus[i].apicid == apicid)
            return &cpus[i];
    }
    panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *
myproc(void) {
    struct cpu *c;
    struct proc *p;
    pushcli();
    c = mycpu();
    p = c->proc;
    popcli();
    return p;
}

int kthread_create(void (*start_func)(), void *stack) {

    int tid;
    struct thread *t;
    struct proc *p;
    void *sp; // used for kstack size counting for space for context etc'

    p = myproc();
    if (p == 0) return -1;

    // find the next unused thread
    acquire(&ptable.lock);
    for (t = &p->threads[0]; t < &p->threads[NTHREAD]; t++) {
        if (t->state == UNUSED) {
            t->state = EMBRYO;
            break;
        }
    }
    release(&ptable.lock);

    if (t->state != EMBRYO)
        return -1; // NO UNUSED THREADS IN OUR PROC!


    // Calculate and fill in all fields of the new thread
    tid = (((p->pid) * thread_constant) + (int) (t - &p->threads[0]) / sizeof(&t));
    t->tid = tid; // note how tid/thread_constant = p->pid. used for debugging
    t->parent = p;

    // allocate kstack and rollback-fail if unable to do so
    if ((t->kstack = kalloc()) == 0) {
        p->state = P_UNUSED;
        t->parent = 0;
        t->state = UNUSED;
        t->tid = 0;
        return 0;
    }
    t->ustack = stack;
    t->chan = 0;
    t->killed = 0;


    sp = t->kstack + KSTACKSIZE;

    // Leave room for trap frame.
    sp -= sizeof *t->tf;
    t->tf = (struct trapframe *) sp;
    // trapframe set-up as if we just returned from an inturrpt.
    // eip holds the pointer to the return address, in our case the user-supplied start_func.
    memset(t->tf, 0, sizeof(*t->tf));
    t->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    t->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    t->tf->es = t->tf->ds;
    t->tf->ss = t->tf->ds;
    t->tf->eflags = FL_IF;
    t->tf->esp = (uint) sp+PGSIZE; // TODO SHOULD BE POINTER TO next free space on ustack
    t->tf->eip = (uint) start_func;  // eip holds the address to resume from


    // Set up new context to start executing at start_func
    sp -= 4;
    *(uint *) sp = (uint) trapret;

    sp -= sizeof *t->context;
    t->context = (struct context *) sp;
    memset(t->context, 0, sizeof *t->context);
    t->context->eip = (uint) forkret;
    // set-up context
    sp -= sizeof *t->context;
    t->context = (struct context *) sp;
    memset(t->context, 0, sizeof *t->context);
    t->context->eip = (uint) start_func;



    // Done, t is not longer EMBRYO
    acquire(&ptable.lock);
    t->state = RUNNABLE;
    release(&ptable.lock);

    return tid;
}

int kthread_id() {
    struct thread *t;
    acquire(&ptable.lock);
    t = mythread();
    release(&ptable.lock);
    return t->tid;

}

// TODO: TEST AND PROFFREAD
// ASSUMING WE ARE HOLDING THE PTABLE.LOCK
// if our thread is the last, this will close the proc as well.
void kthread_exit() {
    close_thread(mythread());
}

//This function suspends the execution of the calling thread until the target thread, indicated by the
//        argument thread id, terminates. If the thread has already exited, execution should not be suspended. If
//        successful, the function returns zero. Otherwise, -1 should be returned to indicate an error.
int kthread_join(int thread_id) {
    struct thread *t;
    struct proc *p;

    acquire(&ptable.lock);
    t = mythread();
    release(&ptable.lock);

    p = t->parent;
    for (t = &p->threads[0]; t < &p->threads[NTHREAD]; t++) {
        if (t->tid == thread_id) break;
    }

    if (t->tid != thread_id) return -1; // No thread with that thread_id, so no reason to suspend

    if (t->state == ZOMBIE) {
        t->tid = 0;
        t->state = UNUSED;
    }// clean t if it's a zombie

    if (t->state == UNUSED) {
        return 0; // No reason to suspend if exited.
    }

    // SUSPEND UNTIL t HAS EXITED!
    sleep(t, &ptable.lock); // TODO: IS THIS THE RIGHT LOCK?!
    return 0;

}


// This functions handles closing a thread given by pointer
// if it is the last thread of a non-zombie
// ASSUMES WE ARE HOLDING THE PTABLE.LOCK THE WHOLE TIME!
int close_thread(struct thread *t) {
    //panic("close thread not implemented!");
    //if(t->kstack != 0) kfree(t->kstack);

    //t->kstack = 0;

    // TODO: IS THAT THE RIGHT PLACE TO DEALLOC tf and context ?
    //kfree(t->tf); no need to dealloc as they exist on the kstack
    t->tf = 0;
    //kfree(t->context); no need to dealloc as they exist on the kstack
    t->context = 0;

    //t->tid = 0; // Don't reset tid yet, we use it for kthead_join(tid) !
    //t->parent = 0;
    //t->cwd = 0; // CWD IS A PROPERTY OF PROC!
    t->killed = 0;

    t->state = ZOMBIE;
    // sched();

    int live_threads_count = 0;
    for (struct thread *thrd = &(t->parent->threads[0]); thrd < &t->parent->threads[NTHREAD]; thrd++) {
        if (thrd != t && !(thrd->state == UNUSED || thrd->state == ZOMBIE)) {
            live_threads_count++;
        }
    }

    wakeup(t);
    if (live_threads_count == 0 || t->parent->state == P_ZOMBIE) exit();

    return 0;
}

// #TASK2
// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct thread *
mythread(void) {
    struct cpu *c;
    struct thread *t;
    pushcli();
    c = mycpu();
    t = c->thrd;
    popcli();
    return t;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *
allocproc(void) {
    struct proc *p;
    char *sp;
    struct thread *t;

    acquire(&ptable.lock);

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->state == P_UNUSED)
            goto found;

    release(&ptable.lock);
    return 0;

    found:
    p->state = P_USED;
    p->pid = nextpid++;

    // init the main thread. as this proc was UNUSED, we can assume the first thread is avilable.
    t = &(p->threads[0]);
    t->parent = p;
    t->state = EMBRYO;
    t->tid = (p->pid) * thread_constant;

    release(&ptable.lock);

    // Allocate kernel stack.
    // reset state if unsuccessful
    if ((t->kstack = kalloc()) == 0) {
        p->state = P_UNUSED;
        t->parent = 0;
        t->state = UNUSED;
        t->tid = 0;
        return 0;
    }
    sp = t->kstack + KSTACKSIZE;

    // Leave room for trap frame.
    sp -= sizeof *t->tf;
    t->tf = (struct trapframe *) sp;

    // Set up new context to start executing at forkret,
    // which returns to trapret.
    sp -= 4;
    *(uint *) sp = (uint) trapret;

    sp -= sizeof *t->context;
    t->context = (struct context *) sp;
    memset(t->context, 0, sizeof *t->context);
    t->context->eip = (uint) forkret;

    return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void) {
    struct proc *p;
    struct thread *t;
    extern char _binary_initcode_start[], _binary_initcode_size[];

    p = allocproc();

    // THERE HAS TO BE ONLY ONE THREAD IN THIS PROC, as it's freshly created.
    t = &(p->threads[0]);


    initproc = p;
    if ((p->pgdir = setupkvm()) == 0)
        panic("userinit: out of memory?");
    inituvm(p->pgdir, _binary_initcode_start, (int) _binary_initcode_size);
    p->sz = PGSIZE;
    memset(t->tf, 0, sizeof(*t->tf));
    t->tf->cs = (SEG_UCODE << 3) | DPL_USER;
    t->tf->ds = (SEG_UDATA << 3) | DPL_USER;
    t->tf->es = t->tf->ds;
    t->tf->ss = t->tf->ds;
    t->tf->eflags = FL_IF;
    t->tf->esp = PGSIZE;
    t->tf->eip = 0;  // beginning of initcode.S

    safestrcpy(p->name, "initcode", sizeof(p->name));
    p->cwd = namei("/");

    // this assignment to p->state lets other cores
    // run this process. the acquire forces the above
    // writes to be visible, and the lock is also needed
    // because the assignment might not be atomic.
    acquire(&ptable.lock);

    t->state = RUNNABLE;

    release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n) {
    uint sz;
//    if(!holding(&ptable.lock))
//        acquire(&ptable.lock);

    struct thread *curthread = mythread();
    struct proc *curproc = curthread->parent;

    sz = curproc->sz;
    if (n > 0) {
        if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    } else if (n < 0) {
        if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
            return -1;
    }
    curproc->sz = sz;
//    acquire(&ptable.lock);
    switchuvm(curthread);
    return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void) {
    int i, pid;
    struct proc *np;
    struct thread *nt;
    struct thread *curthread = mythread();
    struct proc *curproc = curthread->parent;

    // Allocate process.
    if ((np = allocproc()) == 0) {
        return -1;
    }

    nt = &(np->threads[0]);

    // Copy process state from proc.
    // if failed, clean up.
    if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
        kfree(nt->kstack);
        nt->kstack = 0;
        nt->tid = 0;
        nt->parent = 0;
        nt->state = UNUSED;
        np->state = P_UNUSED;
        return -1;
    }

    // Copy thread state from curproc to the main thread of the newly created proc
    np->sz = curproc->sz;
    np->parent = curproc;

    // deep copy TF from original thread
    nt->tf = memset(nt->tf, 0, sizeof(*nt->tf));

    // pusha
    nt->tf->edi = curthread->tf->edi;
    nt->tf->esi = curthread->tf->esi;
    nt->tf->ebp = curthread->tf->ebp;
    nt->tf->oesp = curthread->tf->oesp;
    nt->tf->ebx = curthread->tf->ebx;
    nt->tf->edx = curthread->tf->edx;
    nt->tf->ecx = curthread->tf->ecx;
    nt->tf->eax = curthread->tf->eax;
    // rest of trapframe
    nt->tf->gs = curthread->tf->gs;
    nt->tf->padding1 = curthread->tf->padding1;
    nt->tf->fs = curthread->tf->fs;
    nt->tf->padding2 = curthread->tf->padding2;
    nt->tf->es = curthread->tf->es;
    nt->tf->padding3 = curthread->tf->padding3;
    nt->tf->ds = curthread->tf->ds;
    nt->tf->padding4 = curthread->tf->padding4;
    nt->tf->trapno = curthread->tf->trapno;
    // x86 hardware
    nt->tf->err = curthread->tf->err;
    nt->tf->eip = curthread->tf->eip;
    nt->tf->cs = curthread->tf->cs;
    nt->tf->padding5 = curthread->tf->padding5;
    nt->tf->eflags = curthread->tf->eflags;
    // crossing rings
    nt->tf->esp = curthread->tf->esp;
    nt->tf->ss = curthread->tf->ss;
    nt->tf->padding6 = curthread->tf->padding6;

    // Clear %eax so that fork returns 0 in the child.
    // NOTE: AS TF IS THE SAME POINTER, WE ARE CHANGING THE ORIGINAL FRAME!
    nt->tf->eax = 0;

    // copy open files between procs (IO is shared between threads)
    for (i = 0; i < NOFILE; i++)
        if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);

    // copy this thred's CWD to the main thread of the newly created proc
    np->cwd = idup(curproc->cwd);

    safestrcpy(np->name, curproc->name, sizeof(curproc->name));

    pid = np->pid;

    acquire(&ptable.lock);

    // Setup the main thread of the newly created proc as RUNNALBE. the proc should be USED because of allocproc()
    nt->state = RUNNABLE;

    release(&ptable.lock);

    return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
// Should also kill all of it's threads.
// -->while CPU0 is running exit, CPU1 could be running some thread of this proc.
//    the active thread will be marked as killed=1, and will soon be killed.
void
exit(void) {
    acquire(&ptable.lock);
    struct thread *curthread = mythread();
    release(&ptable.lock);
    struct proc *curproc = curthread->parent;
    struct proc *p;

    int fd;

    if (curproc == initproc)
        panic("init exiting");

    p = curproc;

    acquire(&ptable.lock);
    if(curthread->killed == 1){ close_thread(curthread); return;}

    // Mark all non-empty threads of this proc as Killed.
    // wake threads sleeping on this chan if needed.
    for (struct thread *t = &(p->threads[0]); t < &p->threads[NTHREAD]; t++) {
        if (t->state != UNUSED && t->state != ZOMBIE && t != curthread) {
            t->killed = 1; // even if t is running
            if (t->state == SLEEPING) t->state = RUNNABLE;
            wakeup(t);
        }

    }

    release(&ptable.lock);



    // Once all threads were killed, close the proc
    // WAITS FOR ALL THREADS TO BE ZOMBIE or UNUSED

    for (struct thread *t = &(p->threads[0]); t < &p->threads[NTHREAD]; t++) {
        if (t != curthread && t->state != UNUSED && t->state != ZOMBIE) {
            acquire(&ptable.lock);
            kthread_join(t->tid); // TODO: could cause deadlock if t is waiting our thread?
            release(&ptable.lock);
        }
    }

//    if (holding(&ptable.lock))
//        release(&ptable.lock);

    // Close all open files.
    for (fd = 0; fd < NOFILE; fd++) {
        if (curproc->ofile[fd]) {
            fileclose(curproc->ofile[fd]);
            curproc->ofile[fd] = 0;
        }
    }


    // TODO: this block should be executed after actually closing all threads?
    // MIND: MUST NOT HOLD PTABLE LOCK HERE.
    begin_op();
    iput(curproc->cwd);
    end_op();
    curproc->cwd = 0;


    acquire(&ptable.lock);

    curproc->state = P_ZOMBIE;
    curthread ->killed = 1;


    //close_thread(curthread); // NOT AN INFINITE LOOP WITH EXIT, as the call to exit is conditioned with != P_ZOMBIE



    // Parent might be sleeping in wait().
    // TODO: SHOULD WAKE UP THREADS AS WELL? MAYBE JUST WAKE UP THREADS???
    wakeup1(curproc->parent);

    // Pass abandoned children to init.
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->parent == curproc) {
            p->parent = initproc;
            if (p->state == P_USED || p->state == P_ZOMBIE) {
                int has_zombie = 0;
                for (struct thread *t = &(p->threads[0]); t < &p->threads[NTHREAD]; t++) {
                    if (t->state == ZOMBIE) {
                        has_zombie = 1;
                        break;
                    }
                }
                if (has_zombie || p->state == P_ZOMBIE) wakeup1(initproc);
            }

        }
    }

    // p is now just a zombie. reset most of it
    p->parent = 0;
    p->pid = 0;
    for(int i = 0 ; i < 16 ; i++){
        p->name[i] = 0;
    }
    p->pgdir = 0;
    p->sz = 0;
    p->state = P_ZOMBIE;
    curthread->state = ZOMBIE;
    curthread->killed = 1;


    release(&ptable.lock);
    yield();


    // Jump into the scheduler, never to return.
    //sched();
    panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void) {
    struct thread *t;
    struct proc *p;
    int havekids, pid;
    struct thread *curthread = mythread();
    struct proc *curproc = curthread->parent;

    acquire(&ptable.lock);
    for (;;) {
        // Scan through table looking for exited children.
        havekids = 0;
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->parent != curproc)
                continue;
            havekids = 1;
            if (p->state == P_ZOMBIE) {
                // Found one. Iterate over zombie threads of p, and reset them.
                pid = p->pid;
                for (int i = 0; i < NTHREAD; i++) {
                    t = &(p->threads[i]);
                    if (t->state == ZOMBIE) {
                        // TODO: COULD BE REPLACED WITH A CALL TO close_thread()
                        kfree(t->kstack);
                        t->kstack = 0;
                        t->tid = 0;
                        t->parent = 0;
                        t->state = UNUSED;
                        //t->cwd = 0; // CWD @ PROC
                        t->killed = 0;
                    }
                }
                freevm(p->pgdir); // TODO: SOME THREADS COULD STILL BE RUNNING. MAYBE ADD A TEST? MAYBE THIS ISN'T THE SPOT?
                p->name[0] = 0;
                p->state = P_UNUSED;
                p->pid = 0; // TODO: IS THAT OK IF THERE ARE STILL RUNNING THREADS?
                release(&ptable.lock);
                return pid;
            }
        }

        // No point waiting if we don't have any children.
        if (!havekids || curthread->killed) {
            release(&ptable.lock);
            return -1;
        }

        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curproc, &ptable.lock);  //DOC: wait-sleep #TODO: WHO WAKES THEM UP? CLOSING THE PROC?
    }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void) {
    struct proc *p;
    struct thread *t;
    struct cpu *c = mycpu();
    c->proc = 0;
    c->thrd = 0;
    int count_empty = 0;

    for (;;) {
        // Enable interrupts on this processor.
        sti();

        // Loop over process table looking for process to run.
        // also cleans zombie procs.
        acquire(&ptable.lock);
        for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
            if (p->state == P_UNUSED)
                continue;

            if(p->state == P_ZOMBIE){
                count_empty = 0;
                for(t = p->threads ; t < &p->threads[NTHREAD]; t++){
                    if(t->state == UNUSED) count_empty++;
                    if(t->state == ZOMBIE){
                        t->tid = 0;
                        t->tf = 0;
                        //t->context = 0;
                        t->state = UNUSED;
                        count_empty++;
                    }
                }
                if(count_empty == NTHREAD){
                    p->state = P_UNUSED;
                }

                continue;
            }

            // iterate over threads of this proc, searching for runnables.
            // TODO: such trivial solution might starve threads with large ....index?
            for (t = p->threads; t < &p->threads[NTHREAD]; t++) {
                if (t->state == RUNNABLE) {
                    // Switch to chosen process and thread.
                    // It is the process's job
                    // to release ptable.lock and then reacquire it
                    // before jumping back to us.
                    t->state = RUNNING;
                    c->thrd = t;
                    c->proc = p;
                    switchuvm(t); // TODO: MIGHT EFACTOR TO THREAD


                    swtch(&(c->scheduler), t->context);
                    switchkvm();

                    // Thread is done running for now,
                    // It should have changed it's state before returning to us.
                    c->proc = 0;
                    c->thrd = 0;
//                    release(&ptable.lock);
//                    acquire(&ptable.lock);

                }
            }


            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
            c->thrd = 0;
        }
        release(&ptable.lock);

    }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void) {
    int intena;
    struct thread *t = mythread();
    //struct proc *p = t->parent;

    if (!holding(&ptable.lock))
        panic("sched ptable.lock");
    if (mycpu()->ncli != 1)
        panic("sched locks");
    if (t->state == RUNNING)
        panic("sched running");
    if (readeflags() & FL_IF)
        panic("sched interruptible");
    intena = mycpu()->intena;
    swtch(&t->context, mycpu()->scheduler);
    mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void) {
    struct thread *t;

    acquire(&ptable.lock);  //DOC: yieldlock
    //struct cpu *c = mycpu();
    t = mythread();


    if(t->state != ZOMBIE) t->state = RUNNABLE; // NOTE THE THREAD


    if (t->killed && t->state != ZOMBIE) {
        close_thread(t);
    }

    // This thread could be cleaned!
//    if(t->state == ZOMBIE){
//        t->tid = 0;
//        t->tf = 0;
//        //t->context = 0;
//        t->state = UNUSED;
//        if(t->parent->state == P_ZOMBIE){
//            t->parent->state = P_UNUSED;
//            t->parent = 0;
//        }
//    }


    sched();
    release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void) {
    static int first = 1;
    // Still holding ptable.lock from scheduler.
    release(&ptable.lock);

    if (first) {
        // Some initialization functions must be run in the context
        // of a regular process (e.g., they call sleep), and thus cannot
        // be run from main().
        first = 0;
        iinit(ROOTDEV);
        initlog(ROOTDEV);
    }

    // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk) {
    struct thread *t = mythread();
    if (t == 0 || t->parent == 0) panic("sleep");


    if (lk == 0)
        panic("sleep without lk");

    // Must acquire ptable.lock in order to
    // change p->state and then call sched.
    // Once we hold ptable.lock, we can be
    // guaranteed that we won't miss any wakeup
    // (wakeup runs with ptable.lock locked),
    // so it's okay to release lk.
    if (lk != &ptable.lock) {  //DOC: sleeplock0
        acquire(&ptable.lock);  //DOC: sleeplock1
        release(lk);
    }
    // Go to sleep.
    t->chan = chan;
    t->state = SLEEPING;

    sched();

    // Tidy up.
    t->chan = 0;

    // Reacquire original lock.
    if (lk != &ptable.lock) {  //DOC: sleeplock2
        release(&ptable.lock);
        acquire(lk);
    }
}

//PAGEBREAK!
// Wake up all threads sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan) {
    struct proc *p;
    struct thread *t;

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == P_USED) {
            // iterate over threads of this proc
            for (t = p->threads; t < &p->threads[NPROC]; t++) {
                if (t->state == SLEEPING && t->chan == chan)
                    t->state = RUNNABLE;
            }
        }
    }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan) {
    acquire(&ptable.lock);
    wakeup1(chan);
    release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid) {
    struct proc *p;
    struct thread *t;

    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->pid == pid) {

            //iterate over this proc's threads
            // mark active threads as "killed"
            for (t = p->threads; t < &p->threads[NTHREAD]; t++) {
                if (t->state == UNUSED) continue;

                t->killed = 1;
                // Wake thread from sleep if necessary.
                if (t->state == SLEEPING)
                    t->state = RUNNABLE;
            }


            release(&ptable.lock);
            return 0;
        }
    }
    release(&ptable.lock);
    return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void) {
//  static char *thread_states[] = {
//  [UNUSED]    "unused",
//  [EMBRYO]    "embryo",
//  [SLEEPING]  "sleep ",
//  [RUNNABLE]  "runble",
//  [RUNNING]   "run   ",
//  [ZOMBIE]    "zombie"
//  };

    static char *proc_states[] = {
            [P_UNUSED]  "unsused",
            [P_USED]    "in use ",
            [P_ZOMBIE]  "zombie "
    };


    int i;
    struct proc *p;
    struct thread *t;
    char *state;
    uint pc[10];

    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
        if (p->state == P_UNUSED)
            continue;
        if (p->state >= 0 && p->state < NELEM(proc_states) && proc_states[p->state])
            state = proc_states[p->state];
        else
            state = "???";
        cprintf("%d %s %s", p->pid, state, p->name);
        for (t = &(p->threads[0]); t < &p->threads[NTHREAD]; t++) {
            if (t->state == SLEEPING) {
                getcallerpcs((uint *) t->context->ebp + 2, pc);
                for (i = 0; i < 10 && pc[i] != 0; i++)
                    cprintf(" %p", pc[i]);
            }

        }
        cprintf("\n");
    }
}

void capture_ptable_lock(){
    acquire(&ptable.lock);
    return;
}

void release_ptable_lock(){
    release(&ptable.lock);
    return;
}