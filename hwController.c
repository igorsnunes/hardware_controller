#include  <stdio.h>
#include <string.h>
#include  <sys/types.h>
#include "sllp_server.h"
#include <glib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "/usr/include/pciDriver/driver/pciDriver.h"
#include "/usr/include/pciDriver/lib/pciDriver.h"
#include <netinet/tcp.h>

#define   MAX_DEVICES  5
#define   VAR_NUM 1
#define   CURVE_NUM 1
#define   PORT 7000

#define BRAM_SIZE  0x4000
#define MAXBUFRECV 256
#define SLLP_CURVE_TEST 2

typedef enum {
	TYPE_CHILD_PROCESS,
	TYPE_PARENT_PROCESS
} process_type;


typedef struct {
    int DeviceNumber;
    pd_device_t	*pci_handle;
    void *kernel_memory;
    pd_kmem_t *kmem_handle;
    pd_umem_t *umem_handle;
    void **bar;
    int fd;
    struct sllp_var variable_manager[VAR_NUM];
    struct sllp_curve dma_curve[CURVE_NUM];
    sllp_server_t *sllp;
    sllp_server_t *sllp_dma;
} pcie_dma_pvt;

typedef struct {
	GList *high;
	GList *low;
} command_list;
	
typedef struct {
	command_list *cmds;
	pcie_dma_pvt *ppvt;
	int childfd;
} connection_fd;

typedef struct {
	int fd_dest;
} command;


	
typedef enum {
	CONN_LOW,
	CONN_HIGH,
	CONN_DMA
} thread_parameter_type;

static void pcie_dma_pvt_Initialize(pcie_dma_pvt *pvt){
	pvt->pci_handle = NULL;
	pvt->kernel_memory = NULL;
	pvt->kmem_handle = NULL;
	pvt->umem_handle = NULL;
	pvt->bar = NULL;
	return;
}
static void pcie_dma_pvt_Cleanup(pcie_dma_pvt *pvt){
	int i = 0;
	if (pvt->pci_handle != NULL)
        	pd_close(pvt->pci_handle);
	if(pvt->bar != NULL){
		for(i = 0; i <= 4; i = i+2)
			g_free(pvt->bar[i]);
	}
	g_free(pvt->bar);
	g_free(pvt->kmem_handle);
	g_free(pvt->umem_handle);
	g_free(pvt);
}
void connection_low(void *conn_fd){
	connection_fd *fd_pvt = (connection_fd*)conn_fd;
	int childfd = fd_pvt->childfd;
	int n;
	int i;
	pcie_dma_pvt *pvt = fd_pvt->ppvt;
	uint8_t *buf;	
	uint8_t *auxbuf;
	uint8_t *bufresponse;
	g_free(fd_pvt);
	struct sllp_raw_packet request;
	struct sllp_raw_packet response;
	printf("connection low socket: %u", childfd);
	while (1){
		
		buf = (uint8_t*)g_try_malloc(sizeof(uint8_t)*MAXBUFRECV);
		printf("waiting message from client\n");
		n = read(childfd, (char*)buf, MAXBUFRECV);
		printf("message received: %d %d \n",buf[0],buf[1]);
		for(i = 0;i<buf[1];i++)
			printf("%d ",buf[i+2]);
		printf("\n");
		auxbuf = g_try_malloc(n*sizeof(char));
		if (n < 0) 
			error("ERROR reading from socket");
	
		memcpy(auxbuf,buf,n);
	
		g_free(buf);

		request.data = auxbuf;
		request.len = n;

		bufresponse = (char*)g_try_malloc(sizeof(char)*MAXBUFRECV);
		response.data = bufresponse;
		
		sllp_process_packet (pvt->sllp,&request,&response);

		printf("data to be sent: %d %d \n",response.data[0],response.data[1]);
		for(i = 0;i<response.data[1];i++)
			printf("%d ",response.data[i+2]);
		printf("\n");
		
		printf("pci value: %d\n", *((uint32_t*)(pvt->bar[2]+(uint32_t)0)));
		n = write(childfd, (char*)response.data, response.len);
		if (n < 0) 
			error("ERROR writing to socket");
	
		g_free(auxbuf);
		g_free(bufresponse);
	}
	return;
}
static uint8_t* recv_message(int *n,unsigned int childfd, uint8_t *buf){
	uint8_t *auxbuf;
	int i;

	printf("waiting message from client\n");
	*n = read(childfd, (char*)buf, MAXBUFRECV);
	printf("message received: %d %d \n",buf[0],buf[1]);
	for(i = 0;i<buf[1];i++)
		printf("%d ",buf[i+2]);
	printf("\n");
	auxbuf = g_try_malloc(*(n)*sizeof(uint8_t));
	if (n < 0){ 
		error("ERROR reading from socket");
		return NULL;
	}

	memcpy(auxbuf,buf,*(n));

	g_free(buf);
	return auxbuf;
}
static int send_message(int *n, unsigned int childfd, uint8_t *buf, unsigned int len){	
	int i;
	printf("data to be sent: %d %d \n",buf[0],buf[1]);
	for(i = 0;i<buf[1];i++)
		printf("%d ",buf[i+2]);
	printf("\n");
	
	*n = write(childfd, (char*)buf, len);
	if (n < 0){
		error("ERROR writing to socket");
		return 0;
	}
	return 1;
}
void connection_dma(void *conn_fd){
	connection_fd *fd_pvt = (connection_fd*)conn_fd;
	int childfd = fd_pvt->childfd;
	int n;
	int i;
	pcie_dma_pvt *pvt = fd_pvt->ppvt;
	uint8_t *buf;	
	uint8_t *auxbuf;
	uint8_t *bufresponse;
	g_free(fd_pvt);
	struct sllp_raw_packet request;
	struct sllp_raw_packet response;
	while (1){
		
		buf = (uint8_t*)g_try_malloc(sizeof(uint8_t)*MAXBUFRECV);
		buf = recv_message(&n,childfd,buf);

		request.data = buf;
		request.len = n;

		bufresponse = (char*)g_try_malloc(sizeof(char)*MAXBUFRECV);
		response.data = bufresponse;
		
		sllp_process_packet (pvt->sllp_dma,&request,&response);
		
		send_message(&n, childfd, response.data, response.len);	
		g_free(buf);
		g_free(bufresponse);
	}
	return;
}

