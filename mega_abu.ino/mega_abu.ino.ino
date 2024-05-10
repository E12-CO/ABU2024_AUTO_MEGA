
#define kickball_IN1 33 // เตะสุดกำลัง ถอยชน limit switch
#define kickball_IN2 31 // ถอย 100
#define kickball_PWM 11

#define keepball_IN1 29 // หนีบสุดกำลัง ปล่อยตาม limit switch
#define keepball_IN2 27 // 50
#define keepball_PWM 8

#define pushball_IN1 26 // ลงแตะ limit switch,ขึ้นแตะ limit switch
#define pushball_IN2 28 // 100
#define pushball_PWM 7

#define lim_kick1 38
#define lim_kick2 40
#define lim_keep1 42
#define lim_keep2 44
#define lim_push1 46
#define lim_push2 48
// 1,+ = บน,ซ้าย 2,- = ล่าง,ขวา มุมมองจากหลังหุ่น

#define lim_ball_catch  2

#define color_sensor 7
#define catched_ball 0
bool left = false;
bool right = false;
uint32_t Time = 0;
int pwm = 100;
int numbers = 0;

uint8_t reset_flag;

void motorDrive(uint8_t pin, int val) {
  switch (pin) {
    case kickball_PWM:
      // OCR2A = (uint8_t)((val < 0) ? -val : val);
      analogWrite(kickball_PWM, abs(val));
      digitalWrite(kickball_IN1, (val <= 0) ? 0 : 1);
      digitalWrite(kickball_IN2, (val < 0) ? 1 : 0);
      break;

    case pushball_PWM:
      // OCR1AL = (uint8_t)((val < 0) ? -val : val);
      analogWrite(pushball_PWM, abs(val));
      digitalWrite(pushball_IN1, (val <= 0) ? 0 : 1);
      digitalWrite(pushball_IN2, (val < 0) ? 1 : 0);
      break;

    case keepball_PWM:
      // OCR1CL = (uint8_t)((val < 0) ? -val : val);
      analogWrite(keepball_PWM, abs(val));
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
  pinMode(kickball_IN1, OUTPUT);
  pinMode(kickball_IN2, OUTPUT);
  pinMode(pushball_IN1, OUTPUT);
  pinMode(pushball_IN2, OUTPUT);
  pinMode(keepball_IN1, OUTPUT);
  pinMode(keepball_IN2, OUTPUT);
  pinMode(kickball_PWM, OUTPUT);
  pinMode(pushball_PWM, OUTPUT);
  pinMode(keepball_PWM, OUTPUT);

  //INPUT
  // -- limit switch --
  pinMode(lim_kick1, INPUT_PULLUP);
  pinMode(lim_kick2, INPUT_PULLUP);
  pinMode(lim_push1, INPUT_PULLUP);
  pinMode(lim_push2, INPUT_PULLUP);
  pinMode(lim_keep1, INPUT_PULLUP);
  pinMode(lim_keep2, INPUT_PULLUP);
  pinMode(lim_ball_catch, INPUT_PULLUP);
  // -- rgb sensor

  pinMode(color_sensor, INPUT);
  motorDrive(pushball_PWM, 0);
  Serial.println("Starting...");
  delay(1000);
}

String reset(int kick_back_speed = 50, int scroll_down_speed = 60, int open_keeping_speed = 65) {
  uint8_t flag = 0;
  Serial.println("Got Reset flag");
  if(digitalRead(lim_push2) == 0)
    goto out;
  motorDrive(keepball_PWM, 0);

  // Retreat kicker
  motorDrive(kickball_PWM, kick_back_speed);
  while (((millis() - Time) < 2000) && (!digitalRead(lim_kick1) == 0)) {

  }
  motorDrive(kickball_PWM, 0);

  // Lower the lifter
  motorDrive(pushball_PWM, scroll_down_speed);
  while ((!digitalRead(lim_push2) == 0) && ((millis() - Time) < 10000)) {
   
  }
  motorDrive(pushball_PWM, 0);

  if ((millis() - Time) >= 10000)
    flag = 1;
    
out:
  // Open up the gripper
  Time = millis();
  motorDrive(keepball_PWM, -open_keeping_speed);
  while (!digitalRead(lim_keep1) == 0 && millis() - Time < 2000) {

  }
  motorDrive(keepball_PWM, 0);
  
  if (millis() - Time >= 3000 && flag == 1) 
    flag = 3;
  else if (millis() - Time >= 3000 && flag != 1) 
    flag = 2;

  switch (flag) {
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

String keep(int scroll_up_speed = 200, int keeping_speed = 180) {
  while (millis() - Time < 3000) {
    // if(!digitalRead(lim_keep2)==1){
    //   motorDrive(keepball_PWM,0);
    //   Serial.println("here");
    //   return "not catch";
    // }
    // else
    motorDrive(keepball_PWM, keeping_speed);
    // Serial.println(millis()-Time);
  }
  Time = millis();
  while (!digitalRead(lim_push1) == 0 && millis() - Time < 17000) {
    // if(!digitalRead(lim_keep2)==1){
    // motorDrive(keepball_PWM,0);
    // motorDrive(pushball_PWM,0);
    // return"ball drop";
    // }
    motorDrive(pushball_PWM, -scroll_up_speed);
    // Serial.println(millis()-Time);
  }
  motorDrive(pushball_PWM, 0);
  if (catched_ball == 0) return "not catch";
  if (millis() - Time >= 17000) return "Scroll Timeout";
  return "Successfully keeping";
}

String send(int kick_front_speed = 255) {
  uint8_t flag = 0;
  while (millis() - Time < 2000 && !digitalRead(lim_kick2) == 0) {
    motorDrive(kickball_PWM, -kick_front_speed);
    // Serial.println(millis()-Time);
  }
  motorDrive(kickball_PWM, 0);
  if (millis() - Time >= 2000) flag = 1;
  Time = millis();
  motorDrive(kickball_PWM, 0);
  motorDrive(keepball_PWM, 0);
  if (catched_ball == 1) return "send ball failed";
  switch (flag) {
    case 0:
      return "Successfully kick";
      break;
    case 1:
      return "Kick front Timeout";
      break;
    default:
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


String implementation(String *command, int numbers) {
  switch (numbers) {
    case 1:
      Time = millis();
      if (command[0] == "reset") {
        reset_flag = 1;
        return reset();
      }
      //else if(command[0] == "keep")
      //  return keep();
      else if (command[0] == "send")
        return send();
      break;
    //      case 2:
    //        Time = millis();
    //        if(command[0]=="reset")
    //          return reset(command[1].toInt());
    //        else if(command[0] == "keep")
    //          return keep(command[1].toInt());
    //        else if(command[0] == "send")
    //          return send(command[1].toInt());
    //        break;
    //      case 3:
    //        Time = millis();
    //        if(command[0]=="reset")
    //          return reset(command[1].toInt(),command[2].toInt());
    //        else if(command[0] == "keep")
    //          return keep(command[1].toInt(),command[2].toInt());
    //        else if(command[0] == "send")
    //          return send(command[1].toInt());
    //        break;
    //      case 4:
    //        Time = millis();
    //        if(command[0]=="reset")
    //          return reset(command[1].toInt(),command[2].toInt(),command[3].toInt());
    default:
      break;
  }
}

uint8_t keep_fsm;
unsigned long keep_delay = 0;
void loop() {
  if (Serial.available() > 0) {
    numbers = 0;
    String incomingString = Serial.readStringUntil('\n'); // Read until newline character
    // Serial.println(incomingString);
    if (incomingString == "holding")
      Serial.println("Fault");
    else {
      String *command = receive_command(incomingString);
      String response = implementation(command, numbers);
      Serial.println(response);
    }
  }

  switch (keep_fsm) {
    case 0:
      {
        if (((PINE & (1 << 4)) == 0) && (digitalRead(lim_push1) == 1)) { // Got a ball and not on the top.
          reset_flag = 0;
          Serial.println("Catch Keep!");
          keep_fsm = 1;
          motorDrive(keepball_PWM, 180);
          keep_delay = millis();
        }
      }
      break;

    case 1:
      {
        if ((millis() - keep_delay) > 2000) {
          keep_delay = millis();
          Serial.println("Keep Done!");
          keep_fsm = 2;
        }
      }
      break;

    case 2:
      {
        if (!digitalRead(lim_push1) == 0 && (millis() - keep_delay) < 17000) {
          motorDrive(pushball_PWM, -120);
        } else {
          motorDrive(pushball_PWM, 0);
          keep_fsm = 0;
        }
      }
    break;
  }

}
