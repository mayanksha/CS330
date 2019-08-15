#include<context.h>
#include<apic.h>
#include<memory.h>
#include<schedule.h>
#include<lib.h>
#include<idt.h>
static u64 numticks;

static u64 htt_rbp;
static u64 final_call = 0;
static enum {
	CALLED_FOR_SYSCALL,
	CALLED_FOR_TIMER
};

static struct queue {
	int processes[2 * MAX_PROCESSES];
	int size;
} Q = {
	.processes = {[0 ... 2*MAX_PROCESSES-1] = -1},
	.size = 0
};

static void printArr(){
	printf("Queue size = %d | ", Q.size);
	for(int i = 0; Q.processes[i] != -1; i++){
		printf("%d ", Q.processes[i]);
	}	
	printf("\n");
}
static int isEmpty(struct queue* q) {
	if (q->size == 0) {
		return 1;
	}
	else return 0;
}
static void enqTail(struct queue * q, int val) {
	q->processes[q->size] = val;
	q->size++;
}
static int deqFront(struct queue* q) {
	if (q->size == 0){
		return -1;
	}
	int ret_val = q->processes[0];
	for(int i = 1; q->processes[i] != -1; i++){
		q->processes[i-1] = q->processes[i];
	}
	q->size -= 1;
	q->processes[q->size] = -1;
	return ret_val;
}

static int str_len(char* a);
static void str_cat(char* a, char* b);
static void to_string(int num, char* str);
static int check_someone_sleeping(struct exec_context* list);
/*void printStack(int levels){
 *  long long rbp = 0;
 *  asm volatile("mov %%rbp, %0;": "=r"(rbp)::);
 *
 *  printf("Printing below the current base pointer = %x\n", rbp);
 *  for(int i = 0; i < levels; i++){
 *   printf("Value at %d(rbp) [%x] = %x\n", (i<<3),rbp + (i << 3) , *((u64 *)(rbp + (i << 3))));
 *  }
 *  long long prev_rbp =  *((u64 * )rbp);
 *}*/
static int is_someone_sleeping(struct exec_context* list) {
	int flag = 0;
	for(int i = 0; i < MAX_PROCESSES; i++){
		if(list[i].ticks_to_sleep != 0 || list[i].state == WAITING){
			flag = 1;
		}
	}
	if (flag) printf("Someone Sleeping = YES\n");
		else printf("Someone Sleeping = NO\n");
	return flag;
};
static void to_string(int num, char* str){
	char temp[CNAME_MAX];
	int i = 0;
	while(num != 0){
		temp[i] = (num % 10) + '0';
		i++; 
		num = num / 10;
	}
	temp[i] = '\0';
	int n = str_len(temp);

	for(int i = 0;i < n; i++){
		str[i] = temp[n - i - 1];
	}
	str[n] = '\0';
	return;
}
static void str_cat(char* a, char* b){
	int len_a = str_len(a);
	int len_b = str_len(b);
	int i = 0;
	while(i < len_b){
		a[len_a + i] = b[i];
		i++;
	}
	a[len_a + i] = '\0';
	return;
}

