#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>


#define host_name av[1]
#define PORT_NO 12345

char ready[5];

int subst(char *str,char c1,char c2){/*文字列中の文字c1をc2に置き換える*/
    int n=0;
    
    while(*str){
        if(*str == c1){
            *str = c2;
            n++;
        }
        str++;
    }
    return n;
}

int fget_line(char *line, FILE *fp){/*標準入力から１行読み込む。成功時１、失敗時０を返す。fgetsは改行文字まで読み込んで終端文字を加えるため、substで置き換える必要がある。*/
    if (fgets(line,1025,fp) == NULL){
        return 0; 
    }
    
   subst(line, '\n', '\0');
 
   return 1; 
}

void get_line(char s[], int lim){  
    int c, i;   //getcharの受け取り用変数c、ループ用変数i  

    for (i = 0; i < lim - 1 && (c = getchar()) != '\n'; ++i)  
        s[i] = c;  
      
    s[i] = '\0';  
}  

int main(int ac, char *av[]){
    struct sockaddr_in sa;
    struct hostent *hp;

    FILE *fp;

    char receive_buffer[1024],send_buffer[1024],send_fmessage[1024];
    int cs,i,fd,send_len,send_times=0,recv_ret;

    if(ac!=2){
        fprintf(stderr,"arg error\n");
        return 1;
    }

    memset(receive_buffer,'\0',1024);
    memset(send_buffer,'\0',1024);


    hp=gethostbyname(host_name);
    if(hp==NULL){
        fprintf(stderr,"hostname error\n");
        return 1;
    }

    sa.sin_family=hp->h_addrtype;
    sa.sin_port=htons(PORT_NO);

    bzero((char*)&sa.sin_addr,sizeof(sa.sin_addr));
    memcpy((char*)&sa.sin_addr,(char*)hp->h_addr,hp->h_length);


    cs=socket(AF_INET,SOCK_STREAM,0);
    if(cs==-1){
        fprintf(stderr,"socket error\n");
        return 1;
    }

    if(connect(cs,(struct sockaddr*)&sa,sizeof(sa))){
        fprintf(stderr,"connect error\n");
        return 1;
    }


    while(1){
        memset(send_buffer,'\0',1024);
        get_line(send_buffer,1024);

        if(send_buffer[0]=='%' && send_buffer[1]=='R'){
            if((fp=fopen(&send_buffer[3],"r"))==NULL){
                fprintf(stderr,"file error.\n");continue;
            }
            
            while(fget_line(send_fmessage,fp)){
                send(cs,send_fmessage,strlen(send_fmessage)+1,0);
                recv(cs,&send_times,4,0);
            }
            fclose(fp);
        }

        if(send_buffer[0]=='%' && send_buffer[1]=='W'){
            if((fp=fopen(&send_buffer[3],"w"))==NULL){
                fprintf(stderr,"file error.\n");continue;
            }
        }

        if(send(cs,send_buffer,strlen(send_buffer)+1,0)==-1){
            fprintf(stderr,"send error\n");continue;
        }

        recv(cs,&send_times,4,0);

        for(i=0;i<send_times;i++){
            recv_ret=recv(cs,receive_buffer,1024,0);
            if(recv_ret==-1){
                fprintf(stderr,"receive error\n");
                return 1;
            }
            if(strcmp(receive_buffer,"exit")==0)
                goto LOOPEND;
            else if(send_buffer[1]=='W')
                fprintf(fp,receive_buffer);
            else
                printf("%s",receive_buffer);
            memset(receive_buffer,'\0',1024);
            send(cs,"ready",5,0);
        }

        if(send_buffer[1]=='W')
            fclose(fp);
        
    }
LOOPEND:
    close(cs);
    printf("client exited\n");
    return 0;
}
