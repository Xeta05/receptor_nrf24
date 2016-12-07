

/*
 * 25/11/2016
  RECEPTOR 2

  Termostato inalambrico...
  Esta es la parte receptora:
      -Recibe informacion del sensor de temperatura a traves de un enlace RF de 866MHz.
      -Cambia la temperatura objetivo segun los botones
      -Muestra la temperatura actual y la temperatura objetivo en una pantalla Oled I"C de 128x64
      -Actua sobre un rele biestable que controlara la caldera


  http://www.airspayce.com/mikem/arduino/VirtualWire
*/


#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <LowPower.h>
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
//#include <VirtualWire.h>


#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

// variables
const int UpBtn = 8;          // the number of the pushbutton pin
const int DownBtn = 9;        // the number of the pushbutton pin
const int SelectBtn = 5;      // the number of the pushbutton pin
const int RX_led =17;         // the number of the led pin
const int SetRelay=10;
const int ResetRelay=16;
int UpButtonState = 1;        // variable for reading the pushbutton status
int Last_UpButtonState = 1;   // variable for reading the pushbutton status
int DownButtonState = 1;      // variable for reading the pushbutton status
int Last_DownButtonState = 1; // variable for reading the pushbutton status
int SelectButtonState = 1;    // variable for reading the pushbutton status
enum RelayState {
  on,
  off
}relaystate= on;
int cambia_pantalla = 1;      //a 1 si hay que actualizar la pantalla
int dormir=0;                 //a 1 si podemos irnos a dormir

//Creamos un mensaje
//La constante VW_MAX_MESSAGE_LEN viene definida en la libreria
//byte message[VW_MAX_MESSAGE_LEN];
//byte messageLength = VW_MAX_MESSAGE_LEN;
float Temp_consigna=20.00;
float Temp_actual=20.00;
float New_Temp_actual=20.00;

RF24 radio(9,10);//RF24 (uint8_t _cepin, uint8_t _cspin)
const uint64_t pipe = 0xE8E8F0F0E1LL;
float radio_datos;

void SET_RELAY(){
  if (relaystate == off){
      digitalWrite( SetRelay,HIGH);
      delay (20);
      digitalWrite( SetRelay,LOW);
      relaystate= on;
      cambia_pantalla=1;
  }
}

void RESET_RELAY(){
   if (relaystate== on){
    digitalWrite(ResetRelay,HIGH);
    delay (20);
    digitalWrite(ResetRelay,LOW);
    relaystate= off;
    cambia_pantalla=1;
   }
}

void actualiza_pantalla(void){
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print("Temp. actual:");
      display.print(Temp_actual,2);
      display.println("C");
      display.setCursor(0, 40);
      display.print("Consigna:");
      display.print(Temp_consigna,2);
      display.println("C");
      display.setCursor(0, 50);
      if (relaystate == on)
      display.print("ON ");
      else
      display.print("OFF ");

      display.display();
      cambia_pantalla=0;
   }

void setup()
{
  // The following saves some extra power by disabling some
// peripherals I am not using.

// Disable the ADC by setting the ADEN bit (bit 7)  of the
// ADCSRA register to zero.
ADCSRA = ADCSRA & B01111111;

// Disable the analog comparator by setting the ACD bit
// (bit 7) of the ACSR register to one.
ACSR = B10000000;

// Disable digital input buffers on all analog input pins
// by setting bits 0-5 of the DIDR0 register to one.
// Of course, only do this if you are not using the analog
// inputs for your project.
DIDR0 = DIDR0 | B00111111;


  //Setup display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); // initialize with the I2C addr 0x3C (for the 128x64)
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.display();

    // Pin Configuration
  pinMode(UpBtn, INPUT_PULLUP);
  pinMode(DownBtn, INPUT_PULLUP);
  pinMode(SelectBtn, INPUT_PULLUP);
  pinMode(SetRelay, OUTPUT);
  pinMode(ResetRelay, OUTPUT);

  pinMode(RX_led, OUTPUT); //tambien deshabilito los led RX y TX para bajar el consumo
  digitalWrite(RX_led,HIGH);
  TXLED1;

  //Setup rf
  //vw_set_rx_pin(14);//must be BEFORE vw_setup
  //vw_setup(2000);
  //vw_rx_start();
  radio.begin();
  radio.setChannel(108);  // Above most Wifi Channels
  //radio.openWritingPipe(pipe);
  radio.openReadingPipe(1,pipe);
  radio.startListening();
 RESET_RELAY();//para empezar con el rele desactivado
}


void loop()
{
 //display.clearDisplay();
  //vw_wait_rx( );
  dormir=0;

  if (radio.available())
  {    
    radio.read(&radio_datos, sizeof(radio_datos));
    New_Temp_actual=radio_datos;
    if (!isnan(New_Temp_actual)&&(New_Temp_actual<Temp_actual+10)&&(New_Temp_actual>Temp_actual-10))//si hemos recibido un numero, y tiene sentido actualizamos la temp.
    {
         Temp_actual=New_Temp_actual;
         cambia_pantalla=1;
         dormir=1;
     }
 }
          
/*  if (vw_have_message())
  {
     if (vw_get_message(message, &messageLength))
    {
      message[messageLength] = '\0'; //for terminating the string
      New_Temp_actual = atof((char *)message);
      if (!isnan(New_Temp_actual)&&(New_Temp_actual<Temp_actual+10)&&(New_Temp_actual>Temp_actual-10))//si hemos recibido un numero, y tiene sentido actualizamos la temp.
      {
        Temp_actual=New_Temp_actual;
        cambia_pantalla=1;
        dormir=1;
       }
    }
  }*/




//y hacemos la logica del termostato

   if (Temp_actual<Temp_consigna){
      SET_RELAY();
      //display.setCursor(0, 50);
      //display.print("ON ");
    }
    else{
      RESET_RELAY();
      //display.setCursor(0, 50);
      //display.print("OFF");
    }


    //leidas de esta forma tarda mucho en hacernos caso probablemente porque este en  vw_wait_rx( );
    //Actualizamos el estado de los botones, solo cogemos pulsacion a pulsacion
UpButtonState = digitalRead(UpBtn);
  if ((UpButtonState!=Last_UpButtonState)&&(UpButtonState == LOW)) {
    Temp_consigna+=0.5;
    cambia_pantalla=1;
  }
  Last_UpButtonState=UpButtonState;
  DownButtonState = digitalRead(DownBtn);
  if ((DownButtonState!=Last_DownButtonState)&&(DownButtonState == LOW)) {
    Temp_consigna-=0.5;
    cambia_pantalla=1;
  }
   Last_DownButtonState=DownButtonState;

   //lo escribimos por pantalla si tenemos que
  if(cambia_pantalla) actualiza_pantalla();
   //y si hemos recibido el mensaje nos podemos dormir
   if (dormir==1){
     // Enter power down state for 8 s with ADC and BOD module disabled
      LowPower.powerDown(SLEEP_2S, ADC_OFF,BOD_OFF);
    }
   //display.display();

}
