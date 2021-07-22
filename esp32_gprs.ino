#define TINY_GSM_MODEM_SIM868

#define SerialMon Serial
/* RXD2 pin GPIO16
 * TXD2 pin GPIO17
 */

#define RXD2 36
#define TXD2 4
  
#define SerialAT Serial1

#define TINY_GSM_DEBUG SerialMon

#define GSM_AUTOBAUD_MIN 9600
#define GSM_AUTOBAUD_MAX 57600

//#define TINY_GSM_TEST_GPS true

// set GSM PIN, if any
#define GSM_PIN ""

// Your GPRS credentials, if any
const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";


// Server details to test TCP/SSL
/* github resource */
const char server[] = "thingspeak.com";
const char resource[] = "/TinyGSM/logo.txt";

#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

const char* broker = "broker.hivemq.com";         // MQTT broker
//const char* broker = "192.169.1.61";
const char* uniqueClientId = "telematics_test_dev";

const char* telematics_unit_data = "telematics_unit_data/test";

TinyGsm modem(SerialAT);

TinyGsmClient client(modem);
PubSubClient mqtt(client);

void mqttCallback(char* topic, byte* payload, unsigned int len)
{
    SerialMon.print("Message arrived [");
    SerialMon.print(topic);
    SerialMon.print("]: ");
    SerialMon.write(payload, len);
    SerialMon.println();

    // Only proceed if incoming message's topic matches
    if (String(topic) == telematics_unit_data)
    {
        Serial.printf("%s> Received Topic: %s \n", __FUNCTION__, topic);
    }
}

char data_1[10] = "25", data_2[10] = "26", data_3[10] = "27", data_4[10] = "28";

static unsigned long sq_nr;

boolean MODEM_restarted = false,  
        MODEM_network = false,
        MODEM_gprs = false,
        MODEM_mqttConnection = false;

void setup() 
{
    // Set console baud rate
    SerialMon.begin(115200);
    delay(10);
    DBG("Setup Complete...");
    delay(6000);

    //TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);    //// Set GSM module baud rate, use this when auto-baud
    //Serial2.begin(baud-rate, protocol, RX pin, TX pin);
    SerialAT.begin(115200,SERIAL_8N1,RXD2,TXD2);

    mqtt.setServer(broker, 1883);
    mqtt.setCallback(mqttCallback);

    Serial.printf("Initalizing modem in setup ... \n");

    // To skip it, call init() instead of restart()
    DBG("Initializing modem...");
    if (!modem.restart()) 
    {
    //if (!modem.init()) {
          DBG("Failed to restart modem, delaying 10s and retrying");
          MODEM_restarted = false;
          
          //restart autobaud in case GSM just rebooted
          //TinyGsmAutoBaud(SerialAT, GSM_AUTOBAUD_MIN, GSM_AUTOBAUD_MAX);
          //return;
    }
    else
    {
        MODEM_restarted = true;
    }
    
}

static unsigned long counter;

