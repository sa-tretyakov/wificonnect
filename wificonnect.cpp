#include "wificonnect.h"


WIFICONNECT* WIFICONNECT::instance = nullptr;

// ==== Для ESP32 ====
#if defined(ESP32) || defined(ESP32S2) || defined(ESP32S3)
void WIFICONNECT::handleWiFiEventStatic(WiFiEvent_t event) {
    if (instance) instance->handleWiFiEvent(event);
}

void WIFICONNECT::handleWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            _StaAp = false; // ✅ Связь прервалась — сбрасываем флаг
            Serial.println("Disconnected from WiFi access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            _StaAp = true;
            if (_staTest) ESP.restart();
            break;
        default:
            break;
    }
}
#endif // ESP32

// ==== Для ESP8266 ====
#if defined(ESP8266)
#define CONNECTED_8266     0
#define DISCONNECTED_8266  1
#define GOT_IP_8266        2

void WIFICONNECT::handleWiFiConnect(const WiFiEventStationModeConnected& event) {
    if (instance) instance->handleWiFiEvent(CONNECTED_8266);
}

void WIFICONNECT::handleWiFiDisconnect(const WiFiEventStationModeDisconnected& event) {
    if (instance) instance->handleWiFiEvent(DISCONNECTED_8266);
}

void WIFICONNECT::handleWiFiGotIP(const WiFiEventStationModeGotIP& event) {
    if (instance) instance->handleWiFiEvent(GOT_IP_8266);
}

void WIFICONNECT::handleWiFiEvent(int event) {
    switch (event) {
		//Serial.println(event);
        case CONNECTED_8266:
            Serial.println("Connected to access point");
            break;
        case DISCONNECTED_8266:
            if (!_ssidPassEr) Serial.println("Disconnected from WiFi access point");
            _ssidPassEr = true;
            break;
        case GOT_IP_8266:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            if (_staTest) ESP.restart();
            break;
    }
}
#else

#endif // ESP8266

// ==== Общие методы класса ====

void WIFICONNECT::init(String ssid, String ssidPass, String ssidAP, String ssidApPass) {
    WiFi.mode(WIFI_OFF);
    _ssid = ssid;
    _ssidPass = ssidPass;
    _ssidAP = ssidAP;
    _ssidApPass = ssidApPass;
    _ssidStart = "";
}

void WIFICONNECT::init(String ssid, String ssidPass, String ssidAP, String ssidApPass, String ssidStart) {
    WiFi.mode(WIFI_OFF);
    _ssid = ssid;
    _ssidPass = ssidPass;
    _ssidAP = ssidAP;
    _ssidApPass = ssidApPass;
    _ssidStart = ssidStart;
}

void WIFICONNECT::initIP(String staticIP, String ip, String subnet, String getway) {
    _staticIP = staticIP;
    _ip = ip;
    _subnet = subnet;
    _getway = getway;
}

void WIFICONNECT::setCallback(WIFICONNECTCb pcb) {
    _pcb = pcb;
}

void WIFICONNECT::start() {
    instance = this;
#if defined(ESP8266)
    WiFi.onStationModeConnected(handleWiFiConnect);
    WiFi.onStationModeDisconnected(handleWiFiDisconnect);
    WiFi.onStationModeGotIP(handleWiFiGotIP);
#else
    WiFi.onEvent(handleWiFiEventStatic);
#endif

    startSTA();
    if (!_StaAp) {
        startAP();
    }
}

void WIFICONNECT::setHostname(String hostname) {
    _hostname = hostname;
}

void WIFICONNECT::startSTA() {
	 _StaAp = false;
    if (_ssid.isEmpty()) return;

    if (!_hostname.isEmpty()) {
#if defined(ESP8266)
        WiFi.hostname(_hostname);
#else
        WiFi.setHostname(_hostname.c_str());
#endif
    }	
    if (_staticIP == "1") {
        IPAddress staticIP, staticGateway, staticSubnet;
        if (parseIPAddress(_ip, staticIP) &&
            parseIPAddress(_getway, staticGateway) &&
            parseIPAddress(_subnet, staticSubnet)) {
            WiFi.config(staticIP, staticGateway, staticSubnet);
        }
    }

    WiFi.mode(WIFI_OFF);
	String bestBSSID = findBestBSSID(_ssid);
    if (bestBSSID.isEmpty()) {return; }
#if defined(ESP8266)
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
#else
    WiFi.setSleep(false);
#endif
    WiFi.mode(WIFI_STA);
	WiFi.persistent(false);
#ifdef CONFIG_IDF_TARGET_ESP32S2
WiFi.begin(_ssid.c_str(), _ssidPass.c_str());
#else
	    uint8_t bssid[6];
		parseBSSID(bestBSSID, bssid);
WiFi.begin(_ssid.c_str(), _ssidPass.c_str(), 0, bssid, true);
//WiFi.begin(_ssid.c_str(), _ssidPass.c_str());
#endif
	
    isConnect();
}

