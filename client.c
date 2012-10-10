#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>

//server, buffer and port number
struct hostent *server;  
char *buffer;
int portno;

//utility functions
char* int2bytes(int);
int bytes2int(char*);
void mystrcpy(char *, char *, int); 
int cli_connect();

//functions that takes input from user
char* getUsername(int*);
char* getPassword(int*);
char* getMusicName(int*);
char* getMusicFileName(int*);

void doOperation();
void printGeneralOperations();
void doRegistrationOperation();
void doBrowseOperation();
void printPutOperations();
void displayMaster();
void getbyMID(int, int);
void getbyrange();
void getbyrandom();
int getbiggestMID(int);
void printMusicName(int); 


int main(int argc, char *argv[]){    
	
	if (argc < 3) {
	  	printf("ERROR,  hostname and port aren't given\n"); 
		exit(0);    
	} 
	
	buffer = malloc(sizeof(char) * 1024);
	if (buffer == NULL) {
		printf("ERROR, buffer couldn't be allocated\n");
		exit(0);
	} 
	
	portno = atoi(argv[2]);
	if (portno < 1024) {
		printf("ERROR, portno must be bigger than 1023");
		exit(0);
	}
	
	server = gethostbyname(argv[1]);
	if(server == NULL) {
		printf("ERROR, there is no host with specified hostname: %s", argv[1]);
		exit(0);
	}
	printf("Welcome to the SMS Client\n");
	doOperation();
	
	return 0;
} 

void doOperation() {
	int c = 0;
	while(1) {
		while(1) {
			printGeneralOperations();
			scanf("%d", &c);
			if (c == 1 || c == 2 || c == 3 || c == 4) break;
		}
	
		if (c == 1) {
			doRegistrationOperation();
		} else if (c == 2) {
			doBrowseOperation();
		} else if (c == 3) {
			printPutOperations();
		} else {
			exit(0);
		}
	}
}

void printGeneralOperations() {
	printf("\n");
	printf("\tPress 1 : Registration\n");
	printf("\tPress 2 : Browse\n");
	printf("\tPress 3 : Put\n");
	printf("\tPress 4 : Exit\n");
}

void doRegistrationOperation() {
	
	int i, 
		n, 
		username_length, 
		password_length, 
		sockfd;
	
	char req, 
		*temp,
		*username,
		*password;
		
	//user name is taken from the user
	username = getUsername(&username_length);
	if (username == NULL) {
		printf("ERROR, username couldn't be read\n");
		return;
	}
	
	//password is taken from the user
	password = getPassword(&password_length);
	if(password == NULL) {
		printf("ERROR, password couldn't be read\n");
		return;
	}
	
	i=0;
	sockfd = cli_connect();
	while(sockfd < 0 && i<2){
		i++;
		sockfd = cli_connect();
	}
	if (sockfd < 0) {
		printf("ERROR, Connection is tried 3 times but couldn't be established\n");
		free(username);
		free(password);
		return;
	}
	printf("INFO, Connection is established\n");
	
	//set request operation type
	req = '0';
	n = send(sockfd, &req, 1, 0);
	if (n < 0) {
		printf("ERROR, request-operation type couldn't be written to the socket\n");
		free(username);
		free(password);
		close(sockfd);
		return;
	}
	
	//set username length
	temp = int2bytes(username_length);
	n = send(sockfd, temp, 4, 0);
	free(temp);
	if (n < 0) {
		printf("ERROR, request-user name length couldn't be written to the socket\n");
		free(username);
		free(password);
		close(sockfd);
		return;
	}

	//set username
	n = send(sockfd, username, username_length, 0);
	if (n < 0) {
		printf("ERROR, request-user name couldn't be written to the socket\n");
		free(username);
		free(password);
		close(sockfd);
		return;
	}
		
	//set password length
	temp = int2bytes(password_length);
	n = send(sockfd, temp, 4, 0);
	free(temp);
	if (n < 0) {
		printf("ERROR, request-password length couldn't be written to the socket\n");
		free(username);
		free(password);
		close(sockfd);
		return;
	}
	
	//set password
	n = send(sockfd, password, password_length, 0);
	if (n < 0) {
		printf("ERROR, request-password couldn't be written to the socket\n");
		free(username);
		free(password);
		close(sockfd);
		return;
	}
	
	printf("INFO, request is successfully sent\n");
	
	n = recv(sockfd, &req, 1, 0);
	if (n < 0) {
		printf("ERROR, response-type couldn't be read from the socket\n");
		free(username);
		free(password);
		close(sockfd);
		return;
	}
	
	switch(req) {
		case '0' : printf("Registration is successfully completed\n"); break;
		case '1' : printf("Server returned error\n"); break;
		case '2' : printf("Username contains invalid characters: %s\n", username); break;
		case '3' : printf("Password contains invalid characters: %s\n", password); break;
		case '4' : printf("Weak Password\n"); break;
		case '5' : printf("Username already exits, change username\n"); break;
		default : printf("Unspecified Response\n"); break;
	}
	
	free(username);
	free(password);	
	close(sockfd);
}

