
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <AccelStepper.h>
#include "DHT.h"
#include <Arduino.h>
#include "MHZ19.h"                                        
#include <SoftwareSerial.h>   

//   Czujnik CO2 MH-Z19  //
SoftwareSerial mySerial(A5, A4);  // piny 0 i 1 na komunikację UART czujnika co2
byte command[9] = { 0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79 }; // konfiguracja czujnika { bajt startu,sensor #, tryb pracy (0x866 - odczyt co2),0x00,0x00,0x00,0x00,0x00,checksum})
byte response[9]; // odczytane dane 
String ppmString = " ";
unsigned short ppm;
int CO2;

//   Czujnik temperatury i wilgotności DHT 21 //
#define DHTPIN 7 // pin D7 do odczytu danych z DHT21
#define DHTTYPE DHT21 
DHT dht(DHTPIN, DHTTYPE);
int h; // wilgotność względna
int t; // temperatura

//Silniki NEMA 17
const int stepPin = 5; 
const int dirPin = 4;
const int stepPin2 = 7;
const int dirPin2 = 6;
bool windowCheck; // zmienna pomocnicza - stan położenia okna
bool blindCheck; // zmienna pomocnicza - stan położenia rolet

//Czujnik światła
int lightcheck; // natężenie oświetlenia


// Moduł radiowy nRF24L01 
RF24 radio(9, 10); // radio (CE,CSN)
RF24Network network(radio);  //tworzenie sieci radiowej na podstawie skonfigurowanej komunikacji
const uint16_t this_node = 01; // Adres tego modułu w (zapisany oktalnie)
const uint16_t other_node = 00; // Adres modułu sterującego
unsigned short data[4]; //tablica, do której są zapisywane dane z wiadomości


void co2SET()
{
    mySerial.write(command, 9); // wysłanie polecenia do czujnika - TX
    mySerial.readBytes(response, 9); // odczytanie danych z czujnika - RX

    if (response[8] == (0xff & (~(response[1] + response[2] + response[3] + response[4] + response[5] + response[6] + response[7]) + 1))) // sprawdzenie sumy kontrolnej 
    {
        Serial.println("OK");
    }
    else
    {
        while (mySerial.available() > 0) // sprawdzenie liczby bajtów danych do odczytu z portu szeregowego - czy większe od 0
        {
            mySerial.read();
        }
    }
}

void radioSET()
{
    SPI.begin(); // ustawienie wyjść na SCK, MOSI i CE; stanu niskiego na SCK i MOSI; wysokiego na CE 
    radio.begin(); // sprawdzenie czy został skonfigurowany interfejs SPI, ustawienie CSN jako wej�cie 
    network.begin(90, this_node); // (kanał, adres tego modułu)
    radio.setDataRate(RF24_2MBPS); // ustawienie prędkości transmisji radiowej

}
void motorSET()
{
    pinMode(stepPin, OUTPUT); // stepPin -stan niski a potem stan wysoki na pinie -> 1 krok 
    pinMode(dirPin, OUTPUT); // dirpin (rolety) - stan wysoki - obrót zgodny z ruchem wskazówek zegara
    pinMode(stepPin2, OUTPUT);
    pinMode(dirPin2, OUTPUT);// dirpin (okno) - stan wysoki - obrót zgodny z ruchem wskazówek zegara
    pinMode(A2, OUTPUT); // ENABLE 1 - stan wysoki - odłączenie zasilania silników
    pinMode(A0, OUTPUT); // ENABLE 2 - stan wysoki - odłączenie zasilania silników
    digitalWrite(A0, HIGH);
    digitalWrite(A2, HIGH);
}


void setup(void)
{

    Serial.begin(9600);// rozpoczęcie pracy portu szeregowego - ustwienie prędkości transmisji danych na 9600 BAUD
    mySerial.begin(9600); //ustawienie prędkości transmisji software-owego portu szeregowego na pinach A0 i A1
    dht.begin();
    SPI.begin();
    radioSET();
    motorSET();
    co2SET();
    windowCheck = true; // okno zamknięte
    blindCheck = true; // rolety zwinięte
}


