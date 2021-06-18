#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#define PORT 8080

void tulisLog(char *nama, char *command){
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char log[1024];

	FILE *file;
	file = fopen("client.log", "a");

	sprintf(log, "%d-%.2d-%.2d %.2d:%.2d:%.2d:%s:%s\n", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, nama, command);

	fputs(log, file);
	fclose(file);
	return;
}

int main(int argc, char const *argv[]) {
	char command[1024], cmd[1024][1024], cmd2[1024][1024], username[1024], pass[1024], database[1024], temp[1024];
	
	//cek root/user dan format input
	if(geteuid() == 0){
		printf("root\n");
		strcpy(username, "root");
		strcpy(pass, "root");
	}
	else if(argc==5 && strcmp(argv[1],"-u")==0 && strcmp(argv[3],"-p")==0){
		printf("user\n");
		strcpy(username, argv[2]);
		strcpy(pass, argv[4]);
	}
	else if(argc==7 && strcmp(argv[1],"-u")==0 && strcmp(argv[3],"-p")==0 && strcmp(argv[5],"-d")==0){
		printf("restore\n");
		strcpy(username, argv[2]);
		strcpy(pass, argv[4]);
		strcpy(database, argv[6]);
	}
	else{
		printf("Format input salah\n");
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
		printf("Login berhasil\n");
		send(sock, "login", sizeof("login"), 0);
		
		//restore
		if(argc == 7){
			char stat[1024] = {0};
			read(sock, stat, sizeof(stat));
			
			send(sock, "USE", sizeof("USE"), 0);
			char tmp[1024] = {0};
			
			read(sock, tmp, sizeof(tmp));
			send(sock, database, sizeof(database), 0);
			
			int feed;
			read(sock, &feed, sizeof(feed));
			if(feed){
				char data[1024];
				while(fgets(data, 1024, stdin)){
					if(strlen(data) == 1) continue;
					data[strlen(data)-1] = '\0';
					
					int idx2=0;
					memset(cmd2, 0, sizeof(cmd2));
					for(int i=0, ii=0; i<strlen(data); i++){
						if(data[i] == ' ' || data[i] == ';') idx2++, ii=0;
						else cmd2[idx2][ii] = data[i], ii++;
					}
					
					memset(command, 0, sizeof(command));
					strcpy(command, cmd2[0]);
					for(int i=1; i<idx2; i++){
						strcat(command, " ");
						strcat(command, cmd2[i]);
					}
					tulisLog(username, command);
					
					if(strcmp(cmd2[0], "DROP") == 0){
						if(idx2 == 3 && strcmp(cmd2[1], "TABLE") == 0){
							send(sock, "DROP TABLE", sizeof("DROP TABLE"), 0);
							char tmp[1024] = {0};
							
							read(sock, tmp, sizeof(tmp));
							send(sock, cmd2[2], sizeof(cmd2[2]), 0);
							
							int feed;
							read(sock, &feed, sizeof(feed));
							if(feed) printf("Drop table berhasil\n");
							else printf("Drop table gagal\n");
						}
					}
					else if(strcmp(cmd2[0], "CREATE") == 0){
						if(idx2%2 == 1 && idx2 >= 5 && strcmp(cmd2[1], "TABLE") == 0){
							send(sock, "CREATE TABLE", sizeof("CREATE TABLE"), 0);
							int table_size = idx2-3;
							char tmp[1024] = {0};
							
							read(sock, tmp, sizeof(tmp));
							send(sock, cmd2[2], sizeof(cmd2[2]), 0);
							read(sock, tmp, sizeof(tmp));
							send(sock, &table_size, sizeof(table_size), 0);
							
							for(int i=3; i<idx2; i++){
								read(sock, tmp, sizeof(tmp));
								if(i == 3){
									int len = strlen(cmd2[i]);
									for(int j=1; j<len; j++) cmd2[i][j-1] = cmd2[i][j];
									cmd2[i][len-1] = '\0';
								}
								else if(i%2 == 0) cmd2[i][strlen(cmd2[i])-1] = '\0';
								send(sock, cmd2[i], sizeof(cmd2[i]), 0);
							}
							
							int feed;
							read(sock, &feed, sizeof(feed));
							if(!feed) printf("Create table berhasil\n");
							else printf("Create table gagal\n");
						}
					}
					else if(strcmp(cmd2[0], "INSERT") == 0){
						if(idx2 >= 4 && strcmp(cmd2[1], "INTO") == 0){
							send(sock, "INSERT INTO", sizeof("INSERT INTO"), 0);
							int table_size = idx2-3;
							char tmp[1024] = {0};
							
							read(sock, tmp, sizeof(tmp));
							send(sock, cmd2[2], sizeof(cmd2[2]), 0);
							read(sock, tmp, sizeof(tmp));
							send(sock, &table_size, sizeof(table_size), 0);
							
							for(int i=3; i<idx2; i++){
								read(sock, tmp, sizeof(tmp));
								if(i == 3){
									int len = strlen(cmd2[i]);
									for(int j=1; j<len; j++) cmd2[i][j-1] = cmd2[i][j];
									cmd2[i][len-1] = '\0';
								}
								int len = strlen(cmd2[i]);
								if(cmd2[i][0] == '\''){
									for(int j=1; j<len; j++) cmd2[i][j-1] = cmd2[i][j];
									cmd2[i][len-1] = '\0';
								}
								if(cmd2[i][strlen(cmd2[i])-2] == '\'') cmd2[i][strlen(cmd2[i])-2] = '\0';
								else cmd2[i][strlen(cmd2[i])-1] = '\0';
								send(sock, cmd2[i], sizeof(cmd2[i]), 0);
							}
							
							int feed;
							read(sock, &feed, sizeof(feed));
							if(feed) printf("Insert berhasil\n");
							else printf("Insert gagal\n");
						}
					}
					
					memset(data, 0, sizeof(data));
				}
			}
			else printf("Use gagal\n");
			
			send(sock, "exit", sizeof("exit"), 0);
			read(sock, stat, sizeof(stat));
			send(sock, "exit", sizeof("exit"), 0);
			close(sock);
			return 0;
		}
	}
	else{
		printf("Login gagal\n");
		send(sock, "exit", sizeof("exit"), 0);
		close(sock);
		return 0;
	}
	
	while(1){
		printf("Harap Menunggu\n\n");
		char stat[1024] = {0};
		read(sock, stat, sizeof(stat));
		while(strcmp(stat, "ready") != 0){
			memset(stat, 0, sizeof(stat));
			read(sock, stat, sizeof(stat));
		}
		memset(stat, 0, sizeof(stat));
		
		//read command
		int idx=0;
		memset(cmd, 0, sizeof(cmd));
		while(scanf("%s", cmd[idx++])){
			if(cmd[idx-1][strlen(cmd[idx-1])-1] == ';'){
				cmd[idx-1][strlen(cmd[idx-1])-1] = '\0';
				break;
			}
		}
		
		memset(command, 0, sizeof(command));
		strcpy(command, cmd[0]);
		for(int i=1; i<idx; i++){
			strcat(command, " ");
			strcat(command, cmd[i]);
		}
		tulisLog(username, command);
		
		if(strcmp(cmd[0], "CREATE") == 0){
			if(idx == 6 && strcmp(cmd[1], "USER") == 0 && strcmp(cmd[3], "IDENTIFIED") == 0 && strcmp(cmd[4], "BY") == 0){
				if(strcmp(username, "root") == 0){
					send(sock, "CREATE USER", sizeof("CREATE USER"), 0);
					char tmp[1024] = {0};
					
					read(sock, tmp, sizeof(tmp));
					send(sock, cmd[2], sizeof(cmd[2]), 0);
					
					memset(tmp, 0, sizeof(tmp));
					read(sock, tmp, sizeof(tmp));
					send(sock, cmd[5], sizeof(cmd[5]), 0);
					
					printf("Create user berhasil\n");
				}
				else{
					printf("Anda bukan user root\n");
					goto akhir;
				}
			}
			else if(idx == 3 && strcmp(cmd[1], "DATABASE") == 0){
				send(sock, "CREATE DATABASE", sizeof("CREATE DATABASE"), 0);
				char tmp[1024] = {0};
				
				read(sock, tmp, sizeof(tmp));
				send(sock, cmd[2], sizeof(cmd[2]), 0);
				
				int feed;
				read(sock, &feed, sizeof(feed));
				if(!feed) printf("Create database berhasil\n");
				else printf("Create database gagal\n");
			}
			else goto akhir;
		}
		else if(strcmp(cmd[0], "DROP") == 0){
			if(idx == 3 && strcmp(cmd[1], "DATABASE") == 0){
				send(sock, "DROP DATABASE", sizeof("DROP DATABASE"), 0);
				char tmp[1024] = {0};
				
				read(sock, tmp, sizeof(tmp));
				send(sock, cmd[2], sizeof(cmd[2]), 0);
				
				int feed;
				read(sock, &feed, sizeof(feed));
				if(feed) printf("Drop database berhasil\n");
				else printf("Drop database gagal\n");
			}
			else goto akhir;
		}
		else if(strcmp(cmd[0], "USE") == 0){
			if(idx == 2){
				send(sock, "USE", sizeof("USE"), 0);
				char tmp[1024] = {0};
				
				read(sock, tmp, sizeof(tmp));
				send(sock, cmd[1], sizeof(cmd[1]), 0);
				
				int feed;
				read(sock, &feed, sizeof(feed));
				if(feed){
					printf("Use berhasil\n");
					
					while(1){
						//read command
						int idx2=0;
						memset(cmd2, 0, sizeof(cmd2));
						while(scanf("%s", cmd2[idx2++])){
							if(cmd2[idx2-1][strlen(cmd2[idx2-1])-1] == ';'){
								cmd2[idx2-1][strlen(cmd2[idx2-1])-1] = '\0';
								break;
							}
						}
						
						memset(command, 0, sizeof(command));
						strcpy(command, cmd2[0]);
						for(int i=1; i<idx2; i++){
							strcat(command, " ");
							strcat(command, cmd2[i]);
						}
						tulisLog(username, command);
						
						if(strcmp(cmd2[0], "CREATE") == 0){
							if(idx2%2 == 1 && idx2 >= 5 && strcmp(cmd2[1], "TABLE") == 0){
								send(sock, "CREATE TABLE", sizeof("CREATE TABLE"), 0);
								int table_size = idx2-3;
								char tmp[1024] = {0};
								
								read(sock, tmp, sizeof(tmp));
								send(sock, cmd2[2], sizeof(cmd2[2]), 0);
								read(sock, tmp, sizeof(tmp));
								send(sock, &table_size, sizeof(table_size), 0);
								
								for(int i=3; i<idx2; i++){
									read(sock, tmp, sizeof(tmp));
									if(i == 3){
										int len = strlen(cmd2[i]);
										for(int j=1; j<len; j++) cmd2[i][j-1] = cmd2[i][j];
										cmd2[i][len-1] = '\0';
									}
									else if(i%2 == 0) cmd2[i][strlen(cmd2[i])-1] = '\0';
									send(sock, cmd2[i], sizeof(cmd2[i]), 0);
								}
								
								int feed;
								read(sock, &feed, sizeof(feed));
								if(!feed) printf("Create table berhasil\n");
								else printf("Create table gagal\n");
							}
							else goto akhir2;
						}
						else if(strcmp(cmd2[0], "DROP") == 0){
							if(idx2 == 3 && strcmp(cmd2[1], "TABLE") == 0){
								send(sock, "DROP TABLE", sizeof("DROP TABLE"), 0);
								char tmp[1024] = {0};
								
								read(sock, tmp, sizeof(tmp));
								send(sock, cmd2[2], sizeof(cmd2[2]), 0);
								
								int feed;
								read(sock, &feed, sizeof(feed));
								if(feed) printf("Drop table berhasil\n");
								else printf("Drop table gagal\n");
							}
							else if(idx2 == 5 && strcmp(cmd2[1], "COLUMN") == 0 && strcmp(cmd2[3], "FROM") == 0){
								send(sock, "DROP COLUMN", sizeof("DROP COLUMN"), 0);
								char tmp[1024] = {0};
								
								read(sock, tmp, sizeof(tmp));
								send(sock, cmd2[2], sizeof(cmd2[2]), 0);
								read(sock, tmp, sizeof(tmp));
								send(sock, cmd2[4], sizeof(cmd2[4]), 0);
								
								int feed;
								read(sock, &feed, sizeof(feed));
								if(feed) printf("Drop column berhasil\n");
								else printf("Drop column gagal\n");
							}
							else goto akhir2;
						}
						else if(strcmp(cmd2[0], "INSERT") == 0){
							if(idx2 >= 4 && strcmp(cmd2[1], "INTO") == 0){
								send(sock, "INSERT INTO", sizeof("INSERT INTO"), 0);
								int table_size = idx2-3;
								char tmp[1024] = {0};
								
								read(sock, tmp, sizeof(tmp));
								send(sock, cmd2[2], sizeof(cmd2[2]), 0);
								read(sock, tmp, sizeof(tmp));
								send(sock, &table_size, sizeof(table_size), 0);
								
								for(int i=3; i<idx2; i++){
									read(sock, tmp, sizeof(tmp));
									if(i == 3){
										int len = strlen(cmd2[i]);
										for(int j=1; j<len; j++) cmd2[i][j-1] = cmd2[i][j];
										cmd2[i][len-1] = '\0';
									}
									int len = strlen(cmd2[i]);
									if(cmd2[i][0] == '\''){
										for(int j=1; j<len; j++) cmd2[i][j-1] = cmd2[i][j];
										cmd2[i][len-1] = '\0';
									}
									if(cmd2[i][strlen(cmd2[i])-2] == '\'') cmd2[i][strlen(cmd2[i])-2] = '\0';
									else cmd2[i][strlen(cmd2[i])-1] = '\0';
									send(sock, cmd2[i], sizeof(cmd2[i]), 0);
								}
								
								int feed;
								read(sock, &feed, sizeof(feed));
								if(feed) printf("Insert berhasil\n");
								else printf("Insert gagal\n");
							}
							else goto akhir2;
						}
						else if(strcmp(cmd2[0], "SELECT") == 0){
							if(idx2 >= 4 && strcmp(cmd2[idx2-2], "FROM") == 0){
								send(sock, "SELECT INTO", sizeof("SELECT INTO"), 0);
								int table_size = idx2-3;
								char tmp[1024] = {0};
								
								read(sock, tmp, sizeof(tmp));
								send(sock, cmd2[idx2-1], sizeof(cmd2[idx2-1]), 0);
								read(sock, tmp, sizeof(tmp));
								send(sock, &table_size, sizeof(table_size), 0);
								for(int i=1; i<idx2-2; i++){
									read(sock, tmp, sizeof(tmp));
									send(sock, cmd2[i], sizeof(cmd2[i]), 0);
								}
								
								int feed;
								read(sock, &feed, sizeof(feed));
								if(feed){
									char data_table[1024] = {0};
									send(sock, "temp", 4, 0);
									read(sock, data_table, sizeof(data_table));
									while(strcmp(data_table, "SELESAI") != 0){
										printf("%s", data_table);
										memset(data_table, 0, sizeof(data_table));
										send(sock, "temp", 4, 0);
										read(sock, data_table, sizeof(data_table));
									}
									printf("Select berhasil\n");
								}
								else printf("Select gagal\n");
							}
							else goto akhir2;
						}
						else if(strcmp(cmd2[0], "exit") == 0){
							send(sock, cmd2[0], sizeof(cmd2[0]), 0);
							break;
						}
						else{
							akhir2:
							printf("Input salah\n");
							send(sock, cmd2[0], sizeof(cmd2[0]), 0);
						}
					}
				}
				else printf("Use gagal\n");
			}
			else goto akhir;
		}
		else if(strcmp(cmd[0], "GRANT") == 0){
			if(idx == 5 && strcmp(cmd[1], "PERMISSION") == 0 && strcmp(cmd[3], "INTO") == 0){
				if(strcmp(username, "root") == 0){
					send(sock, "GRANT PERMISSION", sizeof("GRANT PERMISSION"), 0);
					char tmp[1024] = {0};
					
					read(sock, tmp, sizeof(tmp));
					send(sock, cmd[2], sizeof(cmd[2]), 0);
					
					memset(tmp, 0, sizeof(tmp));
					read(sock, tmp, sizeof(tmp));
					send(sock, cmd[4], sizeof(cmd[4]), 0);
					
					int feed;
					read(sock, &feed, sizeof(feed));
					if(feed){
						printf("Grant permission berhasil\n");
						
					}
					else printf("Grant permission gagal\n");
				}
				else{
					printf("Anda bukan user root\n");
					goto akhir;
				}
			}
			else goto akhir;
		}
		else if(strcmp(cmd[0], "exit") == 0){
			send(sock, cmd[0], sizeof(cmd[0]), 0);
			break;
		}
		else{
			akhir:
			printf("Input salah\n");
			send(sock, cmd[0], sizeof(cmd[0]), 0);
		}
	}
	
	close(sock);
    return 0;
}
