//Import headers của các thư viện 
#include <PS2X_lib.h>  
#include <Adafruit_PWMServoDriver.h> 
#include <Wire.h>

//Set up các chân tín hiệu của tay cầm PS2
#define PS2_DAT 12
#define PS2_CMD 13
#define PS2_CLK 14
#define PS2_ATT 15



// Kênh PWM (Chân của các servo và DC motor) điều khiển động cơ (Số chân của động cơ DC hay servo đều gấp đôi số kênh của động cơ đó) (Nếu vẫn sai chân thì invert lại)
//Kênh PWM cho chân thuận (Tay phải)
#define PWM_DC1A 14
#define PWM_DC1B 15
#define PWM_DC4A 12
#define PWM_DC4B 13

//Kenh PWM cho chân ngược (Tay trái)
#define PWM_DC2A 8
#define PWM_DC2B 9
#define PWM_DC3A 10
#define PWM_DC3B 11

// Động cơ DC
#define MOT_LEFT 1
#define MOT_RIGHT 2
#define SHOOTER_1 3

// set tốc độ động cơ
#define SPD_FAST 2047

#define PWM_SERVO0          8
#define PWM_SERVO1          9
#define PWM_SERVO2          10
#define PWM_SERVO3          11
#define PWM_SERVO4          12

// Độ rộng xung cho servo 360 (đã calibrate)
#define SRV_360_T1          698 // độ rộng xung mà tại đó servo quay nhanh nhất theo 1 chiều
#define SRV_360_T2          1362 // biên dưới của khoảng servo dừng quay
#define SRV_360_T3          1440 // biên trên của khoảng servo dừng quay
#define SRV_360_T4          2104 // độ rộng xung mà tại đó servo quay nhanh nhất theo chiều còn lại

// Cổng của servo
#define SRV_INTAKE          0
#define SRV_WHEEL           1

// Tốc độ động cơ
#define SPD_INTAKE          100 // servo
#define SPD_WHEEL           90


//Khai báo linh kiện
Adafruit_PWMServoDriver pwm;  //Hoặc Adafruit_PWMServoDriver pwm = new Adafruit_PWMServoDriver();
PS2X ps2;

void setup() {  //Hàm set up chạy khởi tạo một lần khi khởi động mạch
  Serial.begin(115200); //Bật serial monitor ở tốc độ 115200

  Serial.println("Welcome to Mastodon's BanhMi. Starting Serial Monitor...");

  while (ps2.config_gamepad(PS2_CLK, PS2_CMD, PS2_ATT, PS2_DAT) != 0) {
    Serial.println("Error: PS2 Controller Not Connected");
    delay(500);
  } //Check xem có chân tín hiệu nào của điều khiển nhận đc hay ko

  Serial.println("Controller Connected Successfully!");

  pwm.begin();                           //Khởi tạo xung pwm
  pwm.setOscillatorFrequency(27000000);  //Đặt tần số xung trong PCA9685 là 27000000 (27MHz) (27 triệu)
  pwm.setPWMFreq(50);                    //Đặt tần số giao động trên chân tối đa 50Hz (Để dùng cho cả Servo) (Pulse Width Modulation)
}
  bool intake_toggle = false;
  bool wheel_toggle = false;
  bool intake = false;
  bool wheel = false;

void ctrl_dc(uint8_t motor, int16_t speed) {
  switch (motor) {
    case 1: //Cho bánh bên tay trái
      pwm.setPWM(PWM_DC1A, 0, ((speed > 0) ? speed : 0) );  //Kênh PWM_DC1A, vị trí bắt đầu = 0, xung của 1 chiều là 50% đạt được sau thời gian = 2047 micro second
      pwm.setPWM(PWM_DC1B, 0, ((speed < 0) ? (-speed) : 0) );
      break;
    case 2:  //Cho bánh bên tay trái
      pwm.setPWM(PWM_DC2A, 0, ((speed > 0) ? speed : 0) );
      pwm.setPWM(PWM_DC2B, 0, ((speed < 0) ? (-speed) : 0) ) ;
      break;
    case 3:  //Cho bánh bên tay phải
      pwm.setPWM(PWM_DC3A, 0, ((speed > 0) ? speed : 0) );
      pwm.setPWM(PWM_DC3B, 0, ((speed < 0) ? (-speed) : 0) );
      break;
    case 4:
      pwm.setPWM(PWM_DC4A, 0, ((speed > 0) ? speed : 0) );
      pwm.setPWM(PWM_DC4B, 0, ((speed < 0) ? (-speed) : 0) );
      break;
  }
}

// Hàm điều khiển vị trí servo 180 (nhận số động cơ servo từ 0->4 ứng với kênh PWM 8->12, giá trị góc từ 0 đến 180)
void ctrl_servo180(uint8_t motor, float angle) {
  uint16_t us = (uint16_t) (1000 * (1.0 + angle / 180.0));
  switch(motor) {
    case 0: pwm.writeMicroseconds(PWM_SERVO0, us); break; //writeMicrosecond dùng để cho Servo biét cần phải quay bao nhiều ms để quay đc 1 góc cần và đủ
    case 1: pwm.writeMicroseconds(PWM_SERVO1, us); break;
    case 2: pwm.writeMicroseconds(PWM_SERVO2, us); break;
    case 3: pwm.writeMicroseconds(PWM_SERVO3, us); break;
    case 4: pwm.writeMicroseconds(PWM_SERVO4, us); break;
  }
}

// Hàm điều khiển tốc độ servo 360 (nhận số động cơ servo từ 0->4 ứng với kênh PWM 8->12, giá trị tốc độ từ -100 đến 100)
void ctrl_servo360(uint8_t motor, float speed) {
  uint16_t us = (SRV_360_T2 + SRV_360_T3) / 2;
  if(speed > 0) 
    us = SRV_360_T3 + (speed / 100) * (SRV_360_T4 - SRV_360_T3);
  else if(speed < 0) 
    us = SRV_360_T2 + (speed / 100) * (SRV_360_T2 - SRV_360_T1);
  switch(motor) {
    case 0: pwm.writeMicroseconds(PWM_SERVO0, us); break;
    case 1: pwm.writeMicroseconds(PWM_SERVO1, us); break;
    case 2: pwm.writeMicroseconds(PWM_SERVO2, us); break;
    case 3: pwm.writeMicroseconds(PWM_SERVO3, us); break;
    case 4: pwm.writeMicroseconds(PWM_SERVO4, us); break;
  }
}


void loop() {
    // put your main code here, to run repeatedly: 
    
    ps2.read_gamepad();  //Khởi tạo đọc từ điều kiển bằng hàm read_gamepad()

    ctrl_dc(MOT_LEFT, map(ps2.Analog(PSS_LY), 0, 255, -SPD_FAST, SPD_FAST));
    ctrl_dc(MOT_RIGHT, map(ps2.Analog(PSS_RY), 0, 255, SPD_FAST, -SPD_FAST));

    //Hàm mẫu để sử dụng Servo 
    if (ps2.Button(PSB_R1)){
      if(!intake_toggled) {
        intake_toggled = true;
        intake = !intake;
        ctrl_servo360(SRV_INTAKE, (intake) ?  SPD_INTAKE : 0);
    } else intake_toggle = false;

    if (ps2.Button(PSB_R2)){
      if(wheel_toggle) = true;
        wheel = !wheel;
        ctrl_servo360(SRV_WHEEL, (wheel) ? SPD_WHEEL : 0);
    } else wheel_toggle = false;
     
}
}