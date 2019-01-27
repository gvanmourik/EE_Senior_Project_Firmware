//header files
#include <WiFi.h>
//for OTA with web client
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
//store WiFi Credentials
#include "FS.h"
#include "esp_system.h"
#include <esp_wifi.h>
#include <string.h>
#include <Preferences.h>

//global constants
//Wifi pointers
const char* rssiSSID;
const char* password;

//global variables
//Wifi credentials
String PrefSSID, PrefPassword;
int UpCount = 0;
int WLcount;
int WFstatus;
int32_t rssi;
String getSsid;
String getPass;
String MAC;
Preferences preferences;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  wifiInit();
  IP_info();
  MAC = getMacAddress();


  delay(2000);
}

void loop() {
  // put your main code here, to run repeatedly:
  //check if sensor is connected to the WiFi
  if (WiFi.status() == WL_CONNECTED) {
    //wair for hub command
    /*
      if(1){
      //send out 3 test outputs

      }
    */
    Serial.printf("Connected");
    digitalWrite(2, HIGH);
    delay(500);
    digitalWrite(2, LOW);
    delay(5000);
  }
  else {
    Serial.printf("Not Connected");
    smartConfig();
    delay(3000);
    ESP.restart();
    WiFi.begin(PrefSSID.c_str(), PrefPassword.c_str());
    
    WFstatus = getWifiStatus(WFstatus);
    WiFi.begin(PrefSSID.c_str(), PrefPassword.c_str());
    WLcount = 0;
    while (WiFi.status() != WL_CONNECTED && WLcount << 200) {
      delay(100);
      Serial.printf(".");

      if (UpCount >= 60) {
        UpCount = 0;
        Serial.printf("\n");
      }
      ++UpCount;
      ++WLcount;
    }

    if (getWifiStatus(WFstatus) == 3) {

    }
    delay(1000);
  }


}

//Supporting Function
bool checkPrefsStore() {
  bool val = false;
  String NVssid, NVpass, prefssid, prefpass;

  NVssid = getSsidPass("ssid");
  NVpass = getSsidPass("pass");

  preferences.begin("wifi", false);
  prefssid = preferences.getString("ssid", "none");
  prefpass = preferences.getString("password", "none");
  preferences.end();

  if (NVssid.equals(prefssid) && NVpass.equals(prefpass)) {
    val = true;
  }

  return val;
}

String getMacAddress(void) {
  WiFi.mode(WIFI_AP_STA);
  uint8_t baseMac[6];
  esp_read_mac(baseMac, ESP_MAC_WIFI_STA);
  char macStr[18] = {0};
  return String(macStr);
}

//Return RSSI or 0 if target SSID not found
int32_t getRSSI(const char* target_ssid) {
  byte available_networks = WiFi.scanNetworks();

  for (int network = 0; network < available_networks; network) {
    if (strcmp(WiFi.SSID(network).c_str(), target_ssid) == 0)
    {
      return WiFi.RSSI(network);
    }
  }
  return 0;
}

String getSsidPass(String s) {
  String val = "NONE";
  s.toUpperCase();
  if (s.compareTo("SSID") == 0) {
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    val = String (reinterpret_cast<const char*>(conf.sta.ssid));
  }
  if (s.compareTo("PASS") == 0) {
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    val = String(reinterpret_cast<const char*>(conf.sta.password));
  }
  return val;
}

int getWifiStatus( int WiFiStatus  )
{
  WiFiStatus = WiFi.status();
  Serial.printf("\tStatus %d",  WiFiStatus );
  switch ( WiFiStatus )
  {
    case WL_IDLE_STATUS :                         // WL_IDLE_STATUS     = 0,
      Serial.printf(", WiFi IDLE \n");
      break;
    case WL_NO_SSID_AVAIL:                        // WL_NO_SSID_AVAIL   = 1,
      Serial.printf(", NO SSID AVAIL \n");
      break;
    case WL_SCAN_COMPLETED:                       // WL_SCAN_COMPLETED  = 2,
      Serial.printf(", WiFi SCAN_COMPLETED \n");
      break;
    case WL_CONNECTED:                            // WL_CONNECTED       = 3,
      Serial.printf(", WiFi CONNECTED \n");
      break;
    case WL_CONNECT_FAILED:                       // WL_CONNECT_FAILED  = 4,
      Serial.printf(", WiFi WL_CONNECT FAILED\n");
      break;
    case WL_CONNECTION_LOST:                      // WL_CONNECTION_LOST = 5,
      Serial.printf(", WiFi CONNECTION LOST\n");
      WiFi.persistent(false);                 // don't write FLASH
      break;
    case WL_DISCONNECTED:                         // WL_DISCONNECTED    = 6
      Serial.printf(", WiFi DISCONNECTED ==\n");
      WiFi.persistent(false);                 // don't write FLASH when reconnecting
      break;
  }
  return WiFiStatus;
}

void IP_info()
{
  getSsid = WiFi.SSID();
  getPass = WiFi.psk();
  rssi = getRSSI( rssiSSID );
  WFstatus = getWifiStatus( WFstatus );
  MAC = getMacAddress();

  Serial.printf( "\n\n\tSSID\t%s, ", getSsid.c_str() );
  Serial.print( rssi);  Serial.printf(" dBm\n" );  // printf??
  Serial.printf( "\tPass:\t %s\n", getPass.c_str() );
  Serial.print( "\n\n\tIP address:\t" );  Serial.print(WiFi.localIP() );
  Serial.print( " / " );
  Serial.println( WiFi.subnetMask() );
  Serial.print( "\tGateway IP:\t" );  Serial.println( WiFi.gatewayIP() );
  Serial.print( "\t1st DNS:\t" );     Serial.println( WiFi.dnsIP() );
  Serial.printf( "\tMAC:\t\t%s\n", MAC.c_str() );
}

void smartConfig(void) {
  //Init WiFi as Station, start SmartConfig
  WiFi.mode(WIFI_AP_STA);
  WiFi.beginSmartConfig();

  //Wait for SmartConfig packet from mobile
  //Needs to be a 2.4GHz network
  Serial.println("Waiting for SmartConfig.");
  while (!WiFi.smartConfigDone()) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("SmartConfig received.");

  //Wait for WiFi to connect to AP
  Serial.println("Waiting for WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi Connected.");

  IP_info();

  //storage Wifi Credentials
  preferences.begin("wifi", false);
  preferences.putString("ssid", getSsid);
  preferences.putString("password", getPass);
  preferences.end();
}

void wifiInit() {
  WiFi.mode(WIFI_AP_STA);

  wifi_config_t conf;
  esp_wifi_get_config(WIFI_IF_STA, &conf); //load wifi setting to struct
  rssiSSID = reinterpret_cast<const char*>(conf.sta.ssid);
  password = reinterpret_cast<const char*>(conf.sta.password);

  preferences.begin("wifi", false);
  PrefSSID = preferences.getString("ssid", "none");
  PrefPassword = preferences.getString("password", "none");
  preferences.end();

  //keep from rewriting flash if not needed
  if (!checkPrefsStore()) { //see is NV and Prefs are the same
    if (PrefSSID == "none") {
      smartConfig();
      delay(3000);
      ESP.restart();
    }
  }

  WiFi.begin(PrefSSID.c_str(), PrefPassword.c_str());

  WLcount = 0;
  while (WiFi.status() != WL_CONNECTED && WLcount < 200) {
    delay(100);
    ++WLcount;
  }
}
