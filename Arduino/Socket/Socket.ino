//CONTROLLINO MAXI

#include "Ethernet.h"
#include <SPI.h>
#include <Controllino.h>
#include <EEPROM.h>

#define VALVE_COUNT 9

byte mac[] = {0xE8, 0x2A, 0xEA, 0x4B, 0x1F, 0xC3};
IPAddress ip(192, 168, 178, 254);
unsigned int port = 23;
String stringIn;
EthernetServer server(port);

byte mode = 4;

bool isOffTime = false;//segna se è fuori orario (ad esempio dopo le 17 e prima delle 7)

//Variabili per l'esecuzione automatica
bool schedulerRunning = false;
int startH1 = 7;
int startMinute1 = 0;

bool continuosCycleActive = false;
bool continuosCycleRunning = false;
int continuosCycleIntertime = 200;
int elapsedIntertime = 0;
unsigned long lastIntertimeCheck = 0;


int startH2 = 12;
int startMinute2 = 30;
byte valveOrder[6] = {8,7,2,5,3,1};
byte currentValveIndex;
unsigned long lastValveStartTime;
unsigned long lastTimeCheck = 0;

bool useRainSensor = true;


bool hydrantOn = false;

//R0 è PER FAR FUNZIONARE LA VELOCITà 3



/**
* Classe che definisce un elettrovalvola
*/
class Valve{//valutare se aggiungere il pin a cui è collegata
  public:
    byte id;
    bool isOn; //se la valvola è attiva o meno
    bool pumpStatus; //se deve attivare la pompa 
    int onTime; //tempo per il quale la valvola deve rimanere attiva
    byte valveToActiveId; //id della valovla da attivare per quelle con una valovola principale e le altre da settare
    byte pin;
    bool isReverse = false;
    Valve(){
      id = -1;
      isOn = false;
      pumpStatus = false;
      onTime = 0;
      valveToActiveId = -1;
      pin = -1;
    }
    void initValue(byte idP, bool isOnP, bool pumpStatusP, int onTimeP, byte valveToActiveIdP, byte pinP, bool reverseP){
      id = idP;
      isOn = isOnP;
      pumpStatus = pumpStatusP;
      onTime = onTimeP;
      valveToActiveId = valveToActiveIdP;
      pin = pinP;
      isReverse = reverseP;
    };

    void setId(int idp){
      id = idp;
    }

    void setStatus(bool isOnp){
      isOn = isOnp;
    }

    void setPumpStatus(bool isOn){
      pumpStatus = isOn;
    }

    void setOnTime(int onTimep){
      onTime = onTimep;
    }

    void setValveToActivate(int idv){
      valveToActiveId = idv;
    }

    bool getValveToActivateRequiredStatus(){
      return valveToActiveId != 255 && isOn;
    }


};


Valve allValves[VALVE_COUNT];

// Stampano la risposta direttamente sull'output (client di rete o Serial), senza costruire String.
// Print e' la classe base comune a EthernetClient e Serial.
void printAll(Print &out);
void printStatus(Print &out);
void printSettings(Print &out);

void setup() {
  initLib();
  initPinStatus();
  initValves();
  initData();
  checkTime();
}

void initData(){
  mode = readIntFromEEPROM(0);
  startH1 = readIntFromEEPROM(2);
  startMinute1 = readIntFromEEPROM(4);
  
  startH2 = readIntFromEEPROM(6);
  startMinute2 = readIntFromEEPROM(8);

  continuosCycleIntertime = readIntFromEEPROM(24);

  useRainSensor = EEPROM.read(26);
}

void initLib(){
  Controllino_RTC_init();
  sei();
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.begin(9600);
}

