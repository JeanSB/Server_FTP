/*
30/06/2018

Trabalho Pratico de Programacao em Redes:
Servidor FTP para Linux

Discentes:
Flavia Morales Antunes
Jean da Silva Bragamonte

Disciplina:
Redes de Computadores

Professor:
Leonardo Bidese Pinho

*/
#include <arpa/inet.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
#include <fstream>
#include <signal.h>

using namespace std;
int numcliente = 0;
int create_socket(int);

void* Servidor(void* arg);
int capacidade = 0;
int ocupa = 0;

#ifdef WINDOWS
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif
#define MAXLINE 4096 /*Tamanho ḿáximo*/
#define LISTENQ 10 /*Número máximo de conexoes*/




struct clientestruct
{
    int taxa;
    char ip[20];

};

typedef struct clientestruct client;




int main (int argc, char **argv)
{
    int sockfd;
    struct sockaddr_in servaddr;


    if (argc !=3)  						//Validando os argumentos de entrada
    {
        cerr<<"Use: ./serverfile <numero da porta> <capacidade do servidor em bytes>"<<endl;
        exit(1);
    }

    capacidade = atoi(argv[2]);
    //cout<<capacidade<<endl;

    /*Create a socket for the soclet
    If sockfd<0 there was an error in the creation of the socket

       A socket()chamada do sistema cria um novo soquete. Leva três argumentos.

       O primeiro é o domínio do endereço do soquete. Lembre-se de que existem dois domínios de endereço possíveis, o domínio unix para dois processos que compartilham um sistema de arquivos comum e o domínio da Internet para quaisquer dois hosts na Internet.
       A constante de símbolo AF_UNIXé usada para a primeira e AF_INET para a segunda (na verdade, existem muitas outras opções que podem ser usadas aqui para propósitos especializados).

       O segundo argumento é o tipo de soquete. Lembre-se de que há duas opções aqui, um soquete de fluxo no qual os caracteres são lidos em um fluxo contínuo como se fosse um arquivo ou um pipe e um soquete de datagrama, no qual as mensagens são lidas em blocos.
       As duas constantes simbólicas são SOCK_STREAMe SOCK_DGRAM.

       O terceiro argumento é o protocolo. Se esse argumento for zero (e sempre deve ser, exceto por circunstâncias incomuns), o sistema operacional escolherá o protocolo mais apropriado. Ele escolherá o TCP para soquetes de fluxo e o UDP para soquetes de datagrama.

       A chamada do sistema de soquete retorna uma entrada na tabela do descritor de arquivo (ou seja, um inteiro pequeno). Esse valor é usado para todas as referências subsequentes a esse soquete. Se a chamada do soquete falhar, retornará -1.
       Neste caso, o programa exibe e mensagem de erro e sai. No entanto, é pouco provável que esta chamada de sistema falhe.
       */


    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0)
    {
        cerr<<"Problema na criação do socket!"<<endl;
        exit(2);
    }


    //Preparação do endereço do socket
    //A variável serv_addr é uma estrutura do tipo struct sockaddr_in. Essa estrutura possui quatro campos.

    /*struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
        };
    */

    memset(&servaddr, 0, sizeof (servaddr));//Zera a estrutura

    /*O primeiro campo é short sin_family, que contém um código para a família de endereços.
    Deve sempre ser definido para a constante simbólica AF_INET.*/
    servaddr.sin_family = AF_INET;

    /*Este campo contém o endereço IP do host. Para o código do servidor,
    este será sempre o endereço IP da máquina na qual o servidor está sendo executado,
    e há uma constante simbólica INADDR_ANYque obtém esse endereço.*/
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int port;
    //if(atoi(argv[1])<=1 && atoi(argv[1])<=1024) //Verifica se a porta digitada eh maior que 1024
    if(atoi(argv[1])>=1024) //Verifica se a porta digitada eh maior que 1024
    {
        //cerr<<"Port number must be greater than 1024"<<endl;
        //exit(2);
        port = atoi(argv[1]);
    }
    else
        port = 1024;

    /*O segundo campo de serv_addr eh unsigned short sin_port ,
    que contém o número da porta. No entanto, em vez de simplesmente copiar o número da porta para esse campo,
    é necessário convertê-lo em ordem de bytes de rede usando a função htons()que
    converte um número de porta em ordem de bytes host em um número de porta em ordem de bytes de rede.*/
    servaddr.sin_port = htons(port);