void printBrowseSpecificOperations() {
	printf("\n");
	printf("\tPress 1 : Display the content of the master file\n");
	printf("\tPress 2 : Get by MID\n");
	printf("\tPress 3 : Get by MID range\n");
	printf("\tPress 4 : Get by random\n");
	printf("\tPress 5 : Get the biggest MID\n");
	printf("\tPress 6 : Go top menu\n");
}

void doBrowseOperation() {
	int c = 0;
	while(1) {
		while(1) {
			printBrowseSpecificOperations();
			scanf("%d", &c);
			if (c == 1 || c == 2 || c == 3 || c == 4 || c == 5 || c == 6) break;
		}
	
		if (c == 1) {
			displayMaster();
		} else if (c == 2) {
			getbyMID(-1, 1);
		} else if (c == 3) {
			getbyrange();
		} else if (c == 4) {
			getbyrandom();
		} else if (c == 5) { 
			getbiggestMID(1);
		} else {
			doOperation();
		}
	}

}

void displayMaster() {
	int i, 
		  n, 
		  buffer_length,
		  musicname_length, 
		  music_id,
		  sockfd;
	
	char	req;
	
	i = 0;
	sockfd = cli_connect();
	while (sockfd < 0 && i < 2) {
		sockfd = cli_connect();
	}
	if (sockfd < 0) {
		printf("ERROR, Connection is tried 3 times but couldn't be set up\n");
		return;
	}
	printf("INFO, Connection is established\n");
	
	//set request
	req = '1';
	n = send(sockfd, &req, 1, 0);
	if (n < 0) {
		printf("ERROR, request-operation type couldn't be written to the socket\n");
		close(sockfd);
		return;
	}
	printf("INFO, Request is successfully sent\n");
	
	//get response
	n = recv(sockfd, buffer, 1, 0);
	if (n < 0) {
		printf("ERROR, response-type couldn't be read from the socket\n");
		close(sockfd);
		return;
	}
	
	//interpret the response
	if(buffer[0] == '1') {
		printf("ERROR, response-type is error, something unexpected may have happened in the server\n");
	} else {
		n = recv(sockfd, buffer, 4, 0);
		if (n < 0) {
			printf("ERROR, response-length couldn't be read from the socket\n");
			close(sockfd);
			return;
		}
		buffer_length = bytes2int(buffer);
		
		if(buffer_length == 0) {
			printf("There are music files at the server\n");
			close(sockfd);
			return;
		}
		
		while (buffer_length > 0) {
			n = recv(sockfd, buffer, 4, 0);
			if (n < 0) {
				printf("ERROR, response-music id couldn't be read from the socket\n");
				close(sockfd);
				return;
			}
			music_id = bytes2int(buffer);
			
			n = recv(sockfd, buffer, 4, 0);
			if (n < 0) {
				printf("ERROR, response-music name length couldn't be read from the socket\n");
				close(sockfd);
				return;
			}
			musicname_length = bytes2int(buffer);
			
			buffer_length -= (8 + musicname_length);
			
			printf("Music %d : ", music_id);
			n = recv(sockfd, buffer, musicname_length, 0);
			if (n < 0) {
				printf("ERROR, response-music name couldn't be read from the socket\n");
				return;
				close(sockfd);
			}
			printMusicName(musicname_length);
			printf("\n");
		} 
	}
	close(sockfd);
}

