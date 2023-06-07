#include "Arduino.h"
/**************
 *  numeric Dimmer  ( using robodyn dimmer = 
 *  **************
 *  Upgraded By Cyril Poissonnier 2020
 * 
  *    
 * 
 *  ---------------------- OUTPUT & INPUT Pin table ---------------------
 *  +---------------+-------------------------+-------------------------+
 *  |   Board       | INPUT Pin               | OUTPUT Pin              |
 *  |               | Zero-Cross              |                         |
 *  +---------------+-------------------------+-------------------------+
 *  | Lenardo       | D7 (NOT CHANGABLE)      | D0-D6, D8-D13           |
 *  +---------------+-------------------------+-------------------------+
 *  | Mega          | D2 (NOT CHANGABLE)      | D0-D1, D3-D70           |
 *  +---------------+-------------------------+-------------------------+
 *  | Uno           | D2 (NOT CHANGABLE)      | D0-D1, D3-D20           |
 *  +---------------+-------------------------+-------------------------+
 *  | ESP8266       | D1(IO5),    D2(IO4),    | D0(IO16),   D1(IO5),    |
 *  |               | D5(IO14),   D6(IO12),   | D2(IO4),    D5(IO14),   |
 *  |               | D7(IO13),   D8(IO15),   | D6(IO12),   D7(IO13),   |
 *  |               |                         | D8(IO15)                |
 *  +---------------+-------------------------+-------------------------+
 *  | ESP32         | 4(GPI36),   6(GPI34),   | 8(GPO32),   9(GP033),   |
 *  |               | 5(GPI39),   7(GPI35),   | 10(GPIO25), 11(GPIO26), |
 *  |               | 8(GPO32),   9(GP033),   | 12(GPIO27), 13(GPIO14), |
 *  |               | 10(GPI025), 11(GPIO26), | 14(GPIO12), 16(GPIO13), |
 *  |               | 12(GPIO27), 13(GPIO14), | 23(GPIO15), 24(GPIO2),  |
 *  |               | 14(GPIO12), 16(GPIO13), | 25(GPIO0),  26(GPIO4),  |
 *  |               | 21(GPIO7),  23(GPIO15), | 27(GPIO16), 28(GPIO17), |
 *  |               | 24(GPIO2),  25(GPIO0),  | 29(GPIO5),  30(GPIO18), |
 *  |               | 26(GPIO4),  27(GPIO16), | 31(GPIO19), 33(GPIO21), |
 *  |               | 28(GPIO17), 29(GPIO5),  | 34(GPIO3),  35(GPIO1),  |
 *  |               | 30(GPIO18), 31(GPIO19), | 36(GPIO22), 37(GPIO23), |
 *  |               | 33(GPIO21), 35(GPIO1),  |                         |
 *  |               | 36(GPIO22), 37(GPIO23), |                         |
 *  +---------------+-------------------------+-------------------------+
 *  | Arduino M0    | D7 (NOT CHANGABLE)      | D0-D6, D8-D13           |
 *  | Arduino Zero  |                         |                         |
 *  +---------------+-------------------------+-------------------------+
 *  | Arduino Due   | D0-D53                  | D0-D53                  |
 *  +---------------+-------------------------+-------------------------+
 *  | STM32         | PA0-PA15,PB0-PB15       | PA0-PA15,PB0-PB15       |
 *  | Black Pill    | PC13-PC15               | PC13-PC15               |
 *  | BluePill      |                         |                         |
 *  | Etc...        |                         |                         |
 *  +---------------+-------------------------+-------------------------+
 *  
 *  
 *  Work for dimmer on Domoticz or Web command :
 *  http://URL/?POWER=XX 
 *  0 -> 99 
 *  more than 99 = 99  
 *    
 *  Update 2019 04 28 
 *  correct issue full power for many seconds at start 
 *  Update 2020 01 13 
 *  Version 2 with cute web interface
 *  V2.1    with temperature security ( dallas 18b20 ) 
 *          MQTT to Domoticz for temp 
 */

/***************************
 * Librairy
 **************************/

#include <Arduino.h>
// Dimmer librairy 
#include <RBDdimmer.h>   /// the corrected librairy  in personal depot , the original has a bug
// Web services
#include <ESPAsyncWiFiManager.h>    
#include <ESPAsyncWebServer.h>

#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <ArduinoJson.h> // ArduinoJson v6

