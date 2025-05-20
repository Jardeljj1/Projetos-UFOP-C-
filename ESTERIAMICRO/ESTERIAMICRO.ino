#include <ESP8266WiFi.h>
#include <DHT.h>
#include <ESP8266WebServer.h>

// Definições dos pinos
#define RELAY_1_PIN D15     // Pino do Relé 1 (motor da esteira)
#define RELAY_2_PIN 4       // Pino do Relé 2 (bomba de água)
#define SENSOR_PIN 12       // Pino do sensor óptico

// Definições do Wi-Fi
const char* ssid = "JARDEL";        // Nome da rede Wi-Fi (SSID)
const char* password = "23071996";  // Senha da rede Wi-Fi

ESP8266WebServer server(80);         // Servidor Web na porta 80

bool esteiraAtiva = false;   // Estado da esteira
int garrafasContadas = 0;    // Contagem das garrafas

// Função para iniciar a esteira
void startEsteira() {
    digitalWrite(RELAY_1_PIN, LOW);  // Ativa o motor (Relé 1) - Nível baixo, se necessário
    esteiraAtiva = true;
    garrafasContadas = 0;  // Reseta a contagem de garrafas
    Serial.println("Esteira iniciada");
}

// Função para parar a esteira
void stopEsteira() {
    digitalWrite(RELAY_1_PIN, HIGH);  // Desativa o motor (Relé 1) - Nível alto
    digitalWrite(RELAY_2_PIN, HIGH);  // Desativa a bomba (Relé 2) - Nível alto
    esteiraAtiva = false;
    Serial.println("Esteira parada");
}

// Função para controlar a bomba de água
void acionarBomba() {
    digitalWrite(RELAY_2_PIN, LOW);   // Liga a bomba (Relé 2) - Nível baixo
    delay(800);                       // Mantém a bomba ligada por 1 segundo
    digitalWrite(RELAY_2_PIN, HIGH);    // Desliga a bomba (Relé 2) - Nível alto
}

// Função para ler o sensor óptico
void contarGarrafa() {
    if (digitalRead(SENSOR_PIN) == LOW) {  // Quando o sensor detecta uma garrafa
        garrafasContadas++;
        Serial.print("Garrafa detectada: ");
        Serial.println(garrafasContadas);

        // Quando uma garrafa é detectada, a esteira para por 1 segundo
        digitalWrite(RELAY_1_PIN, HIGH); // Desativa o motor
        delay(1000);                    // Espera 1 segundo
        acionarBomba();                 // Aciona a bomba para encher a garrafa
        delay(1000);                    // Espera 1 segundo para encher
        digitalWrite(RELAY_1_PIN, LOW); // Reativa o motor da esteira
        delay(1000);  
        digitalWrite(RELAY_1_PIN, HIGH);
        delay(2000); 
        digitalWrite(RELAY_1_PIN, LOW); 
    }
}

// Função para a página principal
void handleRoot() {
    String message = "<html><body><h1>Controle da Esteira</h1>";
    message += "<button onclick=\"fetch('/start').then(response => response.text()).then(alert)\">Iniciar Esteira</button><br>";
    message += "<button onclick=\"fetch('/stop').then(response => response.text()).then(alert)\">Parar Esteira</button><br>";
    message += "<h2>Garrafas Contadas: " + String(garrafasContadas) + "</h2>";
    message += "<h3>Estado da Esteira: " + String(esteiraAtiva ? "Ativa" : "Inativa") + "</h3>";
    message += "</body></html>";
    server.send(200, "text/html", message);
}

// Função para quando o comando start for acionado via Node-RED ou navegador
void handleStart() {
    startEsteira();
    server.send(200, "text/plain", "Esteira Iniciada");
}

// Função para quando o comando stop for acionado via Node-RED ou navegador
void handleStop() {
    stopEsteira();
    server.send(200, "text/plain", "Esteira Parada");
}

// Função para retornar a quantidade de garrafas contadas
void handleGarrafas() {
    String garrafas = String(garrafasContadas);  // Converte a contagem de garrafas para String
    String response = "{\"garrafas\":" + garrafas + "}";  // Resposta em formato JSON
    server.send(200, "application/json", response);    // Envia a quantidade de garrafas para o Node-RED
}

// Função para retornar o estado da esteira
void handleEstadoEsteira() {
    String estado = esteiraAtiva ? "Ativa" : "Inativa";  // Verifica o estado da esteira
    String response = "{\"estado\":\"" + estado + "\"}";  // Resposta em formato JSON
    server.send(200, "application/json", response);    // Envia o estado da esteira para o Node-RED
}

void setup() {
    Serial.begin(9600);  // Inicializa a comunicação serial
    pinMode(RELAY_1_PIN, OUTPUT);
    pinMode(RELAY_2_PIN, OUTPUT);
    pinMode(SENSOR_PIN, INPUT_PULLUP);  // Sensor óptico conectado ao pino

    // Garante que os relés não acionem ao iniciar
    digitalWrite(RELAY_1_PIN, HIGH);  // Desativa o motor (Relé 1)
    digitalWrite(RELAY_2_PIN, HIGH);  // Desativa a bomba (Relé 2)

    // Conecta ao Wi-Fi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Conectado ao Wi-Fi!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    // Define as rotas do servidor web
    server.on("/", handleRoot);    // Página principal
    server.on("/start", handleStart); // Comando para iniciar a esteira
    server.on("/stop", handleStop);   // Comando para parar a esteira
    server.on("/garrafas", handleGarrafas); // Rota para enviar a contagem de garrafas
    server.on("/estadoEsteira", handleEstadoEsteira); // Rota para enviar o estado da esteira

    // Inicia o servidor
    server.begin();
    Serial.println("Servidor HTTP iniciado");
}

void loop() {
    server.handleClient();  // Processa as requisições do servidor
    if (esteiraAtiva) {
        contarGarrafa();  // Verifica se uma garrafa foi detectada
    }
}