void initValves(){
  allValves[0].initValue(1, false, true, readIntFromEEPROM(10), -1, 1234, false);//Valvola LINEA 1, spruzzatori lungo la strada

  //G1 SOPRA BARACCA PESA
  allValves[1].initValue(2, false, true, readIntFromEEPROM(12), 4, CONTROLLINO_R1, false);//QUA RENDERLA NA

  
  allValves[2].initValue(3, false, true, readIntFromEEPROM(14), 4, CONTROLLINO_R2, false);//QUA RENDERLA NA

  //G2 DIETRO BARACCA
  allValves[3].initValue(4, false, true, readIntFromEEPROM(16), 4, CONTROLLINO_R3, false);//QUA RENDERLA NA

  
  allValves[4].initValue(5, false, true, readIntFromEEPROM(18), 4, CONTROLLINO_R4, false);//Valvola LINEA 2 PRINCIPALE LINEA 2 Girandole principali con valvola master
  allValves[5].initValue(6, false, true, readIntFromEEPROM(20), 4, CONTROLLINO_R5, false);//QUA RENDERLA NA

  //Valvola principale spruzzatori
  allValves[6].initValue(7, false, true, readIntFromEEPROM(22), 4, CONTROLLINO_R8, false);


  //qua !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //Sono stati invertiti gli id delle girandole perchè non funzionava questo "risolve" il problema solo quando vengono accese separatametne altrimenti rimangono entrambe spente
  //bisogna trovare il modo di segnalare che queste 2 hanno i relè invertiti.
  //Adesso funziona solo perchè uno si attiva per tenerla chiusa ma avendo scabiato gli ingressi in realtà la accende
  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  //Spruzzatori rampa
  allValves[7].initValue(9, false, true, readIntFromEEPROM(30), 6, CONTROLLINO_D0, true);//Valvola LINEA 4 Spruzzatore su nastro con apertura automatica principale
  //spruzzatori piano
  allValves[8].initValue(8, false, true, readIntFromEEPROM(32), 6, CONTROLLINO_D1, true);//Valvola LINEA 4 Spruzzatore su nastro con apertura automatica principale
}

void initPinStatus(){
  pinMode(1234, OUTPUT);
  pinMode(CONTROLLINO_R0, OUTPUT);
  pinMode(CONTROLLINO_R1, OUTPUT);
  pinMode(CONTROLLINO_R2, OUTPUT);
  pinMode(CONTROLLINO_R3, OUTPUT);
  pinMode(CONTROLLINO_R4, OUTPUT);
  pinMode(CONTROLLINO_R5, OUTPUT);
  pinMode(CONTROLLINO_R6, OUTPUT);
  pinMode(CONTROLLINO_R7, OUTPUT);
  pinMode(CONTROLLINO_R8, OUTPUT);
  pinMode(CONTROLLINO_R9, OUTPUT);
  pinMode(CONTROLLINO_D0, OUTPUT);
  pinMode(CONTROLLINO_D1, OUTPUT);

  pinMode(CONTROLLINO_A0, INPUT);
  

  pinMode(CONTROLLINO_RTC_INTERRUPT, INPUT_PULLUP);//pin per l'interrupt del timer

// Di default tutte le valvole sono attive e la pompa è spenta
  digitalWrite(1234, HIGH);
  digitalWrite(CONTROLLINO_R0, HIGH);
  digitalWrite(CONTROLLINO_R1, HIGH);
  digitalWrite(CONTROLLINO_R2, HIGH);
  digitalWrite(CONTROLLINO_R3, HIGH);
  digitalWrite(CONTROLLINO_R4, LOW);
  digitalWrite(CONTROLLINO_R5, HIGH);
  digitalWrite(CONTROLLINO_R6, HIGH);
  digitalWrite(CONTROLLINO_R7, LOW);
  digitalWrite(CONTROLLINO_R8, HIGH);//QUA VERIFICARE SE QUESTA SENZA POMPA DEVE RIMANERE SPENTA DI DEFAULT

  digitalWrite(CONTROLLINO_D0, HIGH);
  digitalWrite(CONTROLLINO_D1, HIGH);
  
}




void loop() {

  EthernetClient client = server.available();
  if (client.connected()) {
    Serial.println(F("Client connesso"));
    char buf[64];
    byte i = 0;
    while (client.available() && i < sizeof(buf) - 1) {
      buf[i++] = client.read();
    }
    buf[i] = '\0';
    stringIn = buf;
    executeCommad(stringIn, client);
    client.stop();
  }
  if(!continuosCycleActive && (!useRainSensor || !digitalRead(CONTROLLINO_A0))){
    checkScheduler();
  }else{
    if(schedulerRunning){
      currentValveIndex = 0;
      lastValveStartTime = 0;
      schedulerRunning = false;
      setAllValveOff();
      setRelay();
      elapsedIntertime = 1;
    }

    if(continuosCycleActive)
      manageContinuosCycle();
    
  }
  if((isOffTime || mode == 4) && !isAllValveOff() && mode != 3){
    if(continuosCycleActive){
      continuosCycleActive = false;
      elapsedIntertime = 0;
    }
    setAllValveOff();
    setRelay();
  }
  
}

