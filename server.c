#include <stdio.h>
#include <string.h> 
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <sys/mman.h>
#include <netinet/in.h> 
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

int *max_music_id; 
int *music_name_total_length; 

char *masterFilePath = NULL;
char *loginFilePath = NULL;
char *musicDirectory = NULL;

int isUsernameExits(char *, int); 
int validate(char *, int, int);
void process(int);
void registration(int); 
void browse(int);
void put(int);
int addProfile(char *, int, char*, int);
int isAuthenticated(char*, int, int);

void getMusicByRandom(int); 
void getMusicByMID(int);
void getMasterFile(int);
void getBiggestMID(int); 

void mystrcpy(char*, char*, int);
char* int2bytes(int);
int bytes2int(char*);
void getlock(int, int);

// writes the error message and exits
void error(char *msg) {    
	perror(msg);    
	exit(0);
}

// socket write error function
void printWriteError() {
	printf("ERROR, socket couldn't be written\n");
}

// socket read error function
void printReadError() {
	printf("ERROR, socket couldn't be read\n");
}

int main(int argc, char *argv[]){    
	int sockfd,               //initial socket handle
		  newsockfd,		 //socket handle of the child
		  portno,				 //port number of the communication
		  childpid,				 //process id of the child process
		  temp_music_id; //temporary music id
		  
	unsigned int  clilen;					 //length of the client address	  
		   
	struct sockaddr_in serv_addr,  //server address
									 cli_addr;     //client address
	
	char *temp_music_name;   //temporary music name
	FILE *masterFile, *loginFile;  //file descriptor of the master file and login file
	
	//masterFilePath = getenv("SMS_MASTER");
	//loginFilePath = getenv("SMS_LOGIN");
	//musicDirectory = getenv("SMS_MUSIC");
	
	masterFilePath = "master.txt";
	loginFilePath = "login.txt";
	musicDirectory = ".";
	
	max_music_id = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*max_music_id = 0;
	music_name_total_length = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*music_name_total_length = 0; 
	 
	//check environment variables to start server
	if (masterFilePath == NULL) {
		error("ERROR, SMS_MASTER isn't set\n");
	}
	if (loginFilePath == NULL) {
		error("ERROR, SMS_LOGIN isn't set\n");
	}
	if (musicDirectory == NULL) {
		error("ERROR, SMS_MUSIC isn't set\n");
	}
	if (argc < 2) {
		error("ERROR, No port is provided\n"); 
	} 
	
	//allocate buffer and check whether it is successful
	temp_music_name = malloc(sizeof(char) * 256);
	if (temp_music_name == NULL) {
		error("ERROR, buffer couldn't be allocated\n");
	}
	
	//check the accessbility of the login file and unless exists, create new one
	loginFile = fopen(loginFilePath, "a");
	if (loginFile == NULL) {
		error("ERROR, login file couldn't be accessed\n");
	}
	fclose(loginFile);
	
	//check the accessbility of the master file and unless exists, create new one
	masterFile = fopen(masterFilePath, "a");
	if (masterFile == NULL) {
		error("ERROR, master file couldn't be accessed\n");
	}
	fclose(masterFile);
	
	//open master file and check whether it is successful
	masterFile = fopen(masterFilePath, "r");
	if (masterFile == NULL) {
		error("ERROR, master file couldn't be opened\n");
	}
	
	//read all entries in the master file, count the musics and calculate the total length names of the musics 
	while(fscanf(masterFile, "%d\t%256[^\n]", &temp_music_id, temp_music_name) == 2) {
		(*music_name_total_length) += strlen(temp_music_name);
		(*max_music_id)++;
	}
	
	//close master file and deallocate the memory
	free(temp_music_name);
	fclose(masterFile);
	
	//open the socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd < 0) {
		error("ERROR, opening socket\n"); 
	}
	
	//reset the memory and set the address and port number
	memset((char *) &serv_addr, sizeof(serv_addr), 0); 
	portno = atoi(argv[1]); 
	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_addr.s_addr = INADDR_ANY; 
	serv_addr.sin_port = htons(portno); 
	
	//bind the socket to the address
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 	{
		error("ERROR, on binding\n"); 
	}
	
	//listen for the socket while instantaneous 5 request is being queued
	listen(sockfd, 5); 
	
	printf("INFO, server is started and waiting for requests :)\n");
	
	//accept and fork 
	for( ; ; ) {
	
		clilen = sizeof(cli_addr); 
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen); 
	
		if (newsockfd < 0) {
			printf("ERROR, on accept\n"); 
		} else {
			
			//accept is successful so fork and pass the request to the child process
			if ( (childpid = fork()) < 0) {
				printf("ERROR, on fork\n");
			} else if (childpid == 0) {	
				close(sockfd);	 //close since child process doesn't need 	
				process(newsockfd);	
				exit(0);
			}

			close(newsockfd); //close since parent process doesn't need
		
		}		
	
	}
	
	//close the socket
	close(sockfd); 
	return 0; 
}

