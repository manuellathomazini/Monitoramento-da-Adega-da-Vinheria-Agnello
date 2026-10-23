/*
  ESP32 + DHT22 + LDR no padrão FIWARE Descomplicado (Ultralight) com timestamp
  Alinhado ao Smart Data Model IndoorEnvironmentObserved

  Ligações:
    DHT22  -> dados no GPIO 4
    LDR A0 -> GPIO 34
    LED    -> LED onboard (GPIO 2)

  Bibliotecas: PubSubClient, DHT sensor library (Adafruit), Adafruit Unified Sensor

  Dispositivo: device888

  Tópicos:
    /TEF/device888/cmd    -> recebe comandos:  device888@on|  /  device888@off|
    /TEF/device888/attrs  -> envia, em UM payload Ultralight:
        t|25.3|h|0.60|p|500|d|2026-10-16T14:30:00Z|TimeInstant|2026-10-16T14:30:00Z
        t = temperatura (°C)                      -> temperature
        h = umidade relativa (fração de 0 a 1)    -> relativeHumidity
        p = iluminância estimada (lux)            -> illuminance
        d = data/hora da observação (ISO 8601 UTC)-> dateObserved
        TimeInstant = data/hora da leitura (ISO 8601, UTC)
    /TEF/device888/attrs  -> também envia o estado do LED:  s|on  /  s|off
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <math.h>
#include <time.h>

// ===================== CONFIGURAÇÕES (EDITE AQUI) =====================
const char* SSID        = "Wokwi-GUEST";   // Wi-Fi (no Wokwi, deixe assim)
const char* PASSWORD    = "";
const char* BROKER_MQTT = "3.135.234.101"; // Broker MQTT do FIWARE da aula
const int   BROKER_PORT = 1883;

const char* ID_MQTT          = "fiware_888";
const char* TOPICO_SUBSCRIBE = "/TEF/device888/cmd";
const char* TOPICO_ATTRS     = "/TEF/device888/attrs";
const char* topicPrefix      = "device888";

// Servidores de hora (NTP)
const char* NTP_1 = "a.st1.ntp.br";
const char* NTP_2 = "b.st1.ntp.br";

const unsigned long INTERVALO_ENVIO = 5000; // ms

// Conversão do LDR para lux (fórmula do módulo fotorresistor do Wokwi)
const float GAMMA_LDR = 0.7;
const float RL10      = 50;      // kOhm a 10 lux
const float LUX_MAX   = 100000;  // limite para evitar valores absurdos
// ======================================================================

#define LED_PIN  2
#define LDR_PIN  34
#define DHT_PIN  4
#define DHT_TYPE DHT22

DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient MQTT(espClient);

char EstadoSaida = '0';
unsigned long ultimoEnvio = 0;

void reconectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Conectando ao WiFi");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.print("\nConectado. IP: ");
  Serial.println(WiFi.localIP());
}

// Hora atual em UTC no formato ISO 8601 (ex.: 2026-10-16T14:30:00Z).
// Retorna false se o NTP ainda não sincronizou.
bool horaAtualISO(char* buf, size_t tam) {
  struct tm t;
  if (!getLocalTime(&t, 1000)) return false;
  strftime(buf, tam, "%Y-%m-%dT%H:%M:%SZ", &t);
  return true;
}

// Iluminância estimada em lux a partir do LDR.
float leLux() {
  int a = constrain(analogRead(LDR_PIN), 1, 4094);   // evita divisão por zero
  float resistencia = 10000.0 * a / (4095.0 - a);    // Ohms (resistor fixo de 10 kOhm do módulo LDR)
  float lux = pow(RL10 * 1e3 * pow(10, GAMMA_LDR) / resistencia, 1.0 / GAMMA_LDR);
  return lux > LUX_MAX ? LUX_MAX : lux;
}

void enviaEstadoLED() {
  MQTT.publish(TOPICO_ATTRS, EstadoSaida == '1' ? "s|on" : "s|off");
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.print("- Mensagem recebida: ");
  Serial.println(msg);

  String onCmd  = String(topicPrefix) + "@on|";
  String offCmd = String(topicPrefix) + "@off|";

  if (msg.equals(onCmd)) {
    digitalWrite(LED_PIN, HIGH);
    EstadoSaida = '1';
    enviaEstadoLED();
  } else if (msg.equals(offCmd)) {
    digitalWrite(LED_PIN, LOW);
    EstadoSaida = '0';
    enviaEstadoLED();
  }
}

void reconnectMQTT() {
  while (!MQTT.connected()) {
    Serial.print("* Conectando ao broker MQTT: ");
    Serial.println(BROKER_MQTT);
    if (MQTT.connect(ID_MQTT)) {
      Serial.println("Conectado ao broker!");
      MQTT.subscribe(TOPICO_SUBSCRIBE);
      enviaEstadoLED();
    } else {
      Serial.println("Falha. Nova tentativa em 2s");
      delay(2000);
    }
  }
}

void enviaSensores() {
  char timestamp[32];
  if (!horaAtualISO(timestamp, sizeof(timestamp))) {
    Serial.println("Hora ainda não sincronizada (NTP). Leitura não enviada.");
    return;  // não envia sem timestamp válido
  }

  float lux  = leLux();
  float temp = dht.readTemperature();
  float umid = dht.readHumidity();

  char payload[160];
  if (isnan(temp) || isnan(umid)) {
    Serial.println("Falha ao ler o DHT22 (confira o GPIO 4)");
    snprintf(payload, sizeof(payload), "p|%.0f|d|%s|TimeInstant|%s",
             lux, timestamp, timestamp);
  } else {
    // umidade enviada como fração (0 a 1), como no Smart Data Model
    snprintf(payload, sizeof(payload), "t|%.1f|h|%.2f|p|%.0f|d|%s|TimeInstant|%s",
             temp, umid / 100.0, lux, timestamp, timestamp);
  }

  MQTT.publish(TOPICO_ATTRS, payload);
  Serial.print("Enviado: ");
  Serial.println(payload);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  analogReadResolution(12);
  dht.begin();

  reconectWiFi();
  configTime(0, 0, NTP_1, NTP_2);  // hora em UTC (sem deslocamento de fuso)
  MQTT.setServer(BROKER_MQTT, BROKER_PORT);
  MQTT.setCallback(mqtt_callback);
}

void loop() {
  reconectWiFi();
  if (!MQTT.connected()) reconnectMQTT();
  MQTT.loop();

  unsigned long agora = millis();
  if (agora - ultimoEnvio >= INTERVALO_ENVIO) {
    ultimoEnvio = agora;
    enviaSensores();
  }
}
