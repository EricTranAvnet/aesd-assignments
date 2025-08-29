#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <fcntl.h>
#include <signal.h>

#define LOCAL_HOST "127.0.0.1"
#define PORT "9000"
#define BACKLOG 10
#define FILE_PATH "/var/tmp/aesdsocketdata"

int server_fd = -1;
int client_fd = -1;
bool signal_received = false;
bool daemon_argument = false;
char *buf = NULL;

void cleanup()
{
    syslog(LOG_DEBUG, "------ CLEAN UP CALLED --------------");
      if(close(client_fd)==-1)
      {
      	  syslog(LOG_DEBUG, "------ CLOSING CLIENT CONNECTION ERROR --------------");
      }
      
      if(close(server_fd)==-1)
      {
          syslog(LOG_DEBUG, "------ CLOSING SERVER ERROR --------------");
      }
      free(buf);
}

void sig_handler(int signo)
{
    syslog(LOG_INFO, "Caught signal, exiting");
    signal_received = true;
}

int main(int argc, char *argv[])
{
    int status;
    struct addrinfo hints, *res;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;
    char ip_str[INET6_ADDRSTRLEN];

    // Allocate buffer
    size_t buf_size = 1024;
    buf = malloc(buf_size);
    if (!buf)
    {
        syslog(LOG_ERR, "MALLOC ERROR");
        return -1;
    }

    ssize_t bytes_recv;
    char *newline_ptr;

    // Check daemon argument
    for (int i = 1; i < argc; i++) 
    {
        if (strcmp(argv[i], "-d") == 0) 
        {
            daemon_argument = true;
        }
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Handle SIGINT and SIGTERM
    struct sigaction sa;
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // No SA_RESTART
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
	 
 
    // Cleaning file
    remove(FILE_PATH);

    // Socket hints
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    syslog(LOG_DEBUG, "------ STARTING CODE --------------");	

    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) 
    {
        syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(status));
        cleanup();
        return -1;
    }

    // Create socket
    server_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_fd == -1) 
    {
        syslog(LOG_ERR, "socket: %s", strerror(errno));
        freeaddrinfo(res);
        cleanup();
        return -1;
    }
    syslog(LOG_DEBUG, "------ FD SOCKET OK --------------");
    
    int allow_reusable = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &allow_reusable, sizeof(int)) == -1) 
    {
        syslog(LOG_ERR, "setsockopt: %s", strerror(errno));
        freeaddrinfo(res);
        cleanup();
        return -1;
    }
    syslog(LOG_DEBUG, "------ REUSABLE SOCKET OK --------------");

    if (bind(server_fd, res->ai_addr, res->ai_addrlen) == -1) 
    {
        syslog(LOG_ERR, "bind: %s", strerror(errno));
        freeaddrinfo(res);
        cleanup();
        return -1;
    }
    syslog(LOG_DEBUG, "------ BINDING SOCKET OK --------------");
    freeaddrinfo(res);

    // Daemonize if requested
    if (daemon_argument)
    {
        pid_t pid = fork();
        if (pid < 0)
        {
            syslog(LOG_ERR, "fork failed: %s", strerror(errno));
            cleanup();
            return -1;
        }
        else if (pid > 0)
        {
            // Parent exits
           // waitpid(pid,&status,0);
            exit(0);
        }
        // Child continues
        setsid();
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }


//// IN CHILD
    setsid();
    syslog(LOG_DEBUG, "In child... executing code");	

    // Listen
    if (listen(server_fd, BACKLOG) == -1) 
    {
        syslog(LOG_ERR, "listen: %s", strerror(errno));
        cleanup();
        return -1;
    }
    syslog(LOG_DEBUG, "Socket listening on port %s ...", PORT);

    while (!signal_received)
    {
        addr_size = sizeof client_addr;
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd == -1) 
        {
            if (errno == EINTR) continue; // Interrupted by signal
            syslog(LOG_ERR, "accept: %s", strerror(errno));
            continue;
        }

        size_t buf_used = 0;

        struct sockaddr_in *s = (struct sockaddr_in *)&client_addr;
        inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof ip_str);
        syslog(LOG_INFO, "Accepted connection from %s", ip_str);

        // Receive until client closes connection
        while ((bytes_recv = recv(client_fd, buf + buf_used, buf_size - buf_used - 1, 0)) > 0)
        {
            buf_used += bytes_recv;
            buf[buf_used] = '\0';

            // Expand buffer if needed
            if (buf_used >= buf_size - 1)
            {
                buf_size *= 2;
                char *new_buf = realloc(buf, buf_size);
                if (!new_buf)
                {
                    syslog(LOG_ERR, "realloc failed");
                    cleanup();
                    exit(1);
                }
                buf = new_buf;
            }

            // Process each newline-terminated packet
            while ((newline_ptr = strchr(buf, '\n')) != NULL)
            {
                size_t packet_len = newline_ptr - buf + 1;
 
                int file_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
                if (file_fd == -1)
                {
                    syslog(LOG_ERR, "open file failed: %s", strerror(errno));
                    break; // skip this packet
                }
		syslog(LOG_DEBUG, "File has opened...");
		 
                if (write(file_fd, buf, packet_len) != (ssize_t)packet_len)
                {
                    syslog(LOG_ERR, "write failed: %s", strerror(errno));
                    close(file_fd);
                    break; 
                }
                fsync(file_fd);
                close(file_fd); 
                syslog(LOG_DEBUG, "Write has been successfull");

                // Send full file back to client
                file_fd = open(FILE_PATH, O_RDONLY);
                if (file_fd != -1)
                {
                    char send_buf[1024];
                    ssize_t read_bytes;
                    while ((read_bytes = read(file_fd, send_buf, sizeof(send_buf))) > 0)
                    {
                        if (send(client_fd, send_buf, read_bytes, 0) == -1)
                        {
                            syslog(LOG_ERR, "send: %s", strerror(errno));
                            break;
                        }
                    }
                    close(file_fd);
                }

                // Shift remaining buffer
                memmove(buf, newline_ptr + 1, buf_used - packet_len);
                buf_used -= packet_len;
                buf[buf_used] = '\0';
            }
        }
        
        // After recv() loop ends:
	if (buf_used > 0) {
    		int file_fd = open(FILE_PATH, O_WRONLY | O_CREAT | O_APPEND, 0644);
    		if (file_fd != -1) 
    		{
			if (write(file_fd, buf, buf_used) != (ssize_t)buf_used) 
			{
		    		syslog(LOG_ERR, "write failed: %s", strerror(errno));
			}
			fsync(file_fd);
			close(file_fd);
			buf_used = 0;
    		}
	}	


        close(client_fd);
        client_fd = -1;
    }

    cleanup();
    closelog();
    return 0;
}