#include <TaskScheduler.h> // gestion des taches

// ota mise à jour sans fil
#include <AsyncElegantOTA.h>
// Dallas 18b20
#include <OneWire.h>
#include <DallasTemperature.h>
//mqtt
#include <AsyncMqttClient.h>
/// config
#include "config/config.h"
#include "config/enums.h"
#include "function/web.h"
#include "function/ha.h"
#include "function/littlefs.h" 
#include "function/mqtt.h"
#include "function/minuteur.h"
#include "function/dimmer.h"

#include "tasks/dallas.h"
#include "tasks/cooler.h"
#include "tasks/get_power.h"

// taches
Task Task_dallas(15000, TASK_FOREVER, &mqttdallas);
Task Task_Cooler(15000, TASK_FOREVER, &cooler);
Task Task_GET_POWER(10000, TASK_FOREVER, &get_dimmer_child_power);
Scheduler runner;

#ifdef ESP32
// Web services
  #include "WiFi.h"
  #include <AsyncTCP.h>
  #include "HTTPClient.h"
// File System
  #include <FS.h>
  #include "SPIFFS.h"
  #define LittleFS SPIFFS // Fonctionne, mais est-ce correct? 
#else
// Web services
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  #include <ESP8266HTTPClient.h> 
// File System
  #include <LittleFS.h>
#endif

/***************************
 * Begin Settings
 **************************/

//***********************************
//************* Gestion du serveur WEB
//***********************************
// Create AsyncWebServer object on port 80
WiFiClient domotic_client;
AsyncMqttClient client;

DNSServer dns;
HTTPClient http;
bool shouldSaveConfig = false;
Wifi_struct wifi_config_fixe; 



//***********************************
//************* Time
//***********************************
int timesync = 0; 
int timesync_refresh = 120; 

// *************************************

//#define USE_SERIAL  SerialUSB //Serial for boards whith USB serial port
#define USE_SERIAL  Serial

//***********************************
//************* Dallas
//***********************************
void dallaspresent ();
//float CheckTemperature(String label, byte deviceAddress[12]);
#define TEMPERATURE_PRECISION 10


////////////////////////////////////
///     AP MODE 
/////////////////////////////////

String routeur="PV-ROUTER";
bool AP = false; 
bool discovery_temp;
String dimmername ="";

OneWire  ds(ONE_WIRE_BUS);  //  (a 4.7K resistor is necessary - 5.7K work with 3.3 ans 5V power)
DallasTemperature sensors(&ds);
DeviceAddress insideThermometer;

  
  byte present = 0;
  
  byte data[12];
  byte addr[8];
  //float celsius = 0.00;
  float previous_celsius = 0.00;
  byte security = 0;
  int refresh = 60;
  int refreshcount = 0; 

/***************************
 * End Settings
 **************************/

//***********************************
//************* Gestion de la configuration
//***********************************

Config config; 
Mqtt mqtt_config; 
System sysvar;
Programme programme; 
Programme programme_relay1;
Programme programme_relay2;

String getmqtt(); 
void savemqtt(const char *filename, const Mqtt &mqtt_config);
String stringbool(bool mybool);
String getServermode(String Servermode);
String switchstate(int state);

String loginit; 
String logs="197}11}1"; 
String getlogs(); 
int childsend =0; 

char buffer[1024];

/***************************
 * init Dimmer
 **************************/

dimmerLamp dimmer(outputPin, zerocross); //initialise port for dimmer for ESP8266, ESP32, Arduino due boards
#ifdef outputPin2
 dimmerLamp dimmer2(outputPin2, zerocross); //initialise port for dimmer2 for ESP8266, ESP32, Arduino due boards
#endif


int outVal = 0;

    //***********************************
    //************* function web 
    //***********************************


unsigned long Timer_Cooler;

IPAddress _ip,_gw,_sn;


    //***********************************
    //************* Setup 
    //***********************************

