/***********************************************************
 *    Modulo de gestion de SNTP para ESP8266
 * 
 *    https://github.com/gmag11/NtpClient
 ***********************************************************/
//No se usan, esta libreria lo tiene hardcodeado
#define MES_CAMBIO_HORARIO_UP   3 //marzo
#define MES_CAMBIO_HORARIO_DOWN 10 //octubre
#define DOMINGO 7

#include <time.h>
#include <TimeLib.h>
#include <NtpClientLib.h>

String Semana[7]={"Domingo","Lunes","Martes","Miercoles","Jueves","Viernes","Sabado"};

int8_t timeZone = 1;             //Madrid es zona UTC+1
int8_t minutesTimeZone = 0;      //la diferencia de tiempo son horas enteras

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent;            // Last triggered event
String lastEvent="";                // Almacena el ultimo evento

const char* NTP_SERVER = "pool.ntp.org";//"ntp.mydomain.com";
//const char* TZ_INFO    = "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/02:00:00";
/*****************************************************
The format of TZ values recognized by tzset() is as follows:
stdoffset[dst[offset][,rule]]copy to clipboard

std and dst
Indicate no fewer than three, but not more than TZNAME_MAX, bytes that are the designation for the standard (std) and daylight savings (dst) time zones. If more than TZNAME_MAX 
bytes are specified for std or dst, tzset() truncates to TZNAME_MAX bytes. Only std is required; if dst is missing, daylight savings time does not apply in this locale. Uppercase 
and lowercase letters are explicitly allowed. Any character except a leading colon (:) or digits, the comma (,), the minus (-), the plus (+), and the NULL character are permitted 
to appear in these fields. The meaning of these letters and characters is unspecified.

offset
Indicates the value that must be added to the local time to arrive at Coordinated Universal Time (UTC). offset has the form: hh[:mm[:ss]] The minutes (mm) and seconds (ss) are 
optional. The hour (hh) is required and may be a single digit. offset following std is required. If no offset follows dst, daylight savings time is assumed to be 1 hour ahead of 
standard time. One or more digits may be used; the value is always interpreted as a decimal number. The hour must be between 0 and 24; minutes and seconds, if present, between 0 
and 59. The difference between standard time offset and daylight savings time offset must be greater than or equal to 0, but the difference may not be greater than 24 hours. Use 
of values outside of these ranges causes tzset() to use the LC_TOD category rather than the TZ environment variable for time conversion information. An offset preceded by a minus 
(-) indicates a time zone east of the Prime Meridian. A plus (+) preceding offset is optional and indicates the time zone west of the Prime Meridian.

rule
Indicates when to change to and back from daylight savings time. The rule has the form: date[/time],date[/time]
The first date describes when the change from standard to daylight savings time occurs and the second date describes when the change back happens. Each time field describes when, 
in current local time, the change to the other time is made.

The format of date must be one of the following:
Jn: The Julian day n (1≤n≤365). Leap days are not counted. That is, in all years—including leap years—February 28 is day 59 and March 1 is day 60. It is impossible to explicitly 
refer to the occasional February 29.
n: The zero-based Julian day (0≤n≤365). Leap days are counted, and it is possible to refer to February 29.
Mm.n.d
The dth day (0≤d≤6) of week n of month m of the year (1≤n≤5, and 1≤m≤12, where week 5 means “ the last d day in month m,” which may occur in either the fourth or the fifth 
week). Week 1 is the first week in which the dth day occurs. Day zero is Sunday.
The time has the same format as offset except that no leading sign, minus (-) or plus (+), is allowed. The default, if time is not given, is 02:00:00.

If dst is specified and rule is not specified by TZ or in LC_TOD category, the default for the daylight savings time start date is M4.1.0 and for the daylight savings time end 
date is M10.5.0.
******************************************************/


struct tm timeinfo;
/***********************
Member  Type  Meaning Range
tm_sec  int seconds after the minute  0-61*
tm_min  int minutes after the hour  0-59
tm_hour int hours since midnight  0-23
tm_mday int day of the month  1-31
tm_mon  int months since January  0-11
tm_year int years since 1900  
tm_wday int days since Sunday 0-6
tm_yday int days since January 1  0-365
tm_isdst int Daylight Saving Time flag 
***********************/