//bind the socket
    /*A bind()chamada do sistema liga um soquete a um endereço, neste caso, o endereço do host atual e o número da porta
    em que o servidor será executado. São necessários três argumentos, o descritor do arquivo de soquete, o endereço ao qual
    ele está vinculado e o tamanho do endereço ao qual ele está vinculado. O segundo argumento é um ponteiro para uma estrutura
    do tipo sockaddr, mas o que é passado é uma estrutura do tipo sockaddr_ine, portanto, isso deve ser convertido para o tipo correto.
    Isso pode falhar por vários motivos, sendo o mais óbvio que esse soquete já está em uso nesta máquina.*/
    if(bind (sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        perror("Problema na criação da Bind!");
        exit(1);
    }


//listen to the socket by creating a connection queue, then wait for clients
    /*A listen chamada do sistema permite que o processo escute no soquete para conexões.
    O primeiro argumento é o descritor de arquivo de soquete e o segundo é o tamanho da fila de pendências, ou seja,
    o número de conexões que podem estar aguardando enquanto o processo está manipulando uma determinada conexão.
    Isso deve ser definido como 5, o tamanho máximo permitido pela maioria dos sistemas.
    Se o primeiro argumento for um soquete válido, essa chamada não poderá falhar e, portanto, o código não verificará erros.*/
    if(listen(sockfd, LISTENQ)<0)
    {
        perror("Problema na criação do Listen!");
        exit(1);
    }

    cout<<"Servidor em execução...aguardando novas conexões."<<endl;

    while(1)
    {
        //cout<<"\nLine: "<<__LINE__<<"\n";
        int clientsockfd, len;
        struct sockaddr_in cliaddr;
        unsigned int clilen;
        clilen = sizeof(cliaddr);
        pthread_t thread[5];
        int comando;
        char controle[100];
        char buffer[MAXLINE];

        /*A accept()chamada do sistema faz com que o processo seja bloqueado até que um cliente se conecte ao servidor.
        Assim, ele acorda o processo quando uma conexão de um cliente foi estabelecida com sucesso.
        Ele retorna um novo descritor de arquivo e toda a comunicação nessa conexão deve ser feita usando o novo descritor de arquivo.
        O segundo argumento é um ponteiro de referência para o endereço do cliente na outra extremidade da conexão,
        e o terceiro argumento é o tamanho dessa estrutura.*/

        if((clientsockfd = accept (sockfd, (struct sockaddr *) &cliaddr, &clilen)) < 0)
        {
            perror("Problema em aceitar conexão!");
            exit(1);
        }



        strcpy(controle, "220 Service ready for new user.\n");
        write(clientsockfd, controle, sizeof(controle));
        read(clientsockfd, buffer, sizeof(buffer));
        cout<<buffer<<endl;
        strcpy(controle, "230 User logged in, proceed.\n");
        write(clientsockfd, controle, sizeof(controle));
        read(clientsockfd, buffer, sizeof(buffer));


        //cout<<"\nclientsockfd: "<<clientsockfd<<"\n";
        cout<<"Recebendo conexão..."<<endl;

        cout<<"\nNumero de clientes: -> "<<numcliente+1<<endl;


        //Inicializa a thread
        if (pthread_create(&(thread[numcliente]), NULL, Servidor, &clientsockfd) != 0)   //if it’s 0, it’s child process
        {
            perror("Problema na criação da thread!");
            exit(1);
        }
        cout<<"\nThread: -> "<<thread[numcliente]<<endl;
        pthread_detach(thread[numcliente]);
        numcliente++;

        /*strcpy(controle, "120 Service ready in 0.05 minutes");
        write(clientsockfd, controle, sizeof(controle));
        cout<<"\nServidor congestionado"<<endl;
        sleep(5);*/

    }


}





