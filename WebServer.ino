/************************************************************************************************
Servicio                       URL                             Formato entrada Formato salida                                            Comentario                                            Ejemplo peticion                      Ejemplo respuesta
Estado de los reles            http://IPActuador/estado        N/A             <id_0>#<nombre_0>#<estado_0>|<id_1>#<nombre_1>#<estado_1> Devuelve el id de cada rele y su estado               http://IPActuador/estado              1#1|2#0
Activa rele                    http://IPActuador/activaRele    id=<id>         <id>#<estado>                                             Activa el rele indicado y devuelve el estado leido    http://IPActuador/activaRele?id=1     1|1
Desactivarele                  http://IPActuador/desactivaRele id=<id>         <id>#<estado>                                             Desactiva el rele indicado y devuelve el estado leido http://IPActuador/desactivaRele?id=0  0|0
Test                           http://IPActuador/test          N/A             HTML                                                      Verifica el estado del Actuadot   
Reinicia el actuador           http://IPActuador/restart
Informacion del Hw del sistema http://IPActuador/info
************************************************************************************************/

//Configuracion de los servicios web
#define MAX_WEB_SERVERS 2
#define IDENTIFICACION  "<h1>Modulo Actuador (entradas/salidas)</h1>"

//Includes
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//Variables globales
ESP8266WebServer server[MAX_WEB_SERVERS];
int serverActivo;

//Pueros de los servicios
unsigned int puertoWebServer[MAX_WEB_SERVERS]; //Permite configurar hasta MAX_WEB_SERVERS servidores web
//Cadenas HTML precargadas
String cabeceraHTML="<HTML><HEAD><TITLE>" + nombre_dispoisitivo + "</TITLE></HEAD><BODY>";
String pieHTML="</BODY></HTML>";
String enlaces="<TABLE>\n<CAPTION>Enlaces</CAPTION>\n<TR><TD><a href=\"info\" target=\"_blank\">Info</a></TD></TR>\n<TR><TD><a href=\"test\" target=\"_blank\">Test</a></TD></TR>\n<TR><TD><a href=\"restart\" target=\"_blank\">Restart</a></TD></TR>\n<TR><TD><a href=\"estado\" target=\"_blank\">Estado</a></TD></TR>\n<TR><TD><a href=\"estadoSalidas\" target=\"_blank\">Estado salidas</a></TD></TR>\n<TR><TD><a href=\"estadoEntradas\" target=\"_blank\">Estado entradas</a></TD></TR>\n<TR><TD><a href=\"planes\" target=\"_blank\">Planes del secuenciador</a></TD></TR></TABLE>\n"; 

/*******************************************************/
/*                                                     */ 
/* Invocado desde Loop, gestiona las peticiones web    */
/*                                                     */
/*******************************************************/
void webServer(int debug)
  {
  for(serverActivo=0;serverActivo<MAX_WEB_SERVERS;serverActivo++) 
    {
    if(puertoWebServer[serverActivo]!=0) server[serverActivo].handleClient();
    }
  }

