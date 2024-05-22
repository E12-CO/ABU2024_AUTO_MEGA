// New controller code for remodel ABU autonomous robot.
// Robot Club Engineering KMITL.

#include <Wire.h>

// Macros
// If you want to debug, Uncomment this line below 
#define DEBUG
// If you want to receive "I'm alive!" message every 2 seconds. Uncomment this line below
//#define ALIVE

#define BALL_OUT_DELAY 3000 // 3 seconds timeout for ball to leave the robot


// I/O stuffs
// Roller motor
#define ROLLER_PWM    8
#define ROLLER_IN1    29
#define ROLLER_IN2    27

#define ROLLER_SPEED  255

// Conveyor motor
#define CONVEYOR_PWM  11
#define CONVEYOR_IN1  31
#define CONVEYOR_IN2  33

#define CONVEYOR_IN_SPEED   120
#define CONVEYOR_OUT_SPEED  255

#define BALLOUT_DELAY       1500 // Delay to make sure that the ball is out to the silo

// Limit sws
#define LIMIT_BALL_IN   39   // Limit switch for ball in+hold detection
#define LIMIT_BALL_OUT  41   // Limit switch for ball out detection 

// Serial runner task's stuffs
uint8_t serial_fsm;
enum SERIALS_FSM {
  SERIAL_FSM_IDLE = 0,
  SERIAL_FSM_CMD_PARSE,
};
char str_input[10];
uint8_t str_idx;

// Commanding and flaging stuffs
uint8_t roller_flag;
uint8_t ball_found;
uint8_t ball_accept;
uint8_t ball_reject;
uint8_t drop_ball_flag;
uint8_t ball_done_flag;

// Ball feed runner task's stuffs
uint8_t ball_fsm;
enum BALLF_FSM {
  BALL_FSM_IDLE = 0,    // Idle state, wait for roller command and start the roller and conveyor(รอสั่งดูดบอล)
  BALL_FSM_BALL_IN,     // Stop the roller and conveyor when ball in(สั่งมอเตอร์คัมภีย์กับสายพานหยุดตอนลูกเข้ามา)
  BALL_FSM_BALL_CHECK,  // Check the Ball color (ตรวจสีบอล)
  BALL_FSM_BALL_HOLD,   // In case of currect ball, just hold it (ถ้าสีถูก ก็holdเก็บไว้ก่อน)
  BALL_FSM_BALL_OUT,    // In case of rejecting ball or drop at Silo (เคสrejectบอลที่ไม่ถูกสี กับเคสที่ใช้หย่อน silo)
  BALL_FSM_BALL_OUT_DLY,// Delay the ball out before stop the motor
};
unsigned long timeout_millis = 0;
unsigned long ballout_millis = 0;

// RGB color sensor
#define DETECTION_RANGE 100

typedef struct {
  uint16_t clear_color;
  uint16_t red_color;
  uint16_t green_color;
  uint16_t blue_color;
} color_read_t;

color_read_t color_get; // typedef struct for getting color
color_read_t color_ball_red;
color_read_t color_ball_blue;
color_read_t color_ball_purple;

// Default readout colors of each ball types
void color_Init() {
  color_ball_red.red_color = 1300;
  color_ball_red.green_color = 700;
  color_ball_red.blue_color = 600;

  color_ball_blue.red_color = 0;
  color_ball_blue.green_color = 0;
  color_ball_blue.blue_color = 0;

  color_ball_purple.red_color = 6400;
  color_ball_purple.green_color = 8300;
  color_ball_purple.blue_color = 8500;
}

uint8_t color_check_purple() {
  uint8_t truth_flag = 0;
  if ((color_get.red_color > (color_ball_purple.red_color - DETECTION_RANGE)) && (color_get.red_color < (color_ball_purple.red_color + DETECTION_RANGE)))
    truth_flag++;

  if ((color_get.green_color > (color_ball_purple.green_color - DETECTION_RANGE)) && (color_get.green_color < (color_ball_purple.green_color + DETECTION_RANGE)))
    truth_flag++;

  if ((color_get.blue_color > (color_ball_purple.blue_color - DETECTION_RANGE)) && (color_get.blue_color < (color_ball_purple.blue_color + DETECTION_RANGE)))
    truth_flag++;

#ifdef DEBUG
  Serial.print("Purple confidence :");
  Serial.println(truth_flag);
#endif

  return truth_flag;
}

