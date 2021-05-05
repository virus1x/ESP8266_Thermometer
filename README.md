# ESP8266_Thermometer
MQTT клиент для контроля темератуы на базе ESP8266 и DS18b20. Просыпается через задание заданое время отравляет темературы и засыпает обратно. 
Для экономии энергии питание DS18b20 берется с вывода ESP, не желательно подключать так более 10 датчиков. 
* [Temperature-Control](https://github.com/milesburton/Arduino-Temperature-Control-Library)
* [OneWire](https://github.com/PaulStoffregen/OneWire)
* [PubSubClient](https://github.com/Imroy/pubsubclient)
