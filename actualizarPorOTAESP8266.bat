@echo off

cd  %TEMP%/arduino_build*

copy c:Actuador.ino.bin .\bin

set IP=%1

REM Controlador
REM <path espota.exe>/espota.exe -i <ip del dispositivo> -p <puerto> --auth=<calve OTA> -f <path del fichero bin/fichero.bin>
python.exe C:\Users\jlopezt\AppData\Local\Arduino15\packages\esp8266\hardware\esp8266\2.5.2/tools/espota.py -i 10.68.0.80 -p 8266 --auth=88716 -f D:\arduino\desarrollos\Sketchs\Actuador\Actuador\bin\Actuador.ino.bin 
