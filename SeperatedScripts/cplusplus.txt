#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <math.h>

#define A 16
#define B 17
#define C 18
#define D 19

const char* ssid = "___"; //your ssid here
const char* password = "___"; //your password here

bool ValveBool = 0;
int MoisturePercent = -1;


// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");



void notifyClientsValve() {   //this will detail the message we send to the user depicting the valve position. each number corresponds to a certain result, and are sent as strings
  int x = -3;

    if(ValveBool == 0){
      x=-1;
    }
    else if(ValveBool == 1){
      x=-2;
    }
  ws.textAll(String(x));
}

void notifyClientsMoisture() {  //same as above but this details moisture percent. The math simply converts our Voltage reading to percentage (ranges from 0-4095 for board at max of 3.3V as the moisture reader goes to 3V we do 4095/3.3*3 = 3723)
  int x = 0;
  x = MoisturePercent;
  if(x>3723){ //if theres an error saying over 100% moisture we correct to 100%
    x=3723;
  }
  x=x*100;
  x = x/3723;
  Serial.println(String(x));
  ws.textAll(String(x));
}

  void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {   //takes in message
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;

    if (strcmp((char*)data, "openValve") == 0) {     //if recieve openValve, we will set the ValveBool that opens it and call the function that will do so. Then finally notify the users of our change
      ValveBool = 0;
      changeValveState(ValveBool);
      notifyClientsValve();
    }
    if (strcmp((char*)data, "closeValve") == 0) {     
    ValveBool = 1;
    changeValveState(ValveBool);
    notifyClientsValve();
    }
    if (strcmp((char*)data, "getMoisture") == 0) {     //if recieve getMoisture, we will take our reading then notify the user of the result
      readingMoisture();
      notifyClientsMoisture();
    }
    
  }
  
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {  //handles event occurances
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {  //initializes websocket protocol
  ws.onEvent(onEvent);  //if theres an event we call onEvent function
  server.addHandler(&ws);
}

String processor(const String& var){    //will set our initial values for PERC and STATE which on the webpage are the placeholders for moisture percent and valve state respectively
  Serial.println(var);

  if(var == "PERC"){
    return "Get Moisture Level";    //this is what we replace the initial placeholder with
  }
  if(var == "STATE"){
  
  return "Click Buttons for Data";
  }
 
}


void changeValveState(bool x){    //simply switch the valve open or closed
      digitalWrite(A,!ValveBool);
  }

void readingMoisture(){   //Powers the moisture reading device, takes 5 readings .1 sec apart, then averages them, and finally powers down the device again (device power is on pin C, which from the top of the code corresponds to GPIO17)
  digitalWrite(C,1);
  MoisturePercent = 0;
  int x =0;
  delay(100);
  x = analogRead(35);
  MoisturePercent = MoisturePercent +x;
  Serial.println(String(x));
  delay(100);
  x = analogRead(35);
  MoisturePercent = MoisturePercent +x;
  delay(100);
  x = analogRead(35);
  MoisturePercent = MoisturePercent +x;
  delay(100);
  x = analogRead(35);
  MoisturePercent = MoisturePercent +x;
  delay(100);
  x = analogRead(35);
  MoisturePercent = MoisturePercent +x;
  MoisturePercent = MoisturePercent/5; 
  Serial.println(String(MoisturePercent));
  digitalWrite(C,0);
}


void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(A, OUTPUT);   //set up our pin modes and then initial values
  pinMode(B, INPUT);
  pinMode(C, OUTPUT);
  digitalWrite(A, LOW);
  digitalWrite(C, LOW);
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);   //initialize wifi connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

 initWebSocket();

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Start server
  server.begin();
}


void loop() {
  ws.cleanupClients();  
 
}
