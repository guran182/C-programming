/**
 * Simplistic Web Server 
 * 
 * @author Matej Guran <matej.guran@yahoo.com>
 */ 
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

#define MAX_CONNECTION_NUM 1

/*
 * Port number - the only thing interesting in this script, isn't it?
 * 
 * Wanna <0-1024> port? You have to run this script as root! 
 */
#define PORT_NUM 1050
#define BUFFER_SIZE 8096 //8 KB is just enough for our purpose

/*
 * We have to define the function header before call 
 * No implicit declaration
 */
void send_data(int socket_descriptor);

/*
 * Error handler with ability to read the errno
 */
void error(char *msg, short err_num, int socket_fd)
{
	perror(msg);
	
	if(err_num == 0)
		(void)write(socket_fd, "HTTP/1.1 403 Forbidden\nContent-Length: 185\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Forbidden</h1>\nThe requested URL, file type or operation is not allowed on this simple static file webserver.\n</body></html>\n", 271);
	else if(err_num == 1)
		(void)write(socket_fd, "HTTP/1.1 404 Not Found\nContent-Length: 136\nConnection: close\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Not Found</h1>\nThe requested URL was not found on this server.\n</body></html>\n",224);
	else if(err_num == 2)
		(void)write(socket_fd, "HTTP/1.1 500 Internal Server Error\nContent-Length: 207\nConnection: close\nContent-Type: text/html\n\n<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1>The server encountered an unexpected condition which prevented it from fulfilling the request.</body></html>", 305);
	
	exit(1);
}

/*
 * The hearth of this socket based application 
 * 
 * DO NOT FUCK UP!!!
 */
int main (int argc, char *argv[])
{
	/*
	 * These few lines are necessary evil but it is needed 
	 * like visit dentist each year
	 */
	int socket_descriptor, 
	    new_socket_descriptor,
	    status = 0;
	    
	socklen_t  client_address_length;
	    
	pid_t pid;

	struct sockaddr_in my_address, client_address;
	
	my_address.sin_family = AF_INET;
	my_address.sin_port = htons(PORT_NUM);
	my_address.sin_addr.s_addr = INADDR_ANY;
	
	/*
	 * The magic starts here and ends with dead of process (parent/children)
	 */
	socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	
	if(socket_descriptor < 0)
	{
		error("Could not define socket: socket()", 2, socket_descriptor);
	}
	
	/*
	 * Bind the socket to port with my IP addr
	 */ 
	if(bind(socket_descriptor, (struct sockaddr *) &my_address, sizeof(my_address)) < 0)
	{
		error("Cannot bind address to socket", 2, socket_descriptor);
	}

	/*
	 * Listen to this...
	 */
	if(listen(socket_descriptor, MAX_CONNECTION_NUM))
	{
		error("Listen function problem", 2, socket_descriptor);
	}
	
	/*
	 * Process works all the time so no need to restrict it - infinitive loop
	 */
	for(;;)
	{
		/*
		 * Must be in own variable. 
		 * Look on accept syntax, pointer needed to this variable
		 */
		client_address_length = sizeof(client_address);
		
		/*
		 * Accept function is blocking that is the reason for forking
		 * And it is also making the new socket descriptor
		 */ 
		new_socket_descriptor = accept(socket_descriptor, (struct sockaddr *) &client_address, &client_address_length);

		if(new_socket_descriptor < 0)
		{
			error("Cannot accept client connection", 2, socket_descriptor);
		}

		if((pid = fork())<0)
		{
			error("Could not fork", 2, socket_descriptor);
		}
		
		/*
		 * Child process created here - let the game begin!
		 */
		if(pid == 0) //child process created here 
		{
			/*
			 * We close the preceeding socket we no need him anymore!
			 */
			close(socket_descriptor);
			
			/*
			 * We have to sleep for small amount of the so user's 
			 * Web browser can prepare for next coming data
			 */
			usleep(250); 
			
			/*
			 * Just sends data - we do not expect the return code 
			 */ 
			(void)send_data(new_socket_descriptor);
		}
		else if(pid > 0)
		{
			/*
			 * Wait for children so no Zombie processes are in process 
			 * table so they do not drain the resources
			 */
			wait(&status);
			
			/*
			 * That's all Folks - the last call for a drink tonight
			 */
			close(new_socket_descriptor);
		}	
	}
	
	return 0;
}