bool WIFICONNECT::parseBSSID(const String& bssidStr, uint8_t* output) {
    int values[6];
    int matches = sscanf(bssidStr.c_str(), "%x:%x:%x:%x:%x:%x",
                         &values[0], &values[1], &values[2],
                         &values[3], &values[4], &values[5]);

    if (matches != 6) {
        return false;
    }

    for (int i = 0; i < 6; ++i) {
        output[i] = static_cast<uint8_t>(values[i]);
    }

    return true;
}


void WIFICONNECT::initStaAndAP() {
    _StaAndAp = true;
}

void WIFICONNECT::initHiddenAP() {
    _hidden = true;
}

void WIFICONNECT::startAP() {
    IPAddress apIP = AP_DEFAULT_IP;
    IPAddress staticGateway = AP_DEFAULT_IP;
    IPAddress staticSubnet = AP_DEFAULT_SUBNET;	
    WiFi.softAPConfig(apIP, staticGateway, staticSubnet);
    WiFi.mode(_StaAndAp ? WIFI_AP_STA : WIFI_AP);

    _Channel = random(AP_CHANNEL_MIN, AP_CHANNEL_MAX);
    WiFi.softAP(_ssidAP.c_str(), _ssidApPass.c_str(), _Channel, _hidden);
    dnsServer.start(AP_DNS_PORT, "*", apIP);

    _StaAp = false;
    _apOn = true;

    StringIP();

    if (!_ssid.isEmpty() && !_ssidPass.isEmpty() && !_ssidPassEr) {
       _staTest = true;
    }
}

bool WIFICONNECT::parseIPAddress(const String& ipStr, IPAddress& ip) {
    if (ipStr.isEmpty()) return true;
    if (!ip.fromString(ipStr)) return false;
    return true;
}

void WIFICONNECT::loop() {
    if (_ssidFound) {
       ESP.restart();
    }

    if (_staTest) {
        unsigned long currentStaTest = millis();
        if (currentStaTest - previousStaTest >= STA_RETRY_INTERVAL_MS) {
            previousStaTest = currentStaTest;
            restartSTA();
        }
    }

    if (_apOn) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= DNS_PROCESS_INTERVAL_MS) {
            previousMillis = currentMillis;
            dnsServer.processNextRequest();
        }
    }
	if (_startTest){
		unsigned long currentStartMillis = millis();
		if (currentStartMillis - previousStartTest >= START_SEARCH_INTERVAL_MS) {
            previousStartTest = currentStartMillis;
			onStart();
            restartSTA();
        }
	}
}

void WIFICONNECT::stop() {}

void WIFICONNECT::searchStart(String ssidStart) {
    _ssidStart = ssidStart;
    if (_ssidStart != _emptyS) {
        _startTest = true;
    }
}

void WIFICONNECT::restartSTA() {
    scan(false);
    if (ssidOn()) {
        _ssidFound = true;
    }
}

void WIFICONNECT::onStart() {
    scan(false);
    if (ssidStartOn()) {
        _ssid = _ssidStart;
        _ssidPass = "";
        if (_pcb) _pcb();
    }
}

String WIFICONNECT::StringIP() {
    if (modeSTA()) {
        return WiFi.localIP().toString();
    }
    return WiFi.softAPIP().toString();
}

uint8_t WIFICONNECT::getChannel() {
    return _Channel;
}

String WIFICONNECT::StringGatewayIP() {
    if (modeSTA()) {
        _getway = gatewayIP().toString();
    } else {
        _getway = WiFi.softAPIP().toString();
    }
    return _getway;
}

String WIFICONNECT::StringSubnetMask() {
    if (modeSTA()) {
        _subnet = subnetMask().toString();
    } else {
        _subnet = "255.255.255.0";
    }
    return _subnet;
}

void WIFICONNECT::anotherDev() {
    WiFi.mode(WIFI_OFF);
    _ssid = _ssidStart;
    _ssidPass = "";
    startSTA();
    if (_pcb) _pcb();
}

void WIFICONNECT::isConnect() {
    int tries = _cAttempts;
    while (--tries && status() != WL_CONNECTED) {
        Serial.print(".");
        delay(CONNECT_POLL_DELAY_MS);
        if (status() == WL_NO_SSID_AVAIL) {
            _ssidPassEr = false;
            tries = 1;
        }
        if (status() == WL_CONNECT_FAILED) {
            tries = 1;
        }
    }
	if (tries == 0) {
		_ssidPassEr = true;
		WiFi.mode(WIFI_OFF);
		}
    if (status() == WL_CONNECTED) {
        _StaAp = true;
        StringIP();
    }
	
}

