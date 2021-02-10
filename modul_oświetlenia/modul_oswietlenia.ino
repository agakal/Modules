#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
RF24 radio(9, 10);
RF24Network network(radio);
const uint16_t this_node = 03;
const uint16_t other_node = 00;


unsigned short data[4];
unsigned short light[2];
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
        if (light[1] == 1)
        {
            digitalWrite(4, HIGH);
        }
        else if (light[1] == 0)
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