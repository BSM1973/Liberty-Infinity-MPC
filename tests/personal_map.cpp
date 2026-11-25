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
    // A source-only map must also work in factory patches without L records.
    // Patch 0 uses source 0 and the next bass source (4).
    {
        std::ofstream f(map); if(!f)return 5;
        f<<"LINFMAP1\nZ 0 2 2 0 4096 0\nZ 4 2 2 0 4096 0\n";
    }
    Infinity sourceOnly;
    std::remove(map.c_str());
    if(!sourceOnly.usingPackedBank())return 6;
    sourceOnly.setProgram(0);sourceOnly.prepare(44100,128);
    ctx.midi=&on;ctx.numMidiEvents=1;
    double energy=0;
    for(int b=0;b<80;++b){
        sourceOnly.process(ctx);ctx.numMidiEvents=0;
        for(int i=0;i<128;++i){
            if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 7;
            energy+=l[i]*l[i]+r[i]*r[i];
        }
    }
    if(energy<1e-7||sourceOnly.activeVoiceCount()!=0)return 8;
    // A custom loop must sustain beyond the imported region.
    {
        std::ofstream f(map); if(!f)return 9;
        f<<"LINFMAP1\nZ 0 2 2 1024 4096 256\nZ 4 2 2 1024 4096 256\n";
    }
    Infinity looped;
    std::remove(map.c_str());
    if(!looped.usingPackedBank())return 10;
    looped.setProgram(0);looped.prepare(44100,128);
    ctx.midi=&on;ctx.numMidiEvents=1;energy=0;
    for(int b=0;b<100;++b){
        looped.process(ctx);ctx.numMidiEvents=0;
        for(int i=0;i<128;++i){
            if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 11;
            if(b>80)energy+=l[i]*l[i]+r[i]*r[i];
        }
    }
    if(energy<1e-7||looped.activeVoiceCount()!=1)return 12;
    std::cout<<"PASS personal map: layer gain, hard-left pan, finite region end, source-only one-shot and loop\n";
}
