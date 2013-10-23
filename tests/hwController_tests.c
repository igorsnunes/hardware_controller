#include "CUnit/CUnit.h"
#include "CUnit/Basic.h"
#include "../readwriteDMA.h"
#include "/usr/include/pciDriver/driver/pciDriver.h"
#include "/usr/include/pciDriver/lib/pciDriver.h"

#include "glib.h"
//include "CUnit/Automated.h"
//#include "CUnit/Console.h"

#define REG_SDRAM_PG 0x1c

#include <stdio.h>  // for printf

/* Test Suite setup and cleanup functions: */

pd_device_t *pci_handle = NULL;
void *kernel_memory = NULL;
pd_kmem_t *kmem_handle = NULL;
pd_umem_t *umem_handle = NULL;
void **bar = NULL;
int DeviceNumber = 0;

int init_suite(void) {
	int i;
	char temp[50];
	sprintf( temp, "/dev/fpga%d", DeviceNumber);
	FILE* filep = fopen(temp, "r");
	if (filep)
	{
		printf("\nfile descriptor found\n");
		fclose(filep);
	}
	else
	{
		printf("\nfile descriptor was not found!\n");
		printf("process %d being closed\n", (unsigned int)getpid());
		return -1;
	}

	/*initialize pci handle*/
	pci_handle = g_new(pd_device_t,1);
	if (pd_open(DeviceNumber, pci_handle) != 0) {
		printf("failed to open device fpga%d\ntry another device id\n",DeviceNumber);
		printf("process %d being closed\n", (unsigned int)getpid());
		return -1;
	}
	else
	{
	        printf("device opened\n");
	}
	/*allocating bars*/

	//void **bar = (void**)malloc(sizeof(void*)*6);

	bar = (void**)g_try_malloc(sizeof(void*)*6);
	if(bar == NULL){
		printf("process %d being closed\n", (unsigned int)getpid());
		return 0;
	}

	printf("Allocating bars\n");
	for (i=0;i<=4;i=i+2){
		bar[i] = pd_mapBAR(pci_handle, i);
		if (bar[i] == NULL){
			printf("process %d being closed\n", (unsigned int)getpid());
			printf("failed mapping bar%d\n",i);
			return 0;
        	}
	}


	printf("allocating kernel memory\n");

	kmem_handle = g_try_new(pd_kmem_t, 1);
	if (!kmem_handle){
		printf("process %d being closed\n", (unsigned int)getpid());
		return 0;
	}
	kernel_memory = pd_allocKernelMemory(pci_handle, BRAM_SIZE, kmem_handle);
	if(kernel_memory == NULL){
		printf("Failed allocating kernel memory\n");
		printf("process %d being closed\n", (unsigned int)getpid());
		return 0;
	}

	printf("allocating user memory\n");
	void *mem = NULL;
	posix_memalign( (void**)&mem,16,BRAM_SIZE );//TODO:compilation warning!!implicit declaration
	if (mem == NULL){
		printf("memory aligned failed\n");
		printf("process %d being closed\n", (unsigned int)getpid());
		return -1;
	}
	umem_handle = g_try_new(pd_umem_t, 1);
	if (!umem_handle){
		printf("process %d being closed\n", (unsigned int)getpid());
		return -1;
	}
	pd_mapUserMemory(pci_handle, mem, BRAM_SIZE, umem_handle);//TODO:check return value
	g_free(mem);

	return 0;
}
int clean_suite(void) {
	int i;
	if (pci_handle != NULL)
	  	pd_close(pci_handle);
	if(bar != NULL){
		for(i = 0; i <= 4; i = i+2)
			pd_unmapBAR(pci_handle,i,bar[i]);
	}
	g_free(bar);
	g_free(kmem_handle);
	g_free(umem_handle);

	pci_handle = NULL;
	kernel_memory = NULL;
	kmem_handle = NULL;
	umem_handle = NULL;
	bar = NULL;

	return 0;
}