static int str_len(char* a){
	int i = 0;
	while(a[i] != '\0') i++;
	return i;
}
static void printStruct(struct exec_context* e) {

	printf("\tPrint Struct pid = %x\n", e->pid);

	/*int arr[2] = {
	 *  MM_SEG_CODE,
	 *  MM_SEG_RODATA,	
	 *  MM_SEG_DATA,
	 *  MM_SEG_STACK
	 *};*/
	/*for(int i = 0; i < 2;i ++){
	 *  switch(arr[i]){
	 *    case(MM_SEG_CODE):printf("\t\tCOOODE\n"); break;
	 *    case(MM_SEG_STACK):printf("\t\tSTACK\n"); break;
	 *    case(MM_SEG_DATA):printf("\t\tDATA\n"); break;
	 *  }
	 *  printf("\t\tstart = %x, end = %x, next_free = %x, diff = %x\n"
	 *      , (e->mms[arr[i]]).start
	 *      , (e->mms[arr[i]]).end
	 *      , (e->mms[arr[i]]).next_free)
	 *    , ((e->mms[i]).end) - (e->mms[i]).start;
	 *}*/
	/*printf("\t type = %x\n", e->type);
	 *printf("\t name= %s\n", e->name);
	 *printf("\t state= %x\n", e->state);
	 *printf("\t used_mem = %x\n", e->used_mem);
	 *printf("\t e->os_stack_pfn = %x\n", e->os_stack_pfn);
	 *printf("\t os_rsp = %x\n", e->os_rsp);
	 *printf("\t ticks_to_sleep = %x\n", e->ticks_to_sleep);
	 *printf("\t ticks_to_alarm = %x\n", e->ticks_to_alarm);*/
	printf("\t e->regs.r15 = %x\n ", e->regs.r15);
	printf("\t e->regs.r14 = %x\n ", e->regs.r14);
	printf("\t e->regs.r13 = %x\n ", e->regs.r13);
	printf("\t e->regs.r12 = %x\n ", e->regs.r12);
	printf("\t e->regs.r11 = %x\n ", e->regs.r11);
	printf("\t e->regs.r10 = %x\n ", e->regs.r10);
	printf("\t e->regs.r9 = %x\n ", e->regs.r9);
	printf("\t e->regs.r8 = %x\n ", e->regs.r8);
	printf("\t e->regs.rbp = %x\n ", e->regs.rbp);
	printf("\t e->regs.rdi = %x\n ", e->regs.rdi);
	printf("\t e->regs.rsi = %x\n ", e->regs.rsi);
	printf("\t e->regs.rdx = %x\n ", e->regs.rdx);
	printf("\t e->regs.rcx = %x\n ", e->regs.rcx);
	printf("\t e->regs.rbx = %x\n ", e->regs.rbx);
	printf("\t e->regs.rax = %x\n ", e->regs.rax);
	printf("\t e->regs.entry_rip = %x\n ", e->regs.entry_rip);
	printf("\t e->regs.entry_cs = %x\n ", e->regs.entry_cs);
	printf("\t e->regs.entry_rflags = %x\n ", e->regs.entry_rflags);
	printf("\t e->regs.entry_rsp = %x\n ", e->regs.entry_rsp);
	printf("\t e->regs.entry_ss = %x\n ", e->regs.entry_ss);
}
static void save_current_context(int condition)
{
	struct exec_context* curr = get_current_ctx();
	
	if (curr->pid == 0) {
		return;
	}
	/*printStruct(curr);*/

	// Ignore if called for the swapper process
	printf("Saving current context state. PID = %d\n", curr->pid);

	long long save_current_context_rbp = 0;
	asm volatile("mov %%rbp, %0;": "=r"(save_current_context_rbp)::);
	long long rbp_schedule_context = *((u64 *)save_current_context_rbp);
	long long rbp_schedule = *((u64 *)rbp_schedule_context);

	// foo = exit / sleep
	long long do_foo = *((u64 *)rbp_schedule);
	long long do_syscall_rbp = *((u64 *)do_foo);
	printf("rbp_entry_point = %x == %x\n", rbp_schedule_context, htt_rbp);
	if (condition == CALLED_FOR_SYSCALL) {
		printf("SCC Called for SYSCALL");
		curr->regs.r15 = *((u64 *)(do_syscall_rbp + 16));
		curr->regs.r14 = *((u64 *)(do_syscall_rbp + 24));
		curr->regs.r13 = *((u64 *)(do_syscall_rbp + 32));
		curr->regs.r12 = *((u64 *)(do_syscall_rbp + 40));
		curr->regs.r11 = *((u64 *)(do_syscall_rbp + 48));
		curr->regs.r10= *((u64 *)(do_syscall_rbp + 56));
		curr->regs.r9 = *((u64 *)(do_syscall_rbp + 64));
		curr->regs.r8 = *((u64 *)(do_syscall_rbp + 72));
		curr->regs.rbp = *((u64 *)(do_syscall_rbp + 80));
		curr->regs.rdi = *((u64 *)(do_syscall_rbp + 88));
		curr->regs.rsi = *((u64 *)(do_syscall_rbp + 96));
		curr->regs.rdx= *((u64 *)(do_syscall_rbp + 104));
		curr->regs.rcx = *((u64 *)(do_syscall_rbp + 112));
		curr->regs.rbx = *((u64 *)(do_syscall_rbp + 120));

		curr->regs.entry_rip = *((u64 *)(do_syscall_rbp + 128));
		curr->regs.entry_cs = *((u64 *)(do_syscall_rbp + 136));
		curr->regs.entry_rflags = *((u64 *)(do_syscall_rbp + 144));
		curr->regs.entry_rsp = *((u64 *)(do_syscall_rbp + 152));
		curr->regs.entry_ss = *((u64 *)(do_syscall_rbp + 160));
	}
	else {
		printf("Printing below the current base pointer = %x\n", rbp_schedule_context);
		for(int i = 0; i < 50; i++){
			printf("Value at %d(rbp) [%x] = %x\n", (i<<3),rbp_schedule_context + (i << 3) , *((u64 *)(rbp_schedule_context + (i << 3))));
		}

	}
} 

