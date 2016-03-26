#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>  
#include <time.h>  
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>

#define BACKLOG 5
#define LENGTH 512 

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int tcp_send(char *ip, int PORT, char *filename) 
{
	/* Variable Definition */
	int sockfd; 
	int nsockfd;
	char revbuf[LENGTH]; 
	struct sockaddr_in remote_addr;

	/* Get the Socket file descriptor */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor! (errno = %d)\n",errno);
		exit(1);
	}

	/* Fill the socket address struct */
	remote_addr.sin_family = AF_INET; 
	remote_addr.sin_port = htons(PORT); 
	inet_pton(AF_INET, ip, &remote_addr.sin_addr); 
	bzero(&(remote_addr.sin_zero), 8);

	/* Try to connect the remote */
	if (connect(sockfd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "ERROR: Failed to connect to the client! (errno = %d)\n",errno);
		exit(1);
	}
	else { 
		printf("[Send Client] Connected to Recv Client at port %d...ok!\n", PORT);
	}

	/* Send File to Server */
	//if(!fork())
	//{
		char* fs_name = filename;
		char sdbuf[LENGTH]; 
		printf("[Send Client] Sending %s to the Recv Client... ", fs_name);
		FILE *fs = fopen(fs_name, "r");
		if(fs == NULL) {
			printf("ERROR: File %s not found.\n", fs_name);
			exit(1);
		}

		fseek(fs, 0, SEEK_END);
		int filelen = ftell(fs);
		rewind(fs); 
		char filelenc[512];
		bzero(filelenc, LENGTH); 
		sprintf(filelenc, "%d", filelen); 
		send(sockfd, filelenc, LENGTH, 0);

		bzero(sdbuf, LENGTH); 
		int fs_block_sz; 
		while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0) {
		    	if(send(sockfd, sdbuf, fs_block_sz, 0) < 0) {
				fprintf(stderr, "ERROR: Failed to send file %s. (errno = %d)\n", fs_name, errno);
				break;
			}
			
		    	bzero(sdbuf, LENGTH);
		}
		printf("Ok File %s from Send Client was Sent!\n", fs_name);
		printf("[Send Client] Connection with Recv Client closed.\n");
	//}
	return 0;
}

