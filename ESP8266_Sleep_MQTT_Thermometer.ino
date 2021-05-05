#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DallasTemperature.h>
#include <Ticker.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/*
выводит данные в топики:
	ESP"ChipId"/adc  		        //показания АЦП
		   /rssi 		        //качество сигнала WI-FI
        	   /uptime 			//время активности за цикл 
	           /wifi 		        //ID подключеной WI-FI сети
	           /sleeped 		        //заданое время сна
	           /vbat 		        //напряжение батареи
	           /dsw"номер датчика"	    	//температура с датчиков 
принимает данные в топиках:
	ESP"ChipId"/sleep 			//задает время сна
*/
//***Парраметры***//

#define ONE_WIRE_BUS 2     		 // Шина датчика температуры с подтяжеой 4,7к на VCC
#define DS18B20_pin  5			 // Пин питания DS18B20
int ssid_quantity = 3;			 // Количество перебираемых WI-FI сетей
const char* ssid[] = {	"WIFI1", 	 // имя точек доступа вашей домашней Wi-Fi сети
			"WIFI2",
			"WIFI3"
		     }; 

const char* pass[] = {	"pass1", // пароли доступа вашей домашней Wi-Fi сети
			"pass2",
			"pass3"
		     };   
const char *mqtt_server = "x.x.x.x";    // ip адес сервер MQTT броккера
const int mqtt_port = 1883;             // порт MQTT броккера
const char *mqtt_user = "user";         // логин MQTT броккера
const char *mqtt_pass = "user";         // пароль MQTT броккера
const char *host = "ESP8266_Temp";      // идентификатор в локальной сети                                                              
int sleep = 10; 		  	// таймер сна в минутах , необходимо соеденить GPIO16 и RESET
int lowsleep = 60;			// таймер сна для низкого заряда 
float TempDS[11];   			// массив хранения температуры c рахных датчиков 
float V_min = 3.10;			// напряжение сабатывания датчика ниского заряда 
float ADC_coefficient = 4.64; 		// коэффициент АЦП VREF * ((DIV_R1 + DIV_R2) / DIV_R2)
//****************//





float Vbat;
int sleeping;
int inDS = 0; 				// индекс датчика в массиве температур
int count = 6;          		// счетчик таймера для ежеминутной отправки данных
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
	String payload = pub.payload_string();       // читаем топики, если что скармливаем переменной  
	if(String(pub.topic()) == EspTopic+"/sleep") sleep = payload.toInt(), Serial.print ("Установка таймера сна на: "), Serial.print (sleep), Serial.println (" минут.");
}

void MqttSend()
{
	inDS = 0;
	sensors.requestTemperatures();              // читаем температуру с трех датчиков
	Serial.print("Работает модуля: "), Serial.println(EspTopic);
	while (inDS < 10)
  	{
    	TempDS[inDS] = sensors.getTempCByIndex(inDS);        // читаем температуру
    	if (TempDS[inDS] == -127.00) break;                     // пока не доберемся до неподключенного датчика
    	inDS++;
  	} 
  	for (int i=0; i < inDS; i++)
  	{
    	client.publish(EspTopic+"/dsw"+ String(i),   String(TempDS[i]));
    	Serial.print("Температура "), Serial.print(EspTopic), Serial.print("/dsw"), Serial.print(String(i)), Serial.print(" "), Serial.println(String(TempDS[i]));
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


  	Serial.print("Данные отправлены, IP Адрес: "),         Serial.println(WiFi.localIP());
  	if (sleep != 0)
  	{
  		client.loop();
  		delay(1000);
  		if (Vbat < V_min )
  		{
  			Serial.print("Низкий заряд батареи, засыпаю на "), Serial.print(sleeping), Serial.println(" мин.");
  		}
  		else
  		{
  			Serial.print("Засыпаю на "), Serial.print(sleeping), Serial.println(" мин.");	
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
            .set_auth(mqtt_user, mqtt_pass)))// Читаем эти топики
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
  	Serial.print("Загрузка модуля: "), Serial.println(EspTopic);
  	//float voltage = (float)analogRead(0) * VREF * ((DIV_R1 + DIV_R2) / DIV_R2) / 1024;
  	adc = analogRead(A0);
  	Serial.print("adc"), Serial.println(adc);
  	Vbat = (adc * ADC_coefficient) / 1024;

  	Serial.print("Заряд батареи: "), Serial.print(Vbat), Serial.println(" вольт");
  	
  
  	sensors.begin();
  	MDNS.begin(host);
  	httpUpdater.setup(&httpServer);
  	httpServer.begin();
  	MDNS.addService("http", "tcp", 80);
  	pinMode(DS18B20_pin, OUTPUT);
  	digitalWrite(DS18B20_pin, HIGH);       //  пин для подключения запитки датчика DS18B20
  	flipper.attach(1, flip);
}

void flip()
{
  	count--, Serial.print("Счетчик: "), Serial.println(count);
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
  httpServer.handleClient();      // проверяем http запрос
  ConMqtt();                      // проверяем соеденение и подписываемся на топики
  
  if (count <= 0) flipper.detach(), MqttSend(), count = 60, flipper.attach(1, flip);  // замеряем и отправляем данные
}
