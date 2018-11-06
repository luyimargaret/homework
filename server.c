//服务器端程序server.c：
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define MAXDATASIZE 256
#define SERVPORT 3333	/*服务器监听端口号*/
#define BACKLOG 1     /*最大同时连接请求数*/
#define STDIN 0        /*标准输入文件描述符*/
#define CHAT 0	/*聊天模式*/
#define HANDSHAKE_CONFERM 1	    /*应答模式_确认*/
#define HANDSHAKE_FILENAME 2	/*应答模式_文件名*/
#define HANDSHAKE_LENGTH 3	    /*应答模式_文件长度*/
#define TRANSPORT_SEND 4		/*发送模式*/
#define TRANSPORT_RECIEVE 5	    /*接收模式*/

long filelen(FILE *fp)
{
	long original_seek=ftell(fp),start_seek,end_seek;
	fseek(fp,0,SEEK_SET);
	start_seek=ftell(fp);
	fseek(fp,0,SEEK_END);
	end_seek=ftell(fp);
	fseek(fp,original_seek,SEEK_SET);
	return end_seek-start_seek;
}

char *ltoa(char *str,long num)
{
	long t=num;
	int i=0;
	while(t)
	{
		i++;
		t/=10;
	}
	*(str+i)='\0';
	i--;
	while(i)
	{
		*(str+i)=num%10+'0';
		num/=10;
		i--;
	}
	*str=num%10+'0';
	return str;
} 
	
int main(void)
{
	FILE *fp;		/*定义文件类型指针fp*/
	int sockfd,client_fd;	/*监听socket.sock_fd,数据传输socket.new_fd*/
	int sin_size;
	struct sockaddr_in my_addr, remote_addr;/*本机地址信息,客户地址信息*/		char buf[256];		/*用于聊天的缓冲区*/
	char buff[256];	/*用于输入用户名的缓冲区*/
	char send_str[256];		/*最多发出的字符不能超过256*/
	int recvbytes;			
	fd_set rfd_set, wfd_set, efd_set;
/*被select()监视的读,写,异常处理的文件描述符集合*/
	struct timeval timeout;		/*本次select的超时结束时间*/
	int ret;			/*与client连接的结果*/
	time_t timep;
	int mode;
	char transport_filename[256];
	long filelength,p;
	char a_filelength[256];
	FILE *fp_transport;
	char c[2];
	

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{ 	/*错误检测*/
		perror("socket");
		exit(1);
	}
	/* 端填充 sockaddr结构  */
	bzero(&my_addr, sizeof(struct sockaddr_in));
	my_addr.sin_family=AF_INET;	/*地址族*/
	my_addr.sin_port=htons(SERVPORT);	/*端口号为4444*/
	inet_aton("127.0.0.1", &my_addr.sin_addr);
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1)
	{ 			/*错误检测*/
		perror("bind");
		exit(1);
	}
	if (listen(sockfd, BACKLOG) == -1)
	{	 /*错误检测*/
		perror("listen");
		exit(1);
	}

	sin_size = sizeof(struct sockaddr_in);
	if ((client_fd = accept(sockfd, (struct sockaddr *)&remote_addr, &sin_size)) == -1)
	{
 /*错误检测*/
		perror("accept");
		exit(1);
	}

	fcntl(client_fd, F_SETFD, O_NONBLOCK);/* 服务器设为非阻塞*/
	recvbytes=recv(client_fd, buff, MAXDATASIZE, 0);