static void schedule_context(struct exec_context *next)
{
	// This is level 4 function
	/*asm volatile("cli;"
	 *    :::"memory");*/
	/*Your code goes in here. get_current_ctx() still returns the old context*/
	
	struct exec_context *curr = get_current_ctx();
	if (curr->pid == next->pid){
		return;
	}
	long long rbp_schedule_context = 0;
	asm volatile("mov %%rbp, %0;": "=r"(rbp_schedule_context)::);
	long long rbp_schedule = *((u64 *)rbp_schedule_context);
	long long rbp_syscallOrDoSleep = *((u64 *)rbp_schedule);
	if (next->pid == 0 && final_call == 0) {
		curr->state = READY;
		if (rbp_syscallOrDoSleep == htt_rbp) {
			save_current_context(CALLED_FOR_TIMER);
		}
		else save_current_context(CALLED_FOR_SYSCALL);
	}
	printf("scheduling old pid = %d, new pid = %d\n", curr->pid, next->pid);
	next->state = RUNNING;
	asm volatile(
			"push %0;"
			"push %1;"
			"push %2;"
			"push %3;"
			"push %4;"
			::
			"r"(next->regs.entry_ss),
			"r"(next->regs.entry_rsp),
			"r"(next->regs.entry_rflags),
			"r"(next->regs.entry_cs),
			"r"(next->regs.entry_rip)
			:"memory");
	set_tss_stack_ptr(next);
	set_current_ctx(next);
	asm volatile(
			"mov %0, %%rbx;"
			"mov %1, %%rcx;"
			"mov %2, %%rdx;"
			"mov %3, %%rsi;"
			"mov %4, %%rdi;"
			"mov %5, %%r8;"
			"mov %6, %%r9;"
			"mov %7, %%r10;"
			"mov %8, %%r11;"
			"mov %9, %%r12;"
			:
			:
			"r"(next->regs.rbx),
			"r"(next->regs.rcx),
			"r"(next->regs.rdx),
			"r"(next->regs.rsi),
			"r"(next->regs.rdi),
			"r"(next->regs.r8),
			"r"(next->regs.r9),
			"r"(next->regs.r10),
			"r"(next->regs.r11),
			"r"(next->regs.r12)
				:"memory");
	asm volatile(
			"mov %1, %%r13;"
			"mov %2, %%r14;"
			"mov %3, %%r15;"
			"mov %0, %%rbp;"
			:
			:
			"r"(next->regs.rbp),
			"r"(next->regs.r13),
			"r"(next->regs.r14),
			"r"(next->regs.r15)
			:"memory");
	/*asm volatile(
	 *    "mov %%rbp, %%rsp; pop %%rbp;":::"memory"
	 *    );*/
	ack_irq();
	printf("\n");
	asm volatile("iretq;":::"memory");
}
static struct exec_context *pick_next_context(struct exec_context *list)
{
	printf("Inside Pick next Context ");
	struct exec_context* curr = get_current_ctx();
	/*printArr();*/
	if (Q.size != 0){

		// a is the process currently executing
		/*printf("Size now = %d\n", Q.size);*/
		if (Q.processes[0] == curr->pid){
			deqFront(&Q);
		}
		/*printf("a->value = %d, A->next->value = %d\n", Q.processes[0], Q.processes[1]);*/

		// Error
		int ret_context_pid = (Q.size == 0) ? curr->pid: Q.processes[0];
		/*printArr();*/
		printf("pick_next_context returns pid = %d, ISS= %d\n", ret_context_pid, is_someone_sleeping(list));
		return get_ctx_by_pid(ret_context_pid);
	}
	else {
		if (is_someone_sleeping(list) == 1){
			return get_ctx_by_pid(0);
		}
		else do_exit();
	}
}
static void schedule()
{
	struct exec_context *current = get_current_ctx(); 
	struct exec_context *all = get_ctx_list(); 

	if (current->state == UNUSED && Q.size == 1){
		if (Q.processes[0] == current->pid){
			deqFront(&Q);
			do_exit();
		}
	}
	switch(current->state) {
		case(UNUSED): printf("UNUSED\n"); break;
		case(RUNNING): printf("RUNNING\n"); break;
		case(WAITING): printf("WAITING\n"); break;
		case(NEW): printf("NEW\n"); break;
	}

	/*printArr();*/
	/*printf("Enq %s, %d", current->name,  current->pid);*/
	if (current->state != UNUSED && current->pid != 0){
		current->state = READY;
		enqTail(&Q, current->pid);	
	}
	else if (current->pid == 0 && is_someone_sleeping(all)) {
		for(int i = 0; i < MAX_PROCESSES;i ++) {
			if (all[i].ticks_to_sleep == 1) {
				enqTail(&Q, i);
				all[i].ticks_to_sleep -= 1;
			}
			else continue;
		}
	}

	struct exec_context *next;
	next = pick_next_context(all);
	printf("Executing schedule_context\n");
	schedule_context(next);
}

