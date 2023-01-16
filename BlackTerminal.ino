#include <Wire.h>
#include <BBQ10Keyboard.h>

const String projectName = "BlackTerminal";
const String deviceNumber = "18035586732";
const String defaultTelephoneNumber = "17044923691";

char OLED_Address = 0x3c;
char OLED_Command_Mode = 0x80;
char OLED_Data_Mode = 0x40;

int current = 0;
int maxVal = 255;
int delayVal = 200;
int linePosition = 0;
bool isUpOn = false;
bool isDownOn = false;
bool isFlashOn = false;

long targetTimeout = 0;
int period = 500;

int currentPage = 0;
int charactersPerLine = 16;
int displayCharacters = 2 * charactersPerLine;

// special character int values
int charaterLeftArrow = 17;
int cursorChar = 18;
int characterUpArrow = 26;
int characterDownArrow = 27;

int commandKeyUp = 6;
int commandKeySelect = 7;
int commandKeyDelete = 8;
int commandKeyEnter = 10;
int commandKeyBack = 17;
int commandKeyDown = 18;

char newline = 0xA;
char carriageReturn = 0xD;

char clearDisplay = 0x01;

String outgoingMessage = "";
String incomingMessage = "";

BBQ10Keyboard keyboard;

void setup() {
  // put your setup code here, to run once:
  // begin serial comms with IDE and SIM7600
  Serial.begin(9600);// serial from arduino ide
  Serial.setTimeout(250);
  
  Serial1.begin(115200);//serial to SIM7600 cellular board
  Serial1.setTimeout(250);

  Serial.print(projectName);
  Serial.println(" started");  

  Wire.begin();

  // start keyboard comms
  keyboard.begin();
  keyboard.setBacklight(0.5f);

  outgoingMessage.reserve(160);
  incomingMessage.reserve(500);

  //initialize screen
  SetupScreen();
  sendCommand(clearDisplay); // ** Clear display
  sendMessage("BlackTerminal0.1started", currentPage);
  // sendMessage("started v0.1", 2);
  delay(1000);//wait one sec to see the started message

  // turn echo off
  Serial1.println("ATE0");    

  sendCommand(clearDisplay); // ** Clear display
  setCursor(0,0);
}

void loop() {
  // put your main code here, to run repeatedly:
  checkForIDEMessages();
  checkForSIM7600Messages();
  checkForKeyboardMessages();
}

void checkForSIM7600Messages() {
  if (Serial1.available() > 0) {
    String messageFromSIM7600 = Serial1.readString();
    //remove incoming end \r\n from the message
    messageFromSIM7600.trim();

    if (messageFromSIM7600.startsWith("+CMGR")){ // request output from single message in memory
      // process single message
      processMessage(messageFromSIM7600);

    } else if (messageFromSIM7600.startsWith("+CMTI")){ // incoming single message received
      // message recieved
      processReceivedMessage(messageFromSIM7600);

    } else if (messageFromSIM7600.startsWith("+CMGL")){ // process in memory list
      // message list recieved
      processInMemoryList(messageFromSIM7600);

    } else {
      Serial.println(messageFromSIM7600);
      Serial.println(messageFromSIM7600.indexOf(newline));// newline character
      incomingMessage = messageFromSIM7600;
      setCursor(0,0);  
      sendMessage(incomingMessage, currentPage);
    }
  }
}


void processSingleMessage(String message)
{
  Serial.println("processSingleMessage");
  Serial.println(message);
  int firstNewLine = message.indexOf(newline);// newline character
  String firstBlock = message.substring(0, firstNewLine);
  Serial.print("first: ");
  Serial.println(firstBlock);

  String secondBlock = message.substring(firstNewLine + 1, message.length());

  int secondNewLine = secondBlock.indexOf(newline);// newline character
  secondBlock = secondBlock.substring(0, secondNewLine);

  Serial.print("second: ");
  Serial.println(secondBlock);
}

