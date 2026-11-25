#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <algorithm>
#include <vector>
using Clock=std::chrono::steady_clock;
int main(){
    Infinity synth;
    if(!synth.usingPackedBank())return 1;
    constexpr int N=128;constexpr double sr=48000;
    synth.prepare(sr,N);
    float l[N],r[N];float* out[]={l,r};
    mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=N;ctx.sampleRate=sr;
    auto load=[&](int p){
        synth.setProgram(p);synth.reset();ctx.numMidiEvents=0;
        for(int i=0;i<2000;++i){synth.process(ctx);if(synth.loadedPatch()==p)return true;std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        return false;
    };
    if(!load(2))return 2;
    std::cout<<"voices,fx,mean_budget_pct,p95_budget_pct,p99_budget_pct,max_budget_pct,over_budget_blocks\n";
    for(int fx:{0,1})for(int n:{8,16,32,64}){
        if(n>synth.maxVoices())continue;
        synth.reset();synth.set(P_delayon,fx);synth.set(P_revon,fx);
        std::vector<mpc::MidiEvent> ev;for(int i=0;i<n;++i)ev.push_back(mpc::MidiEvent::noteOn(0,36+i,100));
        ctx.midi=ev.data();ctx.numMidiEvents=n;synth.process(ctx);ctx.numMidiEvents=0;
        if(synth.activeVoiceCount()!=n)return 3;
        for(int b=0;b<75;++b)synth.process(ctx);
        std::vector<double> values;double sum=0;int over=0;
        for(int b=0;b<750;++b){
            auto t=Clock::now();synth.process(ctx);
            double pct=std::chrono::duration<double>(Clock::now()-t).count()/(N/sr)*100;
            values.push_back(pct);sum+=pct;over+=pct>100;
            for(int i=0;i<N;++i)if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 4;
        }
        std::sort(values.begin(),values.end());
        std::cout<<n<<","<<fx<<","<<std::fixed<<std::setprecision(3)<<sum/values.size()<<","<<values[712]<<","<<values[742]<<","<<values.back()<<","<<over<<"\n";
    }
}
