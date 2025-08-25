#include <Arduino.h>
#include <DHT.h>

// Pines
#define DHTPIN 4
#define DHTTYPE DHT11
#define LDRPIN 34
#define BOTON_A 25
#define BOTON_B 26

// Objetos
DHT dht(DHTPIN, DHTTYPE);

// Estados de la máquina
enum Estado {
  INICIO,
  MONITOREO_T_H,
  MONITOREO_LUZ,
  ALERTA,
  ALARMA
};

Estado estadoActual = INICIO;
unsigned long tiempoCambio = 0;

// Variables globales
float temperatura = 0;
float humedad = 0;
int luz = 0;

// ---------- Tareas ----------
void taskMonitoreoTH(void *pvParameters) {
  for (;;) {
    temperatura = dht.readTemperature();
    humedad = dht.readHumidity();
    vTaskDelay(2000 / portTICK_PERIOD_MS); // lectura cada 2s
  }
}

void taskMonitoreoLuz(void *pvParameters) {
  for (;;) {
    luz = analogRead(LDRPIN);
    vTaskDelay(1000 / portTICK_PERIOD_MS); // lectura cada 1s
  }
}

// ---------- Máquina de estados ----------
void loopMaquina() {
  unsigned long ahora = millis();

  switch (estadoActual) {
    case INICIO:
      Serial.println("Estado: INICIO");
      if (ahora - tiempoCambio >= 5000) {  // timeout 5s
        estadoActual = MONITOREO_T_H;
        tiempoCambio = ahora;
      }
      break;

    case MONITOREO_T_H:
      Serial.printf("Estado: MONITOREO T/H -> Temp=%.2f Hum=%.2f\n", temperatura, humedad);
      if (temperatura > 27) {
        estadoActual = ALERTA;
        tiempoCambio = ahora;
      } else if (ahora - tiempoCambio >= 7000) {  // timeout 7s
        estadoActual = MONITOREO_LUZ;
        tiempoCambio = ahora;
      }
      break;

    case MONITOREO_LUZ:
      Serial.printf("Estado: MONITOREO LUZ -> Luz=%d\n", luz);
      if (luz > 1024) {
        estadoActual = ALARMA;
        tiempoCambio = ahora;
      } else if (ahora - tiempoCambio >= 2000) {  // timeout 2s
        estadoActual = MONITOREO_T_H;
        tiempoCambio = ahora;
      }
      break;

    case ALERTA:
      Serial.println("Estado: ALERTA");
      if (digitalRead(BOTON_A) == LOW) {  // botón A
        estadoActual = INICIO;
        tiempoCambio = ahora;
      } else if (ahora - tiempoCambio >= 4000) {  // timeout 4s
        estadoActual = MONITOREO_T_H;
        tiempoCambio = ahora;
      }
      break;

    case ALARMA:
      Serial.println("Estado: ALARMA");
      if (digitalRead(BOTON_B) == LOW) {  // botón B
        estadoActual = INICIO;
        tiempoCambio = ahora;
      } else if (ahora - tiempoCambio >= 3000) {  // timeout 3s
        estadoActual = MONITOREO_LUZ;
        tiempoCambio = ahora;
      }
      break;
  }
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(BOTON_A, INPUT_PULLUP);
  pinMode(BOTON_B, INPUT_PULLUP);

  // Crear tareas
  xTaskCreatePinnedToCore(taskMonitoreoTH, "TaskTH", 2048, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskMonitoreoLuz, "TaskLuz", 2048, NULL, 1, NULL, 1);

  tiempoCambio = millis();
}

// ---------- Loop principal ----------
void loop() {
  loopMaquina();
  vTaskDelay(500 / portTICK_PERIOD_MS); // evitar saturación
}

