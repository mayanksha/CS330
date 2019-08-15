#include<context.h>
#include<memory.h>
#include<lib.h>

#define ull	unsigned long long  
#define ul	unsigned long  
void prepare_context_mm(struct exec_context *ctx){
	int arr[3] = {
		MM_SEG_CODE,
		MM_SEG_STACK,
		MM_SEG_DATA
	};

	ctx->pgd = os_pfn_alloc(OS_PT_REG);
	ul * l4_base_addr = (unsigned long*)osmap(ctx->pgd);
	for(int i = 0;i < 3;i++){
		/*int a_flag = ((ctx->mms[arr[i]]).access_flags << 1) | 0x1;*/
		int a_flag = 0x7;
		switch(arr[i]){
			case(MM_SEG_CODE):printf("\t\tCOOODE\n"); break;
			case(MM_SEG_STACK):printf("\t\tSTACK\n"); break;
			case(MM_SEG_DATA):printf("\t\tDATA\n"); break;
		}
		printf("\t\tstart = %x, end = %x, AF = %d, diff = %x\n"
				, (ctx->mms[arr[i]]).start
				, (ctx->mms[arr[i]]).end
				, (ctx->mms[arr[i]]).access_flags)
			, ((ctx->mms[i]).end) - (ctx->mms[i]).start;
		// 0x1FF = 0001 1111 1111 in binary

		u32 l4_offset =  (arr[i] == MM_SEG_STACK)?
			((ctx->mms[arr[i]].end-1) >> 39) & 0x1FF:
			(ctx->mms[arr[i]].start >> 39) & 0x1FF;

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

		//printf("l4_base = %x \n", l4_base_addr);
		//printf("l4_offset = %x \n", l4_offset);
		//printf("l4_entry = %x \n", l4_entry);
		//printf("*l4_entry = %x \n", *l4_entry);


		u32 l3_offset =  (arr[i] == MM_SEG_STACK)?
			((ctx->mms[arr[i]].end-1) >> 30) & 0x1FF:
			(ctx->mms[arr[i]].start >> 30) & 0x1FF;

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

		//printf("l3_base = %x \n", l3_base_addr);
		//printf("l3_offset = %x \n", l3_offset);
		//printf("l3_entry = %x \n", l3_entry);
		//printf("*l3_entry = %x \n", *l3_entry);


		u32 l2_offset =  (arr[i] == MM_SEG_STACK)?
			((ctx->mms[arr[i]].end -1) >> 21) & 0x1FF:
			(ctx->mms[arr[i]].start >> 21) & 0x1FF;

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

		//printf("l2_base = %x \n", l2_base_addr);
		//printf("l2_offset = %x \n", l2_offset);
		//printf("l2_entry = %x \n", l2_entry);
		//printf("*l2_entry = %x \n", *l2_entry);

		// ******** Base PFN for Page 1 of Paging 
		u32 l1_offset =  (arr[i] == MM_SEG_STACK)?
			((ctx->mms[arr[i]].end -1) >> 12) & 0x1FF:
			(ctx->mms[arr[i]].start >> 12) & 0x1FF;

		ul* l1_entry = &l1_base_addr[l1_offset];

		ul* PT_base;
		if (arr[i] != MM_SEG_DATA){
			//printf("Not MM SEG DATA!\n");
			if (arr[i] == MM_SEG_CODE)
				*l1_entry = (os_pfn_alloc(USER_REG) << 12) | 0x5;
			else 
				*l1_entry = (os_pfn_alloc(USER_REG) << 12) | a_flag;


			PT_base = (unsigned long*)osmap(*l1_entry >> 12);
		}
		else {
			//printf("YES MM SEG DATA!\n");
			*l1_entry = ((ctx->arg_pfn)<<12) | a_flag;
			PT_base = (unsigned long*)osmap(*l1_entry >> 12);
		}
		//printf("l1_base = %x \n", l1_base_addr);
		//printf("l1_offset = %x \n", l1_offset);
		//printf("l1_entry = %x \n", l1_entry);
		//printf("*l1_entry = %x \n", *l1_entry);
		/*u32 l3_pos = ctx->pgd + 	*/
	}	
	return;
}
void printArray(ul arr[], int n){
	for(int i = 0;i < n;i++){
		printf("arr[%d] = %x\n",i, arr[i]);
	}
}
void cleanup_context_mm(struct exec_context *ctx)
{
	printf("\tid = %d\n", ctx->id);
	printf("\ttype = %d\n", ctx->type);
	printf("\tstatus = %d\n", ctx->status);
	printf("\tused_mem = %d\n", ctx->used_mem);
	printf("\tpgd  = %d\n" , ctx->pgd);
	printf("\tname = %s\n", ctx->name);
	printf("\targ_pfn = %d\n",ctx->arg_pfn);
	printf("\tnum_args = %d\n", ctx->num_args);

	int arr[3] = {
		MM_SEG_CODE,
		MM_SEG_STACK,
		MM_SEG_DATA
	};

	ul codeTables[3];
	ul stackTables[3];
	ul dataTables[3];
	ul pageFrameTables[3];


	// Free Stack Segment
	ul cs = ctx->mms[MM_SEG_CODE].start;
	ul* cBase = (ul* )osmap(ctx->pgd);	
	ul cOffset = (cs >> 39) & 0x1FF;
	ul* cEntry = cBase + cOffset;
	codeTables[0] = (*cEntry) >> 12;

	cBase = (ul*) osmap(*cEntry >> 12);
	cOffset = (cs >> 30) & 0x1FF;
	cEntry = cBase + cOffset;
	codeTables[1] = *cEntry >> 12;

	cBase = (ul*) osmap(*cEntry >> 12);
	cOffset = (cs >> 21) & 0x1FF;
	cEntry = cBase + cOffset;
	codeTables[2] = *cEntry >> 12;

	cBase = (ul*) osmap(*cEntry >> 12);
	cOffset = (cs >> 12) & 0x1FF;
	cEntry = cBase + cOffset;
	pageFrameTables[0] = *cEntry >> 12;

	printArray(codeTables, 3);
	// Free Stack Segment
	ul ss = ctx->mms[MM_SEG_STACK].end - 1;
	ul* sBase = (ul* )osmap(ctx->pgd);	
	ul sOffset = (ss >> 39) & 0x1FF;
	ul* sEntry = sBase + sOffset;
	stackTables[0] = *sEntry >> 12;

	sBase = (ul*) osmap(*sEntry >> 12);
	sOffset = (ss >> 30) & 0x1FF;
	sEntry = sBase + sOffset;
	stackTables[1] = *sEntry >> 12;

	sBase = (ul*) osmap(*sEntry >> 12);
	sOffset = (ss >> 21) & 0x1FF;
	sEntry = sBase + sOffset;
	stackTables[2] = *sEntry >> 12;

	sBase = (ul*) osmap(*sEntry >> 12);
	sOffset = (ss >> 12) & 0x1FF;
	sEntry = sBase + sOffset;
	pageFrameTables[1] = *sEntry >> 12;

	printArray(stackTables, 3);

	// Free Data Segment
	ul ds = ctx->mms[MM_SEG_DATA].start;
	ul* dBase = (ul* )osmap(ctx->pgd);	
	ul dOffset = (ds >> 39) & 0x1FF;
	ul* dEntry = dBase + dOffset;
	dataTables[0] = *dEntry >> 12;

	dBase = (ul*) osmap(*dEntry >> 12);
	dOffset = (ds >> 30) & 0x1FF;
	dEntry = dBase + dOffset;
	dataTables[1] = *dEntry >> 12;

	dBase = (ul*) osmap(*dEntry >> 12);
	dOffset = (ds >> 21) & 0x1FF;
	dEntry = dBase + dOffset;
	dataTables[2] = *dEntry >> 12;

	dBase = (ul*) osmap(*dEntry >> 12);
	dOffset = (ds >> 12) & 0x1FF;
	dEntry = dBase + dOffset;
	pageFrameTables[2] = *dEntry >> 12;
	printArray(dataTables, 3);

	for(int i = 0;i < 3;i++){
		os_pfn_free(USER_REG, pageFrameTables[i]);
	}
	os_pfn_free(OS_PT_REG, stackTables[2]);
	os_pfn_free(OS_PT_REG, codeTables[2]);
	os_pfn_free(OS_PT_REG, dataTables[2]);

	os_pfn_free(OS_PT_REG, stackTables[1]);
	os_pfn_free(OS_PT_REG, codeTables[1]);
	os_pfn_free(OS_PT_REG, dataTables[1]);

	/*os_pfn_free(OS_PT_REG, stackTables[0]);*/
	/*os_pfn_free(OS_PT_REG, codeTables[0]);*/
	os_pfn_free(OS_PT_REG, dataTables[0]);
	/*for(int i = 0;i < 3;i++){
	 *    os_pfn_free(USER_REG, pageFrameTables[i]);
	 *}
	 *for(int i = 2;i >= 2;i--){
	 *    os_pfn_free(OS_PT_REG, stackTables[i]);
	 *}
	 *for(int i = 2;i >= 2;i--){
	 *    os_pfn_free(OS_PT_REG, codeTables[i]);
	 *}
	 *for(int i = 2;i >= 2;i--){
	 *    os_pfn_free(OS_PT_REG, dataTables[i]);
	 *}*/
	os_pfn_free(OS_PT_REG,ctx->pgd);
	return;
}