static void do_sleep_and_alarm_account()
{
	/*All processes in sleep() must decrement their sleep count*/ 
	struct exec_context* curr = get_current_ctx();
	struct exec_context* all_proc = get_ctx_list();	
	printf("Curr-Pid = %d, Ticks_to_alarm = %u\n",curr->pid, curr->ticks_to_alarm);

	// Sleep syscall related stuff

	/*printStruct(curr);*/
	/*printArr();*/
	for (int i = 0;i < MAX_PROCESSES; i++){
		if (all_proc[i].state == UNUSED){
			continue;
		}
		printf("pid = %d name = %s\t\n", all_proc[i].pid, all_proc[i].name);
		int ticks_to_sleep = all_proc[i].ticks_to_sleep;
		if (ticks_to_sleep > 1){
			all_proc[i].ticks_to_sleep -= 1;
			printf("Process %d was sleeping, ticks_to_sleep = %d!\n", all_proc[i].pid, ticks_to_sleep);
		}
		else if (ticks_to_sleep == 1){
			printf("Process %d was sleeping, it is now to be scheduled!\n", all_proc[i].pid);
			all_proc[i].state = READY;

			// Currently, just swap the init and swapper processes. 
			// Later, when schedule is working, change this to schedule the 
			// next process with READY state
			schedule();
		}
	}

	// Alarm syscall related stuff
	if (curr->alarm_config_time != 0 && curr->sighandlers[SIGALRM] != 0){
		if (curr->ticks_to_alarm > 0){
			curr->ticks_to_alarm -= 1;
		}
		else {
			u64 a = 0x0;
			curr->ticks_to_alarm = curr->alarm_config_time;
			invoke_sync_signal(SIGALRM, 0, &a);
		};
	}
	else if (curr->alarm_config_time != 0 && curr->sighandlers[SIGALRM] == 0){
		printf("Default Action for SIGALRM is ignore!\n");
	}
}

