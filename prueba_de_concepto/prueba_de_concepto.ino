//Prueba de laboratorio con un M-Duino 54 ARA+
//Control de relés y lectura de una sonda DHT22 en uno de los dos pines que soportan 5V en los industrial shields
//Configure the Ethernet parameters saved in the EEPROM: MAC address and DHCP/IP address.
//Uso una version simplificada de http://blog.industrialshields.com/en/ethernet-configuration-from-eeprom/

#include <dht.h>
#include <SPI.h>
#include <EEPROM.h>
#ifdef MDUINO_PLUS
#include <Ethernet2.h>
#else
#include <Ethernet.h>
#endif

// Other constants
#define VALID        0xB0070000

#define DHT22_PIN 2

dht DHT;

struct
{
  uint32_t total;
  uint32_t ok;
  uint32_t crc_error;
  uint32_t time_out;
  uint32_t connect;
  uint32_t ack_l;
  uint32_t ack_h;
  uint32_t unknown;
} stat = { 0, 0, 0, 0, 0, 0, 0, 0}; //para lecturas de DHT22

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

//IP: 10.10.10.1
//MAC:0xAE, 0xAE, 0xBE, 0xEF, 0xFE, 0x00
// Define the default configuration variable
const conf_t default_conf = {
  VALID,                                    // validity
  {0xAE, 0xAE, 0xBE, 0xEF, 0xFE, 0x00},     // mac
  0,                                        // dhcp
  {10, 10, 10, 1},                         // ip
};

void setup() {
  Serial.begin(115200);
  Serial.println(MDUINO_PLUS);
  Serial.println("dht22_test.ino");
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT_LIB_VERSION);
  Serial.println();
  Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)\tTime (us)");
  //ENCIENDE LUCES CADA MEDIO SEGUNDO, TEST DE INICIO
  digitalWrite(R1_1, HIGH);
  delay(500);
  digitalWrite(R1_2, HIGH);
  delay(500);
  digitalWrite(R1_3, HIGH);
  delay(500);
  digitalWrite(R1_4, HIGH);
  delay(500);
  digitalWrite(R1_5, HIGH);
  delay(500);
  digitalWrite(R1_6, HIGH);
  delay(500);
  digitalWrite(R1_7, HIGH);
  delay(500);
  digitalWrite(R1_8, HIGH);

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
}

void loop() {
  read_data();
}

void read_data() {
  // READ DATA
  Serial.print("DHT22, \t");

  uint32_t start = micros();
  int chk = DHT.read22(DHT22_PIN);
  uint32_t stop = micros();

  stat.total++;
  switch (chk)
  {
    case DHTLIB_OK:
      stat.ok++;
      Serial.print("OK,\t");
      break;
    case DHTLIB_ERROR_CHECKSUM:
      stat.crc_error++;
      Serial.print("Checksum error,\t");
      break;
    case DHTLIB_ERROR_TIMEOUT:
      stat.time_out++;
      Serial.print("Time out error,\t");
      break;
    default:
      stat.unknown++;
      Serial.print("Unknown error,\t");
      break;
  }
  // DISPLAY DATA
  Serial.print(DHT.humidity, 1);
  Serial.print(",\t");
  Serial.print(DHT.temperature, 1);
  Serial.print(",\t");
  Serial.print(stop - start);
  Serial.println();

  if (stat.total % 20 == 0)
  {
    Serial.println("\nTOT\tOK\tCRC\tTO\tUNK");
    Serial.print(stat.total);
    Serial.print("\t");
    Serial.print(stat.ok);
    Serial.print("\t");
    Serial.print(stat.crc_error);
    Serial.print("\t");
    Serial.print(stat.time_out);
    Serial.print("\t");
    Serial.print(stat.connect);
    Serial.print("\t");
    Serial.print(stat.ack_l);
    Serial.print("\t");
    Serial.print(stat.ack_h);
    Serial.print("\t");
    Serial.print(stat.unknown);
    Serial.println("\n");
    Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)\tTime (us)");
  }

  delay(1000);
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
 
    Serial.println("Invalid EEPROM: using default configuration");
  }
}

void setDefaultConf(const char *arg) {
  // Copy default configuration to the current configuration
  memcpy(&conf, &default_conf, sizeof(conf_t));
}
