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

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <style>

h1{ 
    color: black;
    text-align: center;
}

h2{ 
    color: black;
    text-align: center;
}

.buttonContainer{
    padding-top: 50px;
    background-color: white;
    display: flex;
    flex-direction: column;
}

.openButton{
    height: 200px;
    width: 300px;
    margin: auto;
    margin-bottom: 30px;
}
.closeButton{
    height: 200px;
    width: 300px;
    margin: auto;
}

.getButton{
    height: 200px;
    width: 300px;
    margin: auto;
    margin-bottom: 30px;

}
    </style>


    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Real-Time Plant Irrigation System</title>
    <!---- <link rel="stylesheet" href="./styles.css"> -->
</head>
<body>
    <div>
        <h1> ESP32 Real-Time Plant Irrigation Controller </h1><br>       
        <h2 class="perc"> <span id="perc">%PERC%</span> </h2>
        <h2 class="state"> <span id="state">%STATE%</span> </h2>

    </div>


    <div  class="buttonContainer">
        <button  class="getButton"> Get Current Moisture Percent </button>
        <button  class="openButton"> Open Valve </button>
        <button  class="closeButton"> Close Valve </button>
    </div>

    
    

    <script>

        var gateway = `ws://${window.location.hostname}/ws`;  //entry point to websocket interface. window.location.home finds current address, which will be web server ip address
        var websocket;    
        window.addEventListener('load', onLoad);  //onLoad called when web page loads
        function initWebSocket() {    //function to initialize websocket connection to server
            console.log('Trying to open a WebSocket connection...');
            websocket = new WebSocket(gateway); //uses the gateway address to initialize
            websocket.onopen    = onOpen;       //self explanatory, onOpen called when websocket connection opened, onClose on connection closed and onMessage when recieving message from server
            websocket.onclose   = onClose;
            websocket.onmessage = onMessage; 
        }
        function onOpen(event) {
            console.log('Connection opened');   //lets us know the connection worked
        }
        function onClose(event) {
            console.log('Connection closed');       //lets us know connection failed
            setTimeout(initWebSocket, 2000);        //tries to connect again every 2000ms (2sec)
        }
        function onMessage(event) { //reacts to message sent from server which tells us what position the valve is in or the moisture percentage
            var state;
            var perc;
            if(event.data == "-1" || event.data == "-2" || event.data == "-3"){
              if(event.data == "-1"){
                  state = "Valve Open";
              }
              else if(event.data == "-2"){
                  state = "Valve Closed";
              }
              else{
                  state = "Error";
              }
  
              document.getElementById('state').innerHTML = state; //replaces the text placeholder for valve state
            }
            else{
              var y = "Last Updated Moisture Percent = ";
              var z = "%";
              perc = y + event.data+z;
              document.getElementById('perc').innerHTML = perc;   //replaces the text placeholder for moisture percentage
            }
            
            
            
        }
        function onLoad(event) {  //called when we load in. sets up connection and then sets the functions for our buttons with initButton()
            initWebSocket();
            initButton();
        }
        function initButton() {   //sets buttons up


            document.getElementsByClassName('openButton')[0].addEventListener('click', openValve);   //when the button is clicked we call openValve function in js

            document.getElementsByClassName('closeButton')[0].addEventListener('click', closeValve);      
                       
            document.getElementsByClassName('getButton')[0].addEventListener('click', getMoisture);      


        }
      
        function openValve(){websocket.send('openValve');}  //the functions will simply send a string (in this case openValve) to the server (our esp32) which will react depending on the contents of the message
        function closeValve(){websocket.send('closeValve');}
        function getMoisture(){websocket.send('getMoisture');}
 

    </script>
   
</body>
</html>
)rawliteral";


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