/*The five functions above are just a template. You may change the signatures as you wish*/
void handle_timer_tick()
{
	asm volatile("cli;"
			:::"memory");
	asm volatile (
			"push %%rax;"
			"push %%rbx;"
			"push %%rcx;"
			"push %%rdx;"
			"push %%rdi;"
			"push %%rsi;"
			"push %%r8;"
			"push %%r9;"
			"push %%r10;"
			"push %%r11;"
			"push %%r12;"
			"push %%r13;"
			"push %%r14;"
			"push %%r15;"
			:::"memory"
			);
	asm volatile("mov %%rbp, %0;": "=r"(htt_rbp)::);
	struct exec_context* curr = get_current_ctx();
	long long rsp = 0;
	asm volatile("mov %%rsp, %0;": "=r"(rsp)::);
	long long rbp = 0;
	asm volatile("mov %%rbp, %0;": "=r"(rbp)::);

	if (curr->pid == 0 && final_call == 1){
		do_cleanup();
	}
	curr->regs.rbp = *((u64 *)(rbp + 0));
	curr->regs.entry_rip = *((u64 *)(rbp + 8));
	curr->regs.entry_cs = *((u64 *)(rbp + 16));
	curr->regs.entry_rflags = *((u64 *)(rbp + 24));
	curr->regs.entry_rsp = *((u64 *)(rbp + 32));
	curr->regs.entry_ss = *((u64 *)(rbp + 40));
	/*printStruct(curr);*/

	struct exec_context* all_proc = get_ctx_list();
	if (numticks == 0 || all_proc[1].state == UNUSED){
		enqTail(&Q, 1);
	}
	/*
		 This is the timer interrupt handler. 
		 You should account timer ticks for alarm and sleep
		 and invoke schedule
		 */
	printf("\n--------------------\nGot a tick. #ticks = %u\n", numticks++);   /*XXX Do not modify this line*/ 
	
	do_sleep_and_alarm_account();
	ack_irq();  /*acknowledge the interrupt, before calling iretq */
	asm volatile (
			"pop %%r15;"
			"pop %%r14;"
			"pop %%r13;"
			"pop %%r12;"
			"pop %%r11;"
			"pop %%r10;"
			"pop %%r9;"
			"pop %%r8;"
			"pop %%rsi;"
			"pop %%rdi;"
			"pop %%rdx;"
			"pop %%rcx;"
			"pop %%rbx;"
			"pop %%rax;"
			:::"memory"
			);
	asm volatile("mov %%rbp, %%rsp;"
			"pop %%rbp;"
			"iretq;"
			:::"memory");
}

void do_exit()
{
	struct exec_context* curr = get_current_ctx();
	int a = deqFront(&Q);
	printf("The node dequeued was = %d\n", a);	
	if (Q.size == 0) {
		if (curr->pid != 0){
			/*printf("\t\tPGD = %d Freed for %d\n", curr->os_stack_pfn, curr->pid);
			 *os_pfn_free(OS_PT_REG, curr->os_stack_pfn);
			 *os_pfn_free(OS_PT_REG, curr->os_stack_pfn);*/
		}
		do_cleanup();
	}
	else {
		struct exec_context* curr = get_current_ctx();
		struct exec_context* list = get_ctx_list();
		printf("**************** do_exit called for %d\n", curr->pid);
		curr->state = UNUSED;
		if (curr->pid != 0){
			os_pfn_free(OS_PT_REG, curr->os_stack_pfn);
			printf("\t\tPGD = %d Freed for %d\n", curr->os_stack_pfn, curr->pid);
		}
		schedule();
	}
	/*You may need to invoke the scheduler from here if there are
		other processes except swapper in the system. Make sure you make 
		the status of the current process to UNUSED before scheduling 
		the next process. If the only process alive in system is swapper, 
		invoke do_cleanup() to shutdown gem5 (by crashing it, huh!)
		*/
	/*do_cleanup();  [>Call this conditionally, see comments above<]*/
}

