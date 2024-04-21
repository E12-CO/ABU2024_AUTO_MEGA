
#define kickball_IN1 33 // เตะสุดกำลัง ถอยชน limit switch
#define kickball_IN2 31 // ถอย 100
#define kickball_PWM 11

#define keepball_IN1 29 // หนีบสุดกำลัง ปล่อยตาม limit switch
#define keepball_IN2 27 // 50
#define keepball_PWM 8

#define pushball_IN1 26 // ลงแตะ limit switch,ขึ้นแตะ limit switch
#define pushball_IN2 28 // 100
#define pushball_PWM 7

#define lim_kick1 1 
#define lim_kick2 1
#define lim_keep1 1
#define lim_keep2 1
#define lim_push1 1
#define lim_push2 1
// 1,+ = บน,ซ้าย 2,- = ล่าง,ขวา มุมมองจากหลังหุ่น

#define color_sensor 7
#define catched_ball 0
bool left = false;
bool right = false;
uint32_t Time = 0; 
int pwm = 0;
int numbers = 0;
void motorDrive(uint8_t pin, int val) {
  switch (pin) {
    case kickball_PWM:
      // OCR2A = (uint8_t)((val < 0) ? -val : val);
      analogWrite(kickball_PWM,abs(pwm));
      digitalWrite(kickball_IN1, (val <= 0) ? 0 : 1);
      digitalWrite(kickball_IN2, (val < 0) ? 1 : 0);
      break;

    case pushball_PWM:
      // OCR1AL = (uint8_t)((val < 0) ? -val : val);
      analogWrite(pushball_PWM,abs(pwm));
      digitalWrite(pushball_IN1, (val <= 0) ? 0 : 1);
      digitalWrite(pushball_IN2, (val < 0) ? 1 : 0);
      break;

    case keepball_PWM:
      // OCR1CL = (uint8_t)((val < 0) ? -val : val);
      analogWrite(keepball_PWM,abs(pwm));
      digitalWrite(keepball_IN1, (val <= 0) ? 0 : 1);
      digitalWrite(keepball_IN2, (val < 0) ? 1 : 0);
      break;

    default:
      return;
   }
}

void setup() {
  Serial.begin(115200);
  //OUTPUT
  pinMode(kickball_IN1,OUTPUT);
  pinMode(kickball_IN2,OUTPUT);
  pinMode(pushball_IN1,OUTPUT);
  pinMode(pushball_IN2,OUTPUT);
  pinMode(keepball_IN1,OUTPUT);
  pinMode(keepball_IN2,OUTPUT);
  pinMode(kickball_PWM,OUTPUT);
  pinMode(pushball_PWM,OUTPUT);
  pinMode(keepball_PWM,OUTPUT);

  //INPUT
  // -- limit switch --
  pinMode(lim_kick1,INPUT);
  pinMode(lim_kick2,INPUT);
  pinMode(lim_push1,INPUT);
  pinMode(lim_push2,INPUT);
  pinMode(lim_keep1,INPUT);
  pinMode(lim_keep1,INPUT);
  // -- rgb sensor

  pinMode(color_sensor,INPUT);
  
}

String reset(int scroll_down_speed = 100,int open_keeping_speed = 50){
  uint8_t flag = 0;
  while(lim_push2 == 0 && millis() - Time < 5000){
    motorDrive(pushball_PWM,-scroll_down_speed);
    // Serial.println(millis()-Time);
  }
  motorDrive(pushball_PWM,0);
  if(millis()-Time >= 5000) flag=1;
  Time = millis();
  while(lim_keep1 == 0 && millis()-Time < 3000){
    motorDrive(keepball_PWM,open_keeping_speed);
    // Serial.println(millis()-Time);
  }
  motorDrive(keepball_PWM,0);
  if(millis()-Time >= 3000 && flag == 1) flag=3;
  else if(millis()-Time >= 3000 && flag != 1) flag=2;
  switch(flag){
    case 0:
      return "Successfully setup";
      break;
    case 1:
      return "Scroller Timeout";
      break;
    case 2:
      return "Keeper Timeout";
      break;
    case 3:
      return "Scroller and Keeper Timeout";
      break;
  }
  return "Successfully setup";
}