void manageContinuosCycle(){
  if(!continuosCycleRunning && (elapsedIntertime == 0 || continuosCycleIntertime <= elapsedIntertime)){
    continuosCycleRunning = true;
    setAllValveOff();
    setRelay();
    currentValveIndex = 0;
    elapsedIntertime = 0;
    lastValveStartTime = millis();
    setValveStatus(valveOrder[currentValveIndex], true);
    Serial.println(F("CICLO continuo avviato"));
    //Serial.println(getStatusHumanReadable());
  }else if(continuosCycleRunning){
    int runTime = abs((millis() - lastValveStartTime))/1000;
    Valve currentValve = allValves[valveOrder[currentValveIndex]];
    if(runTime > currentValve.onTime){
      Serial.println(F("CICLO CONTINUO: superato valve time"));
      int prevValveIndex = currentValveIndex;
      //prima devo accendere la nuova valvola e poi spegnere quella vecchi altrimenti la pompa si spegne e riaccende
      
      currentValveIndex += 1;
      //controllo che la valvola non sia l'ultima
      if(currentValveIndex >= 6){
        Serial.println(F("CICLO CONTINUO: superato l'inidice 6"));
        setValveStatus(valveOrder[prevValveIndex], false);
        currentValveIndex = 0;
        lastValveStartTime = 0;
        continuosCycleRunning = false;
        elapsedIntertime = 1;
        setAllValveOff();
        isAllValveOff();
        setRelay();
        lastIntertimeCheck = millis();
      }else{
        Serial.print(F("CICLO CONTINUO: Cambio valvola id: "));
        Serial.println(currentValveIndex);
        lastValveStartTime = millis();
        setValveStatus(valveOrder[currentValveIndex], true);
        setValveStatus(valveOrder[prevValveIndex], false);
        //Serial.println(getStatusHumanReadable());
      }
    }
  }else{
    if( abs(millis() - lastIntertimeCheck) > 1500){
      elapsedIntertime += abs((millis() - lastIntertimeCheck))/1000;
      lastIntertimeCheck = millis();
      Serial.println(elapsedIntertime);
    }
  }
}


void checkScheduler(){
  //qua aggiornare con la lettura effettiva

  int readedHour = 0;
  int readedMinue = 0;  
  if(!schedulerRunning && abs(millis() - lastTimeCheck) <20000){
    return;
  }else if(!schedulerRunning){
    checkTime();
    readedHour = Controllino_GetHour();
    readedMinue = Controllino_GetMinute();
    Serial.print(F("Controllo scheduler, ora letta:   "));
    Serial.print(readedHour);
    Serial.print(F(":"));
    Serial.println(readedMinue);
    lastTimeCheck = millis();
  }
  
  if(!schedulerRunning && ((
      readedHour == startH1 && readedMinue == startMinute1
    )||(
      readedHour == startH2 && readedMinue == startMinute2
    ))){
    setAllValveOff();
    setRelay();
    schedulerRunning = true;
    currentValveIndex = 0;
    lastValveStartTime = millis();
    setValveStatus(valveOrder[currentValveIndex], true);
    Serial.println(F("Scheduler avviato"));
  }else if (schedulerRunning){
    int runTime = abs((millis() - lastValveStartTime))/1000;
    Valve currentValve = allValves[valveOrder[currentValveIndex]];
    if(runTime > currentValve.onTime){
      int prevValveIndex = currentValveIndex;
      //prima devo accendere la nuova valvola e poi spegnere quella vecchi altrimenti la pompa si spegne e riaccende
      
      currentValveIndex += 1;
      //controllo che la valvola non sia l'ultima
      if(currentValveIndex >= 6){
        Serial.println(F("Reset, superato l'inidice 6"));
        setValveStatus(valveOrder[prevValveIndex], false);
        currentValveIndex = 0;
        lastValveStartTime = 0;
        schedulerRunning = false;
        setAllValveOff();
        isAllValveOff();
        setRelay();
      }else{
        Serial.print(F("Cambio valvola id: "));
        Serial.println(currentValveIndex);
        lastValveStartTime = millis();
        setValveStatus(valveOrder[currentValveIndex], true);
        setValveStatus(valveOrder[prevValveIndex], false);
      }
      
    }

  }
}