//FUNÇOES

int create_socket(int port)
{
    int sockfd;
    struct sockaddr_in servaddr;


    /*Create a socket for the soclet
    If sockfd<0 there was an error in the creation of the socket

       A socket()chamada do sistema cria um novo soquete. Leva três argumentos.

       O primeiro é o domínio do endereço do soquete. Lembre-se de que existem dois domínios de endereço possíveis, o domínio unix para dois processos que compartilham um sistema de arquivos comum e o domínio da Internet para quaisquer dois hosts na Internet.
       A constante de símbolo AF_UNIXé usada para a primeira e AF_INET para a segunda (na verdade, existem muitas outras opções que podem ser usadas aqui para propósitos especializados).

       O segundo argumento é o tipo de soquete. Lembre-se de que há duas opções aqui, um soquete de fluxo no qual os caracteres são lidos em um fluxo contínuo como se fosse um arquivo ou um pipe e um soquete de datagrama, no qual as mensagens são lidas em blocos.
       As duas constantes simbólicas são SOCK_STREAMe SOCK_DGRAM.

       O terceiro argumento é o protocolo. Se esse argumento for zero (e sempre deve ser, exceto por circunstâncias incomuns), o sistema operacional escolherá o protocolo mais apropriado. Ele escolherá o TCP para soquetes de fluxo e o UDP para soquetes de datagrama.

       A chamada do sistema de soquete retorna uma entrada na tabela do descritor de arquivo (ou seja, um inteiro pequeno). Esse valor é usado para todas as referências subsequentes a esse soquete. Se a chamada do soquete falhar, retornará -1.
       Neste caso, o programa exibe e mensagem de erro e sai. No entanto, é pouco provável que esta chamada de sistema falhe.
       */


    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0)
    {
        cerr<<"Problema na criação de socket!"<<endl;
        exit(2);
    }


    //Preparação do endereço do socket
    //A variável serv_addr é uma estrutura do tipo struct sockaddr_in. Essa estrutura possui quatro campos.

    /*struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
        };
    */

    memset(&servaddr, 0, sizeof (servaddr));//Zera a estrutura

    /*O primeiro campo é short sin_family, que contém um código para a família de endereços.
    Deve sempre ser definido para a constante simbólica AF_INET.*/
    servaddr.sin_family = AF_INET;

    /*Este campo contém o endereço IP do host. Para o código do servidor,
    este será sempre o endereço IP da máquina na qual o servidor está sendo executado,
    e há uma constante simbólica INADDR_ANYque obtém esse endereço.*/
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);


    /*O segundo campo de serv_addr eh unsigned short sin_port ,
    que contém o número da porta. No entanto, em vez de simplesmente copiar o número da porta para esse campo,
    é necessário convertê-lo em ordem de bytes de rede usando a função htons()que
    converte um número de porta em ordem de bytes host em um número de porta em ordem de bytes de rede.*/
    servaddr.sin_port = htons(port);

