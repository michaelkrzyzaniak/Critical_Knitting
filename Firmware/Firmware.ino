/* For BLE MIDI Setup
 * https://learn.adafruit.com/wireless-untztrument-using-ble-midi/overview
 */

#include <Adafruit_CircuitPlayground.h>
#include <bluefruit.h>
#include <MIDI.h>
#include "Statistics.h"

#define SAMPLE_INTERVAL 10 //millisecs
#define ADAPTIVE_THRESH_HISTORY 10 //secs

BLEDis bledis;
BLEMidi blemidi;

MIDI_CREATE_BLE_INSTANCE(blemidi);

AdaptiveThreshold* adaptive_range_1;
AdaptiveThreshold* adaptive_range_2;

/*------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);

  CircuitPlayground.begin();

  Bluefruit.configPrphBandwidth(BANDWIDTH_MAX);  
  Bluefruit.begin();
  Bluefruit.setName("Benediktes Knit Ball");
  Bluefruit.setTxPower(4);    // Check bluefruit.h for supported values

  // Setup the on board blue LED to be enabled on CONNECT
  Bluefruit.autoConnLed(true);
  bledis.setManufacturer("UiO");
  bledis.setModel("Knit Thing");
  bledis.begin();

  // Initialize MIDI, and listen to all MIDI channels
  // This will also call blemidi service's begin()
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleNoteOn(handleNoteOn);
  MIDI.setHandleNoteOff(handleNoteOff);

  randomSeed(analogRead(A3));

  setup_adaptive_rangefinder(&adaptive_range_1);
  setup_adaptive_rangefinder(&adaptive_range_2);

  start_bluetooth();
  Scheduler.startLoop(midiRead);
}

/*------------------------------------------------------*/
void setup_adaptive_rangefinder(AdaptiveThreshold** adaptive_range)
{
  *adaptive_range = adaptive_threshold_new(1000 / SAMPLE_INTERVAL * ADAPTIVE_THRESH_HISTORY);
  adaptive_threshold_set_threshold    (*adaptive_range, 0.2);
  adaptive_threshold_set_threshold_min(*adaptive_range, 20); /*it is a 10-bit signal*/
  adaptive_threshold_set_smoothing    (*adaptive_range, 0.8 );
}

/*------------------------------------------------------*/
void start_bluetooth(void)
{
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(blemidi);
  Bluefruit.ScanResponse.addName();
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds  
}

/*------------------------------------------------------*/
void handleNoteOn(byte channel, byte pitch, byte velocity)
{

}

/*------------------------------------------------------*/
void handleNoteOff(byte channel, byte pitch, byte velocity)
{

}

/*------------------------------------------------------*/
void read_sensor_and_send(AdaptiveThreshold* adaptive_range, int pin, byte controller_number)
{
  double exponent = 1.5;
  double sig;
  double onset = adaptive_threshold_update(adaptive_range, analogRead(pin), &sig);
  sig /= 2.0;
  if(sig > 0)
    sig = pow(sig, exponent);
  else
    sig = -pow(-sig, exponent);
  sig = scalef(sig, -1, 1, 0, 127);
  sig = clip(sig, 0, 127);
         
  MIDI.sendControlChange(controller_number, sig, 1);   
}

/*------------------------------------------------------*/
void loop()
{
  if((!Bluefruit.connected()) || (!blemidi.notifyEnabled()))
    return;

  //this is how you read the accelerometer and capacitative pins
  //float x = CircuitPlayground.motionX();
  /*1, 2, 3, 6, 9, 10, 12*/
  //uint16_t touch = CircuitPlayground.readCap(1 /*pin*/, 10 /*samples*/);

  unsigned long current_time = millis();

  read_sensor_and_send(adaptive_range_1, A1, 0);
  read_sensor_and_send(adaptive_range_2, A2, 1);
 
  //this will hang after about 70 days of continuous operation and isn't super precise
  while(millis() - current_time < SAMPLE_INTERVAL)
    delay(1);
}

/*------------------------------------------------------*/
void midiRead()
{
  if((!Bluefruit.connected()) || (!blemidi.notifyEnabled()))
    return;

  MIDI.read();
}

/*------------------------------------------------------*/
float scalef(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/*------------------------------------------------------*/
float clip(float x, float out_min, float out_max)
{
  return (x < out_min) ? out_min : (x > out_max) ? out_max : x;
}
