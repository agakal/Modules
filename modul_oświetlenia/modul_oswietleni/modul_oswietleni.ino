#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>

// Konfiguracja odbiornika

RF24 radio(9, 10); // radio (CE,CSN)
RF24Network network(radio);  //tworzenie sieci radiowej na podstawie skonfigurowanej komunikacji
const uint16_t this_node = 03; // Adres tego modułu w (zapisany oktalnie)
const uint16_t other_node = 00; // Adres modułu sterującego

unsigned short data[4];
unsigned short light[2];
void setup() {
    Serial.begin(9600); // rozpoczęcie pracy portu szeregowego - ustwienie prędkoęci transmisji danych na 9600 BAUD
    SPI.begin(); // ustawienie wyj�ć na SCK, MOSI i CE; stanu niskiego na SCK i MOSI; wysokiego na CE 
    radio.begin(); // sprawdzenie czy został skonfigurowany interfejs SPI, ustawienie CSN jako wejście 
    network.begin(90, this_node); // (kanał, adres tego modułu)
    radio.setDataRate(RF24_2MBPS); // ustawienie prędkości transmisji radiowej
    pinMode(4, OUTPUT); // Pin D4 - odpowiedzialny za zmianę stanu na wyjściu przekaźnika 
    digitalWrite(4, LOW); // Przekaźnik ma styk NO (normalnie otwarty)

}
unsigned long lastRecvTime = 0;

void radioread()
{
    network.update(); // sprawdza czy przesłano paczki danych
    while (network.available()) // odczytuje "header" wiadomości
    {
        RF24NetworkHeader header1;
        radio.setDataRate(RF24_2MBPS);
        network.read(header1, &data, sizeof(data));
    }
}

void check() // porównanie danych z obednej i koljnej wiadomości - wykrycie zbocza naastaj�cego/opadaj�cego
{
    if ((millis() - lastRecvTime) < 5000)
    {
        data[1] = light[0];
    }
    else if (((millis() - lastRecvTime) > 5000))
    {
        data[1] = light[1];
        lastRecvTime = millis();
    }
}
void change()
{
    if (light[1] == light[0])
    {
        if (light[1] == 1) // (natężenie światła jest zbyt niskie)
        {
            digitalWrite(4, HIGH);
        }
        else if (light[1] == 0) // (natężenie światła w normie)
        {
            digitalWrite(4, LOW);
        }
    }
}

void loop()
{
    radioread();
    check();
    change();
}