void process(int newsockfd) {
	int n;        //temporary read - write length
	char req; //request type
	
	n = recv(newsockfd, &req, 1, 0); 
	if (n < 0) {
		printReadError();
		req = '1';
		n = send(newsockfd, &req, 1, 0);
		if (n < 0) {
			printWriteError();
		}
		return;
	}
	
	if (req == '0') {
		registration(newsockfd);
	} else if (req == '1') {
		getMasterFile(newsockfd);
	} else if (req == '2') {
		getMusicByMID(newsockfd);
	} else if (req == '3') {
		getMusicByRandom(newsockfd);
	} else if (req == '4') {
		getBiggestMID(newsockfd);
	} else if (req == '5') {
		put(newsockfd);
	} else {
		printf("ERROR, Unspecified request type\n");
		req = '1';
		n = send(newsockfd, &req, 1, 0);
		if (n < 0) {
			printWriteError();
		}
	}
}

void doRegistrationError(int newsockfd) {
	int n;
	char res;
	res = '1';
	n = send(newsockfd, &res, 1, 0);
	if (n < 0) printWriteError();
}

void registration(int newsockfd) {
	int n,  //temporary read-write 
		  username_length,  //user name length
		  password_length;   //password length
	
	char res,       //response type
			 *username, //username
			 *password,  //password
			 *length = malloc(sizeof(char) * 4);
	
	printf("\nINFO, registration operation is called\n");
	
	//buffer for request length parameters
	if (length == NULL) {
		doRegistrationError(newsockfd);
		return;
	}
	
	//get user name length
	n = recv(newsockfd, length, 4, 0);
	if (n < 0) {
		printReadError();
		doRegistrationError(newsockfd);
		free(length);
		return;
	}
	username_length = bytes2int(length);
	
	//allocate user name buffer and read the user name
	username = malloc(sizeof(char) * username_length);
	if (username == NULL) {
		printf("ERROR, memory couldn't be allocated for username\n");
		doRegistrationError(newsockfd);
		free(length);
		return;
	}
	n = recv(newsockfd, username, username_length, 0);
	if (n < 0) {
		printReadError();
		doRegistrationError(newsockfd);
		free(length);
		free(username);
		return;
	}
	
	//get password length
	n = recv(newsockfd, length, 4, 0);
	if (n < 0) {
		printReadError();
		doRegistrationError(newsockfd);
		free(length);
		free(username);
		return;
	}
	password_length = bytes2int(length);
	
	//allocate buffer for password and read the password
	password = malloc(sizeof(char) * password_length);
	if (password == NULL) {
		printf("ERROR, memory couldn't be allocated for password\n");
		doRegistrationError(newsockfd);
		free(length);
		free(username);
		return;
	}
	n = recv(newsockfd, password, password_length, 0);
	if (n < 0) {
		printReadError();
		doRegistrationError(newsockfd);
		free(length);
		free(username);
		free(password);
		return;
	}
	
	if (!validate(username, 0, username_length)) {
		
		res = '2';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) printWriteError();
		printf("INFO, registration is successfully answered\n");
			
	} else if (!validate(password, 0, password_length)) {
		
		res = '3';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) printWriteError();
		printf("INFO, registration is successfully answered\n");
		
	} else if (password_length < 8) {
		
		res = '4';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) printWriteError();
		printf("INFO, registration is successfully answered\n");
		
	} else if (isUsernameExits(username, username_length)) {
		
		res = '5';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) printWriteError();
		printf("INFO, registration is successfully answered\n");
		
	} else {
		n = addProfile(username, username_length, password, password_length);
		if (n < 0) {
			doRegistrationError(newsockfd);
			printf("INFO, registration is successfully answered\n");
		} else {
			res = '0';
			n = send(newsockfd, &res, 1, 0);
			if (n < 0) printWriteError();
			else printf("INFO, registration operation is successfully completed\n");
		}
	}
	
	free(username);
	free(password);
	free(length);
}