/*system call handler for sleep*/
long do_sleep(u32 ticks)
{
	struct exec_context* curr = get_current_ctx();
	curr->state = WAITING;
	curr->ticks_to_sleep = ticks;

	struct exec_context* all = get_ctx_list();
	struct exec_context* swapper_process = &all[0];

	deqFront(&Q);
	enqTail(&Q, 0);
	schedule();
}

/*
	 system call handler for clone, create thread like 
	 execution contexts
	 */

long do_clone(void *th_func, void *user_stack) {
	printf("Inside do_clone - *th_func = %x, *user_stack = %x \n", th_func, user_stack);	

	struct exec_context* child = get_new_ctx();
	struct exec_context* curr = get_current_ctx();

	memcpy(child->name, curr->name, strlen(curr->name));
	char temp[CNAME_MAX] = {0};
	to_string(child->pid, temp);
	str_cat(child->name, temp);
	printf("child->name = %s\n", child->name);

	child->type = curr->type;
	child->state  = NEW;
	child->used_mem  = curr->used_mem;
	child->os_stack_pfn  = os_pfn_alloc(OS_PT_REG);
	child->os_rsp  = curr->os_rsp;

	// ????

	// Alarm syscall's timer isn't applicable for the newly forked child processes
	child->ticks_to_sleep  = 0;
	child->ticks_to_alarm  = 0;
	child->alarm_config_time  = 0;

	child->mms[MM_SEG_CODE].start = curr->mms[MM_SEG_CODE].start;
	child->mms[MM_SEG_CODE].end = curr->mms[MM_SEG_CODE].end;
	child->mms[MM_SEG_CODE].next_free = curr->mms[MM_SEG_CODE].next_free;
	child->mms[MM_SEG_CODE].access_flags = curr->mms[MM_SEG_CODE].access_flags;

	child->mms[MM_SEG_DATA].start = curr->mms[MM_SEG_DATA].start;
	child->mms[MM_SEG_DATA].end = curr->mms[MM_SEG_DATA].end;
	child->mms[MM_SEG_DATA].next_free = curr->mms[MM_SEG_DATA].next_free;
	child->mms[MM_SEG_DATA].access_flags = curr->mms[MM_SEG_DATA].access_flags;

	child->mms[MM_SEG_STACK].start = curr->mms[MM_SEG_STACK].start;
	child->mms[MM_SEG_STACK].end = curr->mms[MM_SEG_STACK].end;
	child->mms[MM_SEG_STACK].next_free = curr->mms[MM_SEG_STACK].next_free;
	child->mms[MM_SEG_STACK].access_flags = curr->mms[MM_SEG_STACK].access_flags;

	child->mms[MM_SEG_RODATA].start = curr->mms[MM_SEG_RODATA].start;
	child->mms[MM_SEG_RODATA].end = curr->mms[MM_SEG_RODATA].end;
	child->mms[MM_SEG_RODATA].next_free = curr->mms[MM_SEG_RODATA].next_free;
	child->mms[MM_SEG_RODATA].access_flags = curr->mms[MM_SEG_RODATA].access_flags;

	long long do_clone_rbp = 0;
	asm volatile("mov %%rbp, %0;": "=r"(do_clone_rbp)::);
	long long do_syscall_rbp = *((u64 *)do_clone_rbp);

	child->regs.r15 = *((u64 *)(do_syscall_rbp + 16));
	child->regs.r14 = *((u64 *)(do_syscall_rbp + 24));
	child->regs.r13 = *((u64 *)(do_syscall_rbp + 32));
	child->regs.r12 = *((u64 *)(do_syscall_rbp + 40));
	child->regs.r11 = *((u64 *)(do_syscall_rbp + 48));
	child->regs.r10= *((u64 *)(do_syscall_rbp + 56));
	child->regs.r9 = *((u64 *)(do_syscall_rbp + 64));
	child->regs.r8 = *((u64 *)(do_syscall_rbp + 72));
	child->regs.rbp = *((u64 *)(do_syscall_rbp + 80));
	child->regs.rdi = *((u64 *)(do_syscall_rbp + 88));
	child->regs.rsi = *((u64 *)(do_syscall_rbp + 96));
	child->regs.rdx= *((u64 *)(do_syscall_rbp + 104));
	child->regs.rcx = *((u64 *)(do_syscall_rbp + 112));
	child->regs.rbx = *((u64 *)(do_syscall_rbp + 120));

	child->regs.entry_rip = (u64)th_func;
	child->regs.entry_rflags = *((u64 *)(do_syscall_rbp + 144));
	child->regs.entry_rsp = (u64)user_stack;
	child->regs.entry_cs  = 0x23;
	child->regs.entry_ss = 0x2b;

	// Set the child to READY
	child->state = READY;
	printf("Enq newly created process %s\n", child->name);
	enqTail(&Q, child->pid);
	/*printArr();*/
	return child->pid;
}

