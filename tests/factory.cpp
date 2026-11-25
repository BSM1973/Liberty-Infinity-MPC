#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <iostream>
int main(){
    Infinity synth;
    if(synth.loadedZones()!=500){std::cerr<<"Missing zones\n";return 1;}
    synth.prepare(48000,256);
    float l[256],r[256];float* out[]={l,r};
    mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=256;ctx.sampleRate=48000;
    for(int p=0;p<factoryCount;++p){
        synth.setProgram(p);synth.reset();
        for(int note:roots){
            synth.reset();auto on=mpc::MidiEvent::noteOn(0,note,110);ctx.midi=&on;ctx.numMidiEvents=1;
            double energy=0;
            for(int b=0;b<100;++b){synth.process(ctx);ctx.numMidiEvents=0;for(int i=0;i<256;++i){if(!std::isfinite(l[i])||!std::isfinite(r[i])||std::abs(l[i])>1||std::abs(r[i])>1)return 2;energy+=l[i]*l[i]+r[i]*r[i];}}
            if(energy<.00001){std::cerr<<"Silent "<<p<<" "<<note<<"\n";return 3;}
        }
        if(factory[p].loop){
            synth.reset();auto on=mpc::MidiEvent::noteOn(0,60,110);ctx.midi=&on;ctx.numMidiEvents=1;
            double energy=0;for(int b=0;b<500;++b){synth.process(ctx);ctx.numMidiEvents=0;if(b>450)for(float v:l){if(!std::isfinite(v))return 4;energy+=v*v;}}
            if(energy<.00001){std::cerr<<"Loop silent "<<p<<"\n";return 5;}
        }
    }
    std::cout<<"500 zones rendered and finite; sustained patches checked after 2 seconds\n";
}