#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

int main(int argc, char *argv[])
{
	int error = 0;

	openlog(NULL,0,LOG_USER);
	if (argc <3)
	{
		syslog(LOG_ERR, "Invalid number of arguments");
		closelog();
		return 1;
	}

	const char *file_path = argv[1];
	const char *write_str = argv[2];

	syslog(LOG_DEBUG, "Trying to write %s to %s", write_str,file_path);

	FILE *file_to_write = fopen (file_path, "w");

	if (file_to_write == NULL)
	{
		syslog(LOG_ERR, "Error opening file %s", file_path);
		closelog();
		return 1;
	}
	error = fputs(write_str, file_to_write);

	if (error == EOF)
	{

		syslog(LOG_ERR, "Error writing file %s", file_path);
		closelog();
		return 1;
	}

	syslog(LOG_DEBUG, "Sucessfully write %s to %s", write_str,file_path);
	fclose(file_to_write);
	closelog();
	return 0;

}
