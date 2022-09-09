#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

struct client_info{

    int sockno;
    char ip[INET_ADDRSTRLEN];

};

//struct que é usada na troca de informacoes
struct Client_ID{

    unsigned char username[100];
    unsigned char buffer[512];
    int bufferlen;
    unsigned char ip[INET_ADDRSTRLEN];

};


int clients[100];       //array que armazena no numero do socket para cada cliente
int n = 0;              //contador que indica a quantidade de clientes conectados
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


//monitor recebe apenas uma string como parametro
void *MonitorWriteEvent(void *data){

    char buff[600];

    strcpy(buff, (char*)data);

    pthread_mutex_lock(&mutex);

    FILE *archive;
    archive = fopen("server_events.txt", "a");
    fprintf(archive,"%s", buff);
    fclose(archive);

    pthread_mutex_unlock(&mutex);

}

//envia para todos os clientes, com excecao de quem enviou
void SendToAll(struct Client_ID *ptrdata, int skt){
    int i;
    pthread_mutex_lock(&mutex);

    for(i = 0; i < n; i++) {

        if(clients[i] != skt) {

            if(write(clients[i], ptrdata, sizeof(struct Client_ID)) < 0) {

                perror("sending failure");
                continue;
            }
        }
    }
    pthread_mutex_unlock(&mutex);

}

//manipula o client, executa a funcao de leitura e envio
void *ManageClient(void *sock){

    struct client_info cl = *((struct client_info *)sock);
    struct Client_ID cdata;
    pthread_t tmonitor;
    char message[600];
    //char msg[500];
    int len;
    int i, j;

        //a troca de mensagens é feita atraves de structs, que contem os dados do client
        while((len = read(cl.sockno, &cdata , sizeof(struct Client_ID))) > 0) {

            cdata.bufferlen = strlen(cdata.buffer);
            cdata.buffer[cdata.bufferlen] = '\0';

            strcpy(cdata.ip, cl.ip);

            printf("Mensagem de [%s] %s: %s\n", cdata.ip, cdata.username, cdata.buffer);

            strcpy(message, "Mensagem de [");
            strcat(message, cdata.ip);
            strcat(message, "] ");
            strcat(message, cdata.username);
            strcat(message, ": ");
            strcat(message, cdata.buffer);
            strcat(message, "\n");

            pthread_create(&tmonitor, NULL, MonitorWriteEvent, &message);

            SendToAll(&cdata, cl.sockno);

            memset(cdata.buffer,'\0',sizeof(cdata.buffer));
        }

        pthread_mutex_lock(&mutex);
        printf("O cliente com o IP [%s] se desconectou\n",cl.ip);

        strcpy(message,"O cliente com o IP [");
        strcat(message, cl.ip);
        strcat(message, "] se desconectou\n");

        pthread_create(&tmonitor, NULL, MonitorWriteEvent, &message);


        //deleta o client que foi desconectado, e da um deslocamento para a esquerda nos elementos do array
        for(i = 0; i < n; i++) {
            if(clients[i] == cl.sockno) {
                j = i;
                while(j < n-1) {
                    clients[j] = clients[j+1];
                    j++;
                }
            }
        }
        n--;
        pthread_mutex_unlock(&mutex);
    
}

int main(int argc,char *argv[]){

    struct sockaddr_in my_addr, client_addr;
    struct client_info cl;
    pthread_t tclient, tmonitor;
    socklen_t client_addr_size;
    int my_sock, client_sock;

    int portn;
    char mon_msg[600];
    int len;
    char ip[INET_ADDRSTRLEN];

    if(argc > 2) {
        printf("muitos argumentos");
        exit(1);
    }

    portn = atoi(argv[1]);     //porta

    my_sock = socket(AF_INET,SOCK_STREAM,0);
    memset(my_addr.sin_zero,'\0',sizeof(my_addr.sin_zero));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(portn);
    my_addr.sin_addr.s_addr = inet_addr("127.0.0.1");   //usei esse ip para teste
    client_addr_size = sizeof(client_addr);

    if(bind(my_sock,(struct sockaddr *)&my_addr,sizeof(my_addr)) != 0) {
        perror("bind nao efetuado");
        exit(1);
    }

    if(listen(my_sock,5) != 0) {
        perror("listen nao efetuado");
        exit(1);
    }


    //zera o arquivo do monitor
    FILE *archive;
    archive = fopen("server_events.txt","w");
    fclose(archive);

    //prepara o paramentro do monitor
    strcpy(mon_msg, "Servidor iniciado na porta ");
    strcat(mon_msg, argv[1]);
    strcat(mon_msg, "\n");
    strcat(mon_msg, "Aguardando novos clientes...\n\n");

    printf("%s", mon_msg);
    
    //passa para o monitor a string que ele vai printar no arquivo
    pthread_create(&tmonitor, NULL, MonitorWriteEvent, &mon_msg);

    //conecta com o cliente
    while(1) {
        if((client_sock = accept(my_sock, (struct sockaddr *)&client_addr, &client_addr_size)) < 0) {
            perror("accept nao efetuado");
            exit(1);
        }

        pthread_mutex_lock(&mutex);
        inet_ntop(AF_INET, (struct sockaddr *)&client_addr, ip, INET_ADDRSTRLEN);
        printf("Novo cliente com IP [%s] se conectou\n",ip);

        strcpy(mon_msg,"Novo cliente com IP [");
        strcat(mon_msg, ip);
        strcat(mon_msg, "] se conectou\n");

        pthread_create(&tmonitor,NULL,MonitorWriteEvent,&mon_msg);

        cl.sockno = client_sock;
        strcpy(cl.ip,ip);
        clients[n] = client_sock;
        n++;
        pthread_create(&tclient,NULL,ManageClient,&cl);

        pthread_mutex_unlock(&mutex);
        
    }

    return 0;

}