void getMasterFile(int newsockfd) {
	int n,
		  music_id;
	
	char res,
			 *temp,
			 *musicName = malloc(sizeof(char) * 257);
	
	FILE *fd;
	
	printf("\nINFO, Browse master file operation is called\n");
	
	//check whether buffer is allocated
	if (musicName == NULL) {
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written");
		}
		return;
	}
	
	//open master file and check whether it is successful
	fd = fopen(masterFilePath, "r");
	if (fd == NULL) {
		
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		free(musicName);
		return;
	}
	getlock(fileno(fd), F_RDLCK); 
	
	//send success
	res = '0';
	n = send(newsockfd, &res, 1, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be written\n");
		free(musicName);
		getlock(fileno(fd), F_UNLCK);
		fclose(fd);
		return;
	}
	
	//send buffer_length
	temp = int2bytes((*music_name_total_length) + 8 * (*max_music_id));
	n = send(newsockfd, temp, 4, 0);
	free(temp);
	if (n < 0) {
		printf("ERROR, socket couldn't be written\n");
		free(musicName);
		getlock(fileno(fd), F_UNLCK);
		fclose(fd);
		return;
	}
	
	//read all music data
	while(fscanf(fd, "%d\t%256[^\n]", &music_id, musicName) == 2) {
			
			//send MID
			temp = int2bytes(music_id);
			n = send(newsockfd, temp, 4, 0);
			free(temp);
			if (n < 0) {
				printf("ERROR, socket couldn't be written\n");
				free(musicName);
				getlock(fileno(fd), F_UNLCK);
				fclose(fd);
				return;
			}
			
			//send music name length
			temp = int2bytes(strlen(musicName));
			n = send(newsockfd, temp, 4, 0);
			free(temp);
			if (n < 0) {
				printf("ERROR, socket couldn't be written\n");
				free(musicName);
				getlock(fileno(fd), F_UNLCK);
				fclose(fd);
				return;
			}
			
			//send the music name
			n = send(newsockfd, musicName, strlen(musicName), 0);
			if (n < 0) {
				printf("ERROR, socket couldn't be written\n");
				free(musicName);
				getlock(fileno(fd), F_UNLCK);
				fclose(fd);
				return;
			}
	}
	
	free(musicName);
	getlock(fileno(fd), F_UNLCK);
	fclose(fd);
	
	printf("INFO, Browse master file is successfully completed\n");
}

