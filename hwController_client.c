#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "sllp_client.h"
#include <stdint.h>
#include <netinet/tcp.h>


int socket_gl;
int socket_gl2;
int sendFPGA(uint8_t *data, uint32_t *count)
{
    printf("send\n");
    printf("count = %u\n", *count);
    size_t wrote;

    wrote = send(socket_gl, data, *count, 0);
    //*count = wrote;

	//printf("wrote: %d\n", wrote);

    return EXIT_SUCCESS;
}

int recvFPGA(uint8_t *data, uint32_t *count){
	uint8_t* header,packet[17000];
	header = (uint8_t*) malloc(2*sizeof(char));
	int n;
	size_t size;
	n = recv(socket_gl, header, 2, 0);
	if(n < 0) printf("Error reading header\n"); //TODO: Return error;

	if(header[1] == 255)
		size = 16386;
	else
		size = header[1];

	uint8_t* payload;

	if(size > 0)
	{
		payload = (uint8_t*) malloc(size*sizeof(char));
		n  = recv(socket_gl, payload, size, 0);
		if(n < 0) printf("Error reading payload\n");
	}

	memcpy(packet, header, 2);
	if(size > 0) memcpy(packet+2, payload, size);

	*count = size+2;

	memcpy(data, packet, *count);

	free(header);
	if(size > 0) free(payload);

	/*data = (uint8_t*)malloc(sizeof(data)*1000);
	int n = recv(socket_gl, data, *count, 0);

	printf("data[0]:%d,data[1]:%d\n",data[0],data[1]);*/
        return EXIT_SUCCESS;
}


int sendFPGA2(uint8_t *data, uint32_t *count)
{
    printf("send2\n");
    printf("count2 = %u\n", *count);
    size_t wrote;

    wrote = send(socket_gl2, data, *count, 0);
    //*count = wrote;

	//printf("wrote: %d\n", wrote);

    unsigned int i;
    return EXIT_SUCCESS;
}

int recvFPGA2(uint8_t *data, uint32_t *count){
	uint8_t* header,packet[17000];
	header = (uint8_t*) malloc(2*sizeof(char));
	int n;
	size_t size;
	n = recv(socket_gl2, header, 2, 0);
	if(n < 0) printf("Error reading header\n"); //TODO: Return error;

	if(header[1] == 255)
		size = 16386;
	else
		size = header[1];

	printf("header %d\n",header[0]);
	uint8_t* payload;

	if(size > 0)
	{
		payload = (uint8_t*) malloc(size*sizeof(char));
		n  = recv(socket_gl2, payload, size, 0);
		if(n < 0) printf("Error reading payload\n");
	}

	memcpy(packet, header, 2);
	if(size > 0) memcpy(packet+2, payload, size);

	*count = size+2;

	memcpy(data, packet, *count);

	free(header);
	if(size > 0) free(payload);

	/*data = (uint8_t*)malloc(sizeof(data)*1000);
	int n = recv(socket_gl2, data, *count, 0);

	printf("data[0]:%d,data[1]:%d\n",data[0],data[1]);*/
        unsigned int i;
        printf("RECV [ ");
        if(*count <= 16)
            for(i = 0; i < *count; ++i)
                printf("%02X ", data[i]);
        else
            printf(" %d bytes ", *count);
        printf("]\n");
        return EXIT_SUCCESS;
}

int main(int argc, char **argv) {

	sllp_client_t *sllp,*sllp2;
	struct sockaddr_in servaddr;
	//sock = (int*)malloc(sizeof(int));
	socket_gl = socket(AF_INET, SOCK_STREAM, 0);
	int err;
	struct sllp_vars_list *vars;

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(7000);
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	int optval; /* flag value for setsockopt */

	optval = 1;
	//setsockopt(socket_gl, IPPROTO_TCP, TCP_NODELAY,(const void*)&optval, sizeof(int));
	if( connect(socket_gl, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1){
		printf("error connecting to server\n");
		return -1;
	}

	socket_gl2 = socket(AF_INET, SOCK_STREAM, 0);
	struct sllp_curves_list *vars2;

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(7001);
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	setsockopt(socket_gl2, IPPROTO_TCP, TCP_NODELAY,(const void*)&optval, sizeof(int));
	if( connect(socket_gl2, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1){
		printf("error connecting to server\n");
		return -1;
	}


	char *writechar;
	writechar = (char*)malloc(sizeof(char)*4);
	writechar[3] = 0x01;
	writechar[2] = 0x02;
	writechar[1] = 0x03;
	writechar[0] = 0x04;

	sllp = sllp_client_new(sendFPGA, recvFPGA);
	if (sllp == NULL){
		printf("sllp initialization error\n");
		return -1;
	}
	if (err = sllp_client_init(sllp)!=SLLP_SUCCESS){
		printf("Client initialization error: %d\n",err);
		return -1;
	}

	if (sllp_get_vars_list(sllp, &vars)!=SLLP_SUCCESS){
		printf("Variable listing error\n");
	}
	if (!sllp)
	{
		printf("SLLP fail\n");
		return -1;
	}

	struct sllp_var_info *var = &vars->list[0];

	/**curves**/
	sllp2 = sllp_client_new(sendFPGA2, recvFPGA2);
	if (sllp2 == NULL){
		printf("sllp initialization error\n");
		return -1;
	}
	if ((err = sllp_client_init(sllp2))!=SLLP_SUCCESS){
		printf("Client initialization error: %s\n", sllp_error_str (err));
		return -1;
	}

	if (sllp_get_curves_list(sllp2, &vars2)!=SLLP_SUCCESS){
		printf("Variable listing error\n");
	}
	if (!sllp2)
	{
		printf("SLLP fail\n");
		return -1;
	}
	uint8_t *curvechar = (uint8_t*)malloc(sizeof(uint8_t)*SLLP_CURVE_BLOCK_SIZE);
	//curvechar[0] = 'A';
	//curvechar[1] = 'B';
	struct sllp_curve_info *var0 = &vars2->list[0];
	struct sllp_curve_info *var1 = &vars2->list[1];
	//receive_answer(sock);
	int i=0;
	while(1) {

		if(sllp_write_var(sllp, var, (uint8_t*)writechar)!=SLLP_SUCCESS)
		{
			printf("write error!\n");
		}
		if(sllp_read_var(sllp, var, (uint8_t*)writechar)!=SLLP_SUCCESS)
			printf("read error!\n");
		for(i=0;i<3;i++)
			printf("byte received: %d\n", writechar[i]);

		//if(sllp_send_curve_block(sllp2,var2,0,curvechar)!=SLLP_SUCCESS)
		//	printf("read curve error\n");


//		if((err = (sllp_request_curve_block(sllp2,var0,0,curvechar)))!=SLLP_SUCCESS)
//			printf("Request curve error: %s\n", sllp_error_str (err));

		if((err = (sllp_request_curve_block(sllp2,var1,0,curvechar)))!=SLLP_SUCCESS)
			printf("Request curve dma  error: %s\n", sllp_error_str (err));
		sleep(10);
	}

    //close(sock);
}
