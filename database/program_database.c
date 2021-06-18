#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <ctype.h>
#include <dirent.h>
#define PORT 8080

void daemonstart() {
    pid_t pid, sid;
    pid = fork();
    if(pid < 0) exit(EXIT_FAILURE);
    if(pid > 0) exit(EXIT_SUCCESS);
    umask(0);
    sid = setsid();
    if(sid < 0) exit(EXIT_FAILURE);
    if((chdir("/home/aldo/Sisop/FinalProject/database")) < 0) exit(EXIT_FAILURE);
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

typedef struct akun {
	char id[1024];
	char ps[1024];
} akun;

typedef struct ijin {
	char db[1024];
	char id[1024];
} ijin;

typedef struct client_serv {
	int cid;
	int login;
} client_serv;

akun list_akun[100];
ijin list_ijin[100];
char list_database[100][1024];
bool akun_cek, ijin_cek, database_cek, conn_cek;
int akun_n, ijin_n, database_n, conn;

void *ready(void *arg){
	client_serv cl = *(client_serv *) arg;
	int cid = cl.cid;
	int log = cl.login;
	
	while(conn_cek);
	conn_cek = 1;
	
	char data_login[1024], id[1024], ps[1024];
	memset(data_login, 0, sizeof(data_login));
	read(cid, data_login, sizeof(data_login));
	
	int idx = 0;
	for(int i=0; i<strlen(data_login); i++){
		if(data_login[i] == '\t') break;
		id[idx] = data_login[i];
		idx++;
	}
	
	int idx2 = 0;
	for(int i=idx+1; i<strlen(data_login); i++){
		ps[idx2] = data_login[i];
		idx2++;
	}
	
	int done = 0;
	for(int i=0; i<akun_n; i++){
		if((strcmp(id, list_akun[i].id) == 0) && (strcmp(ps, list_akun[i].ps) == 0)){
			done = 1;
			break;
		}
	}
	send(cid, &done, sizeof(done), 0);
	
	char data[1024];
	memset(data, 0, sizeof(data));
	read(cid, data, sizeof(data));
	
	if(strcmp(data, "BACKUP") == 0){
		char database[1024];
		memset(database, 0, sizeof(database));
		
		send(cid, "temp", 4, 0);
		read(cid, database, sizeof(database));
		
		int ada = 0;
		for(int i=0; i<ijin_n; i++){
			if((strcmp(database, list_ijin[i].db) == 0) && (strcmp(id, list_ijin[i].id) == 0)){
				ada = 1;
				break;
			}
		}
		send(cid, &ada, sizeof(ada), 0);
		
		if(ada){
			char db_path[1024] = {0};
			sprintf(db_path, "databases/%s", database);
			
			DIR *dirp;
			struct dirent *entry;
			dirp = opendir(db_path);
			while((entry = readdir(dirp)) != NULL) {
				if(entry->d_type == DT_REG) {
					char tbl_path[1024] = {0};
					sprintf(tbl_path, "databases/%s/%s", database, entry->d_name);
					
					char table_name[1024];
					strcpy(table_name, entry->d_name);
					table_name[strlen(table_name)-4] = '\0';
					
					char tmp[1024] = {0}, command[1024];
					sprintf(command, "DROP TABLE %s;", table_name);
					read(cid, tmp, sizeof(tmp));
					send(cid, command, sizeof(command), 0);
					
					char data[1024] = {0}, data2[1024] = {0};
					FILE *filep;
					filep = fopen(tbl_path, "r");
					if(filep == NULL) printf("gagal buka %s\n", entry->d_name), exit(0);
					
					fgets(data, 1024, filep);
					data[strlen(data)-1] = '\0';
					for(int i=0, ii=0; i<strlen(data); i++){
						if(data[i] == '\t') data2[ii++] = ',', data2[ii++] = ' ';
						else data2[ii++] = data[i];
					}
					sprintf(command, "CREATE TABLE %s (%s);\n", table_name, data2);
					read(cid, tmp, sizeof(tmp));
					send(cid, command, sizeof(command), 0);
					
					while(fgets(data, 1024, filep)){
						data[strlen(data)-1] = '\0';
						memset(data2, 0, sizeof(data2));
						for(int i=0, ii=0; i<strlen(data); i++){
							if(data[i] == '\t') data2[ii++] = ',', data2[ii++] = ' ';
							else data2[ii++] = data[i];
						}
						sprintf(command, "INSERT INTO %s (%s);", table_name, data2);
						read(cid, tmp, sizeof(tmp));
						send(cid, command, sizeof(command), 0);
					}
					
					read(cid, tmp, sizeof(tmp));
					send(cid, "", sizeof(""), 0);
					fclose(filep);
				}
			}
			closedir(dirp);
			
			char tmp[1024] = {0};
			read(cid, tmp, sizeof(tmp));
			send(cid, "SELESAI", sizeof("SELESAI"), 0);
			printf("BACKUP berhasil\n");
		}
		else printf("BACKUP gagal\n");
		conn_cek = 0;
		pthread_exit(0);
	}
	else if(strcmp(data, "exit") == 0){
		printf("Exit berhasil\n");
		conn_cek = 0;
		pthread_exit(0);
	}
	
	while(log){
		send(cid, "ready", 5, 0);
		
		char data[1024];
		memset(data, 0, sizeof(data));
		read(cid, data, sizeof(data));
		
		if(strcmp(data, "CREATE USER") == 0){
			char username[1024], password[1024];
			memset(username, 0, sizeof(username));
			memset(password, 0, sizeof(password));
			
			send(cid, "temp", 4, 0);
			read(cid, username, sizeof(username));
			send(cid, "temp", 4, 0);
			read(cid, password, sizeof(password));
			
			akun akun_reg;
			strcpy(akun_reg.id, username);
			strcpy(akun_reg.ps, password);
			list_akun[akun_n] = akun_reg;
			akun_n++;
			
			FILE *filep;
			filep = fopen("databases/admin/userpass.tsv", "a");
			if(filep == NULL) exit(0);
			fprintf(filep, "%s\t%s\n", username, password);
			fclose(filep);
			
			printf("CREATE USER berhasil\n");
		}
		else if(strcmp(data, "CREATE DATABASE") == 0){
			char database[1024];
			memset(database, 0, sizeof(database));
			
			send(cid, "temp", 4, 0);
			read(cid, database, sizeof(database));
			
			int ada = 0;
			for(int i=0; i<database_n; i++){
				if(strcmp(database, list_database[i]) == 0){
					ada = 1;
					break;
				}
			}
			send(cid, &ada, sizeof(ada), 0);
			
			if(ada) printf("Database sudah ada\n");
			else{
				FILE *filep;
				filep = fopen("databases/admin/database.tsv", "a");
				if(filep == NULL) exit(0);
				fprintf(filep, "%s\n", database);
				fclose(filep);
				
				filep = fopen("databases/admin/permission.tsv", "a");
				if(filep == NULL) exit(0);
				fprintf(filep, "%s\t%s\n", database, "root");
				if(strcmp(id, "root") != 0) fprintf(filep, "%s\t%s\n", database, id);
				fclose(filep);
				
				strcpy(list_database[database_n], database);
				database_n++;
				
				ijin ijin_reg;
				strcpy(ijin_reg.db, database);
				strcpy(ijin_reg.id, "root");
				list_ijin[ijin_n] = ijin_reg;
				ijin_n++;
				if(strcmp(id, "root") != 0){
					ijin ijin_reg;
					strcpy(ijin_reg.db, database);
					strcpy(ijin_reg.id, id);
					list_ijin[ijin_n] = ijin_reg;
					ijin_n++;
				}
				
				char dest[1024] = {0};
				sprintf(dest, "databases/%s", database);
				mkdir(dest, 0755);
				printf("CREATE DATABASE berhasil\n");
			}
		}
		else if(strcmp(data, "DROP DATABASE") == 0){
			char database[1024];
			memset(database, 0, sizeof(database));
			
			send(cid, "temp", 4, 0);
			read(cid, database, sizeof(database));
			
			int ada = 0;
			for(int i=0; i<ijin_n; i++){
				if((strcmp(database, list_ijin[i].db) == 0) && (strcmp(id, list_ijin[i].id) == 0) && (strcmp(database, "admin") != 0)){
					ada = 1;
					break;
				}
			}
			send(cid, &ada, sizeof(ada), 0);
			
			if(ada){
				int idx_db = 0, idx_ijin = 0;
				for(int i=0; i<database_n; i++){
					if(strcmp(database, list_database[i]) != 0){
						strcpy(list_database[idx_db], list_database[i]);
						idx_db++;
					}
				}
				database_n = idx_db;
				
				for(int i=0; i<ijin_n; i++){
					if(strcmp(database, list_ijin[i].db) != 0){
						strcpy(list_ijin[idx_ijin].db, list_ijin[i].db);
						strcpy(list_ijin[idx_ijin].id, list_ijin[i].id);
						idx_ijin++;
					}
				}
				ijin_n = idx_ijin;
				
				FILE *filep;
				filep = fopen("databases/admin/database.tsv", "w");
				if(filep == NULL) exit(0);
				for(int i=0; i<database_n; i++) fprintf(filep, "%s\n", list_database[i]);
				fclose(filep);
				
				filep = fopen("databases/admin/permission.tsv", "w");
				if(filep == NULL) exit(0);
				for(int i=0; i<ijin_n; i++) fprintf(filep, "%s\t%s\n", list_ijin[i].db, list_ijin[i].id);
				fclose(filep);
				
				pid_t child_id = fork();
				if(child_id < 0) exit(0);
				if(child_id == 0){
					char dest[1024] = {0};
					sprintf(dest, "databases/%s", database);
					char *arg[] = {"rm", "-r", dest, NULL};
					execv("/bin/rm", arg);
				}
				printf("DROP DATABASE berhasil\n");
			}
			else printf("DROP DATABASE gagal\n");
		}
		else if(strcmp(data, "USE") == 0){
			char database[1024];
			memset(database, 0, sizeof(database));
			
			send(cid, "temp", 4, 0);
			read(cid, database, sizeof(database));
			
			int ada = 0;
			for(int i=0; i<ijin_n; i++){
				if((strcmp(database, list_ijin[i].db) == 0) && (strcmp(id, list_ijin[i].id) == 0)){
					ada = 1;
					break;
				}
			}
			send(cid, &ada, sizeof(ada), 0);
			
			if(ada){
				printf("USE berhasil\n");
				
				while(1){
					char data2[1024];
					memset(data2, 0, sizeof(data2));
					read(cid, data2, sizeof(data2));
					
					if(strcmp(data2, "CREATE TABLE") == 0){
						int table_size;
						char table[1024], kolom[1024][1024];
						memset(table, 0, sizeof(table));
						memset(kolom, 0, sizeof(kolom));
						
						send(cid, "temp", 4, 0);
						read(cid, table, sizeof(table));
						send(cid, "temp", 4, 0);
						read(cid, &table_size, sizeof(table_size));
						
						for(int i=0; i<table_size; i++){
							send(cid, "temp", 4, 0);
							read(cid, kolom[i], sizeof(kolom[i]));
						}
						
						char db_path[1024] = {0};
						sprintf(db_path, "databases/%s", database);
						
						int ada = 0;
						DIR *dirp;
						struct dirent *entry;
						dirp = opendir(db_path);
						while((entry = readdir(dirp)) != NULL) {
							if(entry->d_type == DT_REG) {
								char table_name[1024];
								strcpy(table_name, entry->d_name);
								table_name[strlen(table_name)-4] = '\0';
								if(strcmp(table_name, table) == 0){
									ada = 1;
									break;
								}
							}
						}
						closedir(dirp);
						send(cid, &ada, sizeof(ada), 0);
						
						if(ada) printf("Table sudah ada\n");
						else{
							char tbl_path[1024] = {0};
							sprintf(tbl_path, "databases/%s/%s.tsv", database, table);
							
							FILE *filep;
							filep = fopen(tbl_path, "a");
							if(filep == NULL) exit(0);
							for(int i=0; i<table_size; i+=2){
								fprintf(filep, "%s %s", kolom[i], kolom[i+1]);
								if(i == table_size-2) fprintf(filep, "\n");
								else fprintf(filep, "\t");
							}
							fclose(filep);
							
							printf("CREATE TABLE berhasil\n");
						}
					}
					else if(strcmp(data2, "DROP TABLE") == 0){
						char table[1024];
						memset(table, 0, sizeof(table));
						
						send(cid, "temp", 4, 0);
						read(cid, table, sizeof(table));
						
						char db_path[1024] = {0};
						sprintf(db_path, "databases/%s", database);
						
						int ada = 0;
						DIR *dirp;
						struct dirent *entry;
						dirp = opendir(db_path);
						while((entry = readdir(dirp)) != NULL) {
							if(entry->d_type == DT_REG) {
								char table_name[1024];
								strcpy(table_name, entry->d_name);
								table_name[strlen(table_name)-4] = '\0';
								if(strcmp(table_name, table) == 0){
									ada = 1;
									break;
								}
							}
						}
						closedir(dirp);
						send(cid, &ada, sizeof(ada), 0);
						
						if(ada){
							pid_t child_id = fork();
							if(child_id < 0) exit(0);
							if(child_id == 0){
								char dest[1024] = {0};
								sprintf(dest, "databases/%s/%s.tsv", database, table);
								char *arg[] = {"rm", "-r", dest, NULL};
								execv("/bin/rm", arg);
							}
							printf("DROP TABLE berhasil\n");
						}
						else printf("DROP TABLE gagal\n");
					}
					else if(strcmp(data2, "DROP COLUMN") == 0){
						char table[1024], kolom[1024], data_table[1024][1024];
						memset(table, 0, sizeof(table));
						memset(kolom, 0, sizeof(kolom));
						memset(data_table, 0, sizeof(data_table));
						
						send(cid, "temp", 4, 0);
						read(cid, kolom, sizeof(kolom));
						send(cid, "temp", 4, 0);
						read(cid, table, sizeof(table));
						
						char tbl_path[1024] = {0};
						sprintf(tbl_path, "databases/%s/%s.tsv", database, table);
						
						char nama_kolom[1024];
						int ada = 0, idx_kolom = 0, tbl_size = 0;
						FILE *filep;
						filep = fopen(tbl_path, "r");
						if(filep != NULL){
							fgets(nama_kolom, 1024, filep);
							int idx_col = 0, ii=0;
							printf("%s\n", nama_kolom);
							for(int i=0; i<strlen(nama_kolom); i++){
								if(nama_kolom[i] == '\t' || nama_kolom[i] == '\n') data_table[idx_col][ii] = '\0', idx_col++, ii=0;
								else data_table[idx_col][ii] = nama_kolom[i], ii++;
							}
							for(int i=0; i<idx_col; i++){
								char *tmptmp = strstr(data_table[i], kolom);
								if(tmptmp != NULL && *(tmptmp+1) == ' ') ada = 1;
								printf("%s ", data_table[i]);
							}
							printf("\n");
							printf("judul\n");
							
							while(ada == 1 && fgets(nama_kolom, 1024, filep)){
								//do something
								int idx_col = 0, ii=0;
								printf("%s\n", nama_kolom);
								for(int i=0; i<strlen(nama_kolom); i++){
									if(nama_kolom[i] == '\t' || nama_kolom[i] == '\n') data_table[idx_col][ii] = '\0', idx_col++, ii=0;
									else data_table[idx_col][ii] = nama_kolom[i], ii++;
								}
								for(int i=0; i<idx_col; i++){
									printf("%s ", data_table[i]);
								}
								printf("\n");
							}
							fclose(filep);
						}
						send(cid, &ada, sizeof(ada), 0);
						
						if(ada){
							//do something
							printf("DROP COLUMN berhasil\n");
						}
						else printf("DROP COLUMN gagal\n");
					}
					else if(strcmp(data2, "INSERT INTO") == 0){
						int table_size;
						char table[1024], kolom[1024][1024];
						memset(table, 0, sizeof(table));
						memset(kolom, 0, sizeof(kolom));
						
						send(cid, "temp", 4, 0);
						read(cid, table, sizeof(table));
						send(cid, "temp", 4, 0);
						read(cid, &table_size, sizeof(table_size));
						
						for(int i=0; i<table_size; i++){
							send(cid, "temp", 4, 0);
							read(cid, kolom[i], sizeof(kolom[i]));
						}
						
						char db_path[1024] = {0};
						sprintf(db_path, "databases/%s", database);
						
						int ada = 0;
						DIR *dirp;
						struct dirent *entry;
						dirp = opendir(db_path);
						while((entry = readdir(dirp)) != NULL) {
							if(entry->d_type == DT_REG) {
								char table_name[1024];
								strcpy(table_name, entry->d_name);
								table_name[strlen(table_name)-4] = '\0';
								if(strcmp(table_name, table) == 0){
									ada = 1;
									break;
								}
							}
						}
						closedir(dirp);
						send(cid, &ada, sizeof(ada), 0);
						
						if(ada){
							char tbl_path[1024] = {0};
							sprintf(tbl_path, "databases/%s/%s.tsv", database, table);
							
							FILE *filep;
							filep = fopen(tbl_path, "a");
							if(filep == NULL) exit(0);
							for(int i=0; i<table_size; i++){
								fprintf(filep, "%s", kolom[i]);
								if(i == table_size-1) fprintf(filep, "\n");
								else fprintf(filep, "\t");
							}
							fclose(filep);
							
							printf("INSERT berhasil\n");
						}
						else printf("INSERT gagal\n");
					}
					else if(strcmp(data2, "SELECT INTO") == 0){
						int table_size;
						char table[1024], kolom[1024][1024];
						memset(table, 0, sizeof(table));
						memset(kolom, 0, sizeof(kolom));
						
						send(cid, "temp", 4, 0);
						read(cid, table, sizeof(table));
						send(cid, "temp", 4, 0);
						read(cid, &table_size, sizeof(table_size));
						
						for(int i=0; i<table_size; i++){
							send(cid, "temp", 4, 0);
							read(cid, kolom[i], sizeof(kolom[i]));
						}
						
						char db_path[1024] = {0};
						sprintf(db_path, "databases/%s", database);
						
						int ada = 0;
						DIR *dirp;
						struct dirent *entry;
						dirp = opendir(db_path);
						while((entry = readdir(dirp)) != NULL) {
							if(entry->d_type == DT_REG) {
								char table_name[1024];
								strcpy(table_name, entry->d_name);
								table_name[strlen(table_name)-4] = '\0';
								if(strcmp(table_name, table) == 0){
									ada = 1;
									break;
								}
							}
						}
						closedir(dirp);
						send(cid, &ada, sizeof(ada), 0);
						
						if(ada && strcmp(kolom[0], "*") == 0){
							char tbl_path[1024] = {0}, data_kolom[1024];
							sprintf(tbl_path, "databases/%s/%s.tsv", database, table);
							
							char tmp[1024] = {0};
							FILE *filep;
							filep = fopen(tbl_path, "r");
							if(filep == NULL) exit(0);
							while(fgets(data_kolom, 1024, filep)){
								read(cid, tmp, sizeof(tmp));
								send(cid, data_kolom, sizeof(data_kolom), 0);
								memset(tmp, 0, sizeof(tmp));
							}
							read(cid, tmp, sizeof(tmp));
							send(cid, "SELESAI", sizeof("SELESAI"), 0);
							fclose(filep);
							
							printf("SELECT berhasil\n");
						}
						else printf("SELECT gagal\n");
						
					}
					else if(strcmp(data2, "exit") == 0){
						printf("Exit berhasil\n");
						break;
					}
					else printf("Input salah\n");
				}
			}
			else printf("USE gagal\n");
		}
		else if(strcmp(data, "GRANT PERMISSION") == 0){
			char database[1024], username[1024];
			memset(database, 0, sizeof(database));
			memset(username, 0, sizeof(username));
			
			send(cid, "temp", 4, 0);
			read(cid, database, sizeof(database));
			send(cid, "temp", 4, 0);
			read(cid, username, sizeof(username));
			
			int db_ada = 0, id_ada = 0;
			for(int i=0; i<database_n; i++){
				if(strcmp(database, list_database[i]) == 0){
					db_ada = 1;
					break;
				}
			}
			
			for(int i=0; i<akun_n; i++){
				if(strcmp(username, list_akun[i].id) == 0){
					id_ada = 1;
					break;
				}
			}
			
			if(!db_ada){
				send(cid, &db_ada, sizeof(db_ada), 0);
				printf("Database tidak ada\n");
			}
			if(!id_ada){
				send(cid, &id_ada, sizeof(id_ada), 0);
				printf("Username tidak ada\n");
			}
			if(db_ada & id_ada){
				ijin ijin_reg;
				strcpy(ijin_reg.db, database);
				strcpy(ijin_reg.id, username);
				list_ijin[ijin_n] = ijin_reg;
				ijin_n++;
				
				FILE *filep;
				filep = fopen("databases/admin/permission.tsv", "a");
				if(filep == NULL) exit(0);
				fprintf(filep, "%s\t%s\n", database, username);
				fclose(filep);
				
				send(cid, &db_ada, sizeof(db_ada), 0);
				printf("GRANT berhasil\n");
			}
		}
		else if(strcmp(data, "exit") == 0){
			printf("Exit berhasil\n");
			conn_cek = 0;
			break;
		}
		else printf("Input salah\n");
	}
	
	pthread_exit(0);
}

int main(int argc, char const *argv[]) {
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
      
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
      
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
	
	FILE *filep;
	char tmp[1024], tmp2[1024];
	int idx = 0;
	filep = fopen("databases/admin/userpass.tsv", "r");
	if(filep == NULL) exit(0);
	while(fscanf(filep, "%s\n", tmp) != EOF){
		fscanf(filep, "%s\n", tmp2);
		
		akun akun_baru;
		strcpy(akun_baru.id, tmp);
		strcpy(akun_baru.ps, tmp2);
		list_akun[idx] = akun_baru;
		printf("%s %s\n", list_akun[idx].id, list_akun[idx].ps);
		idx++;
	}
	fclose(filep);
	
	int idx_ijin = 0;
	filep = fopen("databases/admin/permission.tsv", "r");
	if(filep == NULL) exit(0);
	while(fscanf(filep, "%s\n", tmp) != EOF){
		fscanf(filep, "%s\n", tmp2);
		
		ijin ijin_baru;
		strcpy(ijin_baru.db, tmp);
		strcpy(ijin_baru.id, tmp2);
		list_ijin[idx_ijin] = ijin_baru;
		printf("%s %s\n", list_ijin[idx_ijin].db, list_ijin[idx_ijin].id);
		idx_ijin++;
	}
	fclose(filep);
	
	int idx_db = 0;
	filep = fopen("databases/admin/database.tsv", "r");
	if(filep == NULL) exit(0);
	while(fscanf(filep, "%s\n", tmp) != EOF){
		strcpy(list_database[idx_db], tmp);
		printf("%s\n", list_database[idx_db]);
		idx_db++;
	}
	fclose(filep);
	
	akun_n = idx;
	ijin_n = idx_ijin;
	database_n = idx_db;
	akun_cek = 0;
	ijin_cek = 0;
	database_cek = 0;
	conn = 0;
	conn_cek = 0;
	
	int client;
	
	//daemonstart();
	//while(1){
		while(client = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)){
			printf("Client accepted\n");
			
			pthread_t th;
			client_serv client_baru;
			client_baru.cid = client;
			client_baru.login = 1;
			
			pthread_create(&th, NULL, ready, (void *) &client_baru);
			//pthread_join(th, NULL);
		}
		if (client < 0) {
		    perror("accept");
		    exit(EXIT_FAILURE);
		}
	//}
	
	close(client);
    return 0;
}