void checkTime(){
  //controlla se siamo nei weekend oppure se l'orario è compreso nel range di spegnimento
  int day = int(Controllino_GetWeekDay());
  int hour = int(Controllino_GetHour());
  //0 DOMENICA
  if(day == 0 || day == 6 || hour >= 17 || hour < 7){
    isOffTime = true;
  }else{
    isOffTime = false;
  }
}

void executeCommad(String receivedString, EthernetClient client){
  Serial.println(stringIn);
  byte dividerPos = receivedString.indexOf(':');
  String cmd = receivedString.substring(0,dividerPos);
  String data = receivedString.substring(dividerPos+1,receivedString.indexOf(';'));
  Serial.print(F("CMD: "));
  Serial.println(cmd);
  Serial.print(F("DATA: "));
  Serial.println(data);
  if(cmd == "GET_ALL"){
    printAll(client);
  }else if(cmd == "GET_ALL_SERIAL"){
    Serial.print(F("OFF_TIME: "));
    Serial.println(isOffTime);
  }else if(cmd == "GET_DATE"){
    client.print(getDate());
  }else if(cmd == "SET_DATE"){
    setDate(data);
    client.print(F("OK"));
  }else if(cmd == "SET_PARAM"){
    //qua passare il parametro
    byte innerDividerPos = data.indexOf('#');
    String pName = data.substring(0,innerDividerPos);
    String pValue = data.substring(innerDividerPos+1,data.indexOf(';'));
    setParam(pName, pValue);
    client.print(F("OK"));
  }else if(cmd == "MANUAL_CONTROL" && (!isOffTime || mode == 3)){
    setValveStatusManual(data);
    isAllValveOff();
  }else if(cmd == "STORE_CHANGES"){
    updateEEPROM();
    client.print(F("OK"));
  }else if (cmd == "CONTINUOS_CYCLE"){
    setContinuosCycle(data);
    isAllValveOff();
  }
}

void setContinuosCycle(String data){
  if(data == "ON"){
    continuosCycleActive = true;
    continuosCycleRunning = false;
    elapsedIntertime = 0;
    currentValveIndex = 0;
    lastValveStartTime = 0;
    schedulerRunning = false;
    setAllValveOff();
    setRelay();
  }else{
    continuosCycleActive = false;
    continuosCycleRunning = false;
    elapsedIntertime = 0;
    currentValveIndex = 0;
    lastValveStartTime = 0;
    schedulerRunning = false;
    setAllValveOff();
    setRelay();
  }
}

void updateEEPROM(){
  writeIntIntoEEPROM(0, mode);
  writeIntIntoEEPROM(2, startH1);
  writeIntIntoEEPROM(4, startMinute1);
  writeIntIntoEEPROM(6, startH2);
  writeIntIntoEEPROM(8, startMinute2);

  
  writeIntIntoEEPROM(10, allValves[0].onTime);
  writeIntIntoEEPROM(12, allValves[1].onTime);
  writeIntIntoEEPROM(14, allValves[2].onTime);
  writeIntIntoEEPROM(16, allValves[3].onTime);
  writeIntIntoEEPROM(18, allValves[4].onTime);
  writeIntIntoEEPROM(20, allValves[5].onTime);
  writeIntIntoEEPROM(22, allValves[6].onTime);

  //spruzzatori rampa e piano
  writeIntIntoEEPROM(30, allValves[7].onTime);
  writeIntIntoEEPROM(32, allValves[8].onTime);

  writeIntIntoEEPROM(24, continuosCycleIntertime);

  EEPROM.update(26, useRainSensor);

}


