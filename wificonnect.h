#ifndef WIFICONNECT_H
#define WIFICONNECT_H

#if defined(ESP8266)
//#pragma message("Compiling for ESP8266")
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <DNSServer.h>
#else

//#pragma message("Compiling for ESP32")
#include <WiFi.h>
#include <HTTPClient.h>
#include <DNSServer.h>
#endif
#include <Ticker.h>



// ==== Конфигурационные константы ====
// ⏱️ Тайминги (миллисекунды)
#define STA_RETRY_INTERVAL_MS       180000  // Проверка подключения в режиме AP (3 мин)
#define DNS_PROCESS_INTERVAL_MS     30      // Опрос DNS-сервера портала
#define START_SEARCH_INTERVAL_MS    30000   // Поиск сети с префиксом (30 сек)
#define CONNECT_POLL_DELAY_MS       500     // Пауза между попытками в isConnect()

// 📡 Настройки точки доступа (AP)
#define AP_DEFAULT_IP               IPAddress(192, 168, 4, 1)
#define AP_DEFAULT_SUBNET           IPAddress(255, 255, 255, 0)
#define AP_CHANNEL_MIN              1
#define AP_CHANNEL_MAX              12      // random(min, max) не включает max → 1..11
#define AP_DNS_PORT                 53

// 🔍 Прочее
#define BEST_RSSI_INIT              -1000   // Стартовое значение для поиска лучшей точки

typedef std::function<void()> WIFICONNECTCb;

#if defined(ESP8266)
class WIFICONNECT : public ESP8266WiFiClass {
#else
class WIFICONNECT : public WiFiClass {
#endif
public:
// Инициализация
void init(String ssid, String ssidPass, String ssidAP, String ssidApPass, String ssidStart),
init(String ssid, String ssidPass, String ssidAP, String ssidApPass),
initStaAndAP(),
initHiddenAP(),
initIP(String staticIP, String ip, String subnet, String getway),
start(), // Подключить к роутеру, если не удачно - вход в режим AP
startSTA(), // Подключится к роутеру
isConnect(), // Попыки подключится к роуетеру
anotherDev(), // Подключение другого модуля к роутеру используя данные этого
startAP(), // Запустить точку доступа
loop(), // Обработка DNS сервера в режиме AP
stop(), // Отключить WiFi
searchStart(String ssidStart),
setCallback(WIFICONNECTCb pcb),
setHostname(String hostname),
beginRestore(uint8_t n),
endRestore(),
restartSTA(), // Подключится к роутеру
restoreCallback(WIFICONNECTCb abc);

String scan(boolean Async), // Получить список сетей в эфире
network(), // Список сетей json
findBestBSSID(const String& targetSSID),
StringIP(), // Получить IP адрес
StringGatewayIP(),
StringSubnetMask(),
getURL(String urls, boolean norequest),
getURL(String urls); // Отправить GET запрос по адресу

uint8_t getChannel();


boolean modeSTA(), // Вернуть режим WiFi
modeETH(),
modeAP(),
ssidOff(),
ssidStartOn(), // Вернуть признак стартовая сеть найден
ssidOn(), // Вернуть признак стандартная сеть найден
passError(); // Ошибка пароля
private:
	static WIFICONNECT* instance; // Статическая переменная для хранения экземпляра класса
    // ==== ESP32 ====
#if defined(ESP32)
    static void handleWiFiEventStatic(WiFiEvent_t event);
    void handleWiFiEvent(WiFiEvent_t event);

#endif

    // ==== ESP8266 ====
#if defined(ESP8266)
    static void handleWiFiConnect(const WiFiEventStationModeConnected& event);
    static void handleWiFiDisconnect(const WiFiEventStationModeDisconnected& event);
    static void handleWiFiGotIP(const WiFiEventStationModeGotIP& event);
    void handleWiFiEvent(int event);
#endif


bool parseIPAddress(const String& ipStr, IPAddress& ip);
bool parseBSSID(const String& bssidStr, uint8_t* output) ;
void saveRestartCon(uint8_t n),
onStart(); // Подключится к внешнему устройству
uint8_t _cAttempts = 60, // Количество попыток подключения 120 = 60 попыток
led, // Cветодиод индикации процесса подключения
_Channel;
String _ssid, // SSID сети
_ssidPass, // Пароль SSID
_ssidAP, // SSID для точки доступа
_ssidApPass, // Пароль точки доступа
_ssidStart, // Стартовая сеть найдена
_staticIP, // Флаг статический IP
_ip, // IP адрес
_subnet, // Маска сети
_getway, // Шлюз
_dns, // DNS сервер
_emptyS, // Пустая строка
_hostname, //
_net; // Список сетей в формате JSON
uint32_t calculateCRC32(const uint8_t *data, size_t length);
WIFICONNECTCb _pcb;
WIFICONNECTCb _abc;

DNSServer dnsServer;


boolean _StaAp = false, // false если не Sta
_staOff = false, // запускать поиск основной сеть 
_staTest = false, // запускать поиск основной сеть 
_startTest = false, // запускать поиск стартовой сеть 
_StaAndAp = false, // включить гибридный режим для Now
_hidden = false, // скрыть сеть
_apOn = false, // AP режим включен
_ssidStartOn = false, // стартовая сеть найдена сканированием
_ssidOn = false, // сеть найдена сканированием
_ssidFound = false, // основная сеть найдена перезагрузи
_ssidPassEr = false; // Ошибка пароля
const String ssidS = "ssid",
ssidPassS = "ssidPass",
checkboxIPS = "checkboxIP",
ipS = "ip",
subnetS = "subnet",
getwayS = "getway",
dnsS = "dns",
ssidAPS = "ssidAP",
ssidApPassS = "ssidApPass",
ssidStartS = "ssidStart";
unsigned long previousMillis = 0,
previousStartTest = 0,
previousStaTest = 0;

struct {
uint32_t crc32;
byte data[4];
} rtcData;
};

#endif // WC_H