String keep(int scroll_up_speed = 100, int keeping_speed = 255){
  while(millis()-Time < 3000){
    if(lim_keep2==1){
      motorDrive(keepball_PWM,0);
      return "not catch";
    }
    else
      motorDrive(keepball_PWM,-keeping_speed);
    // Serial.println(millis()-Time);
  }
  Time = millis();
  while(lim_push1 == 0 && millis() - Time < 5000){
    motorDrive(pushball_PWM,scroll_up_speed);
    // Serial.println(millis()-Time);
  }
  motorDrive(pushball_PWM,0);
  if(catched_ball == 0) return "not catch";
  if(millis()-Time >= 5000) return "Scroll Timeout";
  return "Successfully keeping";
}

String send(int kick_back_speed = 100,int kick_front_speed = 255){
  uint8_t flag = 0;
  while(millis()-Time < 2000 && lim_kick2==0){
    motorDrive(kickball_PWM,kick_front_speed);
    // Serial.println(millis()-Time);
  }
  motorDrive(kickball_PWM,0);
  if(millis()-Time >= 2000) flag = 1;
  Time = millis();
  while(millis()-Time < 2000 && lim_kick1==0){
    motorDrive(kickball_PWM,kick_back_speed);
    // Serial.println(millis()-Time);
  }
  if(millis()-Time >= 2000 && flag == 1) flag=3;
  else if(millis()-Time >= 2000 && flag != 1) flag=2;
  motorDrive(kickball_PWM,0);
  motorDrive(keepball_PWM,0);
  if(catched_ball == 1) return "send ball failed";
  switch(flag){
    case 0:
      return "Successfully kick";
      break;
    case 1:
      return "Kick front Timeout";
      break;
    case 2:
      return "Kick Back Timeout";
      break;
    case 3:
      return "Kick front and back Timeout";
      break;
  }
  return "Successfully kick";
}
String *receive_command(String incomingString) {
  static String command[10];
  char *message = new char[incomingString.length() + 1];
  strcpy(message, incomingString.c_str());

  char *token = strtok(message, ",");
  while (token != NULL && numbers < 10) {
    command[numbers++] = String(token);
    token = strtok(NULL, ",");
  }

  delete[] message; 
  return command;
}


String implementation(String *command,int numbers){
  switch(numbers){
      case 1:
        Time = millis();
        if(command[0]=="reset")
          return reset();
        else if(command[0] == "keep")
          return keep();
        else if(command[0] == "send")
          return send();
        break;
      case 2:
        Time = millis();
        if(command[0]=="reset")
          return reset(command[1].toInt());
        else if(command[0] == "keep")
          return keep(command[1].toInt());
        else if(command[0] == "send")
          return send(command[1].toInt());
        break;
      case 3:
        Time = millis();
        if(command[0]=="reset")
          return reset(command[1].toInt(),command[2].toInt());
        else if(command[0] == "keep")
          return keep(command[1].toInt(),command[2].toInt());
        else if(command[0] == "send")
          return send(command[1].toInt(),command[2].toInt());
        break;
      
      default:
        break;
    }
}
void loop(){

  if (Serial.available() > 0) {
    numbers = 0;
    String incomingString = Serial.readStringUntil('\n'); // Read until newline character
    String *command = receive_command(incomingString);
    String response = implementation(command,numbers);
    Serial.println(response);
    }
    // delay(1000);
    // String responseString = "Response: " + incomingString;

    // Send the response back to the laptop
    // Serial.print("Arduino sending back: ");
    // Serial.println(responseString);
  }
  // if(Serial.available() > 0){
  //   char incomingChar = Serial.read();
  //   if(incomingChar == 'a'){
  //     pwm = 50;
  //   }else if(incomingChar == 'd'){
  //     pwm = -50;
  //   }
  //   else if(incomingChar != '\n')
  //     pwm = 0;
  // }
  // motorDrive(keepball_PWM,pwm);
  // motorDrive(pushball_PWM,pwm);