void setup() {
  Serial.begin(115200);
   
  /// Correction issue full power at start
  pinMode(outputPin, OUTPUT); 
  #ifdef outputPin2
    pinMode(outputPin2, OUTPUT); 
  #endif

  //digitalWrite(outputPin, LOW);
  
  // relay
  // #ifdef STANDALONE
  // pinMode(RELAY1, OUTPUT);
  // pinMode(RELAY2, OUTPUT);
  // digitalWrite(RELAY1, LOW);
  // digitalWrite(RELAY2, LOW);
  // #endif
  #ifdef RELAY1 // permet de rajouter les relais en ne modifiant que config.h, et pas seulement en STANDALONE
    pinMode(RELAY1, OUTPUT);
    digitalWrite(RELAY1, LOW);
  #endif
  #ifdef RELAY2 // permet de rajouter les relais en ne modifiant que config.h
    pinMode(RELAY2, OUTPUT);
    digitalWrite(RELAY2, LOW);
  #endif


  // cooler init
  pinMode(COOLER, OUTPUT); 
  digitalWrite(COOLER, LOW);

  //démarrage file system
  LittleFS.begin();
  Serial.println("Demarrage file System");
  loginit += loguptime() + "Start filesystem\r\n"; 
  // configuration dimmer
  dimmer.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
  #ifdef outputPin2
    dimmer2.begin(NORMAL_MODE, ON); //dimmer initialisation: name.begin(MODE, STATE) 
  #endif
  ///// correction bug nouveau dimmer...  et config
 
  
  #ifdef POWERSUPPLY2022  
  /// correct bug board
  dimmer.setState(ON);
  
  pinMode(GND_PIN, OUTPUT);  /// board bug
  digitalWrite(GND_PIN, 0);  /// board bug with pin 16 

  pinMode(POS_PIN, OUTPUT); 
  digitalWrite(POS_PIN, 1);
  #endif

    
  dimmer.setPower(outVal); // Pourquoi créer outVal alors qu'on utilise "puissance" ailleurs? 
  #ifdef outputPin2
    dimmer2.setPower(outVal); 
  #endif
  
  
  USE_SERIAL.println("Dimmer Program is starting...");

      //***********************************
    //************* Setup -  récupération du fichier de configuration
    //***********************************
  #ifndef ESP32
    ESP.getResetReason();
  #endif
  // Should load default config if run for the first time
  Serial.println(F("Loading configuration..."));
  loginit += loguptime() + "Load config\r\n"; 
  loadConfiguration(filename_conf, config);

  // Create configuration file
  Serial.println(F("Saving configuration..."));
  loginit +=loguptime() + "Apply config\r\n"; 
  saveConfiguration(filename_conf, config);

  Serial.println(F("Loading mqtt_conf configuration..."));
  loadmqtt(mqtt_conf, mqtt_config);
  Serial.println(F("Loading wifi configuration..."));
  loadwifiIP(wifi_conf, wifi_config_fixe);
 
  Serial.println(F("Loading minuterie"));
  programme.name="dimmer";
  programme.loadProgramme();
  programme.saveProgramme();

#ifdef RELAY1
  programme_relay1.name="relay1";
  programme_relay1.loadProgramme();
  programme_relay1.saveProgramme();

  programme_relay2.name="relay2";
  programme_relay2.loadProgramme();
  programme_relay2.saveProgramme();
#endif
    //***********************************
    //************* Setup - Connexion Wifi
    //***********************************
  Serial.print("start Wifiautoconnect");
  loginit +=loguptime() + "Start Wifiautoconnect\r\n"; 

   // préparation  configuration IP fixe 

  AsyncWiFiManagerParameter custom_IP_Address("server", "IP", wifi_config_fixe.static_ip, 16);
  wifiManager.addParameter(&custom_IP_Address);
  AsyncWiFiManagerParameter custom_IP_gateway("gateway", "gateway", wifi_config_fixe.static_gw, 16);
  wifiManager.addParameter(&custom_IP_gateway);
  AsyncWiFiManagerParameter custom_IP_mask("mask", "mask", wifi_config_fixe.static_sn, 16);
  wifiManager.addParameter(&custom_IP_mask);

    _ip.fromString(wifi_config_fixe.static_ip);
    _gw.fromString(wifi_config_fixe.static_gw);
    _sn.fromString(wifi_config_fixe.static_sn);

    if ( !strcmp(wifi_config_fixe.static_ip, "") == 0) {
          //set static ip
            
          wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);
          Serial.print(String(wifi_config_fixe.static_ip));
  }

  wifiManager.autoConnect("dimmer");
  Serial.print("end Wifiautoconnect");
  wifiManager.setSaveConfigCallback(saveConfigCallback);

      strcpy(wifi_config_fixe.static_ip, custom_IP_Address.getValue());
      strcpy(wifi_config_fixe.static_sn, custom_IP_mask.getValue());
      strcpy(wifi_config_fixe.static_gw, custom_IP_gateway.getValue());

        Serial.println("static adress: " + String(wifi_config_fixe.static_ip) + " mask: " + String(wifi_config_fixe.static_sn) + " GW: " + String(wifi_config_fixe.static_gw));

      savewifiIP(wifi_conf, wifi_config_fixe);
  

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

 
   /// restart si la configuration OP static est différente ip affectée suite changement ip Autoconf
  if ( !strcmp(wifi_config_fixe.static_ip, "" ) == 0 )  {
         char IP[] = "xxx.xxx.xxx.xxx"; 
         IPAddress ip = WiFi.localIP();
         ip.toString().toCharArray(IP, 16);
        if (!strcmp(wifi_config_fixe.static_ip,IP) == 0) {
      Serial.println(wifi_config_fixe.static_ip);
      Serial.println(WiFi.localIP());

      Serial.print("Application de la nouvelle configuration Ip   ");
      ESP.restart();
      }
  }
  //Si connexion affichage info dans console
  Serial.println("");
  Serial.print("Connection ok sur le reseau :  ");
 
  Serial.print("IP address: ");

  Serial.println(WiFi.localIP()); 
  #ifndef ESP32
    Serial.println(ESP.getResetReason());
  #endif
  //// AP MODE 
  if ( routeur.compareTo(WiFi.SSID().substring(0,9)) == 0 ) {
      AP = true; 
  }



    //***********************************
    //************* Setup - OTA 
    //***********************************
    AsyncElegantOTA.begin(&server);    // Start ElegantOTA
   
    //***********************************
    //************* Setup - Web pages
    //***********************************
  //server.serveStatic("/css/", LittleFS, "/css/");
  //.setAuthentication("user", "pass");

  //chargement des url des pages
  call_pages();

  dimmername = WiFi.macAddress().substring(12,14)+ WiFi.macAddress().substring(15,17); 


    //***********************************
    //************* Setup -  demarrage du webserver et affichage de l'oled
    //***********************************
  Serial.println("start server");
  server.begin(); 
    
  Serial.println("start 18b20");
  sensors.begin();
    
  /// recherche d'une sonde dallas
  dallaspresent();

  devices_init(); // initialisation des devices HA
 


  //Serial.println(device_temp.name);
  /// MQTT 
  if (!AP && mqtt_config.mqtt) {
    Serial.println("Connection MQTT" );
    loginit +=loguptime() + "MQTT connexion\r\n"; 
    
      /// connexion MQTT 
    async_mqtt_init();
    connectToMqtt();
    delay(1000);  
    
   // Serial.println(String(mqtt_config.username));
   // Serial.println(String(mqtt_config.password));

    
    //client.setClient(domotic_client);
    //client.setCallback(callback);
    //client.setKeepAlive(60);
    //reconnect();

    
        if (mqtt_config.HA){
          Serial.println("HA discovery" );
        /// création des binary_sensor et enregistrement sous HA  
        device_dimmer_on_off.discovery();
        device_dimmer_on_off.send(String(config.dimmer_on_off));

        device_dimmer.discovery();
        device_dimmer.send(String(sysvar.puissance));

        device_dimmer_power.discovery();
        device_dimmer_power.send(String(sysvar.puissance* config.charge/100));

        device_dimmer_total_power.discovery();
        device_dimmer_total_power.send(String(sysvar.puissance_cumul + (sysvar.puissance * config.charge/100)));

        device_cooler.discovery();
        device_cooler.send(stringbool(false));

    //  device_temp.discovery(); // discovery fait à la 1ere réception sonde ou mqtt.
        #ifdef RELAY1
          device_relay1.discovery();
          device_relay1.send(String(0));
        #endif
        #ifdef RELAY2
          device_relay2.discovery();
          device_relay2.send(String(0));
        #endif
        device_dimmer_starting_pow.discovery();
        device_dimmer_starting_pow.send(String(config.startingpow));

        device_dimmer_minpow.discovery();
        device_dimmer_minpow.send(String(config.minpow));

        device_dimmer_maxpow.discovery();
        device_dimmer_maxpow.send(String(config.maxpow));

        device_dimmer_child_mode.discovery();
        device_dimmer_child_mode.send(String(config.mode));

        device_dimmer_save.discovery();
        }
        reconnect() ;
  }
  
  #ifdef  SSR
  analogWriteFreq(GRIDFREQ) ; 
  analogWriteRange(100);
  analogWrite(JOTTA, 0);
  #endif

