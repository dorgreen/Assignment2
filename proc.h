// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
  struct thread *thrd;          // The thread running on this cpu or null #TASK2
};

#define NTHREAD 16 // TASK2.1
#define thread_constant 1000 // used to create thread ID. threadid = father_pid*thread_constnt + thread index



extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { P_UNUSED, P_USED, P_ZOMBIE };
enum threadstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE};

struct thread {
    char *kstack;                // Bottom of kernel stack for this thread
    char *ustack;                // Bottom of USER STACK for this thread.
    enum threadstate state;      // Thread state
    int tid;                     // Thread ID
    struct proc *parent;         // the process that created this thread
    struct trapframe *tf;        // Trap frame for current syscall // MOVE TO THREAD, each has it's own registers...
    struct context *context;     // swtch() here to run process // MOVE TO THREAD, each has it's own registers...
    void *chan;                  // If non-zero, sleeping on chan // MOVE TO THREAD, blocking calls should only hold this thread
    int killed;                  // If non-zero, have been killed // SHOULD BE IN BOTH?

};



// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table. THREADS SHARE VIRTUAL MEMORY
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process  // SHOULD BE IN BOTH??
  struct file *ofile[NOFILE];  // Open files // STAYS IN PROC AS THESE ARE SHARED BETWEEN THERADS
  struct inode *cwd;           // Current directory  // SHARED BETWEEN THREADS
  char name[16];               // Process name (debugging)
  struct thread threads[NTHREAD]; // TASK2 HOLDS THREADS FOR THIS PROC ; NOT A MACRO AS IT HURTS COMPILATION
};


// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
