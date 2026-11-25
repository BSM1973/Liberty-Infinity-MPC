#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <iostream>
#include <chrono>
#include <thread>
int main(){
    Infinity synth;if(!synth.usingPackedBank())return 1;
    synth.prepare(48000,128);float l[128],r[128];float* out[]={l,r};
    mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=48000;
    auto render=[&](mpc::MidiEvent* e=nullptr,int n=0){ctx.midi=e;ctx.numMidiEvents=n;synth.process(ctx);for(int i=0;i<128;++i)if(!std::isfinite(l[i])||!std::isfinite(r[i]))std::exit(2);};
    auto wait=[&](int p){for(int i=0;i<2000;++i){render();if(synth.loadedPatch()==p)return true;std::this_thread::sleep_for(std::chrono::milliseconds(2));}return false;};
    synth.setProgram(2);auto on=mpc::MidiEvent::noteOn(0,60,110);render(&on,1);
    if(!wait(2)||synth.activeVoiceCount()!=1)return 3;
    synth.setProgram(3);auto a=mpc::MidiEvent::noteOn(0,61,100);render(&a,1);
    auto off=mpc::MidiEvent::noteOff(0,61);render(&off,1);
    if(!wait(3)||synth.activeVoiceCount()>1)return 4;
    synth.setProgram(8);auto b=mpc::MidiEvent::noteOn(0,62,100);render(&b,1);
    auto panic=mpc::MidiEvent::cc(0,120,0);render(&panic,1);
    if(!wait(8)||synth.activeVoiceCount()!=0)return 5;
    for(int i=0;i<300;++i){synth.setProgram((i*37+7)%100);render();}
    synth.setProgram(2);render();if(!wait(2))return 6;
    render(&on,1);if(synth.activeVoiceCount()!=1)return 7;
    std::cout<<"PASS held note replay, cancelled note, panic during load, 300 rapid selections, final preset recovery\n";
}