/*接收从客户端传来的用户名*/	
	buff[recvbytes] = '\0';

	fflush(stdout);
	/*缓冲区刷新*/
	if((fp=fopen("record_server.txt","a+"))==NULL)
	{
		printf("can not open file,exit...\n");
		return -1;
	}
	
	mode=CHAT;
	while (1)
	{
		FD_ZERO(&rfd_set);/*将select()监视的读的文件描述符集合清除*/
		FD_ZERO(&wfd_set);/*将select()监视的写的文件描述符集合清除*/
		FD_ZERO(&efd_set);/*将select()监视的异常的文件描述符集合清除*/
		FD_SET(STDIN, &rfd_set);
/*将标准输入文件描述符加到seletct()监视的读的文件描述符集合中*/
		FD_SET(client_fd, &rfd_set);
/*将新建的描述符加到seletct()监视的读的文件描述符集合中*/
		FD_SET(client_fd, &wfd_set);
/*将新建的描述符加到seletct()监视的写的文件描述符集合中*/
		FD_SET(client_fd, &efd_set);
/*将新建的描述符加到seletct()监视的异常的文件描述符集合中*/
		timeout.tv_sec = 10;/*select在被监视窗口等待的秒数*/
		timeout.tv_usec = 0;/*select在被监视窗口等待的微秒数*/
		ret = select(client_fd + 1, &rfd_set, &wfd_set, &efd_set, &timeout);
		if (ret == 0) 
		{
			continue;
		}
		
		if (ret < 0)
		{
			perror("select error: ");
			fclose(fp);
			exit(-1);
		}
/*判断是否已将标准输入文件描述符加到seletct()监视的读的文件描述符集合中*/
		if (mode==TRANSPORT_SEND)
		{
			for (p=0;p<filelength;p++)
			{
				c[0]=fgetc(fp_transport);
				c[1]='\0';
				send(client_fd, c, strlen(c), 0);
			}
			fclose(fp_transport);
			mode=CHAT;
			printf("Transporting completed.\n");
		}
		if (mode==TRANSPORT_RECIEVE)
		{
			for(p=0;p<filelength;p++)
			{
				recvbytes=recv(client_fd, buf, MAXDATASIZE, 0);
				buf[recvbytes]='\0';
				fputc(buf[0],fp_transport);
			}
			fclose(fp_transport);
			mode=CHAT;
			printf("Transporting completed.\n");
		}
		if(FD_ISSET(STDIN, &rfd_set))
		{
			if (mode==CHAT)
			{
				fgets(send_str, 256, stdin);/*取从输入输入的内容*/
				send_str[strlen(send_str)-1] = '\0';
				if (strncmp("quit", send_str, 4) == 0)
				{	/*退出程序*/
					close(client_fd);
					close(sockfd);	/*关闭套接字*/
					fclose(fp);
					exit(0);
				}
				if (strncmp("#FILE", send_str, 5) == 0)
				{
					mode=HANDSHAKE_CONFERM;
					send(client_fd, send_str, strlen(send_str), 0);
				}	
				else
				{		
					send(client_fd, send_str, strlen(send_str), 0);
					time(&timep);
					fprintf(fp,"%s",asctime(gmtime(&timep)));
					fprintf(fp,"Server: %s\n", send_str);
				}
			}
		}
/*判断是否已将新建的描述符加到seletct()监视的读的文件描述符集合中*/
		if (FD_ISSET(client_fd, &rfd_set))
	 	{
			recvbytes=recv(client_fd, buf, MAXDATASIZE, 0);/*接收从客户端传来的聊天内容*/
			buf[recvbytes] = '\0';
			if (recvbytes == 0) {
				close(client_fd);
				close(sockfd);	/*关闭套接字*/
				fclose(fp);
				exit(0);
			}
			if (mode==CHAT)
			{
				if(strncmp("#FILE", buf, 5) != 0)
				{
					time(&timep);
					fprintf(fp,"%s",asctime(gmtime(&timep)));
					fprintf(fp,"%s: %s\n",buff,buf);
					printf("%s:%s\n",buff,buf);
					printf("Server: ");
					fflush(stdout);
				}
				else
				{
					printf("%s wants to send a file to you, recieve or not?(y/n)",buff);
					scanf("%c",c);
					c[1]='\0';
					send(client_fd, c, strlen(c), 0);
					if ((c[0]=='y') || (c[0]=='Y'))
					{
						mode=HANDSHAKE_FILENAME;
					}
				}
			}
			else if (mode==HANDSHAKE_CONFERM)
			{
				if(strncmp("y", buf, 1)==0 || strncmp("Y", buf, 1)==0)
				{
					printf("%s has accepted.\n",buff);
					printf("Input file name: ");
					scanf("%s",transport_filename);
					getchar();
					if ((fp_transport=fopen(transport_filename,"rb"))==NULL)
					{
						printf("Open file error!");
						close(client_fd);
						close(sockfd);	/*关闭套接字*/
						fclose(fp);
						exit(-1);
					}
					filelength=filelen(fp_transport);
					ltoa(a_filelength,filelength);
					send(client_fd, transport_filename, strlen(transport_filename), 0);
					send(client_fd, a_filelength, strlen(a_filelength), 0);
					printf("File transporting...\n");
					mode=TRANSPORT_SEND;
				}
				else
				{
					printf("%s has refused file transporting.\nNow we will back to chating mode.\n",buff);
					mode=CHAT;
				}
			}
			else if  (mode==HANDSHAKE_FILENAME)
			{
				strcpy(transport_filename,buf);
				if ((fp_transport=fopen(transport_filename,"wb"))==NULL)
				{
					printf("Open file error!");
					close(client_fd);
					close(sockfd);	/*关闭套接字*/
					fclose(fp);
					exit(-1);
				}
				mode=HANDSHAKE_LENGTH;
			}
			else if (mode==HANDSHAKE_LENGTH)
			{
				filelength=atol(buf);
				printf("File transporting...\n");
				mode=TRANSPORT_RECIEVE;
			}
		}
/*判断是否已将新建的描述符加到seletct()监视的异常的文件描述符集合中*/
		if (FD_ISSET(client_fd, &efd_set))
		{
			close(client_fd);	/*关闭套接字*/
			fclose(fp);
			exit(0);
		}
	}
}