void send_data(int fd)
{
	/*
	 * Every string has to be terminated with '\0' that is the cause of +1 
	 */
	char buffer[BUFFER_SIZE+1];
	
	int i,
		file_size,
		array_size,
		retVal,
		file_desc;
		
	char *allowed_suffix[][2] = {
								{"image/jpeg", ".jpg"},
								{"image/png", ".png"},
								{"text/html", ".html"}
							};	
	char *content_type;
	
	memset(buffer, 0, sizeof(buffer)); //initialize the buffer array
	
	retVal = read(fd, buffer, sizeof(buffer)); //read the HTTP request

	/*
	 * HTTP request could not have zero length
	 */
	if(retVal == 0 || retVal == -1)
		error("HTTP request has zero length", 2, fd);
	
	/*
	 * Find the GET parameter first occurence
	 */
	if(strncmp("GET", buffer, 3) != 0 || !strncmp("get", buffer, 3) != 0)
		error("Corrupted HTTP header - could not find GET parameter", 2, fd);
	
	/*
	 * Find end of GET parameter
	 * 
	 * Other stuff than "GET /filename" is unuseful
	 */ 	
	for(i = 5; i < sizeof(buffer); i++)
	{
		if(buffer[i] == ' ')
		{
			buffer[i] = '\0';
			break;
		}
	}

	/*
	 * If GET has no filename just / than replace it with index.html
	 */
	if(strncmp("GET /\0", buffer, 6) == 0 || strncmp("get /\0", buffer, 6) == 0)
		strcpy(buffer, "GET /index.html");

	/*
	 * If someone want listing our server disallow it to him
	 */
	if(strcmp(&buffer[4], "/.") == 0 || strcmp(&buffer[4], "/..") == 0)
		error("Moving through directory tree is not permitted", 0, fd);

	/*
	 * Find the size of array with allowed suffixes dynamically
	 */
	array_size = sizeof(allowed_suffix)/sizeof(allowed_suffix[0]);

	/*
	 * Iterate through the array of allowed suffix and find the positive
	 * match
	 */
	for(i = 0; i < array_size; i++)
	{
		if(strstr(&buffer[5], allowed_suffix[i][1]) != NULL)
		{
			content_type = allowed_suffix[i][0];
			break;
		}
		
		if(i == (array_size-1))
			error("File with forbidden suffix used", 0, fd);
	}	

	/*
	 * If everything is alright (look over) than open the file requested
	 * by user
	 */
	if((file_desc = open(&buffer[5], O_RDONLY)) < 0)
		error("Could not open file located in buffer", 1, fd);

	/*
	 * Find the size of requested file so we can build HTTP header
	 */ 
	file_size = lseek(file_desc, (off_t)0, SEEK_END);
	
	/*
	 * We had to move cursor at the end of file to find size of file
	 * but now we need sending from top to bottom so rewind the cursor
	 */
	lseek(file_desc, (off_t)0, SEEK_SET);
	sprintf(buffer, "HTTP/1.1 200 OK\nContent-length: %d\nContent-Type:%s\n\n", file_size, content_type);
	
	/*
	 * Send header to user at first
	 */
	write(fd, buffer, strlen(buffer));

	/*
	 * Now send user the file he requested
	 */
	while((retVal = read(file_desc, buffer, BUFFER_SIZE)) > 0)
	{
		write(fd, buffer, retVal);
	}
	
	/*
	 * We had to close file descriptors so they do not live like zombies
	 */
	close(fd);	
	close(file_desc);
	
	/*
	 * Return exit status is important!
	 */ 
	exit(1);
}
