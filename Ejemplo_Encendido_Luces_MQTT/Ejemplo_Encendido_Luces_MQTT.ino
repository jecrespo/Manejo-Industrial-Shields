/* Pruebas con Industrial Shield para encendido de luces con MQTT

    CPU - 54ARA+

    LUCES A - R1_5
    LUCES B - R1_6
    LUCES C - R1_7
    LUCES D - R1_8
*/

#include <SPI.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#ifdef MDUINO_PLUS
#include <Ethernet2.h>
#else
#include <Ethernet.h>
#endif

// Other constants
#define VALID        0xB0070000

//Define the configuration structure
//It is used to define how the configuration is stored into the EEPROM
typedef struct {
  uint32_t validity;    // value used to check EEPROM validity
  uint8_t mac[6];       // local MAC address
  uint8_t dhcp;         // 0: static IP address, otherwise: DHCP address
  uint8_t ip[4];        // local IP address
} conf_t;

//Declare the configuration variable
conf_t conf;

const conf_t default_conf = {
  VALID,                                    // validity
  {0xAE, 0xAE, 0xAA, 0x00, 0x00, 0x03},     // mac
  0,                                        // dhcp
  {10, 1, 1, 10},                         // ip
};

IPAddress mosquitto_server(10, 1, 1, 17);

EthernetClient ethClient;

int reles_iluminacion [] = {R1_5, R1_6, R1_7, R1_8};

int presentes = 0;
boolean enciende_luces = 0;
boolean apaga_luces = 0;

// Respuesta de los topic a los que me he subscrito en la función reconnect
void callback(char* topic, char* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  payload[length] = '\0'; //Para que pueda convertir correctamente a String

  //analizar el topic y el payload para ejecutar lo necesario con String
  String topic_S = topic;
  String payload_S = payload;

  if (topic_S == "iluminacion/A") {
    if (payload_S == "ON") {
      digitalWrite(R1_5, HIGH);
    }
    else
      digitalWrite(R1_5, LOW);
  }
  else if (topic_S == "iluminacion/B") {
    if (payload_S == "ON") {
      digitalWrite(R1_6, HIGH);
    }
    else
      digitalWrite(R1_6, LOW);
  }
  else if (topic_S == "iluminacion/C") {
    if (payload_S == "ON") {
      digitalWrite(R1_7, HIGH);
    }
    else
      digitalWrite(R1_7, LOW);
  }
  else if (topic_S == "iluminacion/D") {
    if (payload_S == "ON") {
      digitalWrite(R1_8, HIGH);
    }
    else
      digitalWrite(R1_8, LOW);
  }
  else if (topic_S == "presentes") {
    if (payload_S.toInt() !=  presentes) {
      if (payload_S.toInt() > 0) {
        //encender luces
        enciende_luces = 1;
      }
      else {
        //apagar luces
        apaga_luces = 1;
      }
      presentes = payload_S.toInt();  //Actualizo valor para comparar con el próximo que venga
    }
  }
}

PubSubClient client(mosquitto_server, 1883, callback, ethClient);

void setup() {
  Serial.begin(115200);
  Serial.print("Control Iluminación: ");

  //Configurar Red
  loadConf(nullptr);
  printConf(nullptr);

  // Start Ethernet using the configuration
  if (conf.dhcp) {
    // DHCP configuration
    Ethernet.begin(conf.mac);
  } else {
    // Static IP configuration
    Ethernet.begin(conf.mac, IPAddress(conf.ip));
  }

  delay(5000);

  //MQTT publico el reinicio y me suscribo a los topics
  reconnect();

  Serial.println("Enciendo luces en el inicio...");
  for (int i = 0; i < 4; i++) {
    digitalWrite(reles_iluminacion[i], HIGH);
  }

  //Actualizo los topics
  if (client.connected()) {
    client.publish("mensajes", "Reinicio CPU!!!");
    client.publish("iluminacion/A", "ON");
    client.publish("iluminacion/B", "ON");
    client.publish("iluminacion/C", "ON");
    client.publish("iluminacion/D", "ON");
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (enciende_luces == 1)
    encender_luces();
  if (apaga_luces == 1)
    apagar_luces();
}

void printConf(const char *arg) {
  Serial.print("MAC address: ");
  for (int i = 0; i < 6; ++i) {
    if (i > 0) {
      Serial.print(':');
    }
    if (conf.mac[i] < 0x10) {
      Serial.print('0');
    }
    Serial.print(conf.mac[i], HEX);
  }
  Serial.println();

  Serial.print("DHCP: ");
  Serial.println(conf.dhcp ? "ON" : "OFF");

  Serial.print("IP address: ");
  Serial.println(IPAddress(conf.ip));
}

void loadConf(const char *arg) {
  // Get configuration from EEPROM (EEPROM address: 0x00)
  EEPROM.get(0x00, conf);

  // Check EEPROM values validity
  if (conf.validity != VALID) {
    // Set default configuration
    setDefaultConf(nullptr);
    saveConf(nullptr);  //Guardo configuración por defecto en EEPROM si no hay configuración válida.
    Serial.println("Invalid EEPROM: using default configuration");
  }
}

void setDefaultConf(const char *arg) {
  // Copy default configuration to the current configuration
  memcpy(&conf, &default_conf, sizeof(conf_t));
}

void saveConf(const char *arg) {
  // Put configuration to EEPROM (EEPROM address: 0x00)
  EEPROM.put(0x00, conf);
}

// Función reconexión que se ejecuta en el loop si pierdo conexión
// En reconnect también me subscribo a los topics y publico que me he reiniciado
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient1357", "usuario", "password")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("mensajes", "Reconexion CPU!!!");
      // ... and resubscribe
      client.subscribe("iluminacion/A");
      client.subscribe("iluminacion/B");
      client.subscribe("iluminacion/C");
      client.subscribe("iluminacion/D");
      client.subscribe("presentes");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void encender_luces() {
  client.publish("iluminacion/A", "ON");
  client.publish("iluminacion/B", "ON");
  client.publish("iluminacion/C", "ON");
  client.publish("iluminacion/D", "ON");
  digitalWrite(R1_5, HIGH);
  digitalWrite(R1_6, HIGH);
  digitalWrite(R1_7, HIGH);
  digitalWrite(R1_8, HIGH);
  enciende_luces = 0;
  Serial.println("Enciendo Luces!!!");
}

void apagar_luces() {
  client.publish("iluminacion/A", "OFF");
  client.publish("iluminacion/B", "OFF");
  client.publish("iluminacion/C", "OFF");
  client.publish("iluminacion/D", "OFF");
  digitalWrite(R1_5, LOW);
  digitalWrite(R1_6, LOW);
  digitalWrite(R1_7, LOW);
  digitalWrite(R1_8, LOW);
  apaga_luces = 0;
  Serial.println("Apago Luces!!!");
}
