#include <SPI.h>
#include <Wire.h> 
#include <LoRa.h>
#include <LiquidCrystal_I2C.h>
#include <AESLib.h>

//Pin analógico A0
const int buttonPin = A0;

//Clave AES128
uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

//Instancia de la clase LiquidCrystal_I2C 
LiquidCrystal_I2C lcd(0x27, 16, 2); 


void setup() {

  //Inicialización de la terminal  
  Serial.begin(9600);
  while (!Serial);

  //Configuramos la entrada analógica para el botón
  pinMode(buttonPin, INPUT); 
 // digitalWrite(buttonPin, HIGH);

  //Inicializamos el LCD.
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("LoRa Receiver");    
 
  
  //Configuramos la librería LoRa con los pines CS, RST, DST
  LoRa.setPins(53, 3, 2);
  
  //Esperamos que este activo el módulo LoRa
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    lcd.setCursor(0, 0);
    lcd.print("Starting LoRa failed!");   
    while (1);
  }

  //Activamos el modo "escucha" del LoRa.
  LoRa.receive();
}

void loop() {

  //Se intenta parsear los mensajes recibidos por el módulo
  int packetSize = LoRa.parsePacket();
 
  //Si tenemos un nuevo mensaje
  if (packetSize) {
    Serial.print("Packet size:");
    Serial.println(packetSize);
      
  
    //Preparamos el array para almacenarlo
    char message[16] = {};
   
    //Recivimos el paquete y lo almacenamos
    while (LoRa.available()) {
      for (int i = 0; i < 16; i++)  {
       // Message = Message + (char)LoRa.read();
        message[i] = (char)LoRa.read();
      }
    }

 
    Serial.print("Received encoded: ");
    for (int i=0; i<sizeof(message); i++)
    {
        Serial.print(message[i]&0xFF, HEX);
        Serial.print(" "); //separator
    }
    Serial.println();
        
    //Se decodifica el mensaje
    aes128_dec_single(key, message);

    Serial.print("Deccoded:");
    Serial.println(message);
         
    //Si es un mensaje correcto comenzará por la letra 'O'
    if(message[0] == 'O') {
       //Se pinta en el LCD.
       lcd.setCursor(0, 1);
       lcd.print("            ");
       lcd.setCursor(0, 1);
       lcd.print(message);
    }
    
    //Mostramos por el LCD la fuerza de la señal recibida.
    lcd.setCursor(0, 0);
    lcd.print("                  ");
    lcd.setCursor(0, 0);
    lcd.print("RSSI ");
    lcd.print(LoRa.packetRssi());   

    Serial.print("RSSI ");
    Serial.println(LoRa.packetRssi());
    
  }
  //Leemos el botón por si el usuario lo ha pulsado
  int buttonState = analogRead(buttonPin);

  //Si el valor leido es menor de 100 enviamos mensaje al emisor
  if(buttonState  < 100) {

    //Informamos por LCD de la acción  
    lcd.setCursor(0, 0);
    lcd.print("Enviando !!      ");

    Serial.println("Enviando pulsacion");

    //Trasmitimos el comando 'RELE'
    LoRa.beginPacket();               
    LoRa.print("RELE");                 
    LoRa.endPacket();
    delay(2000);
     
    lcd.setCursor(0, 0);
    lcd.print("                  ");

    //Volvemos a poner el módulo LoRa en escucha
    LoRa.receive();      
  }
}
