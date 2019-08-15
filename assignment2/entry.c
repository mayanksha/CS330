#include<init.h>
#include<lib.h>
#include<context.h>
#include<memory.h>

#define ul unsigned long
#define ull unsigned long long

/*System Call handler*/
long long do_syscall(int syscall, u64 param1, u64 param2, u64 param3, u64 param4)
{
	/*int arr[4] = { MM_SEG_CODE, MM_SEG_RODATA, MM_SEG_DATA, MM_SEG_STACK};*/
	int arr[3] = {
		MM_SEG_RODATA,
		MM_SEG_DATA,
		MM_SEG_STACK
	};

	static int count = 0;
	struct exec_context *current = get_current_ctx();
	switch(syscall)
	{
		case SYSCALL_EXIT: {
				printf("[GemOS] exit code = %d\n", (int) param1);
				do_exit();
				break;
			}
		case SYSCALL_GETPID: {
				printf("[GemOS] getpid called for process %s, with pid = %d\n", current->name, current->id);
				return current->id; 
				break;
			}
		case SYSCALL_WRITE: {
				printf("[GemOS] syscall_write called for process %s\n", current->name);
				ull pageTables[4];

				ul* dBase = (ul* )osmap(current->pgd);	
				ul dOffset = (param1 >> 39) & 0x1FF;
				ul* dEntry = dBase + dOffset;
				if (!((*dEntry) & 0x1)){
					return -1;
				}
				pageTables[0] = *dEntry;
				
				dBase = (ul*) osmap(*dEntry >> 12);
				dOffset = (param1 >> 30) & 0x1FF;
				dEntry = dBase + dOffset;
				if (!((*dEntry) & 0x1)){
					return -1;
				}
				pageTables[1] = *dEntry;

				dBase = (ul*) osmap(*dEntry >> 12);
				dOffset = (param1 >> 21) & 0x1FF;
				dEntry = dBase + dOffset;
				if (!((*dEntry) & 0x1)){
					return -1;
				}
				pageTables[2] = *dEntry;

				dBase = (ul*) osmap(*dEntry >> 12);
				dOffset = (param1 >> 12) & 0x1FF;
				dEntry = dBase + dOffset;
				pageTables[3] = *dEntry;
				if (*dEntry & 0x1){
					int len = strlen((char*)param1);
					printf("Len = %d\n", len);
					if (param2 > 1024){
						return -1;
					}
					for(int k = 0; k < len && k < param2; k++){
						printf("%c", ((char*)param1)[k]);
					}	
					printf("\n");
					/*printf("%s", (char *)param1);*/
					return (len > param2)? param2:len;
				}
				else {
					// Invalid	
					return -1;
				}
			}
			/*current->mms[]*/
			/*Your code goes here*/
		case SYSCALL_SHRINK: {
				printf("[GemOS] syscall_shrink called for process %s\n", current->name);
				// Param1 = Size 
				// Param2 = Flag 

				int region = (param2 == MAP_RD)? MM_SEG_RODATA: MM_SEG_DATA;
				printf("param1 = %d param2 = %x\n", param1, param2);
				unsigned long start = current->mms[region].start;				
				unsigned long next_free = current->mms[region].next_free;				
				printf("Current Next_Free = %x!\n", next_free);

				if (next_free - (param1 << 12) < start)
					return -1;

				// Only MEM_RO_DATA AND MEM_DATA have to be freed
				ull pageTables[4];

				for(int i = 1; i <= param1; i++){
					ul ds = current->mms[region].next_free - (i << 12);
					/*printf("Shrinking Start from = %x\n", ds);*/
					ul* dBase = (ul* )osmap(current->pgd);	
					ul dOffset = (ds >> 39) & 0x1FF;
					ul* dEntry = dBase + dOffset;
					pageTables[0] = *dEntry;

					dBase = (ul*) osmap(*dEntry >> 12);
					dOffset = (ds >> 30) & 0x1FF;
					dEntry = dBase + dOffset;
					pageTables[1] = *dEntry;

					dBase = (ul*) osmap(*dEntry >> 12);
					dOffset = (ds >> 21) & 0x1FF;
					dEntry = dBase + dOffset;
					pageTables[2] = *dEntry;

					dBase = (ul*) osmap(*dEntry >> 12);
					dOffset = (ds >> 12) & 0x1FF;
					dEntry = dBase + dOffset;
					(*dEntry) &= 0xFFFFFFFE;
					pageTables[3] = *dEntry;

					if (*dEntry != 0) {
						os_pfn_free(USER_REG, pageTables[3] >> 12);
					}
					*dEntry = 0x0;
					asm volatile(
							"invlpg (%0);"
							::"r"(ds): "memory"
							);
				}


				next_free -= (param1 << 12);

				// Not efficient since you're flushing the whole TLB in each shrink call
				/*asm volatile(
				 *        "mov %%cr3, %%rax;"
				 *        "mov %%rax, %%cr3;"
				 *        :::
				 *        );*/
				current->mms[region].next_free = next_free;
				printf("Next_free Now = %x\n", next_free);
				return next_free;
			}
		case SYSCALL_EXPAND: {
				// Param1 = Size
				// Param2 = Flag
				printf("[GemOS] syscall_expand called for process %s\n", current->name);

				int region = (param2 == MAP_RD)? MM_SEG_RODATA: MM_SEG_DATA;
				unsigned long end = current->mms[region].end;				
				unsigned long orig_next_free = current->mms[region].next_free;				
				printf("SYSCALL_EXPAND! Old Next_Free = %x\n", orig_next_free);
				unsigned long next_free = current->mms[region].next_free;				
				if (next_free + (param1 << 12) > end){
					return -1;
				}
				current->mms[region].next_free += (param1 << 12);
				/*printf("Next_FREE = %x\n", orig_next_free);*/
				printf("New next_free = %x\n", current->mms[region].next_free);
				return orig_next_free;
				break;
			}
			break;
		default:
			return -1;

	}
	return 0;   /*GCC shut up!*/
}
extern int handle_div_by_zero(void)
{
	u64 rsp, rbp;
	__asm__ __volatile__ (
			"mov %%rsp, %%rax\n\t"
			"mov %%rax, %0\n\t"
			"mov %%rbp, %%rax\n\t"
			"mov %%rax, %1\n\t"
			: "=m" (rsp), "=m" (rbp)
			:
			: "%rax"
			);

	u64 prev_RIP;
	__asm__ __volatile__ (
			"mov 8(%%rbp), %%rax\n\t"
			"mov %%rax, %0\n\t"
			: "=m"(prev_RIP)
			: 
			: "%rax"
			);
	printf("Div-by-zero detected at RIP = %x.  Exiting!\n", prev_RIP);
	do_exit();
}