void printMusicName(int musicname_length) {
	int i;
	for(i=0; i<musicname_length; i++) {
		putchar(buffer[i]);
	}
}

void getbyrange() {
	int i,
		  low,
		  high,
		  biggest;
		  
	printf("Please enter the lower bound MID : ");
	scanf("%d", &low);
	
	printf("Please enter the higher bound MID : ");
	scanf("%d", &high);
	
	if(low > high) {
		i = low;
		low = high;
		high = i;
	}
	
	biggest = getbiggestMID(0);
	if (high > biggest) high = biggest;
	
	if (high > 0 && low < 1) low = 1;
	
	if(low > 0) {
		for(i=low; i<=high; i++) {
			getbyMID(i, 0);
		}
	} else {
		printf("INFO, There are no files in the specified range\n");
	}
}

void getbyMID(int mid, int choice) {
	int	i,
		n,
		sockfd,
		music_id,
		buffer_length, 
		musicname_length, 
		file_length;

	char *temp;
	FILE *fd;
	
	if (choice) {
		printf("Please enter the MID : ");
		scanf("%d", &music_id);
	} else {
		music_id = mid;
	}
	
	if (music_id < 0) {
		printf("MID starts from 1\n");
		return;
	}
	
	i = 0;
	sockfd = cli_connect();
	while (sockfd < 0 && i < 2) {
		sockfd = cli_connect();
	}
	if (sockfd < 0) {
		printf("ERROR, Connection is tried 3 times but couldn't be set up\n");
		return;
	}
	printf("INFO, Connection is established\n");
	
	memset(buffer, 1024, 0);
	//set operation type
	buffer[0] = '2';

	//set MID
	temp = int2bytes(music_id);
	mystrcpy(&buffer[1], temp, 4);
	free(temp);
	
	n = send(sockfd, buffer, 5, 0);    
	if (n < 0) {
		printf("ERROR, request-operation type and music id couldn't be written to the socket\n");
	} else {
		printf("INFO, Request is successfully sent\n");
		n = recv(sockfd, buffer, 1, 0);
		if (n < 0) {
			printf("ERROR, response-type couldn't be read from the socket\n");
			close(sockfd);	
			return;
		}
		
		//interpret the result
		if (buffer[0] == '1') {
			printf("ERROR, response-type is error, something unexpected may have happened in the server\n");
		} else if (buffer[0] == '2') {
			printf("INFO, there is no music file with specified MID : %d\n", music_id);
		} else {
			n = recv(sockfd, buffer, 4, 0);
			if (n < 0) {
				printf("ERROR, response-length couldn't be read from the socket\n");
				close(sockfd);	
				return;
			}
			buffer_length = bytes2int(buffer);
			
			n = recv(sockfd, buffer, 4, 0);
			if (n < 0) {
				printf("ERROR, response-music name length couldn't be read from the socket\n");
				close(sockfd);	
				return;
			}
			musicname_length = bytes2int(buffer);
			
			file_length = buffer_length - 4 - musicname_length;
			
			
			n = recv(sockfd, buffer, musicname_length, 0);
			if (n < 0) {
				printf("ERROR, response-music name couldn't be read from the socket\n");
				close(sockfd);	
				return;
			}
			printf("INFO, MID : %d, ", music_id);
			printMusicName(musicname_length);
			printf(" download is starting\n");
			
			sprintf(buffer, "%d", music_id);
			fd = fopen(buffer, "wb");
			if (fd == NULL) {
				printf("ERROR, file to download the music couldn't be opened\n");
				close(sockfd);	
				return;
			}
			while(file_length > 1024) {
				file_length -= 1024;
				n = recv(sockfd, buffer, 1024, 0);
				if (n < 0) {
					printf("ERROR, response-file part couldn't be read from the socket\n");
					fclose(fd);
					close(sockfd);	
					return;
				}
				for(i=0; i<1024; i++) {
					fputc(buffer[i], fd);
				}
			}
			n = recv(sockfd, buffer, file_length, 0);
			if (n < 0) {
				printf("ERROR, response-file part couldn't be read from the socket\n");
				fclose(fd);
				close(sockfd);	
				return;
			}
			for(i=0; i<file_length; i++) {
				fputc(buffer[i], fd);
			}
			fclose(fd);  
			printf("INFO, MID : %d download is successfully completed\n", music_id);					
		}
	}
	close(sockfd);	
}

