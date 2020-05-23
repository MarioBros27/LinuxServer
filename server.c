//Este codigo esta basado en el el servidor nweb de niggel Griffiths
//https://www.ibm.com/developerworks/system/library/es-nweb/index.html
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE 8096
#define MAX_CONNECTIONS 1000

/* Handler de peticiones http */
void childServer(int socketfd)
{
	int response_file_fd;
	long peticion, longitud_respuesta;
	static char buffer[BUFSIZE + 1];


	peticion = read(socketfd, buffer, BUFSIZE); /* Leer peticion*/
	if (peticion <= 0)
	{
		char bufferWrong[BUFSIZE + 1];
		sprintf(bufferWrong,"HTTP/1.1 403 Forbidden\nContent-Type: text/html\n\n<html><head>\n<title>403 Forbidden</title>\n</head><body>\n<h1>Error 403</h1>\n<body>Algo fallo en el servidor :(\n</body></html>\n");
		write(socketfd, bufferWrong, strlen(bufferWrong));
		exit(3);
	}
	if (peticion > 0 && peticion < BUFSIZE) /* La peticion contiene algo y cabe en el buffer */
		buffer[peticion] = 0;				/* Agregar el 0 que anuncia terminacion del buffer */
	else
		buffer[0] = 0;

	for (int i = 4; i < BUFSIZE; i++)
	{ /* agregar un 0 en el segundo espacio despues de get para ignorar basura en el url */
		if (buffer[i] == ' ')
		{
			buffer[i] = 0;
			break;
		}
	}
	/* Establecer direccion default tc2025.html si despues del get / no encuentra nada, en este caso el 0*/
	if (!strncmp(&buffer[0], "GET /\0", 6) || !strncmp(&buffer[0], "get /\0", 6))
		strcpy(buffer, "GET /tc2025.html");

	/* abrir archivo html si existe*/
	if ((response_file_fd = open(&buffer[5], O_RDONLY)) == -1)
	{
		char bufferWrong[BUFSIZE + 1];
		sprintf(bufferWrong, "HTTP/1.1 404 Not Found\nContent-Type: text/html\n\n<html><head>\n<title>404 Not Found</title>\n</head><body>\n<h1>Error 404</h1>\nNo se encontro el archivo :(\n</body></html>\n");
		write(socketfd, bufferWrong, strlen(bufferWrong));
		exit(3);
	}

	longitud_respuesta = (long)lseek(response_file_fd, (off_t)0, SEEK_END); //posicion al ultimo byte
	lseek(response_file_fd, (off_t)0, SEEK_SET);							//POsicion al primer byte

	/*mandar la respuesta al request, como lo entinedo escribir en el file del socket*/
	sprintf(buffer, "HTTP/1.1 200 OK\nServer: marioServer\nContent-Length: %ld\nConnection: close\nContent-Type: text/html\n\n", longitud_respuesta);
	write(socketfd, buffer, strlen(buffer));

	/* "send file in 8KB block - last block may be smaller" */
	while ((peticion = read(response_file_fd, buffer, BUFSIZE)) > 0)
	{
		write(socketfd, buffer, peticion);
	}
	
	close(socketfd);
	exit(0);
}

int main(int argc, char **argv)
{
	int port, pid, mock_socketfd, socketfd;
	socklen_t socket_length;
	static struct sockaddr_in client_socket_address;
	static struct sockaddr_in server_socket_address;
	/* Validacion de datos de entrada al ejecutar el programa*/
	if (argc != 2)
	{
		printf("escribalo bien: ./server {directorio donde esta el html} &\n");
		exit(1);
	}
	if (chdir(argv[1]) == -1)
	{
		printf("El directorio esta mal; ejemplo /home/astrid/proyectoDeMario\n");
		exit(1);
	}
	

	/* Hacer el proceso un servicio que corre en el background (Daemon) */
	if (fork() != 0)
		return 0;			  /* Matar al padre para que el hijo corra en background*/
	signal(SIGCHLD, SIG_IGN); /*Ignorar que el proceso hijo ha terminado */
	signal(SIGHUP, SIG_IGN);  /* Ignorar que cierre la terminal */
	for (int i = sysconf(_SC_OPEN_MAX); i >= 0; i--)
		close(i); /* cerrar archivos abiertos por el proceso padre */
	setpgrp();	  /* te saca del grupo del proceso? */

	/* Configurar el socket*/
	if ((mock_socketfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) ///AF_INET =  ipv4; SOCK_STREAM = normal communication
		exit(1);
	port = 8080;
	server_socket_address.sin_family = AF_INET;//Conexion simple
	server_socket_address.sin_addr.s_addr = inet_addr("0.0.0.0");//IP
	server_socket_address.sin_port = htons(port);//Puerto 8080

	if (bind(mock_socketfd, (struct sockaddr *)&server_socket_address, sizeof(server_socket_address)) < 0)
		exit(1);
	if (listen(mock_socketfd, MAX_CONNECTIONS) < 0)
		exit(1);
	/*Escuchar requests al servidor*/
	while (1)
	{
		socket_length = sizeof(client_socket_address);
		/* a partir de aqui hace el protocolo http*/
		if ((socketfd = accept(mock_socketfd, (struct sockaddr *)&client_socket_address, &socket_length)) < 0)
			exit(5);
		if ((pid = fork()) < 0)
		{
			exit(1);
		}
		else
		{
			if (pid == 0)
			{
				close(mock_socketfd);
				childServer(socketfd);//Subrutina que hace el http request
			}
			else
			{
				close(socketfd);
			}
		}
	}
}