void radioREAD()
{
    network.update(); // sprawdz czy są przesyłane paczki danych 

    while (network.available()) 
    { // odczytuje header wiadomości

        RF24NetworkHeader header;
        network.read(header, &data, sizeof(data));
        /*
      Serial.print("Czy jest za gorąco? ");
      Serial.println(data[0]);
      Serial.print("Czy jest za wilgotno? ");
      Serial.println(data[1]);
      Serial.print("Czy jest za ciemno? ");
      Serial.println(data[2]);
      Serial.print("Czy stezenie CO2 jest w normie? ");
      Serial.println(data[3]);
            */
    }

}

void lightREAD()
{
    lightcheck = analogRead(A1);
}
void tempREAD() // pomiar co 2s
{
    h = dht.readHumidity();
    t = dht.readTemperature();
}
void co2READ()
{
    mySerial.write(command, 9);
    mySerial.readBytes(response, 9); // {0xFF, CM, HH, LL, TT, SS, Uh, Ul, CS}; CM - komnenda polecenia (0x89); HH/LL -wartość CO2
    ppm = (response[2] << 8) | response[3]; // złożenie wartości CO2 - przesunięcie bitowe w lewo HH i dołożenie LL
    ppmString = String(ppm); //int w string
}
void blind_down()
{
    digitalWrite(A2, LOW); // ENABLe - podanie zasilania na wyprowadzenia uzwojeń silnika
    digitalWrite(dirPin, HIGH); 
    for (int x = 0; x < 2200; x++) // 2200 - liczba kroków 
    {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(1000); // zmniejszenie wartości powoduje zwiększenie prędkości obrotowej
        digitalWrite(stepPin, LOW);
        delayMicroseconds(1000);
    }
    delay(3000);
    digitalWrite(A2, HIGH); // odłączenie zasilania
    blindCheck = false; 
}
void blind_up()
{
    digitalWrite(A2, LOW);
    digitalWrite(dirPin, LOW);
    for (int x = 0; x < 2600; x++) // liczba kroków jest większa ze względu na opuszczenie się rolet po wyłączeniu zasilania silników 
    {
        digitalWrite(stepPin, HIGH);
        delayMicroseconds(1000);
        digitalWrite(stepPin, LOW);
        delayMicroseconds(1000);
    }
    delay(3000);
    digitalWrite(A2, HIGH);
    blindCheck = true;
}
void window_close()
{
    digitalWrite(A0, LOW);
    digitalWrite(dirPin2, HIGH);
    for (int x = 0; x < 300; x++) {
        digitalWrite(stepPin2, HIGH);
        delayMicroseconds(6000);
        digitalWrite(stepPin2, LOW);
        delayMicroseconds(6000);
    }
    delay(3000);
    digitalWrite(A0, HIGH);
    windowCheck = true;
}
void window_open()
{
    digitalWrite(A0, LOW);
    digitalWrite(dirPin2, LOW);
    for (int x = 0; x < 300; x++) {
        digitalWrite(stepPin2, HIGH);
        delayMicroseconds(6000);
        digitalWrite(stepPin2, LOW);
        delayMicroseconds(6000);
    }
    delay(3000);
    digitalWrite(A0, HIGH);
    windowCheck = false;
}
void blind()
{
    if (data[2] == 1 && lightcheck > 100 && blindCheck == false) // ( natężenie oswietlenia węwnątrz i na zewnątrz jest wysokie i rolety są opuszczone)
    {
        blind_up();
        blindCheck = true;
    }
    else if (data[2] == 0 && lightcheck <= 100 && blindCheck == true) // ( natężenie oswietlenia węwnątrz i na zewnątrz jest niskie i rolety są zwinięte)
    {
        blind_down();
        blindCheck = false;
    }
}

void window()
{
    if (windowCheck == true)
    {
        while (data[3] == 1 || data[1] == 2)  // ( stężenie CO2 > 950 lub temperatura > 22)
        {
            window_open();
            windowCheck = false;
        }
    }
    else
    {
        while (data[3] == 0 || data[1] == 1) // (stężenie CO2 <700 lub temperatura < 20)
        {
            window_close();
            windowCheck = true;
        }
    }
}

void loop(void)
{
    radioREAD();
    lightREAD();
    tempREAD();
    co2READ();
    blind();
    window();

    /*Serial.print(t);
    Serial.print(" ");
    Serial.print(h);
    Serial.print(" ");
    Serial.print(lightcheck);
    Serial.println(" ");
    Serial.print(CO2);
    Serial.print(" ");
    Serial.print(Temp);
    Serial.print(" ");
    Serial.print(data[2]);
    Serial.print(" ");
    Serial.print(okno);
    Serial.println(" ");*/
}
