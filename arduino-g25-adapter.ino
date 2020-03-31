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


const uint8_t descriptor[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Joystick)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x20,                    //   REPORT_COUNT (32)
    0x05, 0x09,                    //   USAGE_PAGE (Button)
    0x19, 0x01,                    //   USAGE_MINIMUM (Button 1)
    0x29, 0x20,                    //   USAGE_MAXIMUM (Button 32)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x26, 0xff, 0x03,              //   LOGICAL_MAXIMUM (1023)
    0x75, 0x0a,                    //   REPORT_SIZE (10)
    0x95, 0x03,                    //   REPORT_COUNT (3)
    0x05, 0x01,                    //   USAGE_PAGE (Generic Desktop)
    0x09, 0x36,                    //   USAGE (Slider)
    0x09, 0x36,                    //   USAGE (Slider)
    0x09, 0x36,                    //   USAGE (Slider)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x02,                    //   REPORT_COUNT (2)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs)
    0xc0                           // END_COLLECTION
};

typedef struct {
    uint32_t buttons;
    unsigned acc:10;
    unsigned brk:10;
    unsigned clt:10;
    unsigned unused:2;
} __packed Report_t;

class HIDAdapter : public HIDReporter {
public:
  Report_t report; 
  void begin(void) {};
  void end(void) {};
  HIDAdapter(USBHID& HID, uint8_t reportID=HID_JOYSTICK_REPORT_ID) 
            : HIDReporter(HID, NULL, (uint8_t*)&report, sizeof(report), reportID) {
        report.buttons = 0;
        report.acc = 0;
        report.brk = 0;
        report.clt = 0;
    }
};

USBHID HID;
HIDAdapter adapter(HID);

void setup() {
  pinMode(SHIFTER_X_PIN, INPUT_ANALOG);
  pinMode(SHIFTER_Y_PIN, INPUT_ANALOG);

  pinMode(SHIFTER_LATCH_PIN, OUTPUT);
  pinMode(SHIFTER_CLOCK_PIN, OUTPUT);
  pinMode(SHIFTER_DATA_PIN, INPUT);

  pinMode(PEDAL_ACC_PIN, INPUT_ANALOG);
  pinMode(PEDAL_BRK_PIN, INPUT_ANALOG);
  pinMode(PEDAL_CLT_PIN, INPUT_ANALOG);

  HID.setReportDescriptor(descriptor, sizeof(descriptor));
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
  adapter.report.buttons = ((uint32_t)buttons << 16) | gear;

  uint16_t pedalAcc = analogRead(PEDAL_ACC_PIN);
  uint16_t pedalBrk = analogRead(PEDAL_BRK_PIN);
  uint16_t pedalClt = analogRead(PEDAL_CLT_PIN);

  if(pedalAcc >= PEDAL_ACC_HT)
    adapter.report.acc = -1024;
  else if(pedalAcc <= PEDAL_ACC_LT)
    adapter.report.acc = 1023;
  else
    adapter.report.acc = map(pedalAcc, PEDAL_ACC_LT, PEDAL_ACC_HT, 1023, 0);


  if(pedalBrk >= PEDAL_BRK_HT)
    adapter.report.brk = -1024;
  else if(pedalBrk <= PEDAL_BRK_LT)
    adapter.report.brk = 1023;
  else
    adapter.report.brk = map(pedalBrk, PEDAL_BRK_LT, PEDAL_BRK_HT, 1023, 0);

  if(pedalClt >= PEDAL_CLT_HT)
    adapter.report.clt = 0;
  else if(pedalClt <= PEDAL_CLT_LT)
    adapter.report.clt = 1023;
  else
    adapter.report.clt = map(pedalClt, PEDAL_CLT_LT, PEDAL_CLT_HT, 1023, 0);
  
  adapter.sendReport(); 
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