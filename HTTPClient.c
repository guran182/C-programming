/**
 * Simplistic Web Client 
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

#define MAXBUF  10000

/*
 * Error handler
 * 
 * works with errno number 
 */
void error(char *msg)
{
	perror(msg); 
	exit(1);
}

/*
 * The hearth of whole HTTP Client
 */
int main(int Count, char *Strings[])
{   int sockfd, 
		bytes_read, // use exclusively in recv
		char_num = 0, 
		i = 0, 
		header_found = 0;
    char buffer[MAXBUF+1]; //there is the null termination character, when dealing with strings, so +1
    char *ptr_char = &Strings[2][(int)(strrchr(Strings[2], '/')-Strings[2]+1)], // pretty straighforward here, isn't it?
		 *ptr; // used for finding character position in array manipulation
    struct addrinfo hints, 
					*res;
	FILE *fp_header, *fp_file; // two files there. One header second one your requested file
	memset(buffer, 0, sizeof(buffer)); // clean field the war is coming
	
	/*
	 * You! YEAH YOU MAN!!!
	 * Write it right or get away!
	 */
	if(Count < 3)
	{
		printf("The right format is: ./<this_program> <URL> <PATH>\n");
		printf("Example: ./a.out  lm.uniza.sk  /obrazky/facebook.jpg\n");
		exit(1);
    }
    
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
        error("Socket");
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/*
	 * No need for IP address
	 * 
	 * just type the URL and this resolves right IP through DNS
	 */ 
	getaddrinfo(Strings[1], "80", &hints, &res);

	/*
	 * Creates the socket and connects to remote Web server
	 */ 
	sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if ( connect(sockfd, res->ai_addr, res->ai_addrlen) != 0 )
        error("Connect");

	/*
	 * Copies Request HTTP header and sends it to server
	 */
    sprintf(buffer, "GET %s HTTP/1.0\nHost: %s\n\n", Strings[2], Strings[1]);
    send(sockfd, buffer, strlen(buffer), 0);
	
	/*
	 * File pointer with name from third CMD argument
	 * 
	 * @see char *ptr_char
	 */
	fp_file = fopen(ptr_char, "wb"); 
	
    /*
     * Here comes magic!
     */
    while((bytes_read = recv(sockfd, buffer, (size_t)MAXBUF, 0))>0)
    {
        if(header_found == 0 && (ptr = strstr(buffer, "\r\n\r\n")))
		{
			fp_header = fopen("header.txt", "w+");
			char_num = ptr - buffer; // finds the position where header meets the useful body
			
			for(i=0; i<=char_num; i++)
				fprintf(fp_header, "%c", buffer[i]); // reads header char by char

			/*
			 * All bytes that do not belongs header belongs to body 
			 * 
			 * look at +4 there. We need skip the \r\n\r\n. 
			 * The '\n' or '\r' is one character!
			 */
			fwrite(&buffer[char_num+4], 1, (int)(bytes_read-(char_num+4)), fp_file);
			header_found = 1; // we found header we do not need repeat this again
        }
        else
        {
			fwrite(buffer, 1, bytes_read, fp_file); // if we've got the header just fill the binary file 
		}
		
		memset(buffer, 0, sizeof(buffer));
    }

    /*
     * Clean up, you poor boy!
     */
    fclose(fp_file);
    fclose(fp_header);
    close(sockfd);
    close(bytes_read);
    
    return 0;
}