static GThread *serve_it(void *thread_parameter, thread_parameter_type type){
	connection_fd *fd_n,*fd_o;
	GThread *new_thread;
	GError *err1 = NULL;
	fd_o = (connection_fd*)thread_parameter;
	fd_n = g_new(connection_fd,1);
	memcpy(fd_n,fd_o,sizeof(connection_fd));

	switch(type){
		case CONN_LOW:
			new_thread = g_thread_create((GThreadFunc)connection_low,(void*)fd_n,TRUE,&err1);
			break;
		case CONN_HIGH:
			//new_thread = g_thread_create((GThreadFunc)command_execution,thread_parameter,TRUE,&err1);
			//break;
		case CONN_DMA:
			new_thread = g_thread_create((GThreadFunc)connection_dma,(void*)fd_n,TRUE,&err1);
			break;
	}
	if(!new_thread)
		printf("thread could not be created!\n");

	return new_thread;
}

void read_dio(struct sllp_curve *curve, uint8_t block, uint8_t *data){
	printf("READ DMA");
	uint8_t *data_block = (uint8_t*) curve->user + block*SLLP_CURVE_BLOCK_SIZE;
	memcpy(data, data_block, SLLP_CURVE_BLOCK_SIZE);
	return;
}

void write_dio(struct sllp_curve *curve, uint8_t block, uint8_t *data){
	printf("WRITE DMA");
	// Pega informacao sobre qual curva escrever
	uint8_t *data_block = (uint8_t*) curve->user + block*SLLP_CURVE_BLOCK_SIZE;

	// Copia os dados
	memcpy(data_block, data, SLLP_CURVE_BLOCK_SIZE);
	return;
}
			