int tcp_recv(char *ip, int PORT) 
{
	/* Defining Variables */
	int sockfd; 
	int nsockfd; 
	int num;
	int sin_size; 
	struct sockaddr_in addr_local; /* client addr */
	struct sockaddr_in addr_remote; /* server addr */
	char revbuf[LENGTH]; // Receiver buffer

	/* Get the Socket file descriptor */
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "ERROR: Failed to obtain Socket Descriptor. (errno = %d)\n", errno);
		exit(1);
	}
	else 
		printf("[Recv Client] Obtaining socket descriptor successfully.\n");

	/* Fill the client socket address struct */
	addr_local.sin_family = AF_INET; // Protocol Family
	addr_local.sin_port = htons(PORT); // Port number
	addr_local.sin_addr.s_addr = INADDR_ANY; // AutoFill local address
	bzero(&(addr_local.sin_zero), 8); // Flush the rest of struct

	/* Bind a special Port */
	if( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1) {
		fprintf(stderr, "ERROR: Failed to bind Port. (errno = %d)\n", errno);
		exit(1);
	}
	else { 
		printf("[Recv Client] Binded tcp port %d in addr %s sucessfully.\n",PORT, ip);
	}

	/* Listen remote connect/calling */
	if(listen(sockfd,BACKLOG) == -1) {
		fprintf(stderr, "ERROR: Failed to listen Port. (errno = %d)\n", errno);
		exit(1);
	}
	else {
		printf ("[Recv Client] Listening the port %d successfully.\n", PORT);
	}

	int success = 0;
	while(success == 0) {
		sin_size = sizeof(struct sockaddr_in);

		/* Wait a connection, and obtain a new socket file despriptor for single connection */
		if ((nsockfd = accept(sockfd, (struct sockaddr *)&addr_remote, &sin_size)) == -1)  {
			fprintf(stderr, "ERROR: Obtaining new Socket Despcritor. (errno = %d)\n", errno);
			exit(1);
		}
		else { 
			printf("[Recv Client] Recv Client has got connected from %s.\n", inet_ntoa(addr_remote.sin_addr));
		}

		/*Receive File from Client */
		char* fr_name = "receive.txt";
		FILE *fr = fopen(fr_name, "w");
		if(fr == NULL) {
			printf("File %s Cannot be opened file on server.\n", fr_name);
		}		
		else {
			char filesizec[512];
			int filesize, filetmp, comtmp;
			bzero(filesizec, LENGTH);
			recv(nsockfd, filesizec, LENGTH, 0);
			filesize = atoi(filesizec);
			time_t ltime; /* calendar time */
			int timep;
			struct tm *Tm;

			bzero(revbuf, LENGTH); 
			int fr_block_sz = 0;
			while((fr_block_sz = recv(nsockfd, revbuf, LENGTH, 0)) > 0) {
				int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
				
				if(write_sz < fr_block_sz) {
			        	error("File write failed on server.\n");
			    	}
				bzero(revbuf, LENGTH);
				fseek(fr, 0, SEEK_END);
				filetmp = ftell(fr); 
				//if(((filetmp / filesize) * 100) % 5 == 0) {
					comtmp = (int)((filetmp / filesize) * 100);					
    					ltime=time(NULL); /* get current cal time */
					Tm=localtime(&ltime);
					printf("%d percent done. %d/%d/%d, %d:%d:%d\n", comtmp, Tm->tm_year + 1900, Tm->tm_mon + 1, Tm->tm_mday, Tm->tm_hour, Tm->tm_min, Tm->tm_sec);				
				//}
				if (fr_block_sz == 0 || fr_block_sz != 512) {
					break;
				}
			}
			if(fr_block_sz < 0)	{
		        	if(errno == EAGAIN) {
	                		printf("recv() timed out.\n");
	            		}
	            		else {
	                		fprintf(stderr, "recv() failed due to errno = %d\n", errno);
					exit(1);
	            		}
        		}
			printf("[Recv Client] Ok received from Send Client!\n");
			fclose(fr); 
			success = 1;
		    	close(nsockfd);
		    	printf("[Recv Client] Connection with Send Client closed.\n");
		    	while(waitpid(-1, NULL, WNOHANG) > 0);
		}
	}
	return 0;
}

int udp_send(char *ip, int PORT, char *filename) 
{
	char buff[20];
    	char content[200];
    	int sd, connfd, len, bytes_read;

    	struct sockaddr_in servaddr, cliaddr;

    	sd = socket(AF_INET, SOCK_DGRAM, 0);

    	if (sd == -1) {
        	puts("[Send Client] Socket not created in Send Client.");
        	return 1;
    	} 
	else {
        	puts("[Send Client] Socket created in Send Client.");
    	}

    	bzero(&servaddr, sizeof(servaddr));

    	servaddr.sin_family = AF_INET;
    	servaddr.sin_addr.s_addr = INADDR_ANY;
    	servaddr.sin_port = htons(PORT);

    	if (bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        	puts("[Send Client] Connection is not binded.");
        	return 1;
    	} 
	else {
        	puts("[Send Client] Connection is Binded. Waiting for Recv Client...");
    	}

    	len = sizeof(cliaddr);

    	recvfrom(sd, buff, 1024, 0, (struct sockaddr *)&cliaddr, &len);

    	//printf("%s\n", buff);

    	FILE *fp = fopen(filename, "r");
    	if (fp == NULL) {
        	printf("File does not exist \n");
        	return 1;
    	}

	fseek(fp, 0, SEEK_END);
	int filelen = ftell(fp);
	rewind(fp); 
	char filelenc[512];
	bzero(filelenc, LENGTH); 
	sprintf(filelenc, "%d", filelen); 
	sendto(sd, filelenc, sizeof(filelenc), 0, (struct sockaddr*)&cliaddr, len);

    	while (1) {
        	bytes_read = fread(content, 1, sizeof(content), fp);
        	if (bytes_read == 0) {
            		break;
		}
        	sendto(sd, content, sizeof(content), 0, (struct sockaddr*)&cliaddr, len);
        	bzero(content, 200);
    	}
    	strcpy(content, "end");
    	sendto(sd, content, sizeof(content), 0, (struct sockaddr*)&cliaddr, len);
	puts("[Send Client] Sending the file completed.");	
	puts("[Send Client] Connection with Recv Client closed.");
	return 0;
}