void getMusicByMID(int newsockfd) {
	int i, 
		  n,
		  music_id, 
		  id,
		  music_name_length, 
		  buffer_length, 
		  file_length;
	
	char res,
			 *temp,
			 *buffer;
	
	FILE *fd, *fmusic;
	
	printf("\nINFO, Browse by MID is called\n");
	
	//allocate buffer and check whether it is successful
	buffer = malloc(sizeof(char) * 1024);
	if (buffer == NULL) {
		printf("ERROR, memory couldn't be allocated\n");
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		return;
	}
	
	//get MID
	n = recv(newsockfd, buffer, 4, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		free(buffer);
		return;
	}
	music_id = bytes2int(buffer);
	
	//check MID range
	if (music_id < 1 || music_id > (*max_music_id)) {
		res = '2';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		free(buffer);
		return;
	}
	
	//open relevant music file and check whether it is successful
	sprintf(buffer, "%d", music_id);
	fmusic = fopen(buffer, "rb");
	if (fmusic == NULL) {
		printf("ERROR, music file couldn't be found\n");
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		free(buffer);
		return;
	}
	getlock(fileno(fmusic), F_RDLCK);
	
	//open master file for the music name and check whether it is successful 
	fd = fopen(masterFilePath, "r");
	if (fd == NULL) {
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		free(buffer);
		getlock(fileno(fmusic), F_UNLCK);
		fclose(fmusic);
		return;
	}
	getlock(fileno(fd), F_RDLCK);
	
	//find music name from the master file
	while(fscanf(fd, "%d\t%256[^\n]", &id, buffer) == 2) {
			
			//searched music
			if(id == music_id) {
				
				//start to prepare the response
				res = '0';
				n = send(newsockfd, &res, 1, 0);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				//calculate the response length
				music_name_length = strlen(buffer);
				
				buffer_length = 4 + music_name_length;
				fseek(fmusic, 0, SEEK_END); 
				file_length = ftell(fmusic); 
				fseek(fmusic, 0, SEEK_SET);
				
				buffer_length += file_length;
				
				//send the response length
				temp = int2bytes(buffer_length);
				n = send(newsockfd, temp, 4, 0);
				free(temp);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				
				//send music name length
				temp = int2bytes(music_name_length);
				n = send(newsockfd, temp, 4, 0);
				free(temp);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				
				//send music name
				n = send(newsockfd, buffer, music_name_length, 0);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				
				//send the music file
				while(file_length > 1024) {
					for(i=0; i<1024; i++) {
						buffer[i] = fgetc(fmusic);
					}
					n = send(newsockfd, buffer, 1024, 0);
					if (n < 0) {
						printf("ERROR, socket couldn't be written\n");
						getlock(fileno(fmusic), F_UNLCK);
						fclose(fmusic);
						getlock(fileno(fd), F_UNLCK);
						fclose(fd);
						free(buffer);
						return;
					}
					file_length -= 1024;
				}
				for(i=0; i<file_length; i++) {
					buffer[i] = fgetc(fmusic);
				}
				n = send(newsockfd, buffer, file_length, 0);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				getlock(fileno(fmusic), F_UNLCK);
				fclose(fmusic);
				break;
			}
	}
	
	getlock(fileno(fd), F_UNLCK);
	fclose(fd);
	free(buffer);
	
	printf("INFO, Browse by MID is successfully completed\n");
}