/******************eventos: 
typedef enum {
    timeSyncd, // Time successfully got from NTP server
    noResponse, // No response from server
    invalidAddress // Address not reachable
} NTPSyncEvent_t;      
*******************************************************/

/****************************************************/
/*                                                  */
/*    Inicializa el modulo de SNTP                  */ 
/*                                                  */
/****************************************************/
void inicializaReloj(void)
  {
  //Callback para recibir los eventos SNTP  
  NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
      ntpEvent = event;
      syncEventTriggered = true;
  });
  
  //Inicio el SNTP
  Serial.println("Iniciando objeto NTP");
  NTP.begin(NTP_SERVER, timeZone, true, minutesTimeZone);
  NTP.setDayLight(true);//En España hay horario de verano 
  NTP.setInterval(60*5,3600*24);//ajustado cada 5 minutos hasta sincronizar, luego un vez al dia
  
  Serial.printf("Intervalo de actualizacion inicial: %i segundos | Intervalo de actualizacion normal: %i segundos\n",NTP.getShortInterval(),NTP.getInterval()); 
  Serial.printf("Hora actual: %s\n", NTP.getTimeDateString().c_str()); 
  }

/****************************************************/
/*                                                  */
/*         Procesado de eventos SNTP                */
/*             Solo pinta info                      */ 
/*                                                  */
/****************************************************/
void actualizaSNTP(boolean debug)
  {
  if (syncEventTriggered) 
    {
    if(debug) Serial.printf("Evento recibido %i | hora: %s\n",ntpEvent,NTP.getTimeDateString().c_str());

    if (ntpEvent) 
      {
      lastEvent = "Time Sync error: ";
      if (ntpEvent == noResponse) lastEvent += "Servidor NTP no alcanzable";
      else if (ntpEvent == invalidAddress) lastEvent += "Direccion del servidor NTP invalida";
      } 
    else lastEvent = "Sincronizada hora por NTP: " + NTP.getTimeDateString(NTP.getLastNTPSync ());
  
    lastEvent = "Evento: " + String(ntpEvent) + " - " + lastEvent;
    Serial.printf("Descripcion del evento: %s\n", lastEvent.c_str());    

    syncEventTriggered = false;
    }
  }
  
/*******************************************************************/
/* Cambio horario:                                                 */
/* En España, esta medida se lleva adoptando desde 1974, aunque    */
/* la última regulación a la que nos hemos adaptado ha llegado     */
/* de la mano de la directiva Europea 2000/84, que entre otras     */
/* cosas unifica los días en los que se producen los cambios de    */
/* hora en todos los países de la Unión Europea, siendo estos      */
/* el último Domingo de Marzo y Octubre, respectivamente           */
/*                                                                 */
/* La funcion devuelve las horas que hay que sumar al reloj oficial*/
/*******************************************************************/
int8_t cambioHorario(void) 
  {
  getLocalTime(&timeinfo);
  return timeinfo.tm_isdst;
  }

int8_t diaSemana(void) 
  {
  getLocalTime(&timeinfo);
  if (timeinfo.tm_wday==0) return 7;
  return timeinfo.tm_wday;  
  }
 
/***********************************/
/*        Solo para ordenes        */
/***********************************/
void imprimeDatosReloj(void)
  {
  getLocalTime(&timeinfo);
  Serial.printf("Fecha: %02i-%02i-%02i Hora: %02i:%02i:%02i\n",timeinfo.tm_mday,timeinfo.tm_mon+1,timeinfo.tm_year+1900,timeinfo.tm_hour,timeinfo.tm_min,timeinfo.tm_sec);    
  Serial.printf("Día de la semana (Hoy): %s\n", Semana[timeinfo.tm_wday].c_str());
  }

/***************************************************************/
/*                                                             */
/*  Retorna si es por la tarde en formato booleano             */
/*                                                             */
/***************************************************************/
bool relojPM() 
  {
  getLocalTime(&timeinfo);
  if(timeinfo.tm_hour>11) return true;
  return false;
  }  

