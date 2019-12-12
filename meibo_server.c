#include<netdb.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<stdlib.h>

#define PORT_NO 12345
#define max_connection 5

void parse_line(char *line);

int send_times;
int new_s=-1;
int profile_data_nitems = 0;
char send_buffer[1024];
char ready[5];

struct date {
  int y;
  int m;
  int d;
};

struct profile{
  int id;
  char name[70];
  struct date birthday;
  char home[70];
  char *comment;
};

char *date_to_csv(char buf[],struct date *date);
void print_profile(struct profile *p);
struct profile *new_profile(struct profile *p, char *csv);


struct profile profile_data_store[10000];
struct profile profile_data_store_memo[9];
struct profile *profile_data_store_ptr[10000];

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

int split(char *str,char *ret[],char sep,int max){/*文字列を文字列中の文字sepで区切り、配列retに先頭アドレスを入れていく。*/
  int i=0;

  ret[i++] = str;

  while(*str && i < max){
    if(*str == sep){
      *str = '\0';
      ret[i++] = str + 1;
    }
    str++;
  }
  return i;
}

int get_line(char *line, FILE *fp){/*標準入力から１行読み込む。成功時１、失敗時０を返す。fgetsは改行文字まで読み込んで終端文字を加えるため、substで置き換える必要がある。*/
  if (fgets(line,1025,fp) == NULL){
    return 0; 
  }

   subst(line, '\n', '\0');
 
   return 1; 
}

struct date *new_date(struct date *d, char *str){
  char *ptr[3];
  
  if(split(str,ptr,'-',3)!=3)
    return NULL;

  d->y = atoi(ptr[0]);
  d->m = atoi(ptr[1]);
  d->d = atoi(ptr[2]);

  return d;
}

struct profile *new_profile(struct profile *p, char *csv){
  char *ptr[5]; 

  if(split(csv,ptr,',',5)!=5){
      fprintf(stderr,"input error.\n");
    return NULL;
  }

  
   profile_data_nitems++;


  p->id = atoi(ptr[0]);

  strncpy(p->name,ptr[1],69);
  p->name[69] = '\0';

  if(new_date(&p->birthday,ptr[2])==NULL)
    return NULL;

  strncpy(p->home,ptr[3],69);
  p->home[69] = '\0';

  p->comment = (char*)malloc(sizeof(char)*(strlen(ptr[4])+1));
  strcpy(p->comment,ptr[4]);

  return p;
}

char *sprint_profile_csv(struct profile *p,char *csv){
  char date[11];

  sprintf(csv,"%d,%s,%s,%s,%s\n",p->id,p->name,date_to_csv(date,&p->birthday),p->home,p->comment);
  
  return csv;
}
  

void cmd_erase(int id){
    int i,c=0;

    for(i=0;i<profile_data_nitems;i++){
   
        if(profile_data_store[i].id == id){
            while(i < profile_data_nitems){
                profile_data_store[i] = profile_data_store[i+1];
                i++;
            }
            i = 0;
            c++;
            profile_data_nitems--;
        }

        if(id == 0){
            profile_data_nitems = 0;
            c++;
        }
    }
    if(c == 0)
        fprintf(stderr,"Id : %d は見つかりません．\n",id);
    send_times=0;
    send(new_s,&send_times,4,0);
}
  
void cmd_help(){
    send_times=1;
    sprintf(send_buffer,"\n---------------------------------------------\n%%Q : 終了\n%%C : 登録件数表示\n%%P n : 先頭からn件データ表示(n>0)\nn = 0 : 全件表示\nn < 0 : 後ろからn件データ表示(正順)\n%%R file : ファイル名'file'を読み込む\n%%W file : ファイル名'file'にデータをCSV形式で書き込む\n%%F word : 検索語'word'と一致したデータを表示\nid : 登録されているIDを全件表示\nname : 登録されている学校名を全件表示\nbirth : 登録されている設立年月日を全件表示\naddr : 登録されている所在地を全件表示\ncom : 登録されている備考を全件表示\n%%S n : nで指定した項目について整列する\nn = 1 : ID\nn = 2 : 学校名\nn = 3 : 設立年月日\nn = 4 : 所在地\nn = 5 : 備考データ\n%%H : ヘルプ表示\n%%E id : 指定した'id'と一致するIDのデータ消去\n0 : 全件消去\n---------------------------------------------\n\n");

        if(send(new_s,&send_times,4,0)==-1){
            fprintf(stderr,"send error\n");
        }
        
        if(send(new_s,send_buffer,strlen(send_buffer)+1,0)==-1){
            fprintf(stderr,"send error\n");
        }
         recv(new_s,ready,5,0);
}
void cmd_quit(){
    send_times=1;
    sprintf(send_buffer,"exit");   
        if(send(new_s,&send_times,4,0)==-1){
            fprintf(stderr,"send error\n");
        }
        
        if(send(new_s,send_buffer,strlen(send_buffer)+1,0)==-1){
            fprintf(stderr,"send error\n");
        }
         
       
}