uint8_t color_check_red() {
  uint8_t truth_flag = 0;
  if ((color_get.red_color > (color_ball_red.red_color - DETECTION_RANGE)) && (color_get.red_color < (color_ball_red.red_color + DETECTION_RANGE)))
    truth_flag++;

  if ((color_get.green_color > (color_ball_red.green_color - DETECTION_RANGE)) && (color_get.green_color < (color_ball_red.green_color + DETECTION_RANGE)))
    truth_flag++;

  if ((color_get.blue_color > (color_ball_red.blue_color - DETECTION_RANGE)) && (color_get.blue_color < (color_ball_red.blue_color + DETECTION_RANGE)))
    truth_flag++;

  return truth_flag;
}

void motorDrive(uint8_t channel, int val) {
  switch (channel) {
    case ROLLER_PWM:
      // OCR1AL = (uint8_t)((val < 0) ? -val : val);
      analogWrite(ROLLER_PWM, abs(val));
      digitalWrite(ROLLER_IN1, (val <= 0) ? 0 : 1);
      digitalWrite(ROLLER_IN2, (val < 0) ? 1 : 0);
      break;

    case CONVEYOR_PWM:
      // OCR1CL = (uint8_t)((val < 0) ? -val : val);
      analogWrite(CONVEYOR_PWM, abs(val));
      digitalWrite(CONVEYOR_IN1, (val <= 0) ? 0 : 1);
      digitalWrite(CONVEYOR_IN2, (val < 0) ? 1 : 0);
      break;

    default:
      return;
  }
}

void setup() {
  Serial.begin(38400);
  //OUTPUT
  pinMode(ROLLER_PWM, OUTPUT);
  pinMode(ROLLER_IN1, OUTPUT);
  pinMode(ROLLER_IN2, OUTPUT);

  pinMode(CONVEYOR_PWM, OUTPUT);
  pinMode(CONVEYOR_IN1, OUTPUT);
  pinMode(CONVEYOR_IN2, OUTPUT);

  //INPUT
  // -- limit switch --
  pinMode(LIMIT_BALL_IN,    INPUT_PULLUP);
  pinMode(LIMIT_BALL_OUT,   INPUT_PULLUP);
  // -- rgb sensor
  color_Init();
  Wire.begin();
  tcs3472_Init();


  Serial.println("Starting...");
  delay(1000);
}

#ifdef DEBUG
#ifdef ALIVE
unsigned long heartbeat_millis = 0;
#endif
#endif

void loop() {
  serial_runner();
  ballFeed_runner();
#ifdef DEBUG
#ifdef ALIVE
  if ((millis() - heartbeat_millis) > 2000) {
    heartbeat_millis = millis();
    Serial.println("I'm alive!");
  }
#endif
#endif
}

/* สรุปคำสั่ง
   S\n -> สั่งเริ่มดูดบอล Roller กับ Conveyor หมุน
   H\n -> Pi ถามแบบบอลที่ดูดได้ Mega จะส่งasciiกลับมาดังนี้
      - 'F' คือไม่มีลูกบอลกดอยู่ที่ Limit switch
      - 'A' คือมีลูกบอลที่Limit switchและเอา(แดง/น้ำเงิน)
      - 'R' คือมีลูกบอลที่Limit switch แต่ไม่เอา(ม่วง)
   A\n -> (อาจจะไม่ได้ใช้)รับบอลไว้กรณีเป็นสีที่ต้องการ
   R\n -> (อาจจะไม่ได้ใช้)คายบอลออกกรณีไม่ใช่สีที่ต้องการ
   O\n -> สั่งปล่อยบอลลงที่ Silo
   D\n -> ใช้ Debug จะ print สิ่งที่จำเป็นออกมา monitor
   E\n -> Emer ฉุกเฉิน
*/

