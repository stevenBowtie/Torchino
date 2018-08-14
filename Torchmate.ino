/*
Ideas:
  Indicator lights
  Alarm output and timeouts
*/
//GRBL Mega pins
#define FEED_HOLD 2 
#define START     3
#define SPINDLE   4

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
int state=0;

unsigned long TORCH_RETRACT_TIME = 500; //Retract after touch
unsigned long ARC_TIMEOUT = 2000;		//Time to wait for plasma OK signal
unsigned long arcBegin = 0;				//When the arc was lit

void setup(){
  Serial.begin(115200);

  //Configure pins
  pinMode(FEED_HOLD, OUTPUT);
  pinMode(START, OUTPUT);
  pinMode(SPINDLE, INPUT_PULLUP);
  pinMode(TORCH_UP_SIG, INPUT);
  pinMode(TORCH_DWN_SIG, INPUT);
  pinMode(TORCH_BUMP, INPUT);
  pinMode(TORCH_DRV_UP, OUTPUT);
  digitalWrite(TORCH_DRV_UP,1);
  pinMode(TORCH_DRV_DWN, OUTPUT);
  digitalWrite(TORCH_DRV_DWN,1);
  pinMode(TORCH_UP_LMT, INPUT);
  pinMode(TORCH_DWN_LMT, INPUT);
  digitalWrite(TORCH_DRV_UP,0);
  delay(1);
  while(!digitalRead(TORCH_UP_LMT)){
    Serial.println(digitalRead(TORCH_UP_LMT));
  }
  digitalWrite(TORCH_DRV_UP,1);
}

void loop(){
  Serial.println("Idle");
  while(state==IDLE){
    if( digitalRead(SPINDLE) ){ 
      state=PIERCE; 
      Serial.println("Piercing...");
    }  
  }
  
  while(state==PIERCE){
    digitalWrite(FEED_HOLD,1);          //Pause machine motion, may need a dwell here
    digitalWrite(TORCH_DRV_DWN,0);
    while( !digitalRead(TORCH_BUMP) ){ }  //Leave motor running until we see a limit switch
    digitalWrite(TORCH_DRV_DWN, 1);
    digitalWrite(TORCH_DRV_UP,0);
	  while( digitalRead(TORCH_BUMP) ){  }  //Retract until the switch releases
    delay(TORCH_RETRACT_TIME);
    digitalWrite(TORCH_DRV_UP,1);       //How is the torch being "lit"?
    digitalWrite(PLASMA_START,0);
    arcBegin=millis();
    Serial.println("Arc ignition");
    while( !digitalRead(PLASMA_OK) ){
      if(millis()-arcBegin > ARC_TIMEOUT){
        Serial.println("Arc Timeout reached");
        state=RETRACT;
        break;
      }
  	}
    if(state==RETRACT){ break; }
    digitalWrite(FEED_HOLD,0);          //Release machine hold, shouldn't move until it sees START
    digitalWrite(START,1);              //Resume motion
    delayMicroseconds(100);                   //Debounce, extend as necessary
    digitalWrite(START,0);
    state=CUTTING;
  }

  Serial.println("CUTTING...");
  while(state==CUTTING){
    if( digitalRead(TORCH_UP_SIG) ){
      digitalWrite(TORCH_DRV_UP,0);
      while( digitalRead(TORCH_UP_SIG) && digitalRead(SPINDLE) ){  }  
      digitalWrite(TORCH_DRV_UP,1);
    }
    if( digitalRead(TORCH_DWN_SIG) ){
      digitalWrite(TORCH_DRV_DWN,0);
      while(digitalRead(TORCH_UP_SIG) && !digitalRead(TORCH_BUMP) && digitalRead(SPINDLE) ){  }  
      digitalWrite(TORCH_DRV_DWN,1);
      if(digitalRead(TORCH_BUMP) ){
        //handle torch hard limit
      }
    }
    if( digitalRead(SPINDLE) ){
      state=RETRACT;
    }
  }
  Serial.println("RETRACT...");
  while(state==RETRACT){
    //Turn of torch?
    digitalWrite(FEED_HOLD,1);          //Pause machine motion, may need a dwell here
    digitalWrite(TORCH_DRV_UP,0);
    delay(1);
    while( !digitalRead(TORCH_UP_LMT) ){ }  //Leave motor running until we see a limit switch
    digitalWrite(TORCH_DRV_UP,1);
    digitalWrite(FEED_HOLD,0);          //Release machine hold, shouldn't move until it sees START
    digitalWrite(START,1);              //Resume motion
    delayMicroseconds(100);                   //Debounce, extend as necessary
    digitalWrite(START,0);
    state=IDLE;
  }
  
}