/************************* Gestores de las diferentes URL coniguradas ******************************/
/*************************************************/
/*                                               */
/*  Pagina principal. informacion de E/S         */
/*  Enlaces a las principales funciones          */
/*                                               */
/*************************************************/  
void handleRoot() 
  {
  String cad=cabeceraHTML;
  String orden="";

  //genero la respuesta por defecto  
  cad += IDENTIFICACION;
  cad += "<BR>\n";

  //Entradas  
  cad += "<TABLE style=\"border: 2px solid black\">\n";
  cad += "<CAPTION>Entradas</CAPTION>\n";  
  for(int8_t i=0;i<MAX_ENTRADAS;i++)
    {
    if(entradas[i].configurada==CONFIGURADO) cad += "<TR><TD>" + entradas[i].nombre + "</TD><TD>" + String(entradas[i].estado) + "</TD></TR>\n";
    }
  cad += "</TABLE>\n";
  cad += "<BR>";
  
  //Salidas
  cad += "\n<TABLE style=\"border: 2px solid blue\">\n";
  cad += "<CAPTION>Salidas</CAPTION>\n";  
  for(int8_t i=0;i<MAX_RELES;i++)
    {
    if(releConfigurado(i)==CONFIGURADO)
      {      
      cad += "<TR>\n";
      cad += "<TD>" + nombreRele(i) + "</TD><TD>" + String(estadoRele(i)) + "</TD>";            

      //compruebo si esta asociada a un plan de secuenciador
      if(asociadaASecuenciador(i)!=NO_CONFIGURADO && estadoSecuenciador())//Si esa asociada a un secuenciador o el secuenciador esta on
        {
        cad += "<TD colspan=2>Secuenciador " + String(asociadaASecuenciador(i)) + "</TD>";
        }
      else
        {
	      //Enlace para activar o desactivar
	      if (estadoRele(i)==1) orden="desactiva"; else orden="activa";//para 0 y 2 (apagado y en pulso) activa
	      cad += "<TD><a href=\"" + orden + "Rele\?id=" + String(i) + "\" target=\"_self\">" + orden + " rele</a></TD>\n";  
	      //Enlace para generar un pulso
	      cad += "<TD><a href=\"pulsoRele\?id=" + String(i) + "\" target=\"_self\">Pulso</a></TD>\n";
        }      

      cad += "</TR>\n";  
      }
    }
  cad += "</TABLE>\n";
  
  //Secuenciadores
  cad += "<BR><BR>\n";
  cad += "\n<TABLE style=\"border: 2px solid blue\">\n";
  cad += "<CAPTION>Secuenciadores</CAPTION>\n";  
  for(int8_t i=0;i<MAX_PLANES;i++)
    {
    if(planConfigurado(i)==CONFIGURADO)
      {      
      cad += "<TR>\n";
      if (estadoSecuenciador()) cad += "<TD><a href=\"desactivaSecuenciador\" \" target=\"_self\">Desactiva secuenciador</TD>";            
      else cad += "<TD><a href=\"activaSecuenciador\" \" target=\"_self\">Activa secuenciador</TD>";            
      cad += "</TR>\n";  
      }
    }
  cad += "</TABLE>\n";

  //Enlaces
  cad += "<BR><BR>\n";
  cad += enlaces;
  cad += "<BR><BR>" + nombre_dispoisitivo + " . Version " + String(VERSION) + ".";
  
  server[serverActivo].send(200, "text/html", cad);
  }

/*************************************************/
/*                                               */
/*  Servicio de consulta de estado de            */
/*  las Salidas y las entradas                   */
/*  devuelve un formato json                     */
/*                                               */
/*************************************************/  
void handleEstado(void)
  {
  String cad=generaJsonEstado();
  
  server[serverActivo].send(200, "text/json", cad); 
  }  

/***************************************************/
/*                                                 */
/*  Servicio de consulta de estado de las salidas  */ 
/*  devuelve un formato json                       */
/*                                                 */
/***************************************************/  
void handleEstadoSalidas(void)
  {
  String cad=generaJsonEstadoSalidas();
  
  server[serverActivo].send(200, "text/json", cad); 
  }
  
/*****************************************************/
/*                                                   */
/*  Servicio de consulta de estado de las entradas   */
/*  devuelve un formato json                         */
/*                                                   */
/*****************************************************/  
void handleEstadoEntradas(void)
  {
  String cad=generaJsonEstadoEntradas();
  
  server[serverActivo].send(200, "text/json", cad); 
  }
  
/*********************************************/
/*                                           */
/*  Servicio de actuacion de rele            */
/*                                           */
/*********************************************/  
void handleActivaRele(void)
  {
  int8_t id=0;

  if(server[serverActivo].hasArg("id") ) 
    {
    int8_t id=server[serverActivo].arg("id").toInt();

    //activaRele(id);
    conmutaRele(id, nivelActivo, debugGlobal);

    handleRoot();
    }
    else server[serverActivo].send(404, "text/plain", "");  
  }

/*********************************************/
/*                                           */
/*  Servicio de desactivacion de rele        */
/*                                           */
/*********************************************/  
void handleDesactivaRele(void)
  {
  int8_t id=0;

  if(server[serverActivo].hasArg("id") ) 
    {
    int8_t id=server[serverActivo].arg("id").toInt();

    //desactivaRele(id);
    conmutaRele(id, !nivelActivo, debugGlobal);
    
    handleRoot();
    }
  else server[serverActivo].send(404, "text/plain", ""); 
  }

/*********************************************/
/*                                           */
/*  Servicio de pulso de rele                */
/*                                           */
/*********************************************/  
void handlePulsoRele(void)
  {
  int8_t id=0;

  if(server[serverActivo].hasArg("id") ) 
    {
    int8_t id=server[serverActivo].arg("id").toInt();

    //desactivaRele(id);
    pulsoRele(id);
    
    handleRoot();
    }
  else server[serverActivo].send(404, "text/plain", ""); 
  }