void cmd_check(){
    send_times=1;
    sprintf(send_buffer,"%d profile(s)\n", profile_data_nitems);   
        if(send(new_s,&send_times,4,0)==-1){
            fprintf(stderr,"send error\n");
        }
        
        if(send(new_s,send_buffer,strlen(send_buffer)+1,0)==-1){
            fprintf(stderr,"send error\n");
        }
       recv(new_s,ready,5,0);  
}

char *date_to_string(char buf[],struct date *date){
  sprintf(buf,"%04d/%02d/%02d",date->y,date->m,date->d);
  return buf;
}

void print_profile(struct profile *p){
  char date[11];

  sprintf(send_buffer,"Id    : %d\nName  : %s\nBirth : %s\nAddr  : %s\nCom.  : %s\n\n",p->id,p->name,date_to_string(date,&p->birthday),p->home,p->comment);
 
}

void cmd_print(int n){
  int i;

  if(profile_data_nitems == 0)
      fprintf(stderr,"登録されているデータがありません．\n");

  if(n==0)
      n=profile_data_nitems;
  send_times=n;
  send(new_s,&send_times,4,0);

  for(i=0;i<n;i++){
      print_profile(&profile_data_store[i]);
      send(new_s,send_buffer,strlen(send_buffer)+1,0); 
      memset(send_buffer,'\0',1024);
      recv(new_s,ready,5,0);
           }
          
}
          

void cmd_read(){
  send_times=1;
  send(new_s,&send_times,4,0);

  send(new_s,"read success\n",13,0);

  recv(new_s,ready,5,0);
}

char *date_to_csv(char buf[],struct date *date){
  sprintf(buf,"%04d-%02d-%02d",date->y,date->m,date->d);

  return buf;
}

void fprint_profile_csv(struct profile *p){
  char date[11];

  sprintf(send_buffer,"%d,%s,%s,%s,%s\n",p->id,p->name,date_to_csv(date,&p->birthday),p->home,p->comment);
  
}

void cmd_write(){
  int i;

  send_times=profile_data_nitems;
  send(new_s,&send_times,4,0);

  for(i=0;i < profile_data_nitems;i++){
    fprint_profile_csv(&profile_data_store[i]);
    send(new_s,send_buffer,strlen(send_buffer)+1,0);

     memset(send_buffer,'\0',1024);
     recv(new_s,ready,5,0);
    
  }

}


void cmd_find(char *word){
  int i,c=0;
  char date[11];
  struct profile *p;


  if(strcmp("id",word)==0){
      send_times=profile_data_nitems;
      send(new_s,&send_times,4,0);
    for(i=0;i<profile_data_nitems;i++){
        sprintf(send_buffer,"Id : %d\n",profile_data_store[i].id);
        send(new_s,send_buffer,strlen(send_buffer)+1,0);
        memset(send_buffer,'\0',1024);
        recv(new_s,ready,5,0);
    }
  }

  else if(strcmp("name",word)==0){
      send_times=profile_data_nitems;
      send(new_s,&send_times,4,0);
    for(i=0;i<profile_data_nitems;i++){
        sprintf(send_buffer,"Name : %s\n",profile_data_store[i].name);
        send(new_s,send_buffer,strlen(send_buffer)+1,0);
        memset(send_buffer,'\0',1024);
        recv(new_s,ready,5,0);
    }
  }
  
  else if(strcmp("birth",word)==0){
      send_times=profile_data_nitems;
      send(new_s,&send_times,4,0);
    for(i=0;i<profile_data_nitems;i++){
        sprintf(send_buffer,"Birth : %s\n",date_to_string(date,&profile_data_store[i].birthday));
        send(new_s,send_buffer,strlen(send_buffer)+1,0);
        memset(send_buffer,'\0',1024);
        recv(new_s,ready,5,0);
    }
  }

  else if(strcmp("addr",word)==0){
      send_times=profile_data_nitems;
      send(new_s,&send_times,4,0);
    for(i=0;i<profile_data_nitems;i++){
        sprintf(send_buffer,"Addr : %s\n",profile_data_store[i].home);
        send(new_s,send_buffer,strlen(send_buffer)+1,0);
        memset(send_buffer,'\0',1024);
        recv(new_s,ready,5,0);
    }
  }

  else if(strcmp("com",word)==0){
      send_times=profile_data_nitems;
      send(new_s,&send_times,4,0);
     for(i=0;i<profile_data_nitems;i++){
         sprintf(send_buffer,"Com. : %s\n",profile_data_store[i].comment);
         send(new_s,send_buffer,strlen(send_buffer)+1,0);
        memset(send_buffer,'\0',1024);
        recv(new_s,ready,5,0);
    }
  }

   else{
     for(i=0;i < profile_data_nitems;i++){
       p = &profile_data_store[i];

       if(p->id == atoi(word) ||
	  strcmp(p->name,word) == 0 ||
	  strcmp(date_to_string(date,&p->birthday),word) == 0 ||
	  strcmp(p->home,word) == 0 ||
	  strcmp(p->comment,word) == 0 
	  ){
	 c++;
       }
     }
     send_times=c;
     send(new_s,&send_times,4,0);
     
     for(i=0;i < profile_data_nitems;i++){
       p = &profile_data_store[i];

       if(p->id == atoi(word) ||
	  strcmp(p->name,word) == 0 ||
	  strcmp(date_to_string(date,&p->birthday),word) == 0 ||
	  strcmp(p->home,word) == 0 ||
	  strcmp(p->comment,word) == 0 
	  ){
	 print_profile(p);
     send(new_s,send_buffer,strlen(send_buffer)+1,0); 
           memset(send_buffer,'\0',1024);
           recv(new_s,ready,5,0);
       }
     }
     if(c == 0){
         fprintf(stderr,"%sは見つかりませんでした．\n",word);
     }
   }
}
int compare_date(struct date *d1,struct date *d2){
  if(d1->y != d2->y) return (d1->y) - (d2->y);
  else if(d1->m != d2->m) return (d1->m) - (d2->m);
  else return (d1->d) - (d2->d);
}