/// init du NTP
ntpinit(); 

/// init des tasks
runner.init();
runner.addTask(Task_dallas);    
Task_dallas.enable();

runner.addTask(Task_Cooler);
Task_Cooler.enable();

runner.addTask(Task_GET_POWER );
Task_GET_POWER.enable();

Serial.println(ESP.getFreeHeap());
}

bool alerte=false;

/// LOOP 
///
void loop() {
 
  if ( mqtt_config.mqtt && !AP ) {
    if (!client.connected() ) {
      connectToMqtt();
    }
    //client.loop();  // retiré comme ça faisait clignoter HA en mode delest ou equal 
  }

 runner.execute(); // gestion des taches


  if (logs.length() > LOG_MAX_STRING_LENGTH ) { 
   logs="197}11}1";
  }


  if (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("NO WIFI - Restarting Dimmer");
    ESP.restart();
  }



  ///////////////// minuteur 
 //// Dimmer 
  if (programme.run) { 
      //  minuteur en cours
      if (programme.stop_progr()) { 
        dimmer.setPower(0); 
        dimmer_off();
        sysvar.puissance=0;
        Serial.print("stop minuteur dimmer");
        mqtt(String(config.IDX), String(dimmer.getPower()),"pourcent"); // remonté MQTT de la commande réelle
        if (mqtt_config.HA) {
          int instant_power = dimmer.getPower();
          device_dimmer.send(String(instant_power));
          device_dimmer_power.send(String(instant_power * config.charge/100)); 
          device_dimmer_total_power.send(String(sysvar.puissance_cumul + (sysvar.puissance * config.charge/100)));

        } 
      } 
  } 
  else { 
    // minuteur à l'arret
    if (programme.start_progr()){ 
      sysvar.puissance=config.maxpow; 
      dimmer_on();
      dimmer.setPower(config.maxpow); 
      delay (50);
      Serial.print("start minuteur ");
      mqtt(String(config.IDX), String(dimmer.getPower()),"pourcent"); // remonté MQTT de la commande réelle
      if (mqtt_config.HA) {
        int instant_power = dimmer.getPower();
        device_dimmer.send(String(instant_power));
        device_dimmer_power.send(String(instant_power * config.charge/100)); 
        device_dimmer_total_power.send(String(sysvar.puissance_cumul + (sysvar.puissance * config.charge/100)));
      } 
      offset_heure_ete(); // on corrige l'heure d'été si besoin
    }
  }

#ifdef RELAY1
 //// relay 1 
 if (programme_relay1.run) { 
      if (programme_relay1.stop_progr()) { 
        digitalWrite(RELAY1 , LOW);
      }
 }
 else {
      if (programme_relay1.start_progr()){ 
        digitalWrite(RELAY1 , HIGH);
      }
 }

 if (programme_relay2.run) { 
      if (programme_relay2.stop_progr()) { 
        digitalWrite(RELAY2 , LOW);
      }
 }
 else {
      if (programme_relay2.start_progr()){ 
        digitalWrite(RELAY2 , HIGH);
      }
 }
#endif

  ///////////////// restart /////////
  if (config.restart) {
    delay(5000);
    Serial.print("Restarting Dimmer");
    ESP.restart();
  }
  //// si la sécurité température est active on coupe le dimmer

  if ( sysvar.celsius > ( config.maxtemp + 0.5)) { 
            mqtt(String(config.IDXAlarme), String("Alert Temp :" + String(sysvar.celsius) ),"Alerte");  ///send alert to MQTT
            device_dimmer_alarm_temp.send("Alert temp");
          }

  if ( security == 1 ) { 
      if (!alerte){
        Serial.println("Alert Temp");
        logs += "Alert Temp\r\n";
      
        if (!AP && mqtt_config.mqtt ){
              mqtt(String(config.IDXAlarme), String("Ballon chaud " ),"Alerte");  ///send alert to MQTT
              device_dimmer_alarm_temp.send("Hot water");
        }
        alerte=true;
        
      }
    //// Trigger de sécurité température
      if ( sysvar.celsius <= (config.maxtemp - (config.maxtemp*TRIGGER/100)) ) {  
        security = 0 ;
                if (!AP && mqtt_config.mqtt && mqtt_config.HA) { device_dimmer_alarm_temp.send(stringbool(security)); 
                 mqtt(String(config.IDXAlarme), String("RAS" ),"Alerte");
                }
        sysvar.change = 1 ;
      }
      else {
      dimmer_off();
      }
  }
  else 
  {
    alerte=false;
  }

  

  /// Changement de la puissance (  pb de Exception 9 si call direct ) 
  if ( sysvar.change == 1  && programme.run == false ) {
    sysvar.change = 0; 
    if (config.dimmer_on_off == 0){
      dimmer_off();  
    }
    /// si on dépasse la puissance mini demandé 
    if (sysvar.puissance > config.minpow && sysvar.puissance != 0 && security == 0) 
    {
        if (config.dimmer_on_off == 1){dimmer_on();}  // if off, switch on 

        /// si au dessus de la consigne max configuré alors config.maxpow. 
        if ( sysvar.puissance > config.maxpow )  
        { 
          if (config.dimmer_on_off == 1){
            dimmer.setPower(config.maxpow);
            
            #ifdef outputPin2
              dimmer2.setPower(config.maxpow);
            #endif
          }
          /// si on a une carte fille, on envoie la commande 
          if ( strcmp(config.child,"") != 0 ) {
              if ( strcmp(config.mode,"delester") == 0 ) { child_communication(sysvar.puissance-config.maxpow,false ); } // si mode délest, envoi du surplus
              if ( strcmp(config.mode,"equal") == 0) { child_communication(sysvar.puissance,true); }  //si mode equal envoie de la commande vers la carte fille
          }
          
          #ifdef  SSR
          analogWrite(JOTTA, config.maxpow );
          #endif

        }
        /// fonctionnement normal
        else { 
        if (config.dimmer_on_off == 1){
          dimmer.setPower(sysvar.puissance);
          #ifdef outputPin2
            dimmer2.setPower(sysvar.puissance);
          #endif
        }
          logs += "dimmer at " + String(sysvar.puissance) + "\r\n";
          if ( strcmp(config.child,"") != 0 ) {
              if ( strcmp(config.mode,"equal") == 0) { child_communication(sysvar.puissance,true); }  //si mode equal envoie de la commande vers la carte fille
              if ( strcmp(config.mode,"delester") == 0 && sysvar.puissance < config.maxpow) { child_communication(0,false); }  //si mode délest envoie d'une commande à 0
          }
          #ifdef  SSR
          analogWrite(JOTTA, sysvar.puissance );
          #endif
        }


        
      /// si on est en mode MQTT on remonte les valeurs vers HA et MQTT
      if (!AP && mqtt_config.mqtt) { 
        if (config.dimmer_on_off == 0){
          mqtt(String(config.IDX), String("0"),"pourcent");  // remonté MQTT de la commande 0
          if (mqtt_config.HA) {device_dimmer.send(String("0"));  device_dimmer_power.send(String("0")); device_dimmer_total_power.send(String(sysvar.puissance_cumul)); 
          }  // remonté MQTT HA de la commande 0 
        }
        else if ( sysvar.puissance > config.maxpow ) {
          mqtt(String(config.IDX), String(config.maxpow),"pourcent");  // remonté MQTT de la commande max
          if (mqtt_config.HA) {device_dimmer.send(String(config.maxpow)); device_dimmer_total_power.send(String(sysvar.puissance_cumul + config.maxpow)); }  // remonté MQTT HA de la commande max
        }
        else {
          mqtt(String(config.IDX), String(dimmer.getPower()),"pourcent"); // remonté MQTT de la commande réelle
          if (mqtt_config.HA) {
                    int instant_power = dimmer.getPower();
              device_dimmer.send(String(instant_power));
              device_dimmer_power.send(String(instant_power * config.charge/100)); 
              device_dimmer_total_power.send(String(sysvar.puissance_cumul + (instant_power* config.charge/100)));
            } // remonté MQTT HA de la commande réelle
        }
      }
    

    }
    /// si la sécurité est active on déleste 
    else if (sysvar.puissance > config.minpow && sysvar.puissance != 0 && security == 1)
    {
      if ( strcmp(config.child,"") != 0 ) {
        if ( strcmp(config.mode,"delester") == 0 ) { child_communication(sysvar.puissance ,false); childsend =0 ;} // si mode délest, envoi du surplus
        if ( strcmp(config.mode,"equal") == 0) { child_communication(sysvar.puissance,false); childsend =0 ; }  //si mode equal envoie de la commande vers la carte fille
      }
    }
    //// si la commande est trop faible on coupe tout partout
    else {
        dimmer.setPower(0);
              /// et sur les sous routeur 
          if ( strcmp(config.child,"") != 0 ) {
            if ( strcmp(config.mode,"delester") == 0 ) { child_communication(0,false); } // si mode délest, envoi du surplus
            if ( strcmp(config.mode,"equal") == 0) { child_communication(0,false); }  //si mode equal envoie de la commande vers la carte fille
          }

        if (!AP && mqtt_config.Mqtt::mqtt) {
          mqtt(String(config.IDX), "0","pourcent");
          device_dimmer.send("0");
          device_dimmer_power.send("0");
        }
        #ifdef outputPin2
          dimmer2.setPower(0);
        #endif
        dimmer_off();  
        if ( strcmp(config.mode,"off") != 0) {  if (childsend>2) { child_communication(0,false); childsend++; }}

          #ifdef  SSR
          analogWrite(JOTTA, 0 );
          #endif


      if (!AP && mqtt_config.Mqtt::mqtt) {
        int instant_power = dimmer.getPower();
        mqtt(String(config.IDX), String(instant_power),"Watt");  // correction 19/04
        device_dimmer.send(String(instant_power));
        device_dimmer_power.send(String(instant_power * config.charge/100)); 
        device_dimmer_total_power.send(String(sysvar.puissance_cumul + (instant_power*config.charge/100) ));
      }

    }
  }


    //***********************************
    //************* LOOP - Activation de la sécurité
    //***********************************
if ( sysvar.celsius >= config.maxtemp && security == 0 ) {
  security = 1 ; 
  float temp = sysvar.celsius + 0.2; /// pour être sur que la dernière consigne envoyé soit au moins égale au max.temp  
  mqtt(String(config.IDXTemp), String(temp),"Temperature");  /// remonté MQTT de la température
  if ( mqtt_config.HA ) { device_temp.send(String(temp)); }
  if ( strcmp(config.mode,"delester") == 0 && ( strcmp(config.child,"") != 0 ) ) { child_communication(sysvar.puissance,false); } // si mode délest, envoi du surplus
  if (!AP && mqtt_config.mqtt && mqtt_config.HA ) { device_dimmer_alarm_temp.send(stringbool(security)); }
}



 delay(100);  // 24/01/2023 changement 500 à 100ms pour plus de réactivité
}