void writeIntIntoEEPROM(int address, int number)
{ 
  EEPROM.update(address, number >> 8);
  EEPROM.update(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}

void setParam(String paramName, String paramValue){
  if(paramName == "MODE"){
    setMode(paramValue);
  }else if(paramName == "STARTTIME1"){
    byte innerDividerPos = paramValue.indexOf(':');
    String h = paramValue.substring(0,innerDividerPos);
    String m = paramValue.substring(innerDividerPos+1,paramValue.indexOf(';'));
    startH1 = h.toInt();
    startMinute1 = m.toInt();
  }else if(paramName == "STARTTIME2"){
    byte innerDividerPos = paramValue.indexOf(':');
    String h = paramValue.substring(0,innerDividerPos);
    String m = paramValue.substring(innerDividerPos+1,paramValue.indexOf(';'));
    startH2 = h.toInt();
    startMinute2 = m.toInt();
  }else if(paramName == "ONTIME1"){
    Serial.print(F("name :"));
    Serial.println(paramName);
    Serial.print(F("value :"));
    Serial.println(paramValue);
    allValves[1].setOnTime(paramValue.toInt());
  }else if(paramName == "ONTIME2"){
    allValves[2].setOnTime(paramValue.toInt());
  }else if(paramName == "ONTIME3"){
    allValves[3].setOnTime(paramValue.toInt());
  }else if(paramName == "ONTIME4"){
    allValves[5].setOnTime(paramValue.toInt());
  }else if(paramName == "ONTIME5"){
    allValves[6].setOnTime(paramValue.toInt());
  }else if(paramName == "ONTIME6"){
    allValves[7].setOnTime(paramValue.toInt());
  }else if(paramName == "ONTIME7"){
    allValves[8].setOnTime(paramValue.toInt());
  }else if (paramName == "CONTINUOS_CYCLE_INTERTIME"){
    continuosCycleIntertime = paramValue.toInt();
  }else if (paramName == "RAIN_SENSOR"){
    if(paramValue == "ACTIVE"){
      useRainSensor = true;
    }else{
      useRainSensor = false;
    }
  }
  
}

void setValveStatusManual(String data){
  String valveId = data.substring(0, data.indexOf('='));
  String status = data.substring(data.indexOf('=') + 1);
  Serial.println(valveId);
  Serial.println(status);

  if(valveId=="IDR"){
    Serial.println(F("Controllo idrante"));
    if(status == "ON"){
      hydrantOn = true;
      setPumpStatus(true);
       
    }
    else{
      hydrantOn = false;
      setPumpStatus(false);
    }
  }else{
    Serial.println(F("Comando valvola"));
    Valve currentValve = allValves[valveId.toInt()-1];
  
    //vedere se sostituire questa sezione con quella sotto
    if(status == "ON")
      currentValve.isOn = true;
    else
      currentValve.isOn = false;
    allValves[valveId.toInt()-1] = currentValve;
    applyOutput(currentValve);
  }
  
}

void setValveStatus(int valveId, bool status){
  Valve currentValve = allValves[valveId];
    currentValve.isOn = status;
  allValves[valveId] = currentValve;
  applyOutput(currentValve);

  
}

void applyOutput(Valve currentValve){
  Serial.println(currentValve.valveToActiveId);
  updateMainValve(currentValve);
  setRelay();
}

void updateMainValve(Valve currentValve){
  if(currentValve.valveToActiveId != 255){
    Valve mainValve = allValves[currentValve.valveToActiveId];
    mainValve.isOn = checkMainValveActive();
    allValves[currentValve.valveToActiveId] = mainValve;

    if(currentValve.valveToActiveId == 6){
      Serial.println(F("Attivata 4 tramite 6"));
      Valve mainValve2 = allValves[4];
      mainValve2.isOn = checkMainValveActive();
      allValves[4] = mainValve2;
    }
    
  }
}

void setRelay(){
  if(isAllValveOff()){
    setPumpStatus(false);
    digitalWrite(1234, HIGH);
    digitalWrite(CONTROLLINO_R1, HIGH);
    digitalWrite(CONTROLLINO_R2, HIGH);
    digitalWrite(CONTROLLINO_R3, HIGH);
    digitalWrite(CONTROLLINO_R4, LOW);
    digitalWrite(CONTROLLINO_R5, HIGH);
    digitalWrite(CONTROLLINO_R8, HIGH);//QUA VERIFICARE SE QUESTA SENZA POMPA DEVE RIMANERE SPENTA DI DEFAULT

    digitalWrite(CONTROLLINO_D0, HIGH); //IS REVERSE è A TRUE
    digitalWrite(CONTROLLINO_D1, HIGH); // IS REVERSE è A TRUE
  }else{
    bool needPump = false;
    for(int i = 0; i< VALVE_COUNT; i++){//qua cambiare mettendo la lunghezza dell'array
      if(!allValves[i].isReverse){
        digitalWrite(allValves[i].pin, allValves[i].isOn);
      }else{
        digitalWrite(allValves[i].pin, !allValves[i].isOn);
      }
      if(allValves[i].pumpStatus && allValves[i].isOn){
        needPump = true;
      }
    }
    setPumpStatus(needPump);
  }

}

/**
* Setta lo stato della pompa 
* true: attivata dal plc
* false: in modalità manuale
*/
void setPumpStatus(bool isOn){
  //qua dovrà accendere e spegnere la poma gestendo il deviatore
  //la pompa è collegata al comune tra R6 e R7 
  //R6 è la modalità manuale R7 quella gestita dal plc
  if(isOn){
    digitalWrite(CONTROLLINO_R6, LOW);
    delay(200);
    digitalWrite(CONTROLLINO_R7, HIGH);
    digitalWrite(CONTROLLINO_R0, LOW);
    digitalWrite(CONTROLLINO_R4, HIGH);
    Serial.println(F("pumpOn"));

    /*R0 HIGH VELOCITà LENTA
    if(!allValves[1].isOn && !allValves[3].isOn && !allValves[2].isOn && !allValves[5].isOn && allValves[6].isOn){
      digitalWrite(CONTROLLINO_R0, HIGH);
    }else{
      digitalWrite(CONTROLLINO_R0, LOW);
    }
    */
    
  }else{
    digitalWrite(CONTROLLINO_R7, LOW);
    delay(2000);
    Serial.println(F("pumpOFF"));
    digitalWrite(CONTROLLINO_R6, HIGH);
    digitalWrite(CONTROLLINO_R0, HIGH);
    digitalWrite(CONTROLLINO_R4, LOW);
    if(hydrantOn){
      hydrantOn = false;
    }
  }
}

bool isAllValveOff(){
  for(int i = 0; i< VALVE_COUNT; i++){//qua cambiare mettendo la lunghezza dell'array
    if(allValves[i].isOn && i != 4 && i != 6){
      if(!digitalRead(CONTROLLINO_R9) == HIGH)
        digitalWrite(CONTROLLINO_R9, HIGH);
        
      return false;
    }
  }

  if(digitalRead(CONTROLLINO_R9) == HIGH)
    digitalWrite(CONTROLLINO_R9, LOW);

  //qua per prova
  //setAllValveOff();
  //setRelay();
  return true;
}

void setAllValveOff(){
  for(int i = 0; i< VALVE_COUNT; i++){//qua cambiare mettendo la lunghezza dell'array
    allValves[i].isOn = false;
  }
}

/**
*Controlla se c'è al meno una valvola che richieda la valvola principale attiva
*/
bool checkMainValveActive(){
  for(int i = 0; i< VALVE_COUNT; i++){//qua cambiare mettendo la lunghezza dell'array
    if(allValves[i].getValveToActivateRequiredStatus())
      return true;
  }
  return false;
}

void printStatus(Print &out){
  out.print(F("{\n"));
  out.print(F("\t\"valve1\":")); out.print(allValves[0].isOn); out.print(F(",\n"));
  out.print(F("\t\"valve2\":")); out.print(allValves[1].isOn); out.print(F(",\n"));
  out.print(F("\t\"valve3\":")); out.print(allValves[2].isOn); out.print(F(",\n"));
  out.print(F("\t\"valve4\":")); out.print(allValves[3].isOn); out.print(F(",\n"));
  out.print(F("\t\"valve5\":")); out.print(allValves[4].isOn); out.print(F(",\n"));
  out.print(F("\t\"valve6\":")); out.print(allValves[5].isOn); out.print(F(",\n"));
  out.print(F("\t\"valve7\":")); out.print(allValves[6].isOn); out.print(F(",\n"));
  out.print(F("\t\"valve8\":")); out.print(allValves[7].isOn); out.print(F(",\n"));
  out.print(F("\t\"valve9\":")); out.print(allValves[8].isOn); out.print(F(",\n"));
  out.print(F("\t\"idr\":")); out.print(hydrantOn); out.print(F(",\n"));
  out.print(F("\t\"rainSensor\":")); out.print(!digitalRead(CONTROLLINO_A0)); out.print(F("\n"));
  out.print(F("}"));
}


String getStatusHumanReadable(){
  int day = int(Controllino_GetWeekDay());
    String stat = "{\n";
  stat = stat + "\t\"valve1\":"+ allValves[0].isOn +",\n";
  
  stat = stat + "\t\"G1\":"+ allValves[1].isOn +",\n";
  stat = stat + "\t\"G2\":"+ allValves[3].isOn +",\n";
  stat = stat + "\t\"G3\":"+ allValves[2].isOn +",\n";
  stat = stat + "\t\"G4\":"+ allValves[5].isOn +",\n";
  
  stat = stat + "\t\"valve5\":"+ allValves[4].isOn +",\n";
  
  stat = stat + "\t\"PRINCIPALE ELETTROVALVOLE\":"+ allValves[6].isOn +",\n";
  stat = stat + "\t\"S1\":"+ allValves[7].isOn +",\n";
  stat = stat + "\t\"S2\":"+ allValves[8].isOn +",\n";
  stat = stat + "\t\"Giorno settimana\":"+ day +",\n";
  stat = stat + "\t\"MODE\":"+ mode +",\n";
  stat = stat + "\t\"isOffTime\":"+ isOffTime +",\n";
  stat = stat + "\t\"rainSensor\":"+ !digitalRead(CONTROLLINO_A0)+"\n";
  stat = stat + "}";

  Serial.println(stat);
  return stat;
}

void printSettings(Print &out){
  out.print(F("{\n"));
  out.print(F("\t\"mode\":")); out.print(mode); out.print(F(",\n"));
  out.print(F("\t\"startTime1\":\"")); out.print(startH1); out.print(F(":")); out.print(startMinute1); out.print(F("\",\n"));
  out.print(F("\t\"startTime2\":\"")); out.print(startH2); out.print(F(":")); out.print(startMinute2); out.print(F("\",\n"));
  out.print(F("\t\"currentControllerDate\":\"")); out.print(getDate()); out.print(F("\",\n"));
  out.print(F("\t\"onTime1\":")); out.print(allValves[1].onTime); out.print(F(",\n"));
  out.print(F("\t\"onTime2\":")); out.print(allValves[2].onTime); out.print(F(",\n"));
  out.print(F("\t\"onTime3\":")); out.print(allValves[3].onTime); out.print(F(",\n"));
  out.print(F("\t\"onTime4\":")); out.print(allValves[5].onTime); out.print(F(",\n"));
  out.print(F("\t\"onTime5\":")); out.print(allValves[6].onTime); out.print(F(",\n"));
  out.print(F("\t\"onTime6\":")); out.print(allValves[7].onTime); out.print(F(",\n"));
  out.print(F("\t\"onTime7\":")); out.print(allValves[8].onTime); out.print(F(",\n"));
  out.print(F("\t\"continuosCycleActive\":")); out.print(continuosCycleActive); out.print(F(",\n"));
  out.print(F("\t\"cycleIntertime\":")); out.print(continuosCycleIntertime); out.print(F(",\n"));
  out.print(F("\t\"rainSensorActive\":")); out.print(useRainSensor); out.print(F("\n"));
  out.print(F("}"));
}

//qua da testare
void printAll(Print &out){
  out.print(F("{\n"));
  out.print(F("\t\"statuses\":")); printStatus(out); out.print(F(",\n"));
  out.print(F("\t\"settings\":")); printSettings(out); out.print(F("\n"));
  out.print(F("}"));
}


//La stringa con la data deve essere in questo formato "Jul 25 2022-08:43:00"
String setDate(String dateTime){
  String tmpDate = dateTime.substring(0, dateTime.indexOf('-'));
  String tmpTime = dateTime.substring(dateTime.indexOf('-') + 1);

  char date[tmpDate.length() + 1];
  char time[tmpTime.length() + 1];
  tmpDate.toCharArray(date, tmpDate.length() + 1);
  tmpTime.toCharArray(time, tmpTime.length() + 1);

  Controllino_SetTimeDateStrings(date, time);
}

String getDate(){
  unsigned char day, weekday, month, year, hour, minute, second;
	if (Controllino_ReadTimeDate(&day, &weekday, &month, &year, &hour, &minute, &second) >= 0)
	{
    String res;
    res = String(day);
    res = String(res + '/');
    res = String(res + month);
    res = String(res + '/');
    res = String(res + year);
    res = String(res + 'T');
    res = String(res + hour);
    res = String(res + ':');
    res = String(res + minute);
    res = String(res + ':');
    res = String(res + second);
		return res;
	}
  return "ERROR";
}


/**
* Setta la modalità 
* 1) Automatico
* 2) Manuale
* 3) Controlli disattivati
* 4) Off
* In modalità inverno si vogliono aprire le elettrovalvole?
*/
void setMode(String modep){
  if(modep == "1"){
    mode = 1;
  }else if(modep == "2"){
    mode = 2;
  }else if (modep == "3"){
    mode = 3;
  }else {
    mode = 4;
  }
}
