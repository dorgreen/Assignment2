#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "kthread.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(mythread()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// FOR TASK 2.2
int sys_kthread_create(void){
  int start_func_ptr = 0;
  int stack_ptr = 0;
  // load first arg
    // we shall cast to pointer at send...
  if(argint(0, &start_func_ptr) < 0)
    return -1;

  // load second arg
  // we shall cast to pointer at send...
  if(argint(1, &stack_ptr) < 0)
    return -1;

  return kthread_create((void *)start_func_ptr, (void *) stack_ptr);
}


int sys_kthread_id(void){
  return kthread_id();
}

int sys_kthread_exit(void){
  kthread_exit();
  return 0;

}
int sys_kthread_join(void){
  int tid;
  if(argint(0, &tid) < 0)
    return -1;

  return kthread_join(tid);
}