void loop() {

/*
 * Notes to be checked in-future:
 * 1. Add return if only support will be modem/gprs
 * 2. isNetworkConnected() will return false eventhough the network signals low or in high trafic in network, needs to confirm 3n times
 * 3. isGprsConnected(), double check enabled
 * 4. sq_nr is cloud send count, counter is loop counter count
 */

  counter += 1;
  
  if(MODEM_restarted == false)
  {
      Serial.printf("%s> Configuring the modem ... \n", __FUNCTION__);
      if(!modem.restart())
      {
          Serial.printf("%s> Failed to restart modem ... \n", __FUNCTION__);
          MODEM_restarted = false;
      }
      else
      {
          MODEM_restarted = true;
      }
  }

  if(modem.isNetworkConnected() == false)                                                               //3 check enabled
  {
      if(modem.isNetworkConnected() == false)                                                               
      {   
          if(modem.isNetworkConnected() == false)                                                              
          {
                Serial.printf("%s> Registering to the network ... \n", __FUNCTION__);
        
                if(!modem.waitForNetwork(30000L))
                {
                    Serial.printf("%s> Registering to the network, not happen in 30 seconds !!! \n", __FUNCTION__);
                    MODEM_network = false;
                }
        
                if(modem.isNetworkConnected())
                {
                    Serial.printf("%s> Registered to the network success :-)  \n", __FUNCTION__);
                    MODEM_network = true;
                }
                else
                {
                    Serial.printf("%s> Still not registered to the network !!! \n", __FUNCTION__);
                    delay(1000);
                    MODEM_network = false;
                }
           }
      }
  }
  else
  {
      Serial.printf("%s> Modem is Connected to the Network :-) \n", __FUNCTION__);
      MODEM_network = true;
  }

  if(modem.isGprsConnected() == false)                                                                    // 2 check enabled
  {
      if(modem.isGprsConnected() == false)
      {
          MODEM_gprs = false;
          
          Serial.printf("%s> Connecting to %s \n", __FUNCTION__, apn);
          if(!modem.gprsConnect(apn, gprsUser, gprsPass))
          {
              Serial.printf("%s> GPRS Connection setup failed, retrying !!! \n", __FUNCTION__);
              MODEM_gprs = false;
          }
          else
          {
              MODEM_gprs = true;
          }
      }
      else
      {
          Serial.printf("%s> GPRS status is Connected \n", __FUNCTION__);
          MODEM_gprs = true;
      }
  }
  else
  {
      MODEM_gprs = true;
  }

  if(MODEM_network == true && MODEM_gprs == true)
  {
      Serial.printf("%s> MODEM network and GPRS conncetion stable \n", __FUNCTION__);
      Serial.printf("%s> Connecting to MQTT Broker <%s> \n", __FUNCTION__, broker);

      boolean mqttBrokerCntSts = mqtt.connect(uniqueClientId);

      if(mqttBrokerCntSts == false)
      {
           Serial.printf("%s> MQTT Broker <%s> failed to connect !!! \n", __FUNCTION__, broker);
           MODEM_mqttConnection = false;
      }
      else
      {
           Serial.printf("%s> MQTT Broker connection success :-) \n", __FUNCTION__); 
           MODEM_mqttConnection = true;
      }

      if(mqtt.connected())
      {
           Serial.printf("%s> MQTT Conncetion Success, Going to Publish data ... \n", __FUNCTION__);
           MODEM_mqttConnection = true;
           /* Rightnow, publish from here, infuture have a seperate function */

           sq_nr += 1;
           
           char jsonMsg[1024];
           StaticJsonBuffer<1024> jsonBuffer;
           JsonObject& root = jsonBuffer.createObject();

           root["Data-1"] = data_1;
           root["Data-2"] = data_2;
           root["Data-3"] = data_3;
           root["Data-4"] = data_4;
           root["counter"] = counter;
           root["Sequence Number"] = sq_nr;

           root.printTo(jsonMsg);
           boolean mqttPubSts = mqtt.publish(telematics_unit_data, jsonMsg);

           if(mqttPubSts == true)
           {
                Serial.printf("%s> MQTT Publish Success ... \n", __FUNCTION__);
                Serial.printf("%s> Published Msg: %s \n", __FUNCTION__, jsonMsg);
                delay(10);
           }
           else
           {
                Serial.printf("%s> MQTT Publish Failed !!! \n", __FUNCTION__);
           }
      }
      else
      {
          Serial.printf("%s> MQTT Connection not success, going to connect again !!! \n", __FUNCTION__);
          mqttBrokerCntSts = mqtt.connect(uniqueClientId);
          if(mqttBrokerCntSts == false)
          {
              MODEM_mqttConnection = false;
          }
          else
          {
              MODEM_mqttConnection = true;
          }
      }
  }

  if(MODEM_restarted == true || MODEM_network == true || MODEM_gprs == true || MODEM_mqttConnection == true)
  {
      Serial.printf("%s> ALL Status is true ... \n", __FUNCTION__);  
  }
  else
  {
      Serial.printf("%s> All status not matched !!! \n", __FUNCTION__);
  }

  delay(10000);
  
}