///////////////
////fin de loop 
//////////////





    //***********************************
    //************* Test de la présence d'une 18b20 
    //***********************************

void dallaspresent () {
  byte i;
  byte type_s;

if ( !ds.search(addr)) {
    Serial.println("Dallas not connected");
    loginit += loguptime() + "Dallas not connected\r\n";
    Serial.println();
    ds.reset_search();
    delay(250);
    return ;
  }
  
  Serial.print("ROM =");
  for( i = 0; i < 8; i++) {
    Serial.write(' ');
    Serial.print(addr[i], HEX);
  }

   Serial.println();
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      Serial.println("Device is not a DS18x20 family device.");
      return ;
  } 

  ds.reset();
  ds.select(addr);

  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();    ///  byte 0 > 1 si present
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  Serial.print("  present = ");
  Serial.println(present, HEX);
  loginit += loguptime()+  "Dallas present at "+ String(present, HEX) + "\r\n";

  return ;
   
  }

/// affichage de logs 
String getlogs(){
    logs = logs + loginit + "}1"; 
    loginit = "";
  
    return logs ; 
} 

/*
/// @brief beta 
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
  if(!index){
    Serial.printf("UploadStart: %s\n", filename.c_str());
  }
  for(size_t i=0; i<len; i++){
    Serial.write(data[i]);
  }
  if(final){
    Serial.printf("UploadEnd: %s, %u B\n", filename.c_str(), index+len);
  }
}

void merge(JsonObject dest, JsonObjectConst src)
{
   for (JsonPairConst kvp : src)
   {
     dest[kvp.key()] = kvp.value();
   }
}*/
