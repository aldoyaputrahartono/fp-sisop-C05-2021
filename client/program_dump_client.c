#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#define PORT 8080

int main(int argc, char const *argv[]) {
	char cmd[1024][1024], cmd2[1024][1024], username[1024], pass[1024], database[1024], temp[1024];
	
	//cek root/user dan format input
	/*if(geteuid() == 0){
		printf("root\n");
		strcpy(username, "root");
		strcpy(pass, "root");
	}*/
	if(argc==6 && strcmp(argv[1],"-u")==0 && strcmp(argv[3],"-p")==0){
		//printf("user\n");
		strcpy(username, argv[2]);
		strcpy(pass, argv[4]);
		strcpy(database, argv[5]);
	}
	else{
		//printf("Format input salah\n");
		return 0;
	}
	
    struct sockaddr_in address;
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
  
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
      
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
  
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
	
	//login database
	memset(temp, 0, sizeof(temp));
	strcpy(temp, username);
	strcat(temp, "\t");
	strcat(temp, pass);
	send(sock, temp, strlen(temp), 0);
	
	int feedback;
	read(sock, &feedback, sizeof(feedback));
	if(feedback){
		//printf("Login berhasil\n");
		send(sock, "BACKUP", sizeof("BACKUP"), 0);
		
		char tmp[1024] = {0};
		read(sock, tmp, sizeof(tmp));
		send(sock, database, sizeof(database), 0);
		
		int feed;
		read(sock, &feed, sizeof(feed));
		if(feed){
			char data[1024] = {0};
			send(sock, "temp", 4, 0);
			read(sock, data, sizeof(data));
			
			while(strcmp(data, "SELESAI") != 0){
				printf("%s\n", data);
				send(sock, "temp", 4, 0);
				memset(data, 0, sizeof(data));
				read(sock, data, sizeof(data));
			}
			
			//printf("Backup berhasil\n");
		}
		//else printf("Backup gagal\n");
	}
	else{
		//printf("Login gagal\n");
		send(sock, "exit", sizeof("exit"), 0);
	}
	
	close(sock);
    return 0;
}