int udp_recv(char *ip, int PORT) 
{
	int count = 0;
    	char buff[20], output[20];
    	char file_buffer[200];
    	int sockfd, connfd, len;

    	struct sockaddr_in servaddr, cliaddr;

    	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    	if (sockfd == -1) {
        	puts("[Recv Client] Socket not created in Recv Client.");
        	return 1;
    	} 
	else {
        	puts("[Recv Client] Socket created in Recv Client.");
    	}

    	bzero(&servaddr, sizeof(servaddr));

    	servaddr.sin_family = AF_INET;
    	servaddr.sin_addr.s_addr = INADDR_ANY; // ANY address or use specific address
    	servaddr.sin_port = htons(PORT);  // Port address

    	//puts("enter the name of new file to be saved");
    	//scanf("%s", output);

    	// send msg to server
	strcpy(buff, "test");    
	sendto(sockfd, buff, strlen(buff) + 1, 0, (struct sockaddr *)&servaddr, sizeof(struct sockaddr));
    	count++;
    	//printf("%d\n", count);

    	FILE *fp = fopen("receive.txt", "w");
    	if (fp == NULL) {
        	puts("error in file handling");
        	return 1;
    	}

	char filesizec[512];
	int filesize, filetmp, comtmp;
	bzero(filesizec, LENGTH);
	recvfrom(sockfd, filesizec, sizeof(filesizec), 0, NULL, NULL);
	filesize = atoi(filesizec);
	time_t ltime; /* calendar time */
	int timep;
	struct tm *Tm;

    	recvfrom(sockfd, file_buffer, sizeof(file_buffer), 0, NULL, NULL);
    	while (1) {
        	if (strcmp(file_buffer, "end") == 0) {
            		break;
		}
        	fwrite(file_buffer, 1, strlen(file_buffer), fp);
		fseek(fp, 0, SEEK_END);
		filetmp = ftell(fp); 
		//if(((filetmp / filesize) * 100) % 5 == 0) {
			comtmp = (int)((filetmp / filesize) * 100);					
    			ltime=time(NULL); /* get current cal time */
			Tm=localtime(&ltime);
			printf("%d percent done. %d/%d/%d, %d:%d:%d\n", comtmp, Tm->tm_year + 1900, Tm->tm_mon + 1, Tm->tm_mday, Tm->tm_hour, Tm->tm_min, Tm->tm_sec);				
		//}
        	bzero(file_buffer, 200);
        	recvfrom(sockfd, file_buffer, sizeof(file_buffer), 0, NULL, NULL);
    	}

    	puts("[Recv Client] Receiving the file completed.");
	puts("[Recv Client] Connection with Send Client closed.");
	return 0;
}

int main (int argc, char *argv[])
{
	if(argv[1] == NULL || argv[2] == NULL || argv[3] == NULL || argv[4] == NULL) {
		printf("Wrong command type. Please enter the right line.\n");
		printf("Ex1: ./lab1 tcp send <ip> <port> filename.\n");
		printf("Ex2: ./lab1 udp recv <ip> <port>.\n");
	}
	else if (strcmp(argv[2], "send") == 0 && argv[5] == NULL) {
		printf("Please enter the file name that you want to send.\n");
	}
	else{
		int argv4 = atoi(argv[4]);		
		
		if(strcmp(argv[1], "tcp") == 0) {
			if(strcmp(argv[2], "send") == 0) {
				tcp_send(argv[3], argv4, argv[5]);
			}
			else{
				tcp_recv(argv[3], argv4);
			}
		}
		else if(strcmp(argv[1], "udp") == 0) {
			if(strcmp(argv[2], "send") == 0) {
				udp_send(argv[3], argv4, argv[5]);
			}
			else{
				udp_recv(argv[3], argv4);
			}
		}
	}

	return 0;
}
