#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <HTTPSRedirect.h>
//#include "DebugMacros.h"

//https://github.com/tzapu/WiFiManager
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <Ticker.h>

ESP8266WebServer server(80);

extern "C"{
#include "user_interface.h"
}

#define LED_AZUL 2 // led azul na placa lolin nodemcu v3

os_timer_t mTimer;
WiFiClient  client;

int contador, leituras, cont_pulso;
bool led_medidor, led_medidor_ant=1, pisca;
int pulso, pulso_max; //valor da medicao do led na piscada
String envio;
bool flag_pulso;  //verifica q houve pulso

//Nunca execute nada na interrupcao, apenas setar flags!
void tCallback(void *tCall){
    //now_interrupt = millis();  //calcula o tempo da interrupcao
    pulso = analogRead(A0);

    if(pulso<pulso_max-20) led_medidor = 0; //nao houve pulso do led do medidor, valor de leitura base do opto
    else led_medidor = 1;

    if(!led_medidor_ant & led_medidor) { //se mudou de 0 pra 1, incrementa
      cont_pulso++;
      flag_pulso=1;
      //pisca o led azul da placa 10ms em alto, depois desliga
      //digitalWrite(LED_AZUL, LOW);
    }

    led_medidor_ant = led_medidor;

    if(pulso>pulso_max)  pulso_max = pulso; //pega o valor maximo do pulso do led

    contador++;   //pra calcular quantas vezes essa funcao eh executada por segundo
}

//FUNCAO DE EXECUCAO COM ESTOURO DE TIMER
void usrInit(void){
    os_timer_setfn(&mTimer, tCallback, NULL);  //Chama a funcao tCallback a cada 10ms
    os_timer_arm(&mTimer, 10, true);  //10ms
}

void setup_wifi();
String getPage();

const char* host = "script.google.com";
const char* googleRedirHost = "script.googleusercontent.com";
// Replace with your own script id to make server side changes
const char *GScriptId = "AKfycby6pTof46p1U0oUxtmjEKF5sBDhzzjrdHPBoYb_u_PpHBJ0rWbo";
const int httpsPort = 443;
// echo | openssl s_client -connect script.google.com:443 |& openssl x509 -fingerprint -noout
const char* fingerprint = "CC A6 A3 F4 2A 75 1E 9A CF 33 B0 1B D8 03 C9 5A B6 94 0F 5A"; //############# PREENCHER

// Write to Google Spreadsheet
String url = String("/macros/s/") + GScriptId + "/exec?value=Conected_ESP8266";
// Fetch Google Calendar events for 1 week ahead
//String url2 = String("/macros/s/") + GScriptId + "/exec?cal";
// Read from Google Spreadsheet
//String url3 = String("/macros/s/") + GScriptId + "/exec?read";

//for LED status
Ticker ticker;
void tick()
{
  //toggle state
  int state = digitalRead(LED_AZUL);  // get the current state of GPIO1 pin
  digitalWrite(LED_AZUL, !state);     // set pin to the opposite state
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.1, tick);
}

// Use HTTPSRedirect class to create TLS connection
void Conecta_google(void) {
  ticker.attach(0.5, tick);  //pisca para mostrar q esta tentando se conectar ao google
  //bool flag_fingerprint = 0;
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
  }

  Serial.flush();
  while(!client.verify(fingerprint, host)) {  //se o fingerprint nao foi aceito repete indefinidamente
      Serial.println("Certificate mis-match");
      delay(1000);
  }
  Serial.println("Certificate match.");
}

String getPage();
void handleRoot(){
  server.send ( 200, "text/html", getPage() );
 }

void setup() {
  Serial.begin(115200);
  pinMode(LED_AZUL, OUTPUT);  //define o led da placa como saida
  WiFiManager wifiManager;
  //wifiManager.resetSettings(); //força entrada no portal de configuracao, só pra testes
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("ESP8266", "smolder"); //nome e senha para acessar o portal
  Serial.println("ESP conectado no WIFI !");
  Serial.flush();
  usrInit();  //ativa a interrupção
  Conecta_google(); //tenta se conectar no google
  ticker.detach();
  Serial.print("###### Iniciando Servidor Setup #######: ");
  server.on ( "/", handleRoot );
  server.begin();
  Serial.println("OK");
  Serial.println("==============================================================================");
  digitalWrite(LED_AZUL, HIGH); //desliga o led azul
  //url = String("/macros/s/") + GScriptId + "/exec?now=ESP8266 conectado!";
  //client.printRedir(url, host, googleRedirHost);
  //client.printRedir(url2, host, googleRedirHost);
}


long lastMsg = 0;
int cont = 0, valor_lum;
bool flag_page = 0;

String getPage() {  //Prepara a resposta para o cliente
  String page = "";
  //colocar o tempo de refresh da pagina na linha abaixo
  page = "<html lang='br'><head><meta http-equiv='refresh' content='5' name='viewport' content='width=device-width, initial-scale=1'/>";
  page += "<link rel='stylesheet' href='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css'><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script><script src='https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js'></script>";
  page += "<title>ESP8266 Demo - www.projetsdiy.fr</title></head><body>";
  page += "<!DOCTYPE html>";
  page += "<html lang='en'>";
  page += "  <head>";
  page += "	<meta charset='utf-8'>";
  page += "	<meta http-equiv='X-UA-Compatible' content='IE=edge'>";
  page += "	<meta name='viewport' content='width=device-width, initial-scale=1'>";
  page += "	<link href='css/bootstrappage +=minpage +=css' rel='stylesheet'>";
  page += "	<link href='css/stylepage +=css' rel='stylesheet'>";
  page += "  </head>";
  page += "  <body>";
  page += "	<div class='container-fluid'>";
  page += "	<div class='row'>";
  page += "		<div class='col-md-12'>";
  page += "			<h3 class='text-center text-primary'>SETUP";
  page += "			</h3> ";
  page += "				<span class='label label-default'>";
  page += cont_pulso;
  page +=        "</span>";
  page += "				<span class='label label-primary'>";
  page += pulso;
  page +=        "</span>";
  page += "				<span class='label label-success'>";
  page += pulso_max;
  page +=       "</span>";
  page += "		</div>";
  page += "	</div>";
  page += "</div>";
  page += "	<script src='js/bootstrappage +=minpage +=js'></script>";
  page += "  </body>";
  page += "</html>";
  return page;
}


void loop()
{
  //server.handleClient();
  // while(millis() < now_interrupt + 10);
  // digitalWrite(LED_AZUL, HIGH);
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
  delay(1000);
  // Serial.print("tempo da interrupcao (ms): ");
  // Serial.println(millis()-now_interrupt);

  if(flag_pulso) {  //so envia dados se houver pulso no led do medidor
    flag_pulso=0; //so executa de novo se houver nova piscada no led do medidor
    Serial.println("========================================================== INICIO");
    long now = millis();
    HTTPSRedirect client(httpsPort);
    if (!client.connected()) client.connect(host, httpsPort);
    url = String("/macros/s/") + GScriptId + "/exec?value=" + cont_pulso; //imprime a contagem de pulsos
    client.printRedir(url, host, googleRedirHost);
    Serial.print("tempo de post (ms): ");
    Serial.println(millis()-now);
    Serial.println("============================================================ FIM");
  }
}