long invoke_sync_signal(int signo, u64 *ustackp, u64 *urip)
{
	printf("Signo = %d\n", signo);
	struct exec_context* curr = get_current_ctx();

	/*If signal handler is registered, manipulate user stack and RIP to execute signal handler*/
	/*ustackp and urip are pointers to user RSP and user RIP in the exception/interrupt stack*/

	printf("Called signal with ustackp=%x urip=%x\n", *ustackp, *urip);

	/*Default behavior is exit( ) if sighandler is not registered for SIGFPE or SIGSEGV.
		Ignore for SIGALRM*/
	/*if(signo != SIGALRM)
	 *  do_exit();*/

	*urip = (u64)curr->sighandlers[signo];
	printf("*urip now points to sighandler = %x\n", *urip);
	if (signo == SIGSEGV) {
		printf("Inside invoke_sync_signal SIGSEV catched!\n");
	}
	else if (signo == SIGFPE) {
		printf("Inside invoke_sync_signal SIGFPE catched!\n");
	}
	else if (signo == SIGALRM) {
		printf("Inside invoke_sync_signal SIGALRM catched!\n");
	}
	else {
		printf("Wrong Signal Number provided. Exit (-5)\n");
		do_exit();
	}
	printf("Urip = %x\n" , *urip);
	if (*urip != 0){
		__asm__ __volatile__ (
				"push %%rax;"
				"push %%rdi;"
				"mov %0, %%rdi;"
				"mov %1, %%rax;"
				"callq %%rax;"
				: 
				:  "m"(signo), "r"(*urip)
				: "rax"
				);
		__asm__ __volatile__ (
				"pop %%rdi;"
				"pop %%rax;"
				: 
				: 
				: "rax"
				);
		printf("Returned from signal handler\n");
	}
	if(signo != SIGALRM)
		do_exit();
}
/*system call handler for signal, to register a handler*/
long do_signal(int signo, unsigned long handler)
{
	printf(">>>>>> DO_SIGNAL signo = %x, handler = %x\n", signo, handler);
	void * __handler = (void *) handler;
	struct exec_context* curr = get_current_ctx();
	curr->sighandlers[signo] = __handler;

	if (signo == SIGSEGV){
		printf("Registering Signal handler for SIGSEGV!\n")	;
	}
	else if (signo == SIGFPE){
		printf("Registering Signal handler for SIGFPE!\n")	;
	}
	else if (signo == SIGALRM){
		printf("Registering Signal handler for SIGALRM!\n")	;
	}
	else {
		printf("Wrong Signal Number provided. Exit (-5)\n");
		do_exit();
	}

	// What's the return value?????

}

/*system call handler for alarm*/
long do_alarm(u32 ticks)
{
	struct exec_context* curr = get_current_ctx();
	curr->ticks_to_alarm = ticks;
	curr->alarm_config_time = ticks;

	printf("Alarm invoked!\n");
	// What's the return value?????
	return ticks;
}