boolean WIFICONNECT::modeSTA() {
    return _StaAp;
}

boolean WIFICONNECT::passError() {
    return _ssidPassEr;
}

String WIFICONNECT::network() {
    return _net;
}

String WIFICONNECT::scan(boolean Async) {
    int8_t n;
    if (!Async) {
        n = WiFi.scanNetworks();
    } else n = WiFi.scanComplete();
    switch (n) {
        case -2:
            WiFi.scanNetworks(true, true);
            return _net;
        case -1:
            return _net;
        default:
            _net = "{\"networks\":[";
            for (uint8_t i = 0; i < n; i++) {
                _net += "{\"ssid\":\"";
                String ssid = WiFi.SSID(i);
                if (ssid == _ssid) {
                    _ssidOn = true;
                    _Channel = WiFi.channel(i);
                }
                if (ssid.indexOf(_ssidStart) == 0 && _ssidStart != _emptyS) {
                    _ssidStart = ssid;
                    _ssidStartOn = true;
                }
                _net += ssid + "\",\"pass\":\"";
#if defined(ESP8266)
                _net += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? "" : "*";
#else
                _net += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : "*";
#endif
                _net += "\",\"dbm\":" + String(WiFi.RSSI(i)) + "}";
                if (i != n - 1) _net += ",\r\n";
            }
            _net += "]}";
            WiFi.scanDelete();
            return _net;
    }
}

String WIFICONNECT::findBestBSSID(const String& targetSSID) {
    int networkCount = WiFi.scanNetworks();
    if (networkCount == 0) {
	WiFi.scanDelete();
		return "";
	}
    int bestRSSI = BEST_RSSI_INIT;
    String bestBSSID = "";

    for (int i = 0; i < networkCount; ++i) {

        if (WiFi.SSID(i) == targetSSID && WiFi.RSSI(i) > bestRSSI) {
            bestRSSI = WiFi.RSSI(i);
            bestBSSID = WiFi.BSSIDstr(i);
        }
    }

    WiFi.scanDelete();
    return bestBSSID;
}

boolean WIFICONNECT::ssidStartOn() {
    return _ssidStartOn;
}

boolean WIFICONNECT::ssidOn() {
    return _ssidOn;
}

boolean WIFICONNECT::ssidOff() {
    return _staOff;
}

boolean WIFICONNECT::modeETH() {
    return false;
}

boolean WIFICONNECT::modeAP() {
    return _apOn;
}

String WIFICONNECT::getURL(String urls, boolean norequest) {
	//Serial.println(urls);
    HTTPClient http;
    http.begin(urls);
    String answer;
    if (http.GET() == HTTP_CODE_OK && !norequest) {
        answer = http.getString();
    }
    http.end();
    return answer;
}

String WIFICONNECT::getURL(String urls) {
    return getURL(urls, false);
}

void WIFICONNECT::restoreCallback(WIFICONNECTCb abc) {
    _abc = abc;
}

void WIFICONNECT::endRestore() {
    saveRestartCon(0);
}

#if defined(ESP8266)
void WIFICONNECT::beginRestore(uint8_t n) {
    if (ESP.getResetReason() == "External System") {
        if (ESP.rtcUserMemoryRead(0, (uint32_t*)&rtcData, sizeof(rtcData))) {
            uint32_t crcOfData = calculateCRC32((uint8_t*)&rtcData.data[0], sizeof(rtcData.data));
            if (crcOfData != rtcData.crc32) {
                saveRestartCon(0);
            } else {
                uint8_t nnn = rtcData.data[3];
                if (nnn >= n) {
                    saveRestartCon(0);
                    if (_abc) _abc();
                } else saveRestartCon(nnn + 1);
            }
        }
    }
} 
	
#endif

#if defined(ESP8266)
void WIFICONNECT::saveRestartCon(uint8_t n) {
    for (size_t i = 0; i < sizeof(rtcData.data); ++i) {
        rtcData.data[i] = n;
    }
    rtcData.crc32 = calculateCRC32((uint8_t*)&rtcData.data[0], sizeof(rtcData.data));
    ESP.rtcUserMemoryWrite(0, (uint32_t*)&rtcData, sizeof(rtcData));
}
#endif

uint32_t WIFICONNECT::calculateCRC32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xffffffff;
    while (length--) {
        uint8_t c = *data++;
        for (uint32_t i = 0x80; i > 0; i >>= 1) {
            bool bit = crc & 0x80000000;
            if (c & i) bit = !bit;
            crc <<= 1;
            if (bit) crc ^= 0x04C11DB7;
        }
    }
    return crc;
}