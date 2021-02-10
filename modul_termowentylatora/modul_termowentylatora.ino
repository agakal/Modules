#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
RF24 radio(9, 10);
RF24Network network(radio);
const uint16_t this_node = 02;
const uint16_t other_node = 00;


unsigned short data[4];
unsigned short temp[2];
void setup() {
    SPI.begin();
    Serial.begin(9600);
    radio.begin();
    network.begin(90, this_node);
    radio.setDataRate(RF24_2MBPS);
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);

}
unsigned long lastRecvTime = 0;

void radioread()
{
    network.update();
    while (network.available())
    {
        RF24NetworkHeader header1;
        radio.setDataRate(RF24_2MBPS);
        network.read(header1, &data, sizeof(data));
    }
}

void check()
{
    if ((millis() - lastRecvTime) < 5000)
    {
        data[1] = temp[0];
    }
    else if (((millis() - lastRecvTime) > 5000))
    {
        data[1] = temp[1];
        lastRecvTime = millis();
    }
}
void change()
{
    if (temp[1] == temp[0])
    {
        if (temp[1] == 1)
        {
            digitalWrite(4, HIGH);
        }
        else if (temp[1] == 0 || temp[1] == 2)
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