//bind the socket
    /*A bind()chamada do sistema liga um soquete a um endereço, neste caso, o endereço do host atual e o número da porta
    em que o servidor será executado. São necessários três argumentos, o descritor do arquivo de soquete, o endereço ao qual
    ele está vinculado e o tamanho do endereço ao qual ele está vinculado. O segundo argumento é um ponteiro para uma estrutura
    do tipo sockaddr, mas o que é passado é uma estrutura do tipo sockaddr_ine, portanto, isso deve ser convertido para o tipo correto.
    Isso pode falhar por vários motivos, sendo o mais óbvio que esse soquete já está em uso nesta máquina.*/
    if(bind (sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    {
        perror("Problema na criação da Bind!");
        exit(1);
    }


//listen to the socket by creating a connection queue, then wait for clients
    /*A listen chamada do sistema permite que o processo escute no soquete para conexões.
    O primeiro argumento é o descritor de arquivo de soquete e o segundo é o tamanho da fila de pendências, ou seja,
    o número de conexões que podem estar aguardando enquanto o processo está manipulando uma determinada conexão.
    Isso deve ser definido como 5, o tamanho máximo permitido pela maioria dos sistemas.
    Se o primeiro argumento for um soquete válido, essa chamada não poderá falhar e, portanto, o código não verificará erros.*/
    if(listen(sockfd, LISTENQ)<0)
    {
        perror("Problema na criação do Listen!");
        exit(1);
    }

    return(sockfd);
}



void* Servidor(void* arg)
{
    char  buf[100], controle[100], command[5];
    int sockEntrada = *(int *) arg;
    int  data_port, n=1;						//for data connection
    struct sockaddr_in clienteaddr;
    int socketdados, sockcli;
    unsigned int tamanho;
    bool pasv = false;
    tamanho = sizeof(clienteaddr);
    char addrclient[MAXLINE];
    int portclient;
    char *token,*dummy;
    struct stat obj;
    client *cliente;
    cliente = (client*) malloc(sizeof(client));
    int comando;





    while(1)
    {

        memset(&buf, 0, sizeof(buf));
        memset(&token, 0, sizeof(token));
        memset(&dummy, 0, sizeof(dummy));
        memset(&controle, 0, sizeof(controle));
        int op;

        read(sockEntrada, buf, sizeof(buf));
        sscanf(buf, "%s", command);


        cout<<"String recebida do cliente: "<<buf<<endl;



        dummy=buf;
        token=strtok(dummy," ");




        if (strcmp("SYST",command)==0)
        {
            strcpy(controle, "215 UNIX Type.\n");
            write(sockEntrada, controle, sizeof(controle));
        }



        else if (strcmp("PORT",command)==0)
        {
            int i;
            char ip[20]="", aux3[20], aux2[20];
            int aux;
            FILE *arq;
            pasv = false;
            for(i=0; i<6; i++)
            {
                if(i<3)
                {
                    token = strtok(NULL,",");
                    strcat(ip, token);
                    strcat(ip, ".");
                }
                else if(i==3)
                {
                    token = strtok(NULL,",");
                    strcat(ip, token);
                }
                else if(i==4)
                {
                    token = strtok(NULL,",");
                    portclient = atoi(token);
                    portclient = portclient*256;
                }

                else
                {
                    token = strtok(NULL,"\n");
                    aux = atoi(token);
                    portclient = portclient+aux;
                }
            }

            strcpy(cliente->ip, ip);



            if(!(arq = fopen("taxa.txt", "r")))
            {
                cout<<"error"<<endl;
            }

            char linha[32];
            sscanf(cliente->ip, "%s", aux2);
            while(fgets(linha, sizeof(linha), arq)!=NULL)
            {
                sscanf(linha, "%s", aux3);
                if((strcmp(aux3, aux2))==0)
                {
                    cout<<"\n\n"<<aux3<<"    "<<aux2<<endl;
                    fgets(linha, sizeof(linha), arq);
                    cliente->taxa = atoi(linha);
                }
            }
            pclose(arq);



            cout<<"IP= "<<cliente->ip<<"\nTaxa do cliente= "<<cliente->taxa<<endl;
            cout<<"IP= "<<ip<<"\nPorta= "<<portclient<<endl;


            strcpy(controle, "200 Command okay.\n");
            write(sockEntrada, controle, sizeof(controle));

            if ((socketdados = socket (AF_INET, SOCK_STREAM, 0)) <0)
            {
                cerr<<"Problema na criação do socket!"<<endl;
                exit(2);
            }


            memset(&clienteaddr, 0, sizeof(clienteaddr)); //zera a estrutura

            clienteaddr.sin_family = AF_INET;
            clienteaddr.sin_addr.s_addr= inet_addr(ip);
            clienteaddr.sin_port =  htons(portclient); //converte em binário




        }




        else if (strcmp("PASV",command)==0)
        {
            /*int i;
            char ip[20]="";
            int aux;
            pasv = true;


            portclient = 2000;

            socketdados = create_socket(portclient);

            if((sockcli = accept (socketdados, (struct sockaddr *) &clienteaddr, &tamanho)) < 0)
            {
            perror("Problema em aceitar conexão!");
            exit(1);
            }
            else
            {
            cout<<"\nConexão aceita!"<<endl;
            }





            for(i=0; i<6; i++)
            {
                if(i<3)
                {
                    token = strtok(NULL,",");
                    strcat(ip, token);
                    strcat(ip, ".");
                }
                else if(i==3)
                {
                    token = strtok(NULL,",");
                    strcat(ip, token);
                }
                else if(i==4)
                {
                    token = strtok(NULL,",");
                    portclient = atoi(token);
                    portclient = portclient*256;
                }

                else
                {
                    token = strtok(NULL,"\n");
                    aux = atoi(token);
                    portclient = portclient+aux;
                }




            }



            cout<<"IP= "<<ip<<"\nPorta= "<<portclient<<endl;


            strcpy(controle, "227 Entering Passive Mode (192,168,150,90,195,149)..\n");
            write(sockEntrada, controle, sizeof(controle));

            if ((socketdados = socket (AF_INET, SOCK_STREAM, 0)) <0)
            {
                cerr<<"Problema na criação do socket!"<<endl;
                exit(2);
            }


            memset(&clienteaddr, 0, sizeof(clienteaddr)); //zera a estrutura

            clienteaddr.sin_family = AF_INET;
            clienteaddr.sin_addr.s_addr= inet_addr(ip);
            clienteaddr.sin_port =  htons(portclient); //converte em binário
            */
            strcpy(controle, "502 Command not implemented.\n");
            write(sockEntrada, controle, sizeof(controle));


        }





        else if (strcmp("LIST",command)==0)
        {


            strcpy(controle, "150 about to open data connection\n");
            write(sockEntrada, controle, sizeof(controle));



            if (pasv ==false)
            {
                if (connect(socketdados, (struct sockaddr *) &clienteaddr, sizeof(clienteaddr))<0)
                {
                    cerr<<"Problema na conexão com o servidor!"<<endl;
                    numcliente--;
                    close(sockEntrada);
                    pthread_exit((void*) 0);
                }
                else
                {
                    cout<<"\nConexão aceita!"<<endl;
                }

            }
            else
            {



            }


            system("ls -l>list.txt");

            FILE *in;

            if(!(in = fopen("list.txt", "r")))
            {
                cout<<"error"<<endl;
            }

            int endd;
            char arquivos[32];
            while(fgets(arquivos, sizeof(arquivos), in)!=NULL)
            {
                endd = strlen(arquivos);
                write(socketdados, arquivos, endd);
                memset(&arquivos, 0, sizeof(arquivos));

            }
            close(socketdados);
            strcpy(controle, "226 Closing data connection. Requested file action successful.\n");
            write(sockEntrada, controle, sizeof(controle));
            pclose(in);
        }




        else if (strcmp("RETR",token)==0)
        {
            char port[MAXLINE], *tokk, tokk2, buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE];
            int lSize,num_blks,num_last_blk,i;
            FILE *fp;
            char filename [20];
            long int sizee, taxa=0;
            int tam;

            strcpy(controle, "150 about to open data connection.\n");
            write(sockEntrada, controle, sizeof(controle));




            if (connect(socketdados, (struct sockaddr *) &clienteaddr, sizeof(clienteaddr))<0)
            {
                cerr<<"Problema na conexão com o servidor!"<<endl;
                numcliente--;
                close(sockEntrada);
                pthread_exit((void*) 0);
            }
            else
            {
                cout<<"\nConexão aceita!"<<endl;
            }



            strcpy(filename, strtok(NULL,"."));
            strcat(filename,".txt");

            cout<<filename<<endl;


            if ((fp=fopen(filename,"r"))!=NULL)
            {
                //size of file
                fseek (fp, 0, SEEK_END);
                sizee = ftell (fp);
                rewind (fp);
                num_blks = sizee/MAXLINE;
                num_last_blk = sizee%MAXLINE;
                sprintf(char_num_blks,"%d",num_blks);

                while(fgets(buf, sizeof(buf), fp)!=NULL)
                {
                    tam = strlen(buf);
                    taxa+=tam;
                    if(taxa>=cliente->taxa)
                    {
                        taxa = 0;
                        sleep(1);
                    }
                    write(socketdados, buf, tam);
                    memset(&buf, 0, sizeof(buf));



                }
                close(socketdados);
                strcpy(controle, "226 Closing data connection. Requested file action successful.\n");
                write(sockEntrada, controle, sizeof(controle));
                pclose(fp);

            }
            else
            {
                strcpy(controle, "450 File unavailable.\n");
                write(sockEntrada, controle, sizeof(controle));
                close(socketdados);
            }


        }




        //PRINT WORKING DIRECTORY
        else if (strcmp("PWD",command)==0)
        {
            char curr_dir[1024], cod[1024];
            strcpy(curr_dir, "257 \"");
            int endd;
            GetCurrentDir(cod,sizeof(cod));
            strcat(curr_dir, cod);
            strcat(curr_dir, "\" is the current directory\n");
            endd = strlen(curr_dir);
            write(sockEntrada,curr_dir,endd);
        }




        //CHANGE WORKING DIRECTORY
        else if (strcmp("CWD",token)==0||strcmp("cwd",token)==0)
        {

            //token = strtok(NULL," \n");
            memset(&buf,0,sizeof(buf));
            recv(sockEntrada,buf,sizeof(buf),MSG_WAITALL);
            fflush(stdin);
            //cout<<"\nLine: "<<__LINE__<<"\n";

            //cout<<"Path given is: "<<buf<<endl;

            //cout<<"\nLine: "<<__LINE__<<"\n";

            if(chdir(buf)<0)
            {
                send(socketdados,"0",MAXLINE,0);
                //write(socketdados, "0", sizeof(buf));
            }
            else
            {

                send(socketdados,"1",MAXLINE,0);
                //write(socketdados, "1", sizeof(buf));
            }

        }




        //CHANGE WORKING DIRECTORY
        else if (strcmp("DELE",token)==0||strcmp("dele",token)==0)
        {
            //token = strtok(NULL," \n");
            memset(&buf,0,sizeof(buf));
            fflush(stdin);
            recv(socketdados,buf,sizeof(buf),MSG_WAITALL);
            //cout<<"\nLine: "<<__LINE__<<"\n";

            //cout<<"Path given is: "<<buf<<endl;

            //cout<<"\nLine: "<<__LINE__<<"\n";
            char teste[20] = "rm -f ";
            strcat(teste, buf);
            if(system(teste)<0)
            {
                send(socketdados,"0",MAXLINE,0);
                //write(socketdados, "0", sizeof(buf));
            }
            else
            {

                send(socketdados,"1",MAXLINE,0);
                //write(socketdados, "1", sizeof(buf));
            }

        }



        else if (strcmp("MKD",token)==0||strcmp("mkd",token)==0)
        {
            //token = strtok(NULL," \n");
            memset(&buf,0,sizeof(buf));
            fflush(stdin);
            recv(socketdados,buf,sizeof(buf),MSG_WAITALL);
            //cout<<"\nLine: "<<__LINE__<<"\n";

            //cout<<"Path given is: "<<buf<<endl;

            //cout<<"\nLine: "<<__LINE__<<"\n";
            char teste[20] = "mkdir ";
            strcat(teste, buf);
            if(system(teste)<0)
            {
                send(socketdados,"0",MAXLINE,0);
                //write(socketdados, "0", sizeof(buf));
            }
            else
            {

                send(socketdados,"1",MAXLINE,0);
                //write(socketdados, "1", sizeof(buf));
            }

        }


        else if (strcmp("RMD",token)==0||strcmp("rmd",token)==0)
        {
            //token = strtok(NULL," \n");
            memset(&buf,0,sizeof(buf));
            fflush(stdin);
            recv(socketdados,buf,sizeof(buf),MSG_WAITALL);
            //cout<<"\nLine: "<<__LINE__<<"\n";

            //cout<<"Path given is: "<<buf<<endl;

            //cout<<"\nLine: "<<__LINE__<<"\n";
            char teste[20] = "rmdir ";
            strcat(teste, buf);
            if(system(teste)<0)
            {
                send(socketdados,"0",MAXLINE,0);
                //write(socketdados, "0", sizeof(buf));
            }
            else
            {

                send(socketdados,"1",MAXLINE,0);
                //write(socketdados, "1", sizeof(buf));
            }

        }


        else if (strcmp("SIZE",token)==0||strcmp("size",token)==0)
        {
            char port[MAXLINE],buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE];
            int lSize,num_blks,num_last_blk,i;
            FILE *fp;
            long tamanho;

            memset(&buf,0,sizeof(buf));
            fflush(stdin);

            recv(socketdados,buf,sizeof(buf),MSG_WAITALL);

            //cout<<"\nFilename given is: "<<token<<endl;
            //accepting connnection by client
            if ((fp=fopen(buf,"r"))!=NULL)
            {
                //size of file
                if (fp != NULL)
                {
                    // movimenta a posição corrente de leitura no arquivo
                    // para o seu fim
                    fseek(fp, 0, SEEK_END);

                    // pega a posição corrente de leitura no arquivo
                    tamanho = ftell(fp);

                    sprintf(buf, "%ld", tamanho);

                    //strcpy(buf,tamanho);

                    // imprime o tamanho do arquivo
                    send(socketdados, buf,sizeof(buf),0);

                }
                fclose(fp);
                //cout<<"\nFile upload done."<<endl;

            }
            else
            {
                send(socketdados,"0",MAXLINE,0);
            }
        }




        else if (strcmp("PUT",token)==0 || strcmp("put",token)==0)
        {
            char port[MAXLINE],buffer[MAXLINE],char_num_blks[MAXLINE],char_num_last_blk[MAXLINE],check[MAXLINE];
            int datasock,num_blks,num_last_blk,i;
            FILE *fp;

            memset(&buf,0,sizeof(buf));
            fflush(stdin);

            recv(socketdados,buf,sizeof(buf),MSG_WAITALL);

            //cout<<"Filename given is: "<<token<<endl;
            recv(socketdados,check,MAXLINE,0);
            //cout<<check;
            if(strcmp("1",check)==0)
            {
                if((fp=fopen(buf,"w"))==NULL)
                    cout<<"Erro na criação do arquivo!\n";
                else
                {
                    recv(socketdados, char_num_blks, MAXLINE,0);
                    num_blks=atoi(char_num_blks);
                    for(i= 0; i < num_blks; i++)
                    {
                        recv(socketdados, buffer, MAXLINE,0);
                        fwrite(buffer,sizeof(char),MAXLINE,fp);
                        //cout<<buffer<<endl;
                    }
                    recv(socketdados, char_num_last_blk, MAXLINE,0);
                    num_last_blk=atoi(char_num_last_blk);
                    if (num_last_blk > 0)
                    {
                        recv(socketdados, buffer, MAXLINE,0);
                        fwrite(buffer,sizeof(char),num_last_blk,fp);
                        //cout<<buffer<<endl;
                    }
                    fclose(fp);
                    cout<<"\nDownload do arquivo concluido!."<<endl;
                }
            }
        }




        else if(strcmp("QUIT",command)==0)
        {

            strcpy(controle, "221 service closing control data connection.\n");
            write(sockEntrada, controle, sizeof(controle));

            numcliente--;
            //if (n < 0)
            //    cout<<"Read error"<<endl;

            //cout<<"\nLine: "<<__LINE__<<"\n";

            close(sockEntrada);
            //close(socketdados);
            pthread_exit((void*) 0);



        }


        else
        {
            sleep(1);
        }





    }//End while


    numcliente--;
    //if (n < 0)
    //    cout<<"Read error"<<endl;

    //cout<<"\nLine: "<<__LINE__<<"\n";

    close(sockEntrada);
    //close(socketdados);
    pthread_exit((void*) 0);


    //close socket of the server

}