void getMusicByRandom(int newsockfd) {
	int i, 
		  n,
		  music_id, 
		  id,
		  music_name_length, 
		  buffer_length, 
		  file_length;
	
	char res,
			 *temp,
			 *buffer;
	
	FILE *fd, *fmusic;
	
	printf("\nINFO, Browse by random is called\n");
	
	//check whether there is a music file in the server
	if ((*max_music_id) == 0) {
		res = '2';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		return;
	}
	
	//allocate the buffer and check whether it is successful
	buffer = malloc(sizeof(char) * 1024);
	if (buffer == NULL) {
		printf("ERROR, memort couldn't be allocated\n");
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		return;
	}
	
	//randomly generate a MID
	srand(time(NULL));
	music_id = (rand() % (*max_music_id)) + 1;
	
	//open the relevant music file and check whether it is successful
	sprintf(buffer, "%d", music_id);
	fmusic = fopen(buffer, "rb");
	if (fmusic == NULL) {
		printf("ERROR, music file couldn't be found\n");
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		free(buffer);
		return;
	}
	getlock(fileno(fmusic), F_RDLCK);
	
	//open the master file and check whether it is successful
	fd = fopen(masterFilePath, "r");
	if (fd == NULL) {
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		free(buffer);
		getlock(fileno(fmusic), F_UNLCK);
		fclose(fmusic);
		return;
	}
	getlock(fileno(fd), F_RDLCK);
	
	//find the relevant file info in the master file
	while(fscanf(fd, "%d\t%256[^\n]", &id, buffer) == 2) {
			
			if(id == music_id) {
				
				//start to prepare the response
				res = '0';
				n = send(newsockfd, &res, 1, 0);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				
				//calculate response length
				music_name_length = strlen(buffer);
				
				buffer_length = 4 + 4 + music_name_length;
				fseek(fmusic, 0, SEEK_END); 
				file_length = ftell(fmusic); 
				fseek(fmusic, 0, SEEK_SET);
				
				buffer_length += file_length;
				
				//send response length
				temp = int2bytes(buffer_length);
				n = send(newsockfd, temp, 4, 0);
				free(temp);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				
				//send MID of the chosen music file
				temp = int2bytes(music_id);
				n = send(newsockfd, temp, 4, 0);
				free(temp);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				
				//send the length of the music name
				temp = int2bytes(music_name_length);
				n = send(newsockfd, temp, 4, 0);
				free(temp);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				
				//send the music name
				n = send(newsockfd, buffer, music_name_length, 0);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				
				//send the music file content
				while(file_length > 1024) {
					for(i=0; i<1024; i++) {
						buffer[i] = fgetc(fmusic);
					}
					n = send(newsockfd, buffer, 1024, 0);
					if (n < 0) {
						printf("ERROR, socket couldn't be written\n");
						getlock(fileno(fmusic), F_UNLCK);
						fclose(fmusic);
						getlock(fileno(fd), F_UNLCK);
						fclose(fd);
						free(buffer);
						return;
					}
					file_length -= 1024;
				}
				for(i=0; i<file_length; i++) {
					buffer[i] = fgetc(fmusic);
				}
				n = send(newsockfd, buffer, file_length, 0);
				if (n < 0) {
					printf("ERROR, socket couldn't be written\n");
					getlock(fileno(fmusic), F_UNLCK);
					fclose(fmusic);
					getlock(fileno(fd), F_UNLCK);
					fclose(fd);
					free(buffer);
					return;
				}
				getlock(fileno(fmusic), F_UNLCK);
				fclose(fmusic);
				break;
			}
	}
	
	getlock(fileno(fd), F_UNLCK);
	fclose(fd);
	free(buffer);
	
	printf("INFO, Browse by random is successfully completed\n");
}

void getBiggestMID(int newsockfd) {
	 int n;
	 
	 char res,
			  *temp;
	 
	 printf("\nINFO, Browse the biggest MID is called\n");
	 
	 //start to prepare the response
	 res = '0';
	 n = send(newsockfd, &res, 1, 0);
	 if (n < 0) {
		printf("ERROR, socket couldn't be written\n");
		res = '1';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		return;
	 }
	 
	 //send the biggest music id
	 temp = int2bytes((*max_music_id));
	 n = send(newsockfd, temp, 4, 0);
	 free(temp);
	 if (n < 0) {
		printf("ERROR, socket couldn't be written\n");
	 } else {
		printf("INFO, Browse the biggest MID is successfully completed\n");
	 }
}

//send error to the client 
void printPutError(int newsockfd) {
	char res;
	int n;
	
	res = '1';
	n = send(newsockfd, &res, 1, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be written\n");
	}
}

