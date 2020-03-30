#include <arduino.h>
#include <USBComposite.h>

#define SHIFTER_X_PIN PA6
#define SHIFTER_Y_PIN PA7
#define SHIFTER_CLOCK_PIN PB7
#define SHIFTER_LATCH_PIN PB6
#define SHIFTER_DATA_PIN PB8

#define PEDAL_ACC_PIN PA3
#define PEDAL_ACC_LT 350
#define PEDAL_ACC_HT 3700
#define PEDAL_BRK_PIN PA2
#define PEDAL_BRK_LT 350
#define PEDAL_BRK_HT 3750
#define PEDAL_CLT_PIN PA1
#define PEDAL_CLT_LT 250
#define PEDAL_CLT_HT 3680


#define HID_SIGNED_JOYSTICK_REPORT_DESCRIPTOR(...) \
  0x05, 0x01,           /*  Usage Page (Generic Desktop) */ \
  0x09, 0x04,           /*  Usage (Joystick) */ \
  0xA1, 0x01,           /*  Collection (Application) */ \
  0x85, MACRO_GET_ARGUMENT_1_WITH_DEFAULT(HID_JOYSTICK_REPORT_ID, ## __VA_ARGS__),  /*    REPORT_ID */ \
  0x15, 0x00,           /*   Logical Minimum (0) */ \
  0x25, 0x01,           /*    Logical Maximum (1) */ \
  0x75, 0x01,           /*    Report Size (1) */ \
  0x95, 0x20,           /*    Report Count (32) */ \
  0x05, 0x09,           /*    Usage Page (Button) */ \
  0x19, 0x01,           /*    Usage Minimum (Button #1) */ \
  0x29, 0x20,           /*    Usage Maximum (Button #32) */ \
  0x81, 0x02,           /*    Input (variable,absolute) */ \
  0x15, 0x00,           /*    Logical Minimum (0) */ \
  0x25, 0x07,           /*    Logical Maximum (7) */ \
  0x35, 0x00,           /*    Physical Minimum (0) */ \
  0x46, 0x3B, 0x01,       /*    Physical Maximum (315) */ \
  0x75, 0x04,           /*    Report Size (4) */ \
  0x95, 0x01,           /*    Report Count (1) */ \
  0x65, 0x14,           /*    Unit (20) */ \
    0x05, 0x01,                     /*    Usage Page (Generic Desktop) */ \
  0x09, 0x39,           /*    Usage (Hat switch) */ \
  0x81, 0x42,           /*    Input (variable,absolute,null_state) */ \
    0x05, 0x01,                     /* Usage Page (Generic Desktop) */ \
  0x09, 0x01,           /* Usage (Pointer) */ \
    0xA1, 0x00,                     /* Collection () */ \
  0x16, 0x00, 0xFC,           /*    Logical Minimum (-1024) */ \
  0x26, 0xFF, 0x03,       /*    Logical Maximum (1023) */ \
  0x75, 0x0B,           /*    Report Size (11) */ \
  0x95, 0x04,           /*    Report Count (4) */ \
  0x09, 0x30,           /*    Usage (X) */ \
  0x09, 0x31,           /*    Usage (Y) */ \
  0x09, 0x33,           /*    Usage (Rx) */ \
  0x09, 0x34,           /*    Usage (Ry) */ \
  0x81, 0x02,           /*    Input (variable,absolute) */ \
    0xC0,                           /*  End Collection */ \
  0x15, 0x00,           /*  Logical Minimum (0) */ \
  0x26, 0xFF, 0x03,       /*  Logical Maximum (1023) */ \
  0x75, 0x0A,           /*  Report Size (10) */ \
  0x95, 0x02,           /*  Report Count (2) */ \
  0x09, 0x36,           /*  Usage (Slider) */ \
  0x09, 0x36,           /*  Usage (Slider) */ \
  0x81, 0x02,           /*  Input (variable,absolute) */ \
  0x75, 0x04,                    /*   REPORT_SIZE (4) */ \
  0x95, 0x01,                    /*   REPORT_COUNT (1) */ \
  0x81, 0x03,                    /*   OUTPUT (Cnst,Var,Abs) */ \
  MACRO_ARGUMENT_2_TO_END(__VA_ARGS__)  \
  0xC0

typedef struct {
    uint8_t reportID;
    uint32_t buttons;
    unsigned hat:4;
    int x:11;
    int y:11;
    int rx:11;
    int ry:11;
    unsigned sliderLeft:10;
    unsigned sliderRight:10;
    unsigned unused:4;
} __packed SignedJoystickReport_t;

class HIDSignedJoystick : public HIDReporter {
public:
  SignedJoystickReport_t joyReport; 
  void begin(void) {};
  void end(void) {};
  HIDSignedJoystick(USBHID& HID, uint8_t reportID=HID_JOYSTICK_REPORT_ID) 
            : HIDReporter(HID, NULL, (uint8_t*)&joyReport, sizeof(joyReport), reportID) {
        joyReport.buttons = 0;
        joyReport.hat = 15;
        joyReport.x = 0;
        joyReport.y = 0;
        joyReport.rx = 0;
        joyReport.ry = 0;
        joyReport.sliderLeft = 0;
        joyReport.sliderRight = 0;
    }
};

USBHID HID;
HIDSignedJoystick joy(HID);

uint8 signedJoyReportDescriptor[] = {
  HID_SIGNED_JOYSTICK_REPORT_DESCRIPTOR()
};

void setup() {
  pinMode(SHIFTER_X_PIN, INPUT_ANALOG);
  pinMode(SHIFTER_Y_PIN, INPUT_ANALOG);

  pinMode(SHIFTER_LATCH_PIN, OUTPUT);
  pinMode(SHIFTER_CLOCK_PIN, OUTPUT);
  pinMode(SHIFTER_DATA_PIN, INPUT);

  pinMode(PEDAL_ACC_PIN, INPUT_ANALOG);
  pinMode(PEDAL_BRK_PIN, INPUT_ANALOG);
  pinMode(PEDAL_CLT_PIN, INPUT_ANALOG);

  HID.setReportDescriptor(signedJoyReportDescriptor, sizeof(signedJoyReportDescriptor));
  HID.registerComponent();
  USBComposite.begin();  
  while (!USBComposite);
}

void loop() {
  uint16_t shifterX = analogRead(SHIFTER_X_PIN);
  uint16_t shifterY = analogRead(SHIFTER_Y_PIN);

  boolean reg[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  uint16_t buttons = 0;
  uint16_t gear = 0;

  readShiftRegister(reg);
  for (uint8_t i = 4; i < 16; i++) {
    buttons |= (reg[i] << i);
  }
  
  if(reg[3]){   // Sequential 
    if(shifterY > 2300)
      gear = (1 << 7); // Gear up
    else if(shifterY < 1800)
      gear = (1 << 8); // Gear down
    else
      gear = 0; // Gear: N
  }else{ // H
    if(shifterY > 3000){
      if(shifterX < 1600)
        gear = 1; // Gear: 1
      else if(shifterX > 2600)
        gear = (1 << 4); // Gear: 5
      else
        gear = (1 << 2); // Gear: 3
    } else if(shifterY < 1000){
      if(shifterX < 1600)
        gear = (1 << 1); // Gear: 2
      else if(shifterX > 2600)
        gear = reg[1] ? (1 << 6) : (1 << 5) ; // Gear: R(7) or 6
      else
        gear = (1 << 3); // Gear: 4
    }
    else{
      gear = 0; // Gear: N
    }
  }
  joy.joyReport.buttons = ((uint32_t)buttons << 16) | gear;

  uint16_t pedalAcc = analogRead(PEDAL_ACC_PIN);
  uint16_t pedalBrk = analogRead(PEDAL_BRK_PIN);
  uint16_t pedalClt = analogRead(PEDAL_CLT_PIN);

  if(pedalAcc >= PEDAL_ACC_HT)
    joy.joyReport.x = -1024;
  else if(pedalAcc <= PEDAL_ACC_LT)
    joy.joyReport.x = 1023;
  else
    joy.joyReport.x = map(pedalAcc, PEDAL_ACC_LT, PEDAL_ACC_HT, 1023, -1024);


  if(pedalBrk >= PEDAL_BRK_HT)
    joy.joyReport.y = -1024;
  else if(pedalBrk <= PEDAL_BRK_LT)
    joy.joyReport.y = 1023;
  else
    joy.joyReport.y = map(pedalBrk, PEDAL_BRK_LT, PEDAL_BRK_HT, 1023, -1024);

  if(pedalClt >= PEDAL_CLT_HT)
    joy.joyReport.sliderLeft = 0;
  else if(pedalClt <= PEDAL_CLT_LT)
    joy.joyReport.sliderLeft = 1023;
  else
    joy.joyReport.sliderLeft = map(pedalClt, PEDAL_CLT_LT, PEDAL_CLT_HT, 1023, 0);
  
  joy.sendReport(); 
}

void readShiftRegister(boolean reg[16]){
  latchShiftRegister();
  for (uint8_t i = 0; i < 16; i++) {
    reg[i] = readButton();
  }
}

static inline void latchShiftRegister() {
  digitalWrite(SHIFTER_LATCH_PIN, LOW);
  delayMicroseconds(10);
  digitalWrite(SHIFTER_LATCH_PIN, HIGH);
}

static inline boolean readButton() {
  digitalWrite(SHIFTER_CLOCK_PIN, LOW);
  delayMicroseconds(10);

  boolean button = digitalRead(SHIFTER_DATA_PIN);

  digitalWrite(SHIFTER_CLOCK_PIN, HIGH);
  delayMicroseconds(01);
  return button;
}