void serial_runner() {
  switch (serial_fsm) {
    case SERIAL_FSM_IDLE:
      {
        if (Serial.available() > 0) {
          str_input[str_idx] = Serial.read();
          // Detect new line ending of CMD
          if (str_input[str_idx] == '\n') {
            serial_fsm = SERIAL_FSM_CMD_PARSE;
            break;
          }
          str_idx++;
          if (str_idx > 10) {
            str_idx = 0;
            memset(str_input, 0, 10);// Clear RX buffer
          }
        }
      }
      break;

    case SERIAL_FSM_CMD_PARSE:
      {
        // Command parser
        switch (str_input[0]) {
          case 'S':// Start ball roller (สั่งปั่นคัมภีย์ดูดบอล)
          case 's':
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }
              str_idx = 0;

              roller_flag = 1;
            }
            break;

          case 'H':// Rpi request to check ball color (Pi ขอเช็คว่าเอาหรือไม่เอาบอล)
          case 'h':
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }
              str_idx = 0;
              // Three Types of respond
              // Waiting, ball empty -> Serial.print('F');
              // Correct ball -> Serial.print('A');
              // Rejected ball -> Serial.print('R');

              if (ball_found == 0) {
                Serial.print('F');
              } else {
                if (ball_accept == 1 && ball_reject == 0)
                  Serial.print('A');
                else if (ball_accept == 0 && ball_reject == 1)
                  Serial.print('R');
                else
                  Serial.print('F');
              }

            }
            break;

          case'I':// Ball out status inquiry, Return true when ball out success.
          case'i':
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }
              str_idx = 0;

              if (ball_done_flag == 1)
                Serial.print('T');
              else
                Serial.print('F');


            }
            break;

          case 'A':// Accept ball เก็บบอลไว้ (Might be unused)
          case 'a':
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }
              str_idx = 0;

              ball_accept = 1;
              ball_reject = 0;

            }
            break;

          case 'R':// Reject ball ไม่เก็บบอลไว้ (Might be unused)
          case 'r':
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }
              str_idx = 0;

              ball_accept = 0;
              ball_reject = 1;

            }
            break;

          case 'O':// Drop ball at the silo
          case 'o':
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }
              str_idx = 0;

              drop_ball_flag = 1;
            }
            break;

          case'D':// Debug case, Return All FSM and flag data. ใช้ Debug
          case'd':
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }
              str_idx = 0;
              // Serial.print all necessary internal stuffs for debugging purpose
              // ball_fsm
              // Color sensor
              // flags
              Serial.println("DEBUG MESSAGE:");
              Serial.print("Ball FSM status : ");
              Serial.println(ball_fsm);
              tcs3472_readColor(&color_get);
              Serial.println("Color sensor readout :");
              Serial.print("Red : ");
              Serial.println(color_get.red_color);
              Serial.print("Green : ");
              Serial.println(color_get.green_color);
              Serial.print("Blue : ");
              Serial.println(color_get.blue_color);
              Serial.println("roller\tball_found\taccpet\treject\tdrop_ball\tball_done");
              Serial.print(roller_flag);
              Serial.print('\t');
              Serial.print(ball_found);
              Serial.print("\t\t");
              Serial.print(ball_accept);
              Serial.print('\t');
              Serial.print(ball_reject);
              Serial.print('\t');
              Serial.print(drop_ball_flag);
              Serial.print("\t\t");
              Serial.println(ball_done_flag);
              Serial.println("END DEBUG MESSAGE");
            }
            break;

          case 'E':// Emergency stop หยุดฉุกเฉิน
          case 'e':
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }

              // TODO : STOP all motor and reset ball fsm to IDLE
              motorDrive(ROLLER_PWM, 0);
              motorDrive(CONVEYOR_PWM, 0);
              roller_flag = 0;// Clear roller flag
              ball_accept = 0;
              ball_reject = 0;
              ball_fsm = BALL_FSM_IDLE;// Get back to idle state

              str_idx = 0;

            }
            break;

          default:
            {
              if (str_idx != 1) {
                str_idx = 0;
                break;
              }
#ifdef DEBUG
               Serial.println("Invalid command!");
#endif        
              str_idx = 0;
            }
            break;

        }// Command parser
        memset(str_input, 0, 10);// Clear command buffer
        serial_fsm = SERIAL_FSM_IDLE;
      }
      break;
  }// Serial fsm
}