/*********************************************/
/*                                           */
/*  Servicio de representacion de los        */
/*  planes del secuenciador                  */
/*                                           */
/*********************************************/  
void handlePlanes(void)
  {
  int8_t numPlanes=getNumPlanes();  
  String cad=cabeceraHTML;
  
  cad += IDENTIFICACION; //"Modulo " + String(direccion) + " Habitacion= " + nombres[direccion];
  cad += "<h1>hay " + String(numPlanes) + " plan(es) activo(s).</h1><BR>";
  
  for(int8_t i=0;i<numPlanes;i++)
    {
    //cad += pintaPlanHTML(i);
/******************************************************************/
  cad += "<TABLE style=\"border: 2px solid black\">\n";
  cad += "<CAPTION>Plan " + String(i) + "</CAPTION>\n";  

  //Cabecera
  cad += "<tr>";
  cad += "<th>Hora</th>";
  for(int8_t i=0;i<HORAS_EN_DIA;i++) cad += "<th style=\"width:40px\">" + String(i) + "</th>";
  cad += "</tr>";

  //Cada fila es un intervalo, cada columna un hora
  int mascara=1;  
  
  for(int8_t intervalo=0;intervalo<12;intervalo++)
    {
    //Serial.printf("intervalo: %i | cad: %i\n",intervalo,cad.length());  
    cad += "<tr>";
    cad += "<td>" + String(intervalo) + ": (" + String(intervalo*5) + "-" + String(intervalo*5+4) + ")</td>";    
    for(int8_t k=0;k<HORAS_EN_DIA;k++) cad += "<td style=\"text-align:center;\">" + (planes[i].horas[k] & mascara?String(1):String(0)) + "</td>";
    cad += "</tr>";
    
    mascara<<=1;
    }  
/******************************************************************/
   
    cad += "<BR><BR>";
    }
  
  cad += pieHTML;
    
  server[serverActivo].send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Servicio para activar el secuenciador    */
/*                                           */
/*********************************************/  
void handleActivaSecuenciador(void)
  {
  activarSecuenciador();
  handleRoot();
  }

/*********************************************/
/*                                           */
/*  Servicio para desactivar el secuenciador */
/*                                           */
/*********************************************/  
void handleDesactivaSecuenciador(void)
  {
  desactivarSecuenciador();
  handleRoot();
  }

/*********************************************/
/*                                           */
/*  Servicio de test                         */
/*                                           */
/*********************************************/  
void handleTest(void)
  {
  String cad=cabeceraHTML;
  cad += IDENTIFICACION; //"Modulo " + String(direccion) + " Habitacion= " + nombres[direccion];
  
  cad += "Test OK<br>";
  cad += pieHTML;
    
  server[serverActivo].send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Resetea el dispositivo mediante          */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleReset(void)
  {
  String cad=cabeceraHTML;
  cad += IDENTIFICACION; //"Modulo " + String(direccion) + " Habitacion= " + nombres[direccion];
  
  cad += "Reseteando...<br>";
  cad += pieHTML;
    
  server[serverActivo].send(200, "text/html", cad);
  delay(100);     
  ESP.reset();
  }
  
/*********************************************/
/*                                           */
/*  Reinicia el dispositivo mediante         */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleRestart(void)
  {
  String cad=cabeceraHTML;
  cad += IDENTIFICACION;
  
  cad += "Reiniciando...<br>";
  cad += pieHTML;
    
  server[serverActivo].send(200, "text/html", cad);     
  delay(100);
  ESP.restart();
  }

/*********************************************/
/*                                           */
/*  Lee info del chipset mediante            */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleInfo(void)
  {
  String cad=cabeceraHTML;
  cad += IDENTIFICACION; //"Modulo " + String(direccion) + " Habitacion= " + nombres[direccion];

  cad+= "<BR>-----------------info logica-----------------<BR>";
  cad += "IP: " + String(getIP(debugGlobal));
  cad += "<BR>";  
  cad += "nivelActivo: " + String(nivelActivo);
  cad += "<BR>";  
  for(int8_t i=0;i<MAX_RELES;i++)
    {
    cad += "Rele " + String(i) + " nombre: " + nombreRele(i) + "| estado: " + estadoRele(i);    
    cad += "<BR>";   
    }
  cad += "-----------------------------------------------<BR>";  
  
  cad += "<BR>-----------------WiFi info-----------------<BR>";
  cad += "SSID: " + nombreSSID();
  cad += "<BR>";    
  cad += "IP: " + WiFi.localIP().toString();
  cad += "<BR>";    
  cad += "Potencia: " + String(WiFi.RSSI());
  cad += "<BR>";    
  cad += "-----------------------------------------------<BR>";    
/*
  cad += "<BR>-----------------MQTT info-----------------<BR>";
  cad += "IP broker: " + IPBroker.toString();
  cad += "<BR>";
  cad += "Puerto broker: " +   puertoBroker=0;
  cad += "<BR>";  
  cad += "Usuario: " + usuarioMQTT="";
  cad += "<BR>";  
  cad += "Password: " + passwordMQTT="";
  cad += "<BR>";  
  cad += "Topic root: " + topicRoot="";
  cad += "<BR>";  
  cad += "-----------------------------------------------<BR>";  
*/
  cad += "<BR>-----------------Hardware info-----------------<BR>";
  cad += "Vcc: " + String(ESP.getVcc());
  cad += "<BR>";  
  cad += "FreeHeap: " + String(ESP.getFreeHeap());
  cad += "<BR>";
  cad += "ChipId: " + String(ESP.getChipId());
  cad += "<BR>";  
  cad += "SdkVersion: " + String(ESP.getSdkVersion());
  cad += "<BR>";  
  cad += "CoreVersion: " + ESP.getCoreVersion();
  cad += "<BR>";  
  cad += "FullVersion: " + ESP.getFullVersion();
  cad += "<BR>";  
  cad += "BootVersion: " + String(ESP.getBootVersion());
  cad += "<BR>";  
  cad += "BootMode: " + String(ESP.getBootMode());
  cad += "<BR>";  
  cad += "CpuFreqMHz: " + String(ESP.getCpuFreqMHz());
  cad += "<BR>";  
  cad += "FlashChipId: " + String(ESP.getFlashChipId());
  cad += "<BR>";  
     //gets the actual chip size based on the flash id
  cad += "FlashChipRealSize: " + String(ESP.getFlashChipRealSize());
  cad += "<BR>";  
     //gets the size of the flash as set by the compiler
  cad += "FlashChipSize: " + String(ESP.getFlashChipSize());
  cad += "<BR>";  
  cad += "FlashChipSpeed: " + String(ESP.getFlashChipSpeed());
  cad += "<BR>";  
     //FlashMode_t ESP.getFlashChipMode());
  cad += "FlashChipSizeByChipId: " + String(ESP.getFlashChipSizeByChipId());  
  cad += "<BR>";  
  cad += "-----------------------------------------------<BR>";  

  cad += "<BR>-----------------info fileSystem-----------------<BR>";   
  FSInfo fs_info;
  if(SPIFFS.info(fs_info)) 
    {
    /*        
     struct FSInfo {
        size_t totalBytes;
        size_t usedBytes;
        size_t blockSize;
        size_t pageSize;
        size_t maxOpenFiles;
        size_t maxPathLength;
    };
     */
    cad += "totalBytes: ";
    cad += fs_info.totalBytes;
    cad += "<BR>usedBytes: ";
    cad += fs_info.usedBytes;
    cad += "<BR>blockSize: ";
    cad += fs_info.blockSize;
    cad += "<BR>pageSize: ";
    cad += fs_info.pageSize;    
    cad += "<BR>maxOpenFiles: ";
    cad += fs_info.maxOpenFiles;
    cad += "<BR>maxPathLength: ";
    cad += fs_info.maxPathLength;
    }
  else cad += "Error al leer info";
  cad += "<BR>-----------------------------------------------<BR>"; 
  
  cad += pieHTML;
  server[serverActivo].send(200, "text/html", cad);     
  }

/*********************************************/
/*                                           */
/*  Crea un fichero a traves de una          */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleCreaFichero(void)
  {
  String cad=cabeceraHTML;
  String nombreFichero="";
  String contenidoFichero="";
  boolean salvado=false;

  cad += IDENTIFICACION;//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";

  if(server[serverActivo].hasArg("nombre") && server[serverActivo].hasArg("contenido")) //si existen esos argumentos
    {
    nombreFichero=server[serverActivo].arg("nombre");
    contenidoFichero=server[serverActivo].arg("contenido");

    if(salvaFichero( nombreFichero, nombreFichero+".bak", contenidoFichero)) cad += "Fichero salvado con exito<br>";
    else cad += "No se pudo salvar el fichero<br>"; 
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  cad += pieHTML;
  server[serverActivo].send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Borra un fichero a traves de una         */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleBorraFichero(void)
  {
  String cad=cabeceraHTML;
  String nombreFichero="";
  String contenidoFichero="";

  cad += IDENTIFICACION;//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";
  
  if(server[serverActivo].hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server[serverActivo].arg("nombre");

    if(borraFichero(nombreFichero)) cad += "El fichero " + nombreFichero + " ha sido borrado.\n";
    else cad += "No sepudo borrar el fichero " + nombreFichero + ".\n"; 
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  cad += pieHTML;
  server[serverActivo].send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Lee un fichero a traves de una           */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleLeeFichero(void)
  {
  String cad=cabeceraHTML;
  String nombreFichero="";

  cad += IDENTIFICACION;//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";
  
  if(server[serverActivo].hasArg("nombre") ) //si existen esos argumentos
    {
    nombreFichero=server[serverActivo].arg("nombre");

    //inicializo el sistema de ficheros
    if (SPIFFS.begin()) 
      {
      Serial.println("---------------------------------------------------------------\nmounted file system");  
      //file exists, reading and loading
      if(!SPIFFS.exists(nombreFichero)) cad += "El fichero " + nombreFichero + " no existe.\n";
      else
        {
         File f = SPIFFS.open(nombreFichero, "r");
         if (f) 
           {
           Serial.println("Fichero abierto");
           size_t tamano_fichero=f.size();
           Serial.printf("El fichero tiene un tamaño de %i bytes.\n",tamano_fichero);
           cad += "El fichero tiene un tamaño de ";
           cad += tamano_fichero;
           cad += " bytes.<BR>";
           char buff[tamano_fichero+1];
           f.readBytes(buff,tamano_fichero);
           buff[tamano_fichero+1]=0;
           Serial.printf("El contenido del fichero es:\n******************************************\n%s\n******************************************\n",buff);
           cad += "El contenido del fichero es:<BR>";
           cad += buff;
           cad += "<BR>";
           f.close();
           }
         else cad += "Error al abrir el fichero " + nombreFichero + "<BR>";
        }  
      Serial.println("unmounted file system\n---------------------------------------------------------------");
      }//La de abrir el sistema de ficheros
    }
  else cad += "Falta el argumento <nombre de fichero>"; 

  cad += pieHTML;
  server[serverActivo].send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Lee info del FS                          */
/*  peticion HTTP                            */ 
/*                                           */
/*********************************************/  
void handleInfoFS(void)
  {
  String cad=cabeceraHTML;

  cad += IDENTIFICACION;//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";
  
  //inicializo el sistema de ficheros
  if (SPIFFS.begin()) 
    {
    Serial.println("---------------------------------------------------------------\nmounted file system");  
    FSInfo fs_info;
    if(SPIFFS.info(fs_info)) 
      {
      /*        
       struct FSInfo {
          size_t totalBytes;
          size_t usedBytes;
          size_t blockSize;
          size_t pageSize;
          size_t maxOpenFiles;
          size_t maxPathLength;
      };
       */
      cad += "totalBytes: ";
      cad += fs_info.totalBytes;
      cad += "<BR>usedBytes: ";
      cad += fs_info.usedBytes;
      cad += "<BR>blockSize: ";
      cad += fs_info.blockSize;
      cad += "<BR>pageSize: ";
      cad += fs_info.pageSize;    
      cad += "<BR>maxOpenFiles: ";
      cad += fs_info.maxOpenFiles;
      cad += "<BR>maxPathLength: ";
      cad += fs_info.maxPathLength;
      }
    else cad += "Error al leer info";

    Serial.println("unmounted file system\n---------------------------------------------------------------");
    }//La de abrir el sistema de ficheros

  cad += pieHTML;
  server[serverActivo].send(200, "text/html", cad); 
  }

/*********************************************/
/*                                           */
/*  Pagina no encontrada                     */
/*                                           */
/*********************************************/
void handleNotFound()
  {
  String message = "";//"<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";

  message = "<h1>" + String(NOMBRE_FAMILIA) + "<br></h1>";
  message += "File Not Found\n\n";
  message += "URI: ";
  message += server[serverActivo].uri();
  message += "\nMethod: ";
  message += (server[serverActivo].method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server[serverActivo].args();
  message += "\n";

  for (uint8_t i=0; i<server[serverActivo].args(); i++)
    {
    message += " " + server[serverActivo].argName(i) + ": " + server[serverActivo].arg(i) + "\n";
    }
    
  server[serverActivo].send(404, "text/html", message);
  }

/*********************************** Inicializacion y configuracion *****************************************************************/
void inicializaWebServer(void)
  {
  /*******Configuracion del Servicio Web***********/
  //configuracion por defecto
  for(int8_t i=0;i<MAX_WEB_SERVERS;i++) puertoWebServer[i]=0; //Inicializo todos a cero
  puertoWebServer[0]=80; //el primero le pongo a 80 par aasegurar uno

  //Leo la configuracion del fichero
  recuperaDatosWebServer(debugGlobal);
  /*******Configuracion del Servicio Web***********/
  
  //Inicializo los serivcios  
  for(int8_t i=0;i<MAX_WEB_SERVERS;i++)
    {
    if(puertoWebServer[i]!=0)
      {
      //Servidor en puerto PUERTO_WEBSERVER
      Serial.printf("Configurando servicio web en puerto %i\n", puertoWebServer[i]);
      //decalra las URIs a las que va a responder
      server[i].on("/", handleRoot); //Responde con la iodentificacion del modulo
      server[i].on("/estado", handleEstado); //Servicio de estdo de reles
      server[i].on("/estadoSalidas", handleEstadoSalidas); //Servicio de estdo de reles
      server[i].on("/estadoEntradas", handleEstadoEntradas); //Servicio de estdo de reles    
      server[i].on("/activaRele", handleActivaRele); //Servicio de activacion de rele
      server[i].on("/desactivaRele", handleDesactivaRele);  //Servicio de desactivacion de rele
      server[i].on("/pulsoRele", handlePulsoRele);  //Servicio de pulso de rele
      server[i].on("/planes", handlePlanes);  //Servicio de representacion del plan del secuenciador
      server[i].on("/activaSecuenciador", handleActivaSecuenciador);  //Servicio para activar el secuenciador
      server[i].on("/desactivaSecuenciador", handleDesactivaSecuenciador);  //Servicio para desactivar el secuenciador
      
      server[i].on("/test", handleTest);  //URI de test
    
      server[i].on("/reset", handleReset);  //URI de test  
      server[i].on("/restart", handleRestart);  //URI de test
      server[i].on("/info", handleInfo);  //URI de test
      
      server[i].on("/creaFichero", handleCreaFichero);  //URI de crear fichero
      server[i].on("/borraFichero", handleBorraFichero);  //URI de borrar fichero
      server[i].on("/leeFichero", handleLeeFichero);  //URI de leer fichero
      server[i].on("/infoFS", handleInfoFS);  //URI de info del FS
    
      server[i].onNotFound(handleNotFound);//pagina no encontrada
    
      server[i].begin(puertoWebServer[i]);
    
      Serial.printf("Servicio web iniciado en puerto %i\n", puertoWebServer[i]);
      }
    }
  }

void recuperaDatosWebServer(boolean debug)
  {
  String cad="";

  if (debug) Serial.println("Recupero configuracion del archivo...");
  
  if(leeFichero(WEBSERVER_CONFIG_FILE, cad)) parseaConfiguracionWebServer(cad);
  else 
    {
    Serial.println("Configuracion del servidor web por defecto");
    }
  }

/*********************************************/
/* Parsea el json leido del fichero de       */
/* configuracio Web Server                   */
/*********************************************/
boolean parseaConfiguracionWebServer(String contenido)
  {  
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(contenido.c_str());
  
  //json.printTo(Serial);
  if (json.success()) 
    {
    Serial.println("parsed json");
//******************************Parte especifica del json a leer********************************
    JsonArray& Puertos = json["Puertos"];

    int maximoPuertos=(MAX_WEB_SERVERS>Puertos.size()?Puertos.size():MAX_WEB_SERVERS);//Me quedo con el menor de los dos valores
    
    for(int8_t i=0;i<maximoPuertos;i++)
      { 
      puertoWebServer[atoi(Puertos[i]["id"])]=atoi(Puertos[i]["puerto"]);
      }
      
    //Serial.println("Puertos:"); 
    //for(int8_t i=0;i<MAX_WEB_SERVERS;i++) Serial.printf("%02i: puerto = %i\n",i,puertoWebServer[i]);
//************************************************************************************************
    return true;
    }
  return false;
  }