void getbyrandom() {
	int	i,
			n,
			sockfd,
			music_id,
			buffer_length, 
			musicname_length, 
			file_length;

	FILE *fd;
	
	i = 0;
	sockfd = cli_connect();
	while (sockfd < 0 && i < 2) {
		sockfd = cli_connect();
	}
	if (sockfd < 0) {
		printf("ERROR, Connection is tried 3 times but couldn't be set up\n");
		return;
	}
	printf("INFO, Connection is successfully established\n");
	
	memset(buffer, 1024, 0);
	//set operation type
	buffer[0] = '3';
	n = send(sockfd, buffer, 1, 0); 
	if (n < 0) {
		printf("ERROR, request-type couldn't be written to the socket\n");
		close(sockfd);
		return;
	}
	printf("INFO, Request is successfully sent\n");   
	
	n = recv(sockfd, buffer, 1, 0);
	if (n < 0) {
		printf("ERROR, response-type couldn't be read from the socket\n");
		close(sockfd);
		return;
	}
	
	if (buffer[0] == '1') {
		printf("ERROR, response-type is error, something unexpected may have happened in the server\n");
	} else if (buffer[0] == '2') {
		printf("INFO, there are no music files at the server\n");
	} else {
		n = recv(sockfd, buffer, 4, 0);
		if (n < 0) {
			printf("ERROR, response-length couldn't be read from the socket\n");
			close(sockfd);
			return;
		}
		buffer_length = bytes2int(buffer);
		
		n = recv(sockfd, buffer, 4, 0);
		if (n < 0) {
			printf("ERROR, response-music id couldn't be read from the socket\n");
			close(sockfd);
			return;
		}
		music_id = bytes2int(buffer);
		
		n = recv(sockfd, buffer, 4, 0);
		if (n < 0) {
			printf("ERROR, response-music name length couldn't be read from the socket\n");
			close(sockfd);
			return;
		}
		musicname_length = bytes2int(buffer);
		
		file_length = buffer_length - 4 - 4 - musicname_length;
		
		
		n = recv(sockfd, buffer, musicname_length, 0);
		if (n < 0) {
			printf("ERROR, response-music name couldn't be read from the socket\n");
			close(sockfd);
			return;
		}
		printf("INFO, MID : %d, ", music_id);
		printMusicName(musicname_length);
		printf(" download is starting\n");
		
		sprintf(buffer, "%d", music_id);
		fd = fopen(buffer, "wb");
		if (fd == NULL) {
			printf("ERROR, file to download the music couldn't be opened\n");
			close(sockfd);
			return;
		}
		while(file_length > 1024) {
			file_length -= 1024;
			n = recv(sockfd, buffer, 1024, 0);
			if (n < 0) {
				printf("ERROR, response-file part couldn't be read from the socket\n");
				fclose(fd);
				close(sockfd);
				return;
			}
			for(i=0; i<1024; i++) {
				fputc(buffer[i], fd);
			}
		}
		n = recv(sockfd, buffer, file_length, 0);
		if (n < 0) {
			printf("ERROR, response-file part couldn't be read from the socket\n");
			fclose(fd);
			close(sockfd);
			return;
		}
		for(i=0; i<file_length; i++) {
			fputc(buffer[i], fd);
		} 
		fclose(fd);
		printf("INFO, MID : %d download is successfully completed\n", music_id);
				
	}
	close(sockfd);
}