extern int handle_page_fault(void)
{
	u64 stack;
	__asm__ __volatile__(
			"push %%rax\n\t"
			"push %%rcx\n\t"
			"push %%rdx\n\t"
			"push %%rbx\n\t"
			"push %%rdi\n\t"
			"push %%rsi\n\t"
			"push %%r10\n\t"
			"push %%r11\n\t"
			"push %%r12\n\t"
			"push %%r13\n\t"
			"push %%r14\n\t"
			"push %%r15\n\t"
			"push %%r8\n\t"
			"push %%r9\n\t"
			"mov %%rsp, %0\n\t"
			: "=r"(stack)
			: 
			: 
			);
	struct exec_context *current = get_current_ctx();
	u64 prev_RIP, cr2, err_code, rip;
	__asm__ __volatile__ (
			"mov 16(%%rbp), %%rax\n\t"
			"mov %%rax, %0\n\t"
			"mov 8(%%rbp), %%rax\n\t"
			"mov %%rax, %1\n\t"
			"mov %%cr2, %%rax\n\t"
			"mov %%rax, %2\n\t"
			: "=r" (prev_RIP),"=r"(err_code) , "=r" (cr2)
			: 
			: "rax"
			);

	printf("\n\n[GemOS] Page Fault@prev_RIP = %x, cr2 = %x, err_code = %x\n", prev_RIP, cr2,err_code);
	// Last 3 bits 
	int bit_0 = err_code & 0x1;
	int bit_1 = (err_code & 0x2) >> 1;
	int bit_2 = (err_code & 0x4) >> 2;

	printf("Last 3 bits = %d%d%d\n", bit_2, bit_1, bit_0);
	int flag = 0;
	/*printf("prev_RIP = %x\ncr2 = %x\n", prev_RIP, cr2);*/
	if (bit_0 == 1){
		printf("Protection Violation! Prev_RIP = %x, VA = %x, err_code = %x\n", prev_RIP, cr2,err_code);
		do_exit();
	}
	// Check for MM_SEGMENT_DATA
	if (cr2 <= current->mms[MM_SEG_DATA].end && cr2 >= current->mms[MM_SEG_DATA].start){
		printf("VA = %x inside DATA Segment\n",cr2);
		if (cr2 >= current->mms[MM_SEG_DATA].start && cr2 < current->mms[MM_SEG_DATA].next_free) {
			flag = MM_SEG_DATA;
		}	
		else {
			printf("Page fault occurred at VA = %x which is out of allowed DATA Segment range.\n", cr2);
			do_exit();
		}
	}
	// Check for MM_SEGMENT_RODATA
	else if (cr2 <= current->mms[MM_SEG_RODATA].end && cr2 >= current->mms[MM_SEG_RODATA].start){
		if (cr2 >= current->mms[MM_SEG_RODATA].start && cr2 < current->mms[MM_SEG_RODATA].next_free) {
			flag = MM_SEG_RODATA;
		}	
		else {
			printf("Page fault occurred at VA = %x which is out of allowed RODATA Segment range.\n", cr2);
			do_exit();
		}

		if (bit_1 == 1){
			printf("No Write Access! Prev_RIP = %x, VA = %x, err_code = %x\n", prev_RIP, cr2,err_code);
			do_exit();
		}	
		flag = MM_SEG_RODATA;
	}
	// Check for MM_SEGMENT_STACK
	else if (cr2 <= current->mms[MM_SEG_STACK].end && cr2 >= current->mms[MM_SEG_STACK].start){
		flag = MM_SEG_STACK;
	}
	else {
		printf("VA out of allowable Address Range! Prev_RIP = %x, VA = %x, err_code = %x\n", prev_RIP, cr2,err_code);
		do_exit();
	}

	if (flag){
		ul * l4_base_addr = (unsigned long*)osmap(current->pgd);
		int a_flag = 0x7;
		/*int final_flag = 0x5;*/
		switch(flag){
			case(MM_SEG_RODATA):printf("\t\tRODATA\n"); break;
			case(MM_SEG_STACK):printf("\t\tSTACK\n"); break;
			case(MM_SEG_DATA):printf("\t\tDATA\n"); break;
		}

		// 0x1FF = 0001 1111 1111 in binary
		u32 l4_offset = (cr2 >> 39)&0x1FF;

		ul* l4_entry = l4_base_addr + l4_offset;
		ul* l3_base_addr;
		if (!(*l4_entry & 0x1)){
			// ******** Base PFN for Page 3 of Paging 
			ul l3_base_pfn = os_pfn_alloc(OS_PT_REG);
			*l4_entry = (l3_base_pfn << 12) | a_flag;
			l3_base_addr = (unsigned long *)osmap(*(l4_entry) >> 12);
		}
		else {
			l3_base_addr = (unsigned long *)osmap((*l4_entry) >> 12);
		}
		u32 l3_offset = (cr2 >> 30) & 0x1FF; 
		ul* l3_entry = l3_base_addr + l3_offset;

		// ******** Base PFN for Page 2 of Paging 

		ul* l2_base_addr;
		if (!(*l3_entry & 0x1)){
			ul l2_base_pfn = os_pfn_alloc(OS_PT_REG);
			*l3_entry = (l2_base_pfn << 12) | a_flag;
			l2_base_addr = (unsigned long *)osmap(*l3_entry >> 12);
		}
		else {
			l2_base_addr = (unsigned long *)osmap((*l3_entry) >> 12 );
		}

		u32 l2_offset = (cr2 >> 21) & 0x1FF; 
		ul* l2_entry = &l2_base_addr[l2_offset];

		// ******** Base PFN for Page 1 of Paging 
		ul* l1_base_addr;
		if (!(*l2_entry & 0x1)){
			ul l1_base_pfn = os_pfn_alloc(OS_PT_REG);
			*l2_entry = (l1_base_pfn << 12) | a_flag;
			l1_base_addr= (unsigned long *)osmap(*l2_entry >> 12);
		}
		else {
			l1_base_addr = (unsigned long *)osmap((*l2_entry >> 12));
		}
		// ******** Base PFN for Page 1 of Paging 
		u32 l1_offset = (cr2 >> 12) & 0x1FF; 
		ul* l1_entry = l1_base_addr + l1_offset;

		ul* PT_base;
		if (flag == MM_SEG_RODATA) {
			printf("Resolving PF at RODATA Segment!\n");
			*l1_entry = (os_pfn_alloc(USER_REG) << 12) | 0x5;
		}
		else if (flag == MM_SEG_STACK) {
			printf("Resolving PF at STACK Segment!\n");
			*l1_entry = (os_pfn_alloc(USER_REG) << 12) | a_flag;
		}
		else {
			printf("Resolving PF at DATA Segment!\n");
			*l1_entry = (os_pfn_alloc(USER_REG) << 12) | a_flag;
			PT_base = (unsigned long*)osmap((*l1_entry) >> 12);
		}
	}
	u64 foo;
	__asm__ __volatile__(
			"mov %0, %%rsp\n\t"
			"pop %%r9\n\t"
			"pop %%r8\n\t"
			"pop %%r15\n\t"
			"pop %%r14\n\t"
			"pop %%r13\n\t"
			"pop %%r12\n\t"
			"pop %%r11\n\t"
			"pop %%r10\n\t"
			"pop %%rsi\n\t"
			"pop %%rdi\n\t"
			"pop %%rbx\n\t"
			"pop %%rdx\n\t"
			"pop %%rcx\n\t"
			"pop %%rax\n\t"
			: 
			:"r"(stack)
			: 
			);
	__asm__ __volatile__(
			"mov %%rbp, %%rsp\n\t"
			"add $16, %%rsp\n\t"
			"mov (%%rbp), %%rbp\n\t"
			"iretq"
			: 
			: 
			:
			);
	return 0;
}
