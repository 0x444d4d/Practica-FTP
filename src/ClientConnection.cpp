//****************************************************************************
//                         REDES Y SISTEMAS DISTRIBUIDOS
//                      
//                     2º de grado de Ingeniería Informática
//                       
//              This class processes an FTP transactions.
// 
//****************************************************************************



#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cerrno>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <locale.h>
#include <langinfo.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h> 
#include <iostream>
#include <dirent.h>

//#include <stringstream>

#include "common.h"

#include "ClientConnection.h"




const unsigned short PATH_BUFF = 4098;


ClientConnection::ClientConnection(int s) {
    int sock = (int)(s);
  
    char buffer[MAX_BUFF];

    control_socket = s;
    // Check the Linux man pages to know what fdopen does.
    fd = fdopen(s, "a+");

    if (fd == NULL) {
        std::cout << "Connection closed" << std::endl;

        fclose(fd);
        close(control_socket);
        ok = false;
        return ;
    }
    
    ok = true;
    data_socket = -1;
    parar = false;
  
};


ClientConnection::~ClientConnection() {
 	fclose(fd);
	close(control_socket); 
  
}


int ClientConnection::connect_TCP( uint32_t address,  uint16_t  port) {

     // Implement your code to define a socket here

   //char addr[25] = {address};
   printf( "Connecting to %d, %d\n", address, port );

   data_socket = socket(AF_INET, SOCK_STREAM, 0);
   if (data_socket < 0) {
       printf("Error al asignar socket a data_socket\n");
   } else {
       printf("Socket asignado correctamente\n");
   }



   struct sockaddr_in sin;
   memset( &sin, 0, sizeof(sin));
   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = address;
   sin.sin_port = htons(port);

   if ( connect(data_socket, (struct sockaddr *) &sin, sizeof(sin)) ) {
       printf("Connection stablished\n");
       return 0;
   } else {
       printf("Connection not stablished\n");
       return -1;
   }


}






void ClientConnection::stop() {
    close(data_socket);
    close(control_socket);
    parar = true;
  
}





    
#define COMMAND(cmd) strcmp(command, cmd)==0

// This method processes the requests.
// Here you should implement the actions related to the FTP commands.
// See the example for the USER command.
// If you think that you have to add other commands feel free to do so. You 
// are allowed to add auxiliary methods if necessary.

void ClientConnection::WaitForRequests() {
    //bool parar = false;
    if (!ok) {
	 return;
    } 
    
    fprintf(fd, "220 Service ready\n");
  
    while(!parar) {


 
      fscanf(fd, "%s", command);
      if (COMMAND("USER")) {
	    fscanf(fd, "%s", arg);
	    fprintf(fd, "331 User name ok, need password\n");
      }
      else if (COMMAND("PWD")) {
          char cwdirectory[PATH_BUFF]; //Tamaño maximo de path en linux.
          if (getcwd(cwdirectory, sizeof(cwdirectory) ) != nullptr) {
              fprintf(fd, "257 \"%s\"\n",cwdirectory); //REVISAR
          } else {
              //Enviar error, directorio no existe o no es accesible.
          }
          
          //Conectar con cliente y enviar cwdirectory
	   
      }
      else if (COMMAND("PASS")) {
          fscanf(fd, "%s", arg);
          if ( strcmp(arg, "pepe") ) {
              fprintf(fd, "230 Correct password\n");
          } else {
              //Error contraseña
          }
	   
      }
      else if (COMMAND("PORT")) {
          uint32_t a1, a2, a3, a4, p1, p2;
          int port, address;
          fscanf(fd, "%u, %u, %u, %u, %u, %u", &a1, &a2, &a3 ,&a4, &p1, &p2);

          port = p1 << 8;
          port = port | p2;
          
          address = a1 << 24 | a2 << 16 | a3 << 8 | a4;

          if (connect_TCP( address, port ) < 0) {
              fprintf(fd, "421 Cannot stablish connection (%d.%d.%d.%d %d, %d)\n",a1, a2, a3 , a4, p1, p2);
              ok = false;
          } else {
              fprintf(fd, "200 Ok.\n");
              ok = true;
          }
      }
      else if (COMMAND("PASV")) {
          //Revisar puerto.
          //Seleccionar puerto local.
          uint32_t address = 0;
          uint16_t port = 2122;
          uint8_t aux = 127;
          uint8_t plow = 0, phigh = 0; //parte baja y alta del puerto.
          phigh = 190;
          plow = 81;

          port = port | phigh;
          port = (port << 8 ) | plow;
          //plow = port | plow;
          //phigh = (port >> 8) | phigh;

          address = address | aux;
          address = address << 8;
          aux = 0;
          address = address | aux;
          address = address << 8;
          address = address | aux;
          address = address << 8;
          aux = 1;
          address = address | aux;
          address = address << 8;

          if (connect_TCP(address, port) ) {
              fprintf(fd, "421, cannot stablish connection\n");
          } else {
              printf("Entrando al modo pasivo\n");
              fprintf(fd, "200, Ok\n");
          }
          fprintf(fd, "227 Entering Passive Mode (127.0.0.1,190,81)\n");
      }
      else if (COMMAND("CWD")) {
          fscanf(fd, "%s", arg);
          if (!chdir(arg)) {
              //Error detectado, errno contiene el codigo
          }
	   
      }
      else if (COMMAND("STOR") ) {
          FILE* file;
          fscanf(fd, "%s", arg);
          file = fopen(arg, "r+");
          fprintf(fd, "150 File creation ok, about to open data connection\n");
          //Realizar conexión y guardar datos.
	    
      }
      else if (COMMAND("SYST")) {
          fprintf(fd, "215 UNIX Type: L8\n");
	   
      }
      else if (COMMAND("TYPE")) {
	  
      }
      else if (COMMAND("RETR")) {
	   
      }
      else if (COMMAND("QUIT")) {
	 
      }
      else if (COMMAND("LIST")) {

          fscanf(fd, "%s", arg);

          if (arg != NULL) {
              DIR *dir;
              struct dirent *ent;
              if ((dir = opendir (arg)) != NULL) {
                  while ((ent = readdir (dir)) != NULL) {
                      printf ("%s\n", ent->d_name);
                      send(data_socket, ent->d_name, 256, 0);
                  }
                  closedir (dir);
              } else {
                  printf("Error al abrir el directorio\n");
                  //return EXIT_FAILURE;
              }
          }
	
      }
      else  {
	    fprintf(fd, "502 Command not implemented.\n"); fflush(fd);
	    printf("Comando : %s %s\n", command, arg);
	    printf("Error interno del servidor\n");
	
      }
      
    }
    
    fclose(fd);

    
    return;
  
};