int getbiggestMID(int choice) {
	int i,
		  n,
		  sockfd;
	
	memset(buffer, 1024, 0);
	//set operation type
	buffer[0] = '4';
	
	i = 0;
	sockfd = cli_connect();
	while (sockfd < 0 && i < 2) {
		sockfd = cli_connect();
	}
	if (sockfd < 0) {
		printf("ERROR, Connection is tried 3 times but couldn't be set up\n");
		return 0;
	}
	n = send(sockfd, buffer, 1, 0);    
	if (n < 0) {
		printf("ERROR, request-type couldn't be written to the socket\n");
	} else {
		if (choice) {
			printf("INFO, Request is successfully sent\n");
		}
		n = recv(sockfd, buffer, 1, 0);
		if (n < 0) {
			printf("ERROR, response-type couldn't be read from the socket\n");
			close(sockfd);
			return 0;
		}
		
		if (buffer[0] == '1') {
			printf("ERROR, response-type is error, something unexpected may have happened in the server\n");
		} else {
			n = recv(sockfd, buffer, 4, 0);
			if (n < 0) {
				printf("ERROR, response-biggest music id couldn't be read from the socket\n");
				close(sockfd);
				return 0;
			}
			if (choice) {
				printf("The biggest MID : %d\n", bytes2int(buffer));
			} else {
				close(sockfd);
				return bytes2int(buffer);
			}
		}
	}
	close(sockfd); 
	return 0;
}

void printBrowseOperations() {

	printBrowseSpecificOperations();
	
}

void printPutOperations() {
	int i, 
		  n,
		  sockfd,
		  buffer_length,
		  username_length, 
		  password_length, 
		  musicname_length, 
		  musicFileName_length,
		  musicFile_length;
		    
	char *temp,
			 *username = NULL,
			 *password = NULL,
			 *musicname = NULL,
			 *musicFileName = NULL;
	
	FILE *fd;
	
	username = getUsername(&username_length);
	if (username == NULL) {
		printf("ERROR, username couldn't be read\n");
		return;
	}
	password = getPassword(&password_length);
	if (password == NULL) {
		printf("ERROR, password couldn't be read\n");
		return;
	}
	
	musicname = getMusicName(&musicname_length);
	if (musicname == NULL) {
		printf("ERROR, music name couldn't be read\n");
		return;
	}
	
	musicFileName = getMusicFileName(&musicFileName_length);
	if (musicFileName == NULL) {
		printf("ERROR, music file name couldn't be read\n");
		return;
	}
	
	fd = fopen(musicFileName, "rb");
	if (fd == NULL) {
		printf("ERROR, music file couldn't be opened to read\n");
		free(username);
		free(password);
		free(musicname);
		return;
	}
	free(musicFileName);
	
	//get file size
	fseek(fd, 0, SEEK_END); 
	musicFile_length = ftell(fd); 
	fseek(fd, 0, SEEK_SET);
		
	i = 0;
	sockfd = cli_connect();
	while (sockfd < 0 && i < 2) {
		sockfd = cli_connect();
	}
	if (sockfd < 0) {
		printf("ERROR, Connection is tried 3 times but couldn't be set up\n");
		free(username);
		free(password);
		free(musicname);
		free(musicFileName);
		fclose(fd);
		return;
	}
	printf("INFO, Connection is successfully established\n");
	
	buffer_length = 4 + username_length + 4 + password_length + 4 + musicname_length + musicFile_length;
	
	memset(buffer, 1024, 0);
	//set operation type
	buffer[0] = '5';

	//set buffer length
	temp = int2bytes(buffer_length);
	mystrcpy(&buffer[1], temp, 4);
	free(temp);
	
	//set username length
	temp = int2bytes(username_length);
	mystrcpy(&buffer[5], temp, 4);
	free(temp);
	
	//set username
	mystrcpy(&buffer[9], username, username_length);
	free(username);
	
	//set password_length
	temp = int2bytes(password_length);
	mystrcpy(&buffer[9 + username_length], temp, 4);
	free(temp);
	
	//set password
	mystrcpy(&buffer[13 + username_length], password, password_length);
	free(password);
	
	//set music name length
	temp = int2bytes(musicname_length);
	mystrcpy(&buffer[13 + username_length + password_length], temp, 4);
	free(temp);
	
	//set music name
	mystrcpy(&buffer[17 + username_length + password_length], musicname, musicname_length);
	free(musicname);
	
	n = send(sockfd, buffer, 5 + buffer_length - musicFile_length, 0);
	if (n < 0) {
		printf("ERROR, request-type, user name, password and music name couldn't be written to the socket\n");
		fclose(fd);
		close(sockfd);
		return;
	}
	printf("INFO, meta data of file and user is successfully sent\n");
	printf("INFO, file is being uploaded\n");	
	while(musicFile_length > 1024) {
		for(i=0; i<1024; i++) {
			buffer[i] = fgetc(fd);
		}
		n = send(sockfd, buffer, 1024, 0);
		if (n < 0) {
			printf("ERROR, request-part file couldn't be written to the socket\n");
			fclose(fd);
			close(sockfd);
			return;
		}
		musicFile_length -= 1024;
	}
	for(i=0; i<musicFile_length; i++) {
		buffer[i] = fgetc(fd);
	}
	fclose(fd);
	n = send(sockfd, buffer, musicFile_length, 0);
	if (n < 0) {
		printf("ERROR, request-part file couldn't be written to the socket\n");
		close(sockfd);
		return;
	} 
	printf("INFO, file data is sent, waiting for response\n");
	
	n = recv(sockfd, buffer, 1, 0);
	if (n < 0) {
		printf("ERROR, response-type couldn't be read from the socket\n");
		close(sockfd);
		return;
	}
	if (buffer[0] == '0') {
		printf("INFO, Music file is successfully uploaded to the server\n");
	} else if (buffer[0] == '1') {
		printf("ERROR, response-type is error, something unexpected may have happened in the server\n");
	} else {
		printf("ERROR, response-type is error, server returned authentication failure\n");
	}
	
	close(sockfd);
}