// takes the messages in 7600 memory and iterates through the list
// should construct a string that can be sent to the screen
void processInMemoryList(String messages) {
    Serial.println("processInMemoryList");
    // messages needs a large size
    int firstIndex = messages.indexOf('\n');
    int index = 0;
    incomingMessage = "";
    int chars = 0;
    String blank = "                ";
    while (firstIndex > 0) {
        // figure out max size of messageData
        String messageData = messages.substring(0, firstIndex);

        String remainingMessages = messages.substring(firstIndex + 1, messages.length());
        int nextIndex = remainingMessages.indexOf('\n');

        String message = remainingMessages.substring(0, nextIndex);

        String number  = parseMessageData(messageData, 2);

        //incomingMessage = incomingMessage + message;

        int len = message.length();

        int mod = len % 16;

        int remain = 16 - mod;

        String blanks = blank.substring(0,remain);

        Serial.print(index++);
        Serial.print(": ");
        Serial.print(message);
        Serial.print(blanks);
        Serial.print(": ");
        Serial.print(len);
        Serial.print(": ");
        Serial.print(mod);
        Serial.print(": ");
        Serial.print(blanks.length());
        Serial.print(": ");
        Serial.println(remain);

        
        incomingMessage = incomingMessage +  message + blanks;


        // trim of first endline and message
        messages = messages.substring(nextIndex + firstIndex + 2, messages.length());
        //start at the follwing line
        firstIndex = messages.indexOf('\n');    
    }

    sendCommand(clearDisplay); // ** Clear display
    sendMessage(incomingMessage, currentPage);
}

// reads the meta data from each message to parse the type and incoming number
// section is the zero based comma deliminated section of the message to 
// be retrieved, 2 being the string incoming number
String parseMessageData(String messageData, int section){
  // incoming format:
  // +CMGL: 10,"REC READ","+17044923691","","23/01/03,06:08:37-32"
  //console.log(`messageData: ${messageData}`);
  int firstIndex = messageData.indexOf(',');
  int index = 0;
  while (firstIndex > 0) {
      // figure out max size of messageData
      String messagePart = messageData.substring(0, firstIndex);

      // if this is the section we are looking for by index, return
      if(section == index) return messagePart;

      messageData = messageData.substring(firstIndex + 1, messageData.length());

      //start at the follwing line
      firstIndex = messageData.indexOf(',');    
      index++;          
  }
}

void processMessage(String message)
{
  Serial.println(message);
  // request output from single message in memory  
  int firstNewLine = message.indexOf(newline);// newline character

  String secondBlock = message.substring(firstNewLine + 1, message.length());

  int secondNewLine = secondBlock.indexOf(newline);// newline character
  secondBlock = secondBlock.substring(0, secondNewLine);

  Serial.print("message: ");
  Serial.println(secondBlock);

  incomingMessage = secondBlock;
  setCursor(0,0);  
  sendMessage(secondBlock, currentPage);
  //return secondBlock;
}

void processReceivedMessage(String message)
{
  Serial.println(message);
  //+CMTI: "SM",3  
  int messageSplitChar = message.indexOf(",");// newline character

  String messageNumber = message.substring(messageSplitChar + 1, message.length());

  Serial.print("message: ");
  Serial.println(messageNumber);

  Serial1.println("AT+CMGR=" + messageNumber);
}



// for test communication via arduino serial, use if you plan on controlling/communicating with phone via 
// the arduino IDE or other serial communication tools (FOR TESTING OR DEV NOT REQUIRED FOR OPERATION)
void checkForIDEMessages() {
  // ------------------------------------------
  if (Serial.available() > 0)
  {
    String incoming = Serial.readString();
    incoming.trim();

    Serial.print("ide message: \"");        
    Serial.print(incoming);  
    Serial.println("\"");    
    
    // send to SIM7600
    Serial1.println(incoming);
  }
  // ------------------------------------------ 
}