void ballFeed_runner() {
  switch (ball_fsm) {
    case BALL_FSM_IDLE:// Wait for roller command 'S'
      {
        if (roller_flag == 1) {
#ifdef DEBUG
          Serial.println("Motor on");
#endif
          // Clear flag
          roller_flag = 0;
          ball_done_flag = 0;

          // Start roller and conveyor
          motorDrive(ROLLER_PWM, ROLLER_SPEED);
          motorDrive(CONVEYOR_PWM, CONVEYOR_IN_SPEED);
          ball_fsm = BALL_FSM_BALL_IN;
        }
      }
      break;

    case BALL_FSM_BALL_IN:// Wait until the ball touch limit switch and stop motor
      {
        if (digitalRead(LIMIT_BALL_IN) == 0) {
          tcs3472_readColor(&color_get);// Sample color real quick
#ifdef DEBUG
          Serial.println("Ball detected");
#endif
          // Stop roller and conveyor
          motorDrive(ROLLER_PWM, 0);
          motorDrive(CONVEYOR_PWM, 0);
          ball_found = 1;
          ball_fsm = BALL_FSM_BALL_CHECK;
        }
      }
      break;

    case BALL_FSM_BALL_CHECK:// Check ball color and decided whether to accept or reject
      {
        // Check color
#ifdef DEBUG
        Serial.println("Checking color...");
#endif
        // Process each color channel
        if (color_check_purple() >= 2) {
#ifdef DEBUG
          Serial.println("Ball REJECTED!");
#endif
          // Confidence less than 3, reject the ball
          ball_accept = 0;
          ball_reject = 1;
          motorDrive(CONVEYOR_PWM, CONVEYOR_OUT_SPEED);
          timeout_millis = millis();
          ball_fsm = BALL_FSM_BALL_OUT;
        } else {
          // Confidence == 3, accept the ball
#ifdef DEBUG
          Serial.println("Ball ACCEPTED!");
#endif
          ball_accept = 1;
          ball_reject = 0;
          ball_fsm = BALL_FSM_BALL_HOLD;
        }

      }
      break;

    case BALL_FSM_BALL_HOLD:// Hold the ball until we got drop ball flag
      {
        if (drop_ball_flag == 1) { // Got a flag to send the ball out
#ifdef DEBUG
          Serial.println("sending Ball...");
#endif
          drop_ball_flag = 0;
          motorDrive(CONVEYOR_PWM, CONVEYOR_OUT_SPEED);
          timeout_millis = millis();
          ball_fsm = BALL_FSM_BALL_OUT;
        }
      }
      break;

    case BALL_FSM_BALL_OUT:// Send the ball out to the silo, or just reject from the robot.
      {
        if (((millis() - timeout_millis) > BALL_OUT_DELAY) || (digitalRead(LIMIT_BALL_OUT) == 0)) {
          // Delayed stop.
          ballout_millis = millis();
          ball_fsm = BALL_FSM_BALL_OUT_DLY;
        }

      }
      break;

    case BALL_FSM_BALL_OUT_DLY:// After the ball arrived at the ball out point, delay to make sure that the ball is out
      {
        if ((millis() - ballout_millis) > BALLOUT_DELAY) {
#ifdef DEBUG
          Serial.println("Ball is out!");
#endif
          motorDrive(CONVEYOR_PWM, 0);
          // Clear all flags
          ball_found = 0;
          ball_accept = 0;
          ball_reject = 0;
          drop_ball_flag = 0;
          roller_flag = 0;

          // Set Ball done flag.
          ball_done_flag = 1;

          ball_fsm = BALL_FSM_IDLE;
        }
      }
      break;

  }

}
