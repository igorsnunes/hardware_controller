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

int socket_gl;

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

int main(int argc, char **argv) {

	sllp_client_t *sllp;
	struct sockaddr_in servaddr;
	//sock = (int*)malloc(sizeof(int));
	socket_gl = socket(AF_INET, SOCK_STREAM, 0);
	int err;
	struct sllp_vars_list *vars;

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(7000);
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if( connect(socket_gl, (struct sockaddr *)&servaddr, sizeof(servaddr))==-1){
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
		return 0;	
	}

    //close(sock);
}