/************* Test case functions ****************/

void test_case_sample(void)
{
	CU_ASSERT(CU_TRUE);
	CU_ASSERT_NOT_EQUAL(2, -1);
	CU_ASSERT_STRING_EQUAL("string #1", "string #1");
	CU_ASSERT_STRING_NOT_EQUAL("string #1", "string #2");

	CU_ASSERT(CU_FALSE);
	CU_ASSERT_EQUAL(2, 3);
	CU_ASSERT_STRING_NOT_EQUAL("string #1", "string #1");
	CU_ASSERT_STRING_EQUAL("string #1", "string #2");
}
void test_dma_transfer(void){
	int i;
	uint32_t *ptr = (uint32_t*)kernel_memory;
	uint32_t *bar2 = bar[2];
	uint32_t data[10];

	printf("CU_ASSERT_EQUAL(bar[i],ptr[i])\n");
	for(i=0;i<10;i++){
		printf("%d = %d ?\n",bar2[i],ptr[i]);
		CU_ASSERT_EQUAL(bar2[i],ptr[i]);
	}
	memset(ptr ,2, 10);
	DMAKernelMemoryWrite((uint32_t*)bar[0], (uint32_t*)bar[2], NULL, kmem_handle, 10, kernel_memory, 1, 0);

	printf("CU_ASSERT_EQUAL(bar2[i],2)\n");
	for(i=0;i<10;i++){
		printf("bar[2] = %d\n",bar2[i]);
		CU_ASSERT_EQUAL(bar2[i],2);
	}
	printf("CU_ASSERT_EQUAL(bar2[i],ptr[i])\n");
	for(i=0;i<10;i++){
		printf("%d = %d ?\n",bar2[i],ptr[i]);
		CU_ASSERT_EQUAL(bar2[i],ptr[i]);
	}

	memset(ptr ,0, 10);
	DMAKernelMemoryRead(bar[0], bar[2], NULL, kmem_handle, 10, kernel_memory, 1, 0);
	memcpy(data,ptr,10);

	printf("CU_ASSERT_EQUAL(data[i],2)");
	for(i=0;i<10;i++){
		printf("%d = 2 ?\n",bar2[i]);
		CU_ASSERT_EQUAL(data[i],2);
	}

}
void simple_pointer_test(void){
	int a = 2;
	int *p = &a;
	int *p2 = (int*)malloc(sizeof(int));
	*p2 = 3;
	CU_ASSERT_EQUAL(2,2);
	CU_ASSERT_EQUAL(2,a);
	CU_ASSERT_EQUAL(2,*p);
	CU_ASSERT_EQUAL(*p2,3);
}

/************* Test Runner Code goes here **************/

int main ( void )
{
	CU_pSuite pSuite = NULL;

	/* initialize the CUnit test registry */
	if ( CUE_SUCCESS != CU_initialize_registry() )
		return CU_get_error();

	/* add a suite to the registry */

	pSuite = CU_add_suite( "test_dma_suite", init_suite, clean_suite );
	if ( NULL == pSuite ) {
		CU_cleanup_registry();
		return CU_get_error();
	}

	/* add the tests to the suite */
	if ( (NULL == CU_add_test(pSuite, "simple_pointer_test", simple_pointer_test))){
		CU_cleanup_registry();
		return CU_get_error();
	}
	if ( (NULL == CU_add_test(pSuite, "test_dma_transfer", test_dma_transfer))){
		CU_cleanup_registry();
		return CU_get_error();
	}

	// Run all tests using the basic interface
	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	printf("\n");
	CU_basic_show_failures(CU_get_failure_list());
	printf("\n\n");
	/*
	// Run all tests using the automated interface
	CU_automated_run_tests();
	CU_list_tests_to_file();

	// Run all tests using the console interface
	CU_console_run_tests();
	*/
	/* Clean up registry and return */
	CU_cleanup_registry();
	return CU_get_error();
}