void checkForKeyboardMessages() {
  const int keyCount = keyboard.keyCount();

  // no keypresses found
  if (keyCount == 0)
    return;

  //keypresses found
  const BBQ10Keyboard::KeyEvent key = keyboard.keyEvent();
  String state = "pressed";

  // character keys
  if (key.state == BBQ10Keyboard::StateRelease) {
    if (!isCommandKey(key)) {
      //Serial.printf("character key: '%c' (dec %d, hex %02x) %s\r\n", key.key, key.key, key.key, state.c_str());
      characterKeyPressed(key);

    } else if(isCommandKey(key)){
      //Serial.printf("command key: '%c' (dec %d, hex %02x) %s\r\n", key.key, key.key, key.key, state.c_str());
      if (key.key == commandKeyUp) { // up button pressed
        upCommandPressed();
      } else if (key.key == commandKeySelect) { // select button pressed
        Serial.println("cursor select");  

      } else if (key.key == commandKeyDelete) { // delete character button pressed
        deleteCharacterPressed();

      } else if (key.key == commandKeyEnter) { // enter key pressed
        enterCommandPressed();

      } else if (key.key == commandKeyBack) { // back button pressed
        Serial.println("cursor back");  
      } else if (key.key == commandKeyDown) { // down button pressed
        Serial.print("command key: ");
        Serial.println(key.key);  
        downCommandPressed();      
      } else { // unknown
        Serial.print("unknown command key: ");
        Serial.println(key.key);  
      }
    }
  }
}

// KEYBOARD METHODS
bool isCommandKey(BBQ10Keyboard::KeyEvent key)
{
  if(key.key == commandKeyUp 
    || key.key == commandKeySelect 
    || key.key == commandKeyDelete 
    || key.key == commandKeyEnter
    || key.key == commandKeyBack
    || key.key == commandKeyDown) {
    return true;    
  }

 return false;
}

void characterKeyPressed(BBQ10Keyboard::KeyEvent key)
{
  //String state = "released";
  //Serial.printf("character key: '%c' (dec %d, hex %02x) %s\r\n", key.key, key.key, key.key, state.c_str());

  currentPage = 0;
  outgoingMessage += key.key;
  Serial.println(outgoingMessage);

  setCursor(0,0);  
  sendMessage(outgoingMessage, currentPage);
}

void upCommandPressed()
{
  if ((currentPage + 1) < (incomingMessage.length() / charactersPerLine)) currentPage++;
  Serial.print("up");  
  Serial.print("len: ");  
  Serial.println(incomingMessage.length());  
  Serial.print("div: ");  
  Serial.println(incomingMessage.length() / charactersPerLine);  
  Serial.print("currentPage: ");  
  Serial.println(currentPage);  
  sendMessage(incomingMessage, currentPage);
}

void downCommandPressed()
{
  if (currentPage > 0) currentPage--;
  Serial.print("down, currentPage: ");  
  Serial.println(currentPage);  
  sendMessage(incomingMessage, currentPage);
}

void deleteCharacterPressed()
{
  currentPage = 0;      

  outgoingMessage.remove(outgoingMessage.length() - 1);
  setCursor(0,0);  
  sendCommand(0x01); // ** Clear display        
  sendMessage(outgoingMessage, currentPage);
  Serial.println(outgoingMessage);
}

void enterCommandPressed()
{
  
  Serial.print("keyboard message: \"");        
  Serial.print(outgoingMessage);  
  Serial.println("\"");    


  if(outgoingMessage == "msg"){
    Serial1.println("AT+CMGL=\"ALL\"");    
  } else if (outgoingMessage.startsWith("msg")) {
    String message = "AT+CMGR=" + outgoingMessage.substring(3, outgoingMessage.length());
    //Serial.println(message);  
    Serial1.println(message);    
  } else if (outgoingMessage.startsWith("AT") || outgoingMessage.startsWith("at")) {
    Serial1.println(outgoingMessage);    
  } else {
    // SEND VIA SERIAL TO CELL BOARD
    // Serial1.println(outgoingMessage);
    sendTextToDefaultNumber(outgoingMessage);
  }

  // resets current data
  currentPage = 0; 
  outgoingMessage = "";

  // clear display
  sendCommand(clearDisplay); // ** Clear display
  setCursor(0,0);     
}

void sendTextToDefaultNumber(String message) {
  Serial.print("SEND TEXT VIA KEYBOARD");

  Serial1.print("AT+CMGS=\"");// must have quotes on number
  Serial1.print(defaultTelephoneNumber);
  Serial1.println("\"");// must have quotes on number
  
  //wait for > response from SIM7600
  delay(200);
  
  // send message to SIM7600
  Serial1.print(message);
  // Serial1.print(" @");
  // Serial1.print(millis());
  
  // log text part of sent message
  Serial.print("sent: ");
  Serial.println(message);
  
  // close message sending process with control-z character
  Serial1.write(0x1A);  // sends ctrl+z end of message


  Serial.println("COMPLETED");
}