void put(int newsockfd) {
	int i,
		  n,
		  buffer_length,
		  username_length,
		  password_length,
		  musicname_length,
		  file_length;
	
	char res,
			 *buffer;
	
	FILE *fd;
	
	printf("\nINFO, Put is called\n");
	
	//allocate the buffer and check whether it is successful
	buffer = malloc(sizeof(char) * 1024);
	if (buffer == NULL) {
		printf("ERROR, buffer couldn't be allocated\n");
		printPutError(newsockfd);
		return;
	}
	memset(buffer, 1024, 0);
	
	//get request length
	n = recv(newsockfd, buffer, 4, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	buffer_length = bytes2int(buffer);
	
	//get user name length
	n = recv(newsockfd, buffer, 4, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	username_length = bytes2int(buffer);
	
	//get user name
	n = recv(newsockfd, buffer, username_length, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	memset(buffer, 4, 0);
	
	//get password length
	n = recv(newsockfd, &buffer[username_length], 4, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	password_length = bytes2int(&buffer[username_length]); 
	memset(&buffer[username_length], 4, 0);
	
	//get password
	n = recv(newsockfd, &buffer[username_length], password_length, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	
	if(!isAuthenticated(buffer, username_length, password_length)) {
		buffer_length = buffer_length - 4 -username_length - 4 -password_length;
		while(buffer_length > 0) {
			n = recv(newsockfd, buffer, 1, 0);
			buffer_length--;
		}
		res = '2';
		n = send(newsockfd, &res, 1, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be written\n");
		}
		free(buffer);
		printf("INFO, User isn't authenticated\n");
		printf("INFO, Put is successfully answered\n");
		return;
	}
	
	memset(buffer, 1024, 0);
	
	//get music name length
	n = recv(newsockfd, buffer, 4, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	musicname_length = bytes2int(buffer);
	memset(buffer, 4, 0);
	
	//get music name
	n = recv(newsockfd, buffer, musicname_length, 0); 
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	
	//open master file to add new music and check whether it is successful
	fd = fopen(masterFilePath, "a");
	if (fd == NULL) {
		printf("ERROR, master file couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	getlock(fileno(fd), F_WRLCK);
	
	//increment the number of musics and increase music name length counter accordingly
	(*max_music_id)++;
	(*music_name_total_length) += musicname_length;
	
	//add MID and music name to the master file
	fprintf(fd, "%d\t", *max_music_id);
	for(i=0; i<musicname_length; i++) {
		fputc(buffer[i], fd);
	}
	fputc('\n', fd);
	getlock(fileno(fd), F_UNLCK);
	fclose(fd);
	fd = NULL;
	
	//open the relevant music file to save uploaded data and check whether it is successful	
	sprintf(buffer, "%d", (*max_music_id));
	fd = fopen(buffer, "wb");
	if (fd == NULL) {
		printf("ERROR, music file couldn't be created\n");
		free(buffer);
		printPutError(newsockfd);
		return;
	}
	getlock(fileno(fd), F_WRLCK);
	
	//save the uploaded music data
	file_length = buffer_length - 4 -username_length - 4 -password_length - 4 - musicname_length;
	while(file_length > 1024) {
		n = recv(newsockfd, buffer, 1024, 0);
		if (n < 0) {
			printf("ERROR, socket couldn't be read\n");
			free(buffer);
			printPutError(newsockfd);
			getlock(fileno(fd), F_UNLCK);
			fclose(fd);
			return;
		}
		for(i=0; i<1024; i++) {
			fputc(buffer[i], fd);
		}
		file_length -= 1024;
	}
	n = recv(newsockfd, buffer, file_length, 0);
	if (n < 0) {
		printf("ERROR, socket couldn't be read\n");
		free(buffer);
		printPutError(newsockfd);
		getlock(fileno(fd), F_UNLCK);
		fclose(fd);
		return;
	}
	for(i=0; i<file_length; i++) {
		fputc(buffer[i], fd);
	}
	getlock(fileno(fd), F_UNLCK);
	fclose(fd);
	
	free(buffer);
	
	res = '0';
	n = send(newsockfd, &res, 1, 0);
	i = 0;
	while(n < 0 && i < 2) {
		n = send(newsockfd, &res, 1, 0);
	}
	if (n < 0) {
		printf("ERROR, socket couldn't be written\n");
	} else {
		printf("INFO, Put is successfully completed\n");
	}
}

int isUsernameExits(char *username, int username_length) {
	int diff;
	char local_buffer[257];
	
	FILE *fd = fopen(loginFilePath, "r");
	if (fd == NULL) return 0;
	getlock(fileno(fd), F_RDLCK);
	while(fscanf(fd, "%256s", local_buffer) == 1) {
		diff = strncmp(username, local_buffer, username_length);
		if (diff == 0) {
			getlock(fileno(fd), F_UNLCK);
			fclose(fd);
			return 1;
		} else fscanf(fd, "%256s", local_buffer);
	}
	getlock(fileno(fd), F_UNLCK);
	fclose(fd);
	return 0;
}

int isAuthenticated(char *buffer, int ulen, int plen) {
	int udiff, pdiff;
	char local_buffer[257];

	FILE *fd = fopen(loginFilePath, "r");
	if (fd == NULL) return 0;
	getlock(fileno(fd), F_RDLCK);
	while(fscanf(fd, "%256s", local_buffer) == 1) {
		udiff = strncmp(buffer, local_buffer, ulen);
		if (udiff == 0) {
			fscanf(fd, "%256s", local_buffer);
			pdiff = strncmp(&buffer[ulen], local_buffer, plen);
			if (pdiff == 0) {
				getlock(fileno(fd), F_UNLCK);
				fclose(fd);
				return 1;
			}
		} else {
			fscanf(fd, "%256s", local_buffer);
		}
	}
	getlock(fileno(fd), F_UNLCK);
	fclose(fd);
	return 0;
}

int addProfile(char * username, int username_length, char *password, int password_length) {
	int i;
	FILE *fd = fopen(loginFilePath, "a");
	if (fd == NULL) return -1;
	getlock(fileno(fd), F_WRLCK);
	for(i=0; i<username_length; i++) {
		fputc(username[i], fd);
	}
	fputc(' ', fd);
	
	for(i=0; i<password_length; i++) {
		fputc(password[i], fd);
	}
	fputc('\n', fd);
	getlock(fileno(fd), F_UNLCK);
	fclose(fd);
	return 0; 
}

int validate(char *str, int start, int len) {
	int i, val;
	for(i=start; i<start+len; i++) {
		val = (int)str[i];
		if((val < 48 && val != 46) || 
		    (val > 57 && val < 65) || 
		    (val > 90 && val < 97 && val != 95) || 
		    val > 122) {
			return 0;
		}
	}
	return 1;
}

char* int2bytes(int integer) {
	char *bytes = malloc(sizeof(char) * 4);
	int base2 = 256 * 256;
	int base3 = base2 * 256;
	
	bytes[0] = (integer / base3);
	integer -= base3 * bytes[0]; 
	bytes[1] = (integer / base2);
	integer -= base2 * bytes[1];
	bytes[2] = (integer / 256);
	integer -= 256 * bytes[2];
	bytes[3] = integer;
	return bytes;   
}

int bytes2int(char *bytes) {
	int integer = 0;
	if (bytes[3] < 0) integer += 256 + bytes[3];
	else integer += bytes[3];
	
	if (bytes[2] < 0) integer += (256 + bytes[2]) * 256;
	else integer += bytes[2] * 256;
	
	if (bytes[1] < 0) integer += (256 + bytes[1]) * 256 * 256;
	else integer += bytes[1] * 256 * 256;
	
	if (bytes[0] < 0) integer += (256 + bytes[0]) * 256 * 256 * 256 ;
	else integer += bytes[0] * 256 * 256 * 256;
	
	return integer;
}

void mystrcpy(char *dest, char *src, int len) {
	int i;
	for(i=0; i<len; i++) {
		dest[i] = src[i];
	}
}

void getlock(int fd, int type) {
    struct flock lockinfo;

    /* we'll lock the entire file */
    lockinfo.l_whence = SEEK_SET;
    lockinfo.l_start = 0;
    lockinfo.l_len = 0;

    /* keep trying until we succeed */
    while (1) {
        lockinfo.l_type = type;
	/* if we get the lock, return immediately */
        if (!fcntl(fd, F_SETLK, &lockinfo)) return;
    }
}