/***************************************************************/
/*                                                             */
/*  Devuleve la hora del sistema                               */
/*                                                             */
/***************************************************************/
int hora() 
  {
  getLocalTime(&timeinfo); 
  return (timeinfo.tm_hour);
  } 

/***************************************************************/
/*                                                             */
/*  Devuleve el minuto del sistema                             */
/*                                                             */
/***************************************************************/
int minuto() 
  {
  getLocalTime(&timeinfo); 
  return (timeinfo.tm_min);
  } 

/***************************************************************/
/*                                                             */
/*  Devuleve el segundo del sistema                            */
/*                                                             */
/***************************************************************/
int segundo() 
  {  
  getLocalTime(&timeinfo); 
  return (timeinfo.tm_sec);
  } 
  
/***************************************************************/
/*                                                             */
/*  Devuleve el dia del sistema                                */
/*                                                             */
/***************************************************************/
int dia() 
  {
  getLocalTime(&timeinfo); 
  return (timeinfo.tm_mday);
  } 

/***************************************************************/
/*                                                             */
/*  Devuleve el mes del sistema                                */
/*                                                             */
/***************************************************************/
int mes() 
  {
  getLocalTime(&timeinfo); 
  return (timeinfo.tm_mon+1);
  } 

/***************************************************************/
/*                                                             */
/*  Devuleve el año del sistema                                */
/*                                                             */
/***************************************************************/
int anno() 
  {
  getLocalTime(&timeinfo); 
  return (timeinfo.tm_year+1900);
  } 

  
/***************************************************************/
/*                                                             */
/*  Genera una cadena con la hora en formato HH:MM             */
/*                                                             */
/***************************************************************/
String getHora(void)
  {
  String cad="";  
  char ss[3];

  getLocalTime(&timeinfo);
  
  dtostrf(timeinfo.tm_hour,2,0,ss);ss[2]=0;
  if(timeinfo.tm_hour<10) ss[0]='0';
  cad=String(ss);

  dtostrf(timeinfo.tm_min,2,0,ss);ss[2]=0;
  if(timeinfo.tm_min<10)ss[0]='0';
  cad += ":" + String(ss);

  return cad;    
  }

/***************************************************************/
/*                                                             */
/*  Genera un acadena con la fecha en formato DD/MM/AAAA       */
/*                                                             */
/***************************************************************/
String getFecha(void)
  {
  String cad=""; 
  char ss[5];

  getLocalTime(&timeinfo);
  
  dtostrf(timeinfo.tm_mday,2,0,ss);ss[2]=0;
  if(timeinfo.tm_mday<10) ss[0]='0';
  cad=String(ss);

  dtostrf(timeinfo.tm_mon+1,2,0,ss);ss[2]=0;
  if(timeinfo.tm_mon+1<10)ss[0]='0';
  cad += "/" + String(ss);

  dtostrf(timeinfo.tm_year+1900,4,0,ss);ss[4]=0;
  cad += "/" + String(ss); 
  
  return cad;   
  }  

/***************************************************************/
/*                                                             */
/*  Rellena una struct tm con los datos horarios               */
/*
Member  Type  Meaning Range
tm_sec  int seconds after the minute  0-61*
tm_min  int minutes after the hour  0-59
tm_hour int hours since midnight  0-23
tm_mday int day of the month  1-31
tm_mon  int months since January  0-11
tm_year int years since 1900  
tm_wday int days since Sunday 0-6
tm_yday int days since January 1  0-365
tm_isdst int Daylight Saving Time flag
                                                               */
/*                                                             */
/***************************************************************/
 void getLocalTime(struct tm* timeInfo)  
  {
  timeInfo->tm_sec=second();
  timeInfo->tm_min=minute();
  timeInfo->tm_hour=hour();
  timeInfo->tm_mday=day();
  timeInfo->tm_mon=month()-1;//de 0 a 11 por compatibilidad
  timeInfo->tm_year=year()-1900;//años despues de 1900 por compatibilidad
  timeInfo->tm_wday=weekday()-1; //de 0 a 6 por compatibilidad, weekday retorna 1-7 con 1 sunday
  timeInfo->tm_yday=0;
  timeInfo->tm_isdst=NTP.isSummerTime();
  }
