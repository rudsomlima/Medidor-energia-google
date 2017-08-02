#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <HTTPSRedirect.h>
//#include "DebugMacros.h"

extern "C"{
#include "user_interface.h"
}

#define wifi_ssid "PAPAYA"
#define wifi_password "papaya2014"

#define LED_AZUL 2 // led azul na placa lolin nodemcu v3

os_timer_t mTimer;

int contador, leituras, cont_pulso;
bool led_medidor, led_medidor_ant=1, pisca;
int pulso, pulso_max; //valor da medicao do led na piscada
String envio;
bool flag_pulso;  //verifica q houve pulso

//Nunca execute nada na interrupcao, apenas setar flags!
void tCallback(void *tCall){
    pulso = analogRead(A0);

    if(pulso<pulso_max-20) led_medidor = 0; //nao houve pulso do led do medidor, valor de leitura base do opto
    else led_medidor = 1;

    if(!led_medidor_ant & led_medidor) {
      cont_pulso++; //se mudou de 0 pra 1, incrementa
      flag_pulso=1;
    }
    led_medidor_ant = led_medidor;

    if(pulso>pulso_max)  pulso_max = pulso; //pega o valor maximo do pulso do led

    contador++;   //pra calcular quantas vezes essa funcao eh executada
}

//FUNCAO DE EXECUCAO COM ESTOURO DE TIMER
void usrInit(void){
    os_timer_setfn(&mTimer, tCallback, NULL);  //Chama a funcao tCallback a cada 10ms
    os_timer_arm(&mTimer, 10, true);  //10ms
}

void setup_wifi();

const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
// Replace with your own script id to make server side changes
const char *GScriptId = "AKfycby6pTof46p1U0oUxtmjEKF5sBDhzzjrdHPBoYb_u_PpHBJ0rWbo";
const int httpsPort = 443;
// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = ""; //#### GERAR E preencher ####

// Write to Google Spreadsheet
String url = String("/macros/s/") + GScriptId + "/exec?value=Conected_ESP8266";
// Fetch Google Calendar events for 1 week ahead
//String url2 = String("/macros/s/") + GScriptId + "/exec?cal";
// Read from Google Spreadsheet
//String url3 = String("/macros/s/") + GScriptId + "/exec?read";

void setup() {
  Serial.begin(115200);
  Serial.flush();
  pinMode(LED_AZUL, OUTPUT);
  setup_wifi();
  usrInit();  //ativa a interrupção

    // Use HTTPSRedirect class to create TLS connection
  HTTPSRedirect client(httpsPort);
  Serial.print("Connecting to ");
  Serial.println(host);

  bool flag = false;
  for (int i=0; i<5; i++){
    int retval = client.connect(host, httpsPort);
    if (retval == 1) {
       flag = true;
       break;
    }
    else
      Serial.println("Connection failed. Retrying...");
  }

  Serial.flush();
  if (!flag){
    Serial.print("Could not connect to server: ");
    Serial.println(host);
    Serial.println("Exiting...");
    return;
  }

  Serial.flush();
  if (client.verify(fingerprint, host)) {
    Serial.println("Certificate match.");
  }
  else {
    Serial.println("Certificate mis-match");
  }

  // Note: setup() must finish within approx. 1s, or the the watchdog timer
  // will reset the chip. Hence don't put too many requests in setup()
  // ref: https://github.com/esp8266/Arduino/issues/34


  Serial.println("==============================================================================");
  url = String("/macros/s/") + GScriptId + "/exec?now=ESP8266 conectado!";
  client.printRedir(url, host, googleRedirHost);
  //client.printRedir(url2, host, googleRedirHost);
  Serial.println("==============================================================================");
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);
  WiFi.begin(wifi_ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}

long lastMsg = 0;
int cont = 0, valor_lum;

void loop()
{
  Serial.print("pulsos: ");
  Serial.println(cont_pulso);
  Serial.print("leituras por loop: ");
  Serial.println(contador);
  Serial.print("opto_min: ");
  Serial.println(pulso);
  Serial.print("opto_max: ");
  Serial.println(pulso_max);
  pulso_max = 0; //reinicia o pulso_max para se recalibrar
  contador = 0;  //zera a contagem de interrupcoes por segundo q esta ocorrendo
  yield();
  digitalWrite(LED_AZUL, LOW);
  delay(10);
  digitalWrite(LED_AZUL, HIGH);
  delay(990);
  yield();

  if(flag_pulso) {  //so envia dados se houver pulso no led do medidor
    Serial.println("========================================================== INICIO");
    long now = millis();
    flag_pulso=0; //so executa de novo se houver nova piscada no led do medidor
    HTTPSRedirect client(httpsPort);
    if (!client.connected()) client.connect(host, httpsPort);
    url = String("/macros/s/") + GScriptId + "/exec?value=" + cont_pulso; //imprime a contagem de pulsos
    client.printRedir(url, host, googleRedirHost);
    Serial.print("tempo de post (ms): ");
    Serial.println(millis()-now);
    Serial.println("============================================================ FIM");
  }
}
