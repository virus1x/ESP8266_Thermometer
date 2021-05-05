# ESP8266_Thermometer
MQTT клиент для контроля темератуы на базе ESP8266 и DS18b20. Просыпается через задание заданое время отравляет темературы и засыпает обратно. 
Для экономии энергии питание DS18b20 берется с вывода ESP, не желательно подключать так более 10 датчиков. 
## Функции:
- подключение к WIFI сети из списка.
- измерение темперратуры и отравка данных MQTT брокеу.
- контроль заядада батареи. 
- два режима сна. 
## О протоколе MQTT в конкретной прошивке:
* выводит данные в топики:
  * ESP"ChipId*"/adc  	//показания АЦП 
  * ESP"ChipId*"/rssi 	//сила сигнала WI-FI
			       /uptime 			        //время активности за цикл 
	           /wifi 				        //ID подключеной WI-FI сети
	           /sleeped 		        //заданое время сна
	           /vbat 				        //напряжение батареи
	           /dsw"номер датчика"	//температура с датчиков 
принимает данные в топиках:
	ESP"ChipId"/sleep 				      //задает время сна

## Библиотека 
* [Temperature-Control](https://github.com/milesburton/Arduino-Temperature-Control-Library)
* [OneWire](https://github.com/PaulStoffregen/OneWire)
* [PubSubClient](https://github.com/Imroy/pubsubclient)
