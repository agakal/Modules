
#include "DHT.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_TSL2591.h"
#include <LiquidCrystal.h> 
#include <SoftwareSerial.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

//////////////////POŁĄCZENIA////////////////////////
/*
A0,A1 - MH-Z14A (tx i rx)
D8 - DHT22
D2,D3,D4,D5,D6,D7 -LCD
A4,A5-TSL2591
SCL,SDA - ccs811
*/

///   Czujnik temperatury i wilgotności DHT 22///
#define DHTPIN 8   //przypisanie pinu D8 jako odczyt z sensora
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
int h; // wilgotność względna
int t; // temperatura

///   Czujnik CO2 MH-Z14A  ///
SoftwareSerial mySerial(A1, A0);  // piny 0 i 1 na komunikację UART czujnika co2

byte command[9] = { 0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79 }; // konfiguracja czujnika { bajt startu,sensor #, tryb pracy (0x866 - odczyt co2),0x00,0x00,0x00,0x00,0x00,checksum})
byte response[9]; // odczytane dane - RX
String ppmString = " ";
unsigned short ppm;

///   Czujnik natężenia światła  TSL2591  ///
Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591); // pass do identyfikacji czujnika
uint32_t lux; // natężenie oświtlenia


///    Wyświetlacz LCD   ///
LiquidCrystal lcd(2, 3, 4, 5, 6, 7); //ustawienie pinów

///    Moduł radiowy NRF24 - TRANSMITER ///
RF24 radio(9, 10); // radio (CE,CSN)
RF24Network network(radio);  //tworzenie sieci radiowej na podstawie skonfigurowanej komunikacji
//wartości zapisane oktalnie
const uint16_t this_node = 00; // moduł sterujący 
const uint16_t node1 = 01;     // moduł okienny
const uint16_t node2 = 02;     // moduł termowentylatora
const uint16_t node3 = 03;     // moduł oświetlenia



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void luxSET()
{
    tsl.setGain(TSL2591_GAIN_MED); // Ustawienie gain - 25x gain
    tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS); // Czas ekspozycji pobierania próbki - wydłużenie dla gorszych warunków świetlnych
}

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
void LCDSET()
{
    lcd.begin(16, 2); // zwmiarowanie ekranu 
    lcd.clear();
    lcd.setCursor(0, 0); // ustawienie pozycji kursora
    lcd.print("0.0  C");
    lcd.setCursor(7, 0);
    lcd.print("     lux");
    lcd.setCursor(0, 1);
    lcd.print("0.0  %");
    lcd.setCursor(7, 1);
    lcd.print("     CO2");
}
void radioSET()
{
    SPI.begin(); // ustawienie wyjść na SCK, MOSI i CE; stanu niskiego na SCK i MOSI; wysokiego na CE 
    radio.begin(); // sprawdzenie czy został skonfigurowany interfejs SPI, ustawienie CSN jako wej�cie 
    network.begin(90, this_node); // (kanał, adres tego modułu)
    radio.setDataRate(RF24_2MBPS); // ustawienie prędkości transmisji radiowej

}

void setup()
{
    Serial.begin(9600);// rozpoczęcie pracy portu szeregowego - ustwienie prędkości transmisji danych na 9600 BAUD
    mySerial.begin(9600);
    dht.begin();
    tsl.begin();
    luxSET();
    co2SET();
    LCDSET();
    radioSET();

}
void co2READ()
{
    mySerial.write(command, 9);
    mySerial.readBytes(response, 9); // {0xFF, CM, HH, LL, TT, SS, Uh, Ul, CS}; CM - komnenda polecenia (0x89); HH/LL -wartość CO2
    ppm = (response[2] << 8) | response[3]; // złożenie wartości CO2 - przesunięcie bitowe w lewo HH i dołożenie LL
    ppmString = String(ppm); //int w string
}
void luxREAD(void)
{
    uint32_t lum = tsl.getFullLuminosity(); // pomiar podczerwieni i pełengo spektrum światłą
    uint16_t ir, full;
    ir = lum >> 16; // wyznaczenie wartość podczeriweni przez przesunięcie bitowe w prawo
    full = lum & 0xFFFF; // bitand dla pełnego spektrum
    lux = tsl.calculateLux(full, ir); // obliczenie wartości w lx z wykorzystaniem wzorów zaproponowanych przez producenta bazujących na gain-ie i czasie ekspozycji

}
void tempREAD()
{
    h = dht.readHumidity();
    t = dht.readTemperature();

}
void LCD()
{
    lcd.setCursor(0, 0);
    lcd.print(t);
    lcd.setCursor(0, 1);
    lcd.print(h);
    lcd.setCursor(7, 0);
    lcd.print(lux);
    lcd.setCursor(7, 1);
    lcd.print(ppm);
}


unsigned short data[4];
void choice()
{ 
    // wilgotność względna
    if (h > 60 && h < 40) 
    {
        data[0] = 0;
    }
    else if (h < 40)
    {
        data[0] = 1;
    }
    else
    {
        data[0] = 2;
    }
    //temperatura
    if (t >= 20.0 && t <= 22.0)
    {
        data[1] = 0;
    }
    else if (t < 20.0)
    {
        data[1] = 1;
    }
    else
    {
        data[1] = 2;
    }
    //natężenie oświetlenia
    if (lux >= 450)
    {
        data[2] = 0;
    }
    else
    {
        data[2] = 1;
    }
    // stężenie CO2
    if (ppm < 700)
    {
        data[3] = 0;
    }
    else if (ppm > 950)
    {
        data[3] = 1;
    }
}


void radioREAD()
{
    network.update();//// sprawdz czy są przesyłane paczki danych 

    /// nadawanie ///

    RF24NetworkHeader header(node1);
    bool ok = network.write(header, &data, sizeof(data)); 
    if (ok)                                              // sprawdzenie czy otrzymano pakiet ACK po 130 µs
    {
        Serial.println("ok - windowCheck");
    }
    else
    {
        Serial.println("błąd w transmisji - windowCheck");
    }

    RF24NetworkHeader header1(node2);
    bool ok1 = network.write(header1, &data[1], sizeof(data[1]));
    if (ok1)
    {
        Serial.println("ok - termowentylator");
    }
    else
    {
        Serial.println("błąd w transmisji - termowentylator");
    }

    RF24NetworkHeader header2(node3);
    bool ok2 = network.write(header2, &data[2], sizeof(data[2]));
    if (ok2)
    {
        Serial.println("ok - oświetlenie");
    }
    else
    {
        Serial.println("błąd w transmisji - oświetlenie");
    }
}



void loop()
{
    co2READ();
    tempREAD();
    luxREAD();
    LCD();
    choice();
    radioREAD();
    Serial.print(t);
    Serial.print(" ");
    Serial.print(h);
    Serial.print(" ");
    Serial.print(lux);
    Serial.print(" ");
    Serial.print(ppm);
    Serial.print(" ");
    Serial.print(data[0]);
    Serial.print(" ");
    Serial.print(data[1]);
    Serial.print(" ");
    Serial.print(data[2]);
    Serial.print(" ");
    Serial.print(data[3]);
    Serial.print(" ");
    Serial.print(data[4]);
    Serial.println(" ");
    delay(2000);
}