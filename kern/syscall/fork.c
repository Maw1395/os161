//#include <kern/errno.h>
//#include <kern/syscall.h>
//#include <types.h>
//#include <machine/trapframe.h>
//#include <pid.h>

//#include <thread.h>
//#include <proc.h>
//#include <current.h>
//#include <addrspace.h>
//#include <syscall.h>
//#include <vnode.h>

//#include <kern/errno.h>
//#include <kern/syscall.h>
//#include <lib.h>
//#include <limits.h>


#include <types.h>
#include <kern/errno.h>
#include <kern/fcntl.h>
#include <lib.h>
#include <limits.h>
#include <proc.h>
#include <current.h>
#include <addrspace.h>
#include <pid.h>
#include <syscall.h>
#include <mips/trapframe.h>



/*
 * Enter user mode for a newly forked process.
 *
 * This function is provided as a reminder. You need to write
 * both it and the code that calls it.
 *
 * Thus, you can trash it and do things another way if you prefer.
 */
static int enter_forked_process(void *tf,  unsigned long n)
{
	(void)n;
	
	// copy trapframe
	struct trapframe trapf = (*((struct trapframe *) tf));// = *((struct trapframe*)tf);
	//trapf = kmalloc(sizeof(struct trapframe));
	//memcpy(trapf, tf, sizeof(struct trapframe));

	// Set the trapframe values
	// set returnvalue 0
	trapf.tf_v0 = 0;
	// signal no error
	trapf.tf_a3 = 0;
	// increase pc such that systemcall is not called again
	trapf.tf_epc += 4;

	// free allocated memory
	kfree(tf);

	// change back to usermode
	mips_usermode(&trapf);

	// Panic if user mode returns // should not happen
	panic("Returned from user mode!");
	return -1;
}


/*
--- fork
fork() makes an exact copy of the invoking process. It creates a thread 
(each user process has exact one thread) and copies the address space, 
program code, a list of file_descriptors from files the parent process is using, 
working directory of the parent process and makes sure that the parent and child 
processes each observe the correct return value 
(that is, 0 for the child and the newly created pid for the parent).
*/
int sys_fork(struct trapframe *tf, int32_t *ret){
	int new_pid = 0;
	struct addrspace *new_as = NULL;
	struct proc* new_proc = NULL;
	struct thread* new_thread = NULL;
	struct thread* curt = curthread;
	struct proc* curp = curt->t_proc;
	struct trapframe *trapf;
	char name[16];
	int result;
	spinlock_acquire(&curp->p_lock);

	// check if there are already too many processes on the system
	if(false)// TODO
	{
		return ENPROC;
	}

	// check if current user already has too many processes
	if(false)// TODO
	{
		return EMPROC;
	}

	// generate process name
      	result = snprintf(name, sizeof(name), "child_%d", new_pid);
	if (result < 0 ) {
		return result; 
	}

	// create child process
	new_proc = proc_create_runprogram(name);
	if (new_proc == NULL) {
		return -1; 
	}

	new_pid = new_proc->PID;
	// check if generated pid is valid
	/*
	if(pid < PID_MIN || pid > PID_MAX)
	{
		proc_destroy(proc);
		return EPIDOOR; 
	}
	*/
	
	memcpy(ret, &new_pid, sizeof(int));

	/* Create a new address space and copy content of current process in it. */
	result = as_copy(curp->p_addrspace, &new_as);
	if (result || new_as == NULL) {
		proc_destroy(new_proc);
		return result; 
	}
	new_proc->p_addrspace = new_as;


	// TODO
	// Create file descriptor table for child process and copy content of parent file descriptor table into it.
	// Create open file table for child and copy content of parent open file handle table into it. Remove files from the child open file table that are opened as writable in the parent list. 

	// Copy the trapframe to the heap so it's available to the child
	trapf = kmalloc(sizeof(*tf));
	memcpy(tf,trapf,sizeof(*tf));

	result = thread_fork(name, &new_thread, new_proc, &enter_forked_process, trapf, 0);
	if (result) {
		kfree(trapf);
		proc_destroy(new_proc);
		return result; 
	}


	/* we don't need to lock proc->p_lock as we have the only reference */
	if (new_proc->p_cwd != NULL) {
		VOP_INCREF(curp->p_cwd);
		new_proc->p_cwd = curp->p_cwd;
	}
	

	/* Thread subsystem fields */
	list_push_back(&curp->p_childlist, (void*)new_proc);
	spinlock_release(&curp->p_lock);
	
	return 0;
}