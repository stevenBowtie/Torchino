/*
Ideas:
  Indicator lights
  Alarm output and timeouts
*/
//GRBL Mega pins
int FEED_HOLD = 2; 
int START =     3;
int SPINDLE =   4;

//inputs from torch
int TORCH_UP_SIG=    7;
int TORCH_DWN_SIG=   8;
int TORCH_BUMP=      9;
int TORCH_DRV_UP=    10;
int TORCH_DRV_DWN=   11;
int TORCH_DWN_LMT=   12;
int TORCH_UP_LMT =   13;


//Lotos Connector
int PLASMA_START =	5;
int PLASMA_OK =   	6;

//state definitions
#define IDLE    0
#define PIERCE  1
#define CUTTING 2
#define RETRACT 3
#define ERROR   4
int state=0;

unsigned long TORCH_RETRACT_TIME = 500; //Retract after touch
unsigned long ARC_TIMEOUT = 5000;		//Time to wait for plasma OK signal
unsigned long arcBegin = 0;				//When the arc was lit
unsigned long torch_pierce_delay = 5000;				//Wait after arc lit

void setup(){
  Serial.begin(115200);

  //Configure pins
  pinMode(FEED_HOLD, OUTPUT);
  pinMode(START, OUTPUT);
  pinMode(SPINDLE, INPUT);
  pinMode(TORCH_UP_SIG, INPUT_PULLUP);
  pinMode(TORCH_DWN_SIG, INPUT_PULLUP);
  pinMode(TORCH_BUMP, INPUT);
  pinMode(TORCH_DRV_UP, OUTPUT);
  digitalWrite(TORCH_DRV_UP,1);
  pinMode(TORCH_DRV_DWN, OUTPUT);
  digitalWrite(TORCH_DRV_DWN,1);
  pinMode(TORCH_UP_LMT, INPUT);
  pinMode(TORCH_DWN_LMT, INPUT);
  digitalWrite(TORCH_DRV_UP,0);
  pinMode(PLASMA_START, OUTPUT);
  digitalWrite(PLASMA_START, 1);
  pinMode(PLASMA_OK, INPUT_PULLUP);
  delay(1);
  while(!digitalRead(TORCH_UP_LMT)){
    Serial.println(digitalRead(TORCH_UP_LMT));
  }
  digitalWrite(TORCH_DRV_UP,1);
  //Latch until GRBL controller is sane
  //Serial.println("Wait for spindle");
  //while( !digitalRead(SPINDLE) ){ } //Loop to wait for spindle signal cancel
  //Serial.println("Wait for !spindle");
  //while( digitalRead(SPINDLE) ){ } //Loop to wait for spindle signal cancel
}

void loop(){
  Serial.println("Idle");
  while(state==IDLE){
    delay(10);//Noise debouncing 
    if( digitalRead(SPINDLE) ){ 
      state=PIERCE; 
      Serial.println("Piercing...");
    }  
  }
  
  while(state==PIERCE){
    digitalWrite(FEED_HOLD,1);          //Pause machine motion, may need a dwell here
    digitalWrite(TORCH_DRV_DWN,0);
    delay(10);//Noise debouncing
    while( !digitalRead(TORCH_BUMP) ){ }  //Leave motor running until we see a limit switch
    digitalWrite(TORCH_DRV_DWN, 1);
    digitalWrite(TORCH_DRV_UP,0);
    delay(10);//Noise debouncing
	  while( digitalRead(TORCH_BUMP) ){  }  //Retract until the switch releases
    delay(TORCH_RETRACT_TIME);
    digitalWrite(TORCH_DRV_UP,1);       //How is the torch being "lit"?
    digitalWrite(PLASMA_START,0);
    arcBegin=millis();
    Serial.println("Arc ignition");
    delay(100);//Noise debouncing
    while( 1 ){
      if(!digitalRead(PLASMA_OK)){
        Serial.println("Arc lit");
        delay(torch_pierce_delay);
        break;
      }
      if(millis()-arcBegin > ARC_TIMEOUT){
        digitalWrite(PLASMA_START,1);
        Serial.println("Arc Timeout reached");
        state=ERROR;
        break;
      }
  	}
    if(state==ERROR){ break; }
    digitalWrite(FEED_HOLD,0);          //Release machine hold, shouldn't move until it sees START
    digitalWrite(START,1);              //Resume motion
    delayMicroseconds(100);                   //Debounce, extend as necessary
    digitalWrite(START,0);
    state=CUTTING;
  }

  Serial.println("CUTTING...");
  while(state==CUTTING){
    if( !digitalRead(TORCH_UP_SIG) ){
      Serial.println("Move up");
      digitalWrite(TORCH_DRV_UP,0);
      while( !digitalRead(TORCH_UP_SIG) && digitalRead(SPINDLE) ){ }  
      digitalWrite(TORCH_DRV_UP,1);
    }
    if( !digitalRead(TORCH_DWN_SIG) ){
      Serial.println("Move down");
      digitalWrite(TORCH_DRV_DWN,0);
      while(!digitalRead(TORCH_DWN_SIG) && !digitalRead(TORCH_BUMP) && digitalRead(SPINDLE) ){ 
        Serial.println("Moving down");
      }  
      digitalWrite(TORCH_DRV_DWN,1);
      if(digitalRead(TORCH_BUMP) ){
        //handle torch hard limit
      }
    }
    delay(10);
    if( !digitalRead(SPINDLE) ){
      Serial.println("Spindle off");
      state=RETRACT;
    }
    if( digitalRead(PLASMA_OK) ){
      Serial.println("Lost arc while cutting");
      digitalWrite(PLASMA_START,1);       //Turn off torch
      state=ERROR;
    }
  }
  Serial.println("RETRACT...");
  while(state==RETRACT || state==ERROR){
    digitalWrite(PLASMA_START,1);       //Turn off torch
    digitalWrite(FEED_HOLD,1);          //Pause machine motion, may need a dwell here
    digitalWrite(TORCH_DRV_UP,0);
    delay(1);
    while( !digitalRead(TORCH_UP_LMT) ){ }  //Leave motor running until we see a limit switch
    digitalWrite(TORCH_DRV_UP,1);
    digitalWrite(FEED_HOLD,0);          //Release machine hold, shouldn't move until it sees START
    digitalWrite(START,1);              //Resume motion
    delayMicroseconds(100);                   //Debounce, extend as necessary
    digitalWrite(START,0);
    if(state==ERROR){ break; }
    state=IDLE;
  }
  
  while(state==ERROR){
    Serial.println("Error");
    delay(100);
    while( digitalRead(SPINDLE) ){} //Loop to wait for spindle signal cancel
    state=IDLE;
  }
}
