#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

//struct que contem as informacoes do client
struct Client_ID{

	unsigned char username[100];
	unsigned char buffer[512];
	int bufferlen;
	unsigned char ip[INET_ADDRSTRLEN];

};

//funcao que recebe as mensagens do servidor
void *RecieveMsg(void *sock)
{
	struct Client_ID rcvdata;
	int their_sock = *((int *)sock);
	int len;

	//recebe as informacoes de quem enviou a mensagem
	while((len = read(their_sock, &rcvdata, sizeof(struct Client_ID))) > 0) {

		rcvdata.bufferlen = strlen(rcvdata.buffer);
		rcvdata.buffer[rcvdata.bufferlen] = '\0';

		printf("%s: %s\n",rcvdata.username, rcvdata.buffer);
		memset(rcvdata.buffer,'\0',sizeof(rcvdata.buffer));

	}

}

int main(int argc, char *argv[])
{

	struct sockaddr_in serv_addr;
	int my_sock;
	int portn;
	char servip[20];
	pthread_t recvt;

	struct Client_ID cdata;
	char ip[INET_ADDRSTRLEN];
	int len;

	if(argc > 4) {
		printf("Muitos argumentos");
		exit(1);
	}

	strcpy(servip, argv[1]);				//ip do server
	portn = atoi(argv[2]);					//porta
	strcpy(cdata.username,argv[3]);			//username


	my_sock = socket(AF_INET,SOCK_STREAM,0);
	memset(serv_addr.sin_zero,'\0',sizeof(serv_addr.sin_zero));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portn);
	serv_addr.sin_addr.s_addr = inet_addr(servip);

	if(connect(my_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
		perror("Conexao nao efetuada");
		exit(1);
	}


	inet_ntop(AF_INET, (struct sockaddr *)&serv_addr, ip, INET_ADDRSTRLEN);

	printf("CONECTADO ao servidor com endereço IP %s na porta %d\n", servip, portn);

	//inicia a thread que recebe as mensagens do servidor
	pthread_create(&recvt,NULL,RecieveMsg,&my_sock);


	//digitar e enviar a mensagem
	while(fgets(cdata.buffer,512,stdin) > 0) {

		cdata.bufferlen = strlen(cdata.buffer);
        cdata.buffer[cdata.bufferlen-1] = '\0';

		if(!strcmp(cdata.buffer, "SAIR")){
			close(my_sock);
			exit(0);
		}
		
		printf("%s: %s\n", cdata.username, cdata.buffer);

		//a struct que contem as informacoes do client é enviada para o servidor
		len = write(my_sock, &cdata, sizeof(struct Client_ID));

		if(len < 0) {
			perror("Mensagem nao enviada");
			exit(1);
		}

		memset(cdata.buffer,'\0',sizeof(cdata.buffer));
	}

	pthread_join(recvt,NULL);
	close(my_sock);

	return 0;

}