int compare_profile(struct profile *p1,struct profile *p2, int param){

  switch(param){
  case 1 : return (p1->id) - (p2->id);
  case 2 : return strcmp(p1->name,p2->name);
  case 3 : return compare_date(&p1->birthday,&p2->birthday);
  case 4 : return strcmp(p1->home,p2->home);
  case 5 : return strcmp(p1->comment,p2->comment);
  }
}

void swap(struct profile *p1,struct profile *p2){
  struct profile tmp;

  tmp = *p2;
  *p2 = *p1;
  *p1 = tmp;
}

void make_profile_shadow(struct profile data_store[],struct profile *shadow[],int size){
  int i;
  
  for(i=0;i<size;i++){
    shadow[i] = &data_store[i];
  }
}

void cmd_sort(int param){
  int i,j;
  if(param != 1 && param != 2 && param != 3 && param != 4 && param != 5)fprintf(stderr,"引数が正しくありません．\n");
  
  make_profile_shadow(profile_data_store,profile_data_store_ptr,10000);

  for(i=0;i<=profile_data_nitems - 1;i++){
    for(j=0;j<=profile_data_nitems - 2;j++){
      if(compare_profile(&profile_data_store[j],&profile_data_store[j+1],param) > 0){
	swap(profile_data_store_ptr[j],profile_data_store_ptr[j+1]);
      }
    }
  }
  send_times=0;
  send(new_s,&send_times,4,0);
}

void exec_command(char cmd, char *param){/*引数で示された処理を実行*/
    FILE *fp;
    size_t filesize=1024;
  switch(cmd){
  case 'Q': cmd_quit(); break;
  case 'C': cmd_check(); break;
  case 'P': cmd_print(atoi(param)); break;
  case 'R': cmd_read();break;
  case 'W': cmd_write(); break;
  case 'F': cmd_find(param); break;
  case 'S': cmd_sort(atoi(param)); break;
  case 'H': cmd_help(); break;  
  case 'E': cmd_erase(atoi(param)); break;
  default:
    fprintf(stderr,"コマンド%cは存在しません．\n",cmd);
    send_times=0;
    send(new_s,&send_times,4,0);
    break;
  }
}


void parse_line(char *line){/*入力された文字列がコマンドか新しい名簿情報か調べる*/

  if(*line == '%'){
      exec_command(line[1],&line[3]);
  }
  else{

      if(profile_data_nitems > 10000) 
          fprintf(stderr,"これ以上データを登録できません．\n");
   
      new_profile(&profile_data_store[profile_data_nitems],line);
      send_times=0;
      send(new_s,&send_times,4,0);
  }
}



int main(void){
    int ss=-1,size,num_receive;
    struct sockaddr_in sa;
    char receive_buffer[1024];

    memset(receive_buffer,'\0',1024);
    memset(send_buffer,'\0',1024);


    memset((char*)&sa,0,sizeof(sa));
    sa.sin_family=AF_INET;
    sa.sin_addr.s_addr=htonl(INADDR_ANY);
    sa.sin_port=htons(PORT_NO);

    ss=socket(AF_INET,SOCK_STREAM,0);
    if(ss==-1){
        fprintf(stderr,"socket error\n");
        return 1;
    }
    
     if( bind(ss,(struct sockaddr*)&sa,sizeof(sa)) ){
        fprintf(stderr,"bind error\n");
        return 1;
        }
    
    if(listen(ss,max_connection)){
        fprintf(stderr,"listen error\n");
        return 1;
    }

    while(1){
    size=sizeof(sa);
    new_s=accept(ss,(struct sockaddr*)&sa,(socklen_t*)&size);
    if(new_s==-1){
        fprintf(stderr,"accept error\n");
        return 1;
    }

    
    while(1){
        num_receive=recv(new_s,receive_buffer,1024,0);
        if(num_receive==-1||num_receive==0){
            fprintf(stderr,"receive error\n");
            return 1;
        }

        parse_line(receive_buffer);
        if(strcmp(send_buffer,"exit")==0)
            break;
        memset(receive_buffer,'\0',1024);
        memset(send_buffer,'\0',1024);
    }
    close(new_s);
    }
    printf("server exited\n");
    close(ss);
    return 0;
}




