#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// ======================================================
// CONFIGURACIÓN WIFI
// ======================================================

const char* WIFI_SSID = "Carlos's S21 Ultra";
const char* WIFI_PASSWORD = "cblk5450";   

// ======================================================
// CONFIGURACIÓN MQTT
// ======================================================

const char* MQTT_SERVER = "192.168.4.14";   
const int MQTT_PORT = 1883;

// Topics utilizados por el dashboard React.
const char* TOPIC_TEMPERATURA = "clima/esp32/temperatura";
const char* TOPIC_HUMEDAD = "clima/esp32/humedad";
const char* TOPIC_LUZ = "clima/esp32/luz";

// ======================================================
// CONFIGURACIÓN DE SENSORES
// ======================================================

#define DHT_PIN 26        
#define DHT_TYPE DHT11
#define LDR_PIN 32        

DHT dht(DHT_PIN, DHT_TYPE);

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Enviar una lectura cada 2 segundos.
const unsigned long INTERVALO_ENVIO = 2000;

unsigned long ultimoEnvio = 0;

void conectarWiFi() {
  Serial.print("Conectando a WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando a Mosquitto...");

    // Crear un Client ID diferente para este ESP32.
    String clientId = "ESP32-";
    clientId += String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println(" conectado");
    } else {
      Serial.print(" error, rc=");
      Serial.print(mqttClient.state());
      Serial.println(". Reintentando en 2 segundos...");
      delay(2000);
    }
  }
}

// ======================================================
// LEER Y PUBLICAR SENSORES
// ======================================================

void publicarSensores() {
  // 1-2.
  float temperatura = dht.readTemperature();
  float humedad = dht.readHumidity();

  // 3.
  int adc = analogRead(LDR_PIN);
  int luz = constrain(map(adc, 0, 4095, 0, 100), 0, 100);

  // 4-5.
  if (isnan(temperatura) || isnan(humedad)) {
    Serial.println("Error leyendo el DHT11");
  } else {
    mqttClient.publish(TOPIC_TEMPERATURA, String(temperatura, 2).c_str(), true);
    mqttClient.publish(TOPIC_HUMEDAD, String(humedad, 2).c_str(), true);

    // 6. Mostrar las mediciones en el Monitor Serial.
    Serial.printf("Temperatura: %.2f °C\n", temperatura);
    Serial.printf("Humedad: %.2f %%\n", humedad);
  }

  mqttClient.publish(TOPIC_LUZ, String(luz).c_str(), true);
  Serial.printf("LDR ADC: %d | Luz: %d %%\n", adc, luz);
  Serial.println("------------------------------");
}

void setup() {
  Serial.begin(115200);

  // Inicializar los sensores y configurar los pines necesarios.
  dht.begin();
  analogReadResolution(12);   

  conectarWiFi();

  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

void loop() {
  // Si se pierde WiFi, volver a conectarse.
  if (WiFi.status() != WL_CONNECTED) {
    conectarWiFi();
  }

  // Si se pierde MQTT, volver a conectarse.
  if (!mqttClient.connected()) {
    conectarMQTT();
  }

  mqttClient.loop();

  // Publicar sensores cada 2 segundos.
  if (millis() - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = millis();
    publicarSensores();
  }
}
