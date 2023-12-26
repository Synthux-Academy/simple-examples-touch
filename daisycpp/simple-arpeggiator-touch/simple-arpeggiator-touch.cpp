#include "daisy_seed.h"
#include "daisysp.h"

#include "vox.h"
#include "arp.h"
#include "term.h"
#include "scale.h"

using namespace daisy;
using namespace daisysp;


using namespace synthux;

////////////////////////////////////////////////////////////
////////////////////////// CONTROLS ////////////////////////

GPIO asPlayedSwitch;

enum AdcChannel {
   speedKnobb= 0,
   lengthKnob,
   directionKnob,
   NUM_ADC_CHANNELS
};

#define S30 daisy::seed::D15 // SWITCH : ORDERED / AS PLAYED
#define S31 daisy::seed::A1  // KNOB : SPEED
#define S32 daisy::seed::A2  // KNOB : LENGTH
#define S33 daisy::seed::A3 // KNOB : DIRECTION / RANDOM

////////////////////////////////////////////////////////////
///////////////////// MODULES //////////////////////////////

static const uint8_t kNotesCount = 8;
static const uint8_t kPPQN = 24;

static Scale scale;
static Terminal term;
static Arp<kNotesCount, kPPQN> arp;
static Metro metro;
static Vox vox;

////////////////////////////////////////////////////////////
/////////////////// TEMPO 40 - 240BMP //////////////////////
//Metro F = ppqn * (minBPM + BPMRange * (0...1)) / secPerMin
static const float kMinBPM = 40;
static const float kBPMRange = 200;
static const float kSecPerMin = 60.f;
static const float kMinFreq = 24 * 40 / 60.f;
static const float kFreqRange = kPPQN * kBPMRange / kSecPerMin;

////////////////////////////////////////////////////////////
///////////////////// CALLBACKS ////////////////////////////

void OnScaleSelect(uint8_t index) { scale.SetScaleIndex(index); }
void OnTerminalNoteOn(uint8_t num, uint8_t vel) { arp.NoteOn(num, vel); }
void OnTerminalNoteOff(uint8_t num) { arp.NoteOff(num); }
void OnArpNoteOn(uint8_t num, uint8_t vel) { vox.NoteOn(scale.FreqAt(num), vel / 127.f); }
void OnArpNoteOff(uint8_t num) { vox.NoteOff(); }

DaisySeed hw;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
  for (size_t i = 0; i < size; i++) {
    if (metro.Process()) arp.Trigger();
		out[0][i] = out[1][i] = vox.Process();
  }
}

int main(void) 
{
  hw.Init();
  hw.SetAudioBlockSize(4); // number of samples handled per callback
  hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

 	float sample_rate = hw.AudioSampleRate();
	vox.Init(sample_rate);

  metro.Init(48, sample_rate); //48Hz = 24ppqn @ 120bpm

  term.Init(hw);
  term.SetOnNoteOn(OnTerminalNoteOn);
	term.SetOnNoteOff(OnTerminalNoteOff);
	term.SetOnScaleSelect(OnScaleSelect);

	arp.SetOnNoteOn(OnArpNoteOn);
	arp.SetOnNoteOff(OnArpNoteOff);

  //Configure input
  asPlayedSwitch.Init(S30, GPIO::Mode::INPUT);

  //Create an ADC configuration
	AdcChannelConfig adcConfig[NUM_ADC_CHANNELS];
	adcConfig[speedKnobb].InitSingle(S31);
	adcConfig[lengthKnob].InitSingle(S32);
	adcConfig[directionKnob].InitSingle(S33);
  
  //Initialize the adc with the config we just made
  hw.adc.Init(adcConfig, NUM_ADC_CHANNELS);
  hw.adc.Start();

  hw.SetLed(false);

	hw.StartAudio(AudioCallback);

  // Enable Logging, and set up the USB connection.
  hw.StartLog(true);

  // And Print Hello World!
  hw.PrintLine("Hello World !!!");

  ///////////////////////////////////////////////////////////////
	////////////////////////// LOOP ///////////////////////////////

  while (1) {
    float speed = hw.adc.GetFloat(speedKnobb);
		float freq = kMinFreq + kFreqRange * speed;

		metro.SetFreq(freq); 
    
		hw.SetLed(term.IsLatched());

		term.Process();
      		
		float arp_lgt = hw.adc.GetFloat(lengthKnob); //duino analogRead
		float arp_ctr = hw.adc.GetFloat(directionKnob); //duino analogRead
		ArpDirection arp_dir = arp_ctr < .5f ? ArpDirection::fwd : ArpDirection::rev;
		float arp_rnd = arp_ctr < .5f ? 2.f * arp_ctr : 2.f * (1.f - arp_ctr);
		arp.SetDirection(arp_dir);
		arp.SetRandChance(arp_rnd);
		arp.SetAsPlayed(asPlayedSwitch.Read()); //duino digitalRead
		arp.SetNoteLength(arp_lgt);

    System::Delay(4);
  }
}