void mystrcpy(char *dest, char *src, int len) {
	int i;
	for(i=0; i<len; i++) {
		dest[i] = src[i];
	}
}

char* getMusicFileName(int *musicFileName_length) {
	char *musicFileName;
	
	//use the type-ahead input
	while(fgetc(stdin) != '\n');
	
	musicFileName = malloc(sizeof(char) * 257);
	if (musicFileName == NULL) return musicFileName;
	printf("Please enter music file name (max-256) : ");
	scanf("%256[^\n]", musicFileName);
	*musicFileName_length = strlen(musicFileName);
	return musicFileName;
}

char* getMusicName(int *musicname_length) {
	char *musicname;
	
	//use the type-ahead input
	while(fgetc(stdin) != '\n');
	
	musicname = malloc(sizeof(char) * 257);
	if(musicname == NULL) return musicname;
	printf("Please enter music name (max-256) : ");
	scanf("%256[^\n]", musicname);
	*musicname_length = strlen(musicname);
	return musicname;
}

char* getUsername(int *username_length) {
	char *username;
	
	//use the type-ahead input
	while(fgetc(stdin) != '\n');
			 
	username = malloc(sizeof(char) * 257);
	if (username == NULL) return username;
	printf("Please enter username (max-256) : ");
	scanf("%256[^\n]", username);
	*username_length = strlen(username);
	return username;
}

char* getPassword(int *password_length) {
	char *password;
	
	//use the type-ahead input
	while(fgetc(stdin) != '\n');
			 
	password = malloc(sizeof(char) * 257);
	if (password == NULL) return password;
	printf("Please enter password (max-256) : ");
	scanf("%256[^\n]", password);
	*password_length = strlen(password);
	return password;
}


int cli_connect() {
	int sockfd;    
	struct sockaddr_in serv_addr;        
	
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);    
	if (sockfd < 0)  {
		printf("ERROR, socket couldn't be opened\n"); 
		return -1;   
	}  	
	
	memset((char *) &serv_addr, sizeof(serv_addr), 0);    
	serv_addr.sin_family = AF_INET;    
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	   
	serv_addr.sin_port = htons(portno);    

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {  
		printf("ERROR, connection couldn't be set up\n");
		return -1;   
	}
	return sockfd;
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