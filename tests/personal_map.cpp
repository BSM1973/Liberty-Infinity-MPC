#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <iostream>
#include <fstream>
int main(){
    const auto path=mpc::contentPath("Factory.pcm");
    const auto map=path+".map";
    {
        std::ofstream f(map); if(!f)return 1;
        f<<"LINFMAP1\nL 0 0 0 0.5 -1 0\nZ 0 2 2 0 8192 0\n";
    }
    Infinity synth;
    std::remove(map.c_str());
    if(!synth.usingPackedBank())return 2;
    synth.setProgram(0);
    synth.prepare(44100,128);
    float l[128],r[128]; float* out[]={l,r};
    mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=44100;
    auto on=mpc::MidiEvent::noteOn(0,60,80);ctx.midi=&on;ctx.numMidiEvents=1;
    double left=0,right=0;
    for(int b=0;b<80;++b){
        synth.process(ctx);ctx.numMidiEvents=0;
        for(int i=0;i<128;++i){
            if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 3;
            left+=l[i]*l[i];right+=r[i]*r[i];
        }
    }
    if(left<1e-7||right>left*1e-7||synth.activeVoiceCount()!=0)return 4;
    std::cout<<"PASS personal map: layer gain, hard-left pan, finite region end\n";
}
