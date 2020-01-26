#include <SPI.h>
#include <LoRa.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <AESLib.h>

int counter = 0;

// Pin donde se conecta el bus 1-Wire
const int pinDatosDQ = 4;

// Pin donde se conecta el rele
const int relePin = 17;
boolean releActivo = false;

// Instancia a las clases OneWire y DallasTemperature
OneWire oneWireObjeto(pinDatosDQ);
DallasTemperature sensorDS18B20(&oneWireObjeto);

//Clave AES128
uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

void setup() {


  //Inicialización de la terminal  
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender");

  //Configuramos el pin del rele de salida
  pinMode(relePin, OUTPUT); 
  
  //Configuramos la conexión con el DS18B20
  sensorDS18B20.begin(); 

  //Configuramos la librería LoRa con los pines CS, RST, DST
  LoRa.setPins(53, 3, 2);

  //Esperamos que este activo el módulo LoRa
  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  //Configuramos el callback que se ejecutará cuando recibamos un mensaje
  LoRa.onReceive(onReceive);

  //Activamos el modo "escucha" del LoRa.
  LoRa.receive();  
  
}

void loop() {

 // Cada 5 segundos enviaremos la temperatura al receptor
 if (runEvery(5000)) { 
   sendTemperatura();
 }
  
}

void sendTemperatura() {

    //Recuperamos la temperatura del sensor
    sensorDS18B20.requestTemperatures();

    //Cadena donde almacenaremos la temperatura convertida en un array de char
    char buffn[10]= {};  

    //Convertimos la temperatura
    dtostrf(sensorDS18B20.getTempCByIndex(0),6,2,buffn); 

    //Preparamos un buffer para almacenar el mensaje: temperatura y estado de relé
    char buffer[16]= {};  
    sprintf(buffer, "%s %12s", releActivo?"On ":"Off",buffn);

    Serial.print("Sending data: ");
    Serial.print( sizeof(buffer));
    Serial.print("  [");
    Serial.print(buffer);
    Serial.println("]");
   
    //Encriptamos el buffer con la clave definida
    aes128_enc_single(key, buffer);
    
    Serial.print("Sending encoded: ");
    Serial.print( sizeof(buffer));
    Serial.print("  ");

    for (int i=0; i<sizeof(buffer); i++)
    {
        Serial.print(buffer[i]&0xFF, HEX);
        Serial.print(" "); //separator
    }
    Serial.println();

    //Enviamos el mensaje
    LoRa.beginPacket();
    LoRa.print(buffer);
    LoRa.endPacket();
    
    delay(1000);
    
    //Volvemos a poner en escucha el módulo LoRa por si llega algún comando del receptor
    LoRa.receive();  
    
    counter++;
}

void onReceive(int packetSize) {

  //Leemos el mensaje recibido.
  String message = "";
  while (LoRa.available()) {
    message += (char)LoRa.read();
  }

  Serial.print("Message receive: ");
  Serial.println(message);
  
  //Verificamos que la orden sera "RELE" para cambiar el estado.
  if(message == "RELE") {
     if(releActivo) {
       digitalWrite(relePin, LOW);   
     } else {
       digitalWrite(relePin, HIGH);
     }
     releActivo = !releActivo;

     //Notificamos al receptor el nuevo estado del relé
     sendTemperatura();
  }
}


boolean runEvery(unsigned long interval)
{
  static unsigned long previousMillis = 0;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    return true;
  }
  return false;
}