int main(void)
{
	int deviceNumber;
	int i;
	/***
	Create process
	***/
	
	//Creating process for each device
	
	pid_t  pid;
	process_type process = TYPE_PARENT_PROCESS;
	for(i=0; i<MAX_DEVICES; i++){
		pid = fork();
		if (pid == 0){
			process = TYPE_CHILD_PROCESS;
			deviceNumber = i;
			break;
		}
	}

	//Parent process waiting other process*/
	if (process == TYPE_PARENT_PROCESS)
		while(1);

	/***
	Pcie Configuration
	***/
	
	pcie_dma_pvt *pvt;
	pvt = g_new(pcie_dma_pvt,1);
	pcie_dma_pvt_Initialize(pvt);

	pvt->DeviceNumber = deviceNumber;


	/*check known device host*/

	char temp[50];
	sprintf( temp, "/dev/fpga%d", deviceNumber);
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
		pcie_dma_pvt_Cleanup(pvt);
		return -1;	
	}

	/*initialize pci handle*/
	pvt->pci_handle = g_new(pd_device_t,1);
	if (pd_open(pvt->DeviceNumber, pvt->pci_handle) != 0) {
		printf("failed to open device fpga%d\ntry another device id\n",pvt->DeviceNumber);
		printf("process %d being closed\n", (unsigned int)getpid());
		return -1;
	}
	else
	{
	        printf("device opened\n");
	}
	/*allocating bars*/

	//void **bar = (void**)malloc(sizeof(void*)*6);

	void **bar = (void**)g_try_malloc(sizeof(void*)*6);
	if(bar == NULL){
		printf("process %d being closed\n", (unsigned int)getpid());
		pcie_dma_pvt_Cleanup(pvt);
		return 0;
	}

	printf("Allocating bars\n");
	for (i=0;i<=4;i=i+2){
		bar[i] = pd_mapBAR(pvt->pci_handle, i);
		if (bar[i] == NULL){
			printf("process %d being closed\n", (unsigned int)getpid());
			printf("failed mapping bar%d\n",i);
			pcie_dma_pvt_Cleanup(pvt);
			return 0;
        	}
	}

	pvt->bar = bar;

	/*allocating kernel memory*/

	printf("process %d being closed\n", (unsigned int)getpid());

	pvt->kmem_handle = g_try_new(pd_kmem_t, 1);
	if (!pvt->kmem_handle){
		printf("process %d being closed\n", (unsigned int)getpid());
		pcie_dma_pvt_Cleanup(pvt);
		return 0;
	}
	pvt->kernel_memory = pd_allocKernelMemory(pvt->pci_handle, BRAM_SIZE, pvt->kmem_handle);
	if(pvt->kernel_memory == NULL){
		printf("Failed allocating kernel memory\n");
		printf("process %d being closed\n", (unsigned int)getpid());
		pcie_dma_pvt_Cleanup(pvt);
		return 0;
	}

	/*allocating user memory*/
	void *mem = NULL;
	posix_memalign( (void**)&mem,16,BRAM_SIZE );//TODO:compilation warning!!implicit declaration
	if (mem == NULL){
		printf("memory aligned failed\n");
		printf("process %d being closed\n", (unsigned int)getpid());
		pcie_dma_pvt_Cleanup(pvt);
		return 0;
	}
	pvt->umem_handle = g_try_new(pd_umem_t, 1);
	if (!pvt->kmem_handle){
		printf("process %d being closed\n", (unsigned int)getpid());
		pcie_dma_pvt_Cleanup(pvt);
		return -1;
	}
	pd_mapUserMemory(pvt->pci_handle, mem, BRAM_SIZE, pvt->umem_handle);//TODO:check return value
	g_free(mem);
	
	/***
	Initialize sllp libs
	***/
	enum sllp_err err;
	//test variable for sdram
	uint32_t offset = 0;
	uint32_t *sdram_mem = (uint32_t*)(pvt->bar[2]+offset);
	//struct sllp_var variable_manager[VAR_NUM];

	/*Dma curve*/
	pvt->sllp_dma = sllp_server_new();

	pvt->dma_curve[0].info.id = 0;
	pvt->dma_curve[0].info.writable = false;
	pvt->dma_curve[0].info.nblocks = 2;
	pvt->dma_curve[0].read_block = read_dio;	
	pvt->dma_curve[0].write_block = write_dio;
	pvt->dma_curve[0].user = (uint8_t*)sdram_mem;

	if((err = sllp_register_curve(pvt->sllp_dma, &pvt->dma_curve[0])))
	{
		printf("process %d being closed\n", (unsigned int)getpid());
		pcie_dma_pvt_Cleanup(pvt);
		fprintf(stderr, "sllp_register_curve: %s\n", sllp_error_str(err));
		return 0;
	}

	
	/*Low priority sllp*/
	
	uint8_t test_var[1] = { 0x03 };
	pvt->sllp = sllp_server_new();


	pvt->variable_manager[0].info.id = 0;	
	pvt->variable_manager[0].info.writable = true;
	pvt->variable_manager[0].info.size = sizeof(uint32_t);//4 bytes...I know
	pvt->variable_manager[0].data = (uint8_t*)(sdram_mem);
	sdram_mem[0]  = (uint32_t) 10;
	printf("teste: %u\n", sdram_mem[0]);

	if((err = sllp_register_variable(pvt->sllp, &pvt->variable_manager[0])))
	{
		printf("process %d being closed\n", (unsigned int)getpid());
		pcie_dma_pvt_Cleanup(pvt);
		fprintf(stderr, "sllp_register_variable: %s\n", sllp_error_str(err));
		return 0;
	}

	/***
	Initiate TCP server
	***/

	printf ("\nInitiating TCP server on process %d\n", (unsigned int)getpid());
	i = 0;
	int parentfd[3]; /* parent socket */
	int childfd[3]; /* child socket */
	int clientlen[3]; /* byte size of client's address */
	struct sockaddr_in serveraddr[3]; /* server's addr */
	struct sockaddr_in clientaddr[3]; /* client addr */
	struct hostent *hostp; /* client host info */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */
	for(i=0;i<2;i++){
		printf ("Trying connection on %d, on process %d\n",(PORT+i),(unsigned int)getpid());
		parentfd[i] = socket(AF_INET, SOCK_STREAM, 0);
		if (parentfd[i] < 0) 
			error("ERROR opening socket");

		optval = 1;
		setsockopt(parentfd[i], SOL_SOCKET, SO_REUSEADDR, 
		     (const void *)&optval , sizeof(int));

		setsockopt(parentfd[i], IPPROTO_TCP, TCP_NODELAY,(const void*)&optval, sizeof(int));
 
		bzero((char *) &serveraddr[i], sizeof(serveraddr));

		serveraddr[i].sin_family = AF_INET;

		serveraddr[i].sin_addr.s_addr = htonl(INADDR_ANY);

		printf("port: %u\n", PORT+i+3*(pvt->DeviceNumber)); 
		serveraddr[i].sin_port = htons((unsigned short)(PORT+i+3*(pvt->DeviceNumber)));

		if (bind(parentfd[i], (struct sockaddr *) &serveraddr[i], 
		 sizeof(serveraddr[i])) < 0) 
			error("ERROR on binding");

		if (listen(parentfd[i], 5) < 0) /* allow 5 requests to queue up */ 
			 error("ERROR on listen");
		clientlen[0] = sizeof(clientaddr[i]);
		clientlen[1] = sizeof(clientaddr[i]);
		
	//clientlen = sizeof(clientaddr);
		//childfd[i] = accept(parentfd[i], (struct sockaddr *) &clientaddr[i], &clientlen[i]);
		//if (childfd[i] < 0) 
			//error("ERROR on accept");
   	}
	
	/**
	Create threads
	**/

	if(g_thread_supported()){
		g_thread_init(NULL);
		//gdk_threads_init();
	}
	else{
		printf("g_thread not supported\n");
		printf("process %d being closed\n", (unsigned int)getpid());
		pcie_dma_pvt_Cleanup(pvt);
		return -1;
	}

	GThread *low_thread,*dma_thread;
	connection_fd *fd_struct;
	while(1){
		printf("accept: wait for a connection request on process %d  \n", (unsigned int)getpid());
		childfd[0] = accept(parentfd[0], (struct sockaddr *) &clientaddr[0], &clientlen[0]); 
		childfd[1] = accept(parentfd[1], (struct sockaddr *) &clientaddr[1], &clientlen[1]);
		printf("accept: wait for a connection request on process %d  \n", (unsigned int)getpid());
		printf("childfd[0] = %d childfd[1] =  %d\n",childfd[0], childfd[1]);
	
		if ( (childfd[0] < 0) || (childfd[1] < 0)){
		
			error("ERROR on accept");
			continue;
		}
		else{
			printf("connections accepted\n");
			fd_struct = g_new(connection_fd,1);
			fd_struct->ppvt = pvt;
			
			fd_struct->childfd = childfd[0];
			printf("socket main thread: %d\n",childfd[1]);
			printf("socket main thread: %d\n",fd_struct->childfd);
			low_thread = serve_it(fd_struct,CONN_LOW);
			printf("socket main thread: %d\n",fd_struct->childfd);
			fd_struct->childfd = childfd[1];
			printf("socket main thread: %d\n",fd_struct->childfd);
			dma_thread = serve_it(fd_struct,CONN_DMA);
			printf("socket main thread: %d\n",fd_struct->childfd);
			g_free(fd_struct);
		}
	}

	return 0;
}
