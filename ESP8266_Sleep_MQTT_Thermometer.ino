#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DallasTemperature.h>
#include <Ticker.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/*
������� ������ � ������:
	ESP"ChipId"/adc  				        //��������� ���
			       /rssi 				        //�������� ������� WI-FI
			       /uptime 			        //����� ���������� �� ���� 
	           /wifi 				        //ID ����������� WI-FI ����
	           /sleeped 		        //������� ����� ���
	           /vbat 				        //���������� �������
	           /dsw"����� �������"	//����������� � �������� 
��������� ������ � �������:
	ESP"ChipId"/sleep 				      //������ ����� ���
*/
//***����������***//

#define ONE_WIRE_BUS 2     		 // ���� ������� ����������� � ��������� 4,7� �� VCC
#define DS18B20_pin  5			 // ��� ������� DS18B20
int ssid_quantity = 3;			 // ���������� ������������ WI-FI �����
const char* ssid[] = {	"WIFI1", // ��� ����� ������� ����� �������� Wi-Fi ����
						"WIFI2",
						"WIFI3"
					 }; 

const char* pass[] = {	"pass1", // ������ ������� ����� �������� Wi-Fi ����
						"pass2",
						"pass3"
					 };   
const char *mqtt_server = "x.x.x.x";   // ip ���� ������ MQTT ��������
const int mqtt_port = 1883;            // ���� MQTT ��������
const char *mqtt_user = "user";        // ����� MQTT ��������
const char *mqtt_pass = "user";        // ������ MQTT ��������
const char *host = "ESP8266_Temp";     // ������������� � ��������� ����                                                              
int sleep = 10; 				// ������ ��� � ������� , ���������� ��������� GPIO16 � RESET
int lowsleep = 60;				// ������ ��� ��� ������� ������ 
float TempDS[11];   			// ������ �������� ����������� c ������ �������� 
float V_min = 3.10;				// ���������� ����������� ������� ������� ������ 
float ADC_coefficient = 4.64; 	// ����������� ��� VREF * ((DIV_R1 + DIV_R2) / DIV_R2)
//****************//





float Vbat;
int sleeping;
int inDS = 0; 				// ������ ������� � ������� ����������
int count = 6;          	// ������� ������� ��� ����������� �������� ������
String EspTopic = "";  
int ssid_count = 0; 
float adc; 

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Ticker flipper;


#define BUFFER_SIZE 100;
WiFiClient wclient;
PubSubClient client(wclient, mqtt_server, mqtt_port);
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;



void callback(const MQTT::Publish& pub)
{  
	String payload = pub.payload_string();       // ������ ������, ���� ��� ����������� ����������  
	if(String(pub.topic()) == EspTopic+"/sleep") sleep = payload.toInt(), Serial.print ("��������� ������� ��� ��: "), Serial.print (sleep), Serial.println (" �����.");
}

void MqttSend()
{
	inDS = 0;
	sensors.requestTemperatures();              // ������ ����������� � ���� ��������
	Serial.print("�������� ������: "), Serial.println(EspTopic);
	while (inDS < 10)
  	{
    	TempDS[inDS] = sensors.getTempCByIndex(inDS);        // ������ �����������
    	if (TempDS[inDS] == -127.00) break;                     // ���� �� ��������� �� ��������������� �������
    	inDS++;
  	} 
  	for (int i=0; i < inDS; i++)
  	{
    	client.publish(EspTopic+"/dsw"+ String(i),   String(TempDS[i]));
    	Serial.print("����������� "), Serial.print(EspTopic), Serial.print("/dsw"), Serial.print(String(i)), Serial.print(" "), Serial.println(String(TempDS[i]));
  	}
  	if (Vbat < V_min )
  	{
  		sleeping = lowsleep;
  	}
  	else
  	{
  		sleeping = sleep;
  	}
  
  	client.publish(EspTopic+"/adc",		String(adc));
  	client.publish(EspTopic+"/rssi",    String(WiFi.RSSI()));
  	client.publish(EspTopic+"/wifi",	String(ssid[ssid_count]));
  	client.publish(EspTopic+"/sleeped",	String(sleeping));
  	client.publish(EspTopic+"/vbat",	String(Vbat));
  	client.publish(EspTopic+"/uptime",	String(millis()/1000));


  	Serial.print("������ ����������, IP �����: "),         Serial.println(WiFi.localIP());
  	if (sleep != 0)
  	{
  		client.loop();
  		delay(1000);
  		if (Vbat < V_min )
  		{
  			Serial.print("������ ����� �������, ������� �� "), Serial.print(sleeping), Serial.println(" ���.");
  		}
  		else
  		{
  			Serial.print("������� �� "), Serial.print(sleeping), Serial.println(" ���.");	
  		}
  		ESP.deepSleep(sleeping*60000000);
	}
}

void ConMqtt() 
{
  	if (WiFi.status() == WL_CONNECTED)
  	{
  		if (!client.connected())
    	{
        	if (client.connect(MQTT::Connect(host)
            .set_auth(mqtt_user, mqtt_pass)))// ������ ��� ������
            {              
            	client.set_callback(callback);  
            	client.subscribe(EspTopic+"/sleep"); 
            }
    	}
   		if (client.connected()) client.loop();
   	}
}

void setup() 
{
  	Serial.begin(115200);
  	Serial.println("");
  	EspTopic = "ESP" + String (ESP.getChipId());
  	Serial.print("�������� ������: "), Serial.println(EspTopic);
  	//float voltage = (float)analogRead(0) * VREF * ((DIV_R1 + DIV_R2) / DIV_R2) / 1024;
  	adc = analogRead(A0);
  	Serial.print("adc"), Serial.println(adc);
  	Vbat = (adc * ADC_coefficient) / 1024;

  	Serial.print("����� �������: "), Serial.print(Vbat), Serial.println(" �����");
  	
  
  	sensors.begin();
  	MDNS.begin(host);
  	httpUpdater.setup(&httpServer);
  	httpServer.begin();
  	MDNS.addService("http", "tcp", 80);
  	pinMode(DS18B20_pin, OUTPUT);
  	digitalWrite(DS18B20_pin, HIGH);       //  ��� ��� ����������� ������� ������� DS18B20
  	flipper.attach(1, flip);
}

void flip()
{
  	count--, Serial.print("�������: "), Serial.println(count);
}

void loop()
{
	if (WiFi.status() != WL_CONNECTED) {
    	Serial.print("Connecting to ");
    	Serial.print(ssid[ssid_count]);
    	Serial.println("...");
    	WiFi.begin(ssid[ssid_count], pass[ssid_count]);

    	if (WiFi.waitForConnectResult() != WL_CONNECTED)
    	{	
    		Serial.println("ERROR");
    		count = 5;
    		ssid_count++;
    		if (ssid_count > ssid_quantity - 1)
    		{
    			ssid_count = 0;
    		} 
    		return;
    	}
    Serial.println("WiFi connected");
  	}
  httpServer.handleClient();      // ��������� http ������
  ConMqtt();                      // ��������� ���������� � ������������� �� ������
  
  if (count <= 0) flipper.detach(), MqttSend(), count = 60, flipper.attach(1, flip);  // �������� � ���������� ������
}