// ONE WIRE OLED METHODS
// ------------------------------
void sendMessage(String message, int page)
{
  //Serial.print("send message: ");
  //Serial.println(page);
  sendCommand(clearDisplay); // ** Clear display
  setCursor(0,0);  
  String displayMessage = "";
  int msgLength = message.length();
  int start = 0;
  int stop = 0;
  int pageOffset = (charactersPerLine * currentPage);

  if(msgLength > displayCharacters) {
    //assumes we want the last 32 characters of message
    start = (msgLength - displayCharacters) - (pageOffset);
    stop = msgLength - (pageOffset);

    if(start < 0) {
      start = 0;
      stop = displayCharacters;
    }

    Serial.print("currentPage: ");        
    Serial.print(currentPage);  
    Serial.print(" start: ");        
    Serial.print(start);  
    Serial.print(" stop: ");    
    Serial.println(stop);     

    displayMessage = message.substring(start, stop);
    
  }else{
    displayMessage =  message;
  }

  unsigned char i = 0;
  setCursor(0, 0);
  while(displayMessage[i] && i <= 32)
  {
    sendData(displayMessage[i]); // * Show String to OLED
    if(i == 15) {
      setCursor(0, 1);
    }
    i++;
  }
}

void sendData(unsigned char data)
{
  // Serial.print("sendData");
  Wire.beginTransmission(OLED_Address); // ** Start I2C
  Wire.write(OLED_Data_Mode); // ** Set OLED Data mode
  Wire.write(data);
  Wire.endTransmission(); // ** End I2C
}

void sendCommand(unsigned char command)
{
  Wire.beginTransmission(OLED_Address); // ** Start I2C
  Wire.write(OLED_Command_Mode); // ** Set OLED Command mode
  Wire.write(command);
  Wire.endTransmission(); // ** End I2C
  delay(10);
}

void setCursor(uint8_t col, uint8_t row)
{
  int row_offsets[] = { 0x00, 0x40 };
  sendCommand(0x80 | (col + row_offsets[row]));
}

void SetupScreen(void){
  // * I2C initial * //
  delay(100);
  sendCommand(0x2A); // ** Set "RE"=1 00101010B
  sendCommand(0x71);
  sendCommand(0x5C);
  sendCommand(0x28);

  sendCommand(0x08); // ** Set Sleep Mode On
  sendCommand(0x2A); // ** Set "RE"=1 00101010B
  sendCommand(0x79); // ** Set "SD"=1 01111001B

  sendCommand(0xD5);
  sendCommand(0x70);
  sendCommand(0x78); // ** Set "SD"=0

  sendCommand(0x08); // ** Set 5-dot, 3 or 4 line(0x09), 1 or 2 line(0x08)

  sendCommand(0x06); // ** Set Com31->Com0 Seg0->Seg99

  // ** Set OLED Characterization * //
  sendCommand(0x2A); // ** Set "RE"=1
  sendCommand(0x79); // ** Set "SD"=1

  // ** CGROM/CGRAM Management * //
  sendCommand(0x72); // ** Set ROM
  sendCommand(0x00); // ** Set ROM A and 8 CGRAM

  sendCommand(0xDA); // ** Set Seg Pins HW Config
  sendCommand(0x10);

  sendCommand(0x81); // ** Set Contrast
  sendCommand(0xFF);

  sendCommand(0xDB); // ** Set VCOM deselect level
  sendCommand(0x30); // ** VCC x 0.83

  sendCommand(0xDC); // ** Set gpio - turn EN for 15V generator on.
  sendCommand(0x03);

  sendCommand(0x78); // ** Exiting Set OLED Characterization
  sendCommand(0x28);
  sendCommand(0x2A);
  //sendCommand(0x05); // ** Set Entry Mode
  sendCommand(0x06); // ** Set Entry Mode
  sendCommand(0x08);
  sendCommand(0x28); // ** Set "IS"=0 , "RE" =0 //28
  sendCommand(0x01);
  sendCommand(0x80); // ** Set DDRAM Address to 0x80 (line 1 start)

  delay(100);
  sendCommand(0x0C); // ** Turn on Display
}

