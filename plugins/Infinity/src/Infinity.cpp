#include "mpc_plugin.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

namespace {
constexpr int roots[]={36,48,60,72,84};
constexpr int patches=4, zones=5, rate=24000, frames=rate*3;
struct Voice {
    int note=-1, patch=0, zone=0;
    double pos=0, step=1;
    float vel=0, env=0, l=0, r=0, releaseStep=0;
    int stage=0;
    bool held=false;
    uint64_t age=0;
};
enum Param { Sound, Attack, Decay, Sustain, Release, Cutoff, Pan, Volume };
}

class Infinity final: public mpc::Plugin {
public:
    Infinity(): mpc::Plugin(info()) {
        addParam(mpc::choiceParam("sound","Sound",{"Deep Bass","Crystal Keys","Velvet Pad","Silver Pluck"},0));
        addParam(mpc::logParam("attack","Attack",2,2000,10,"ms",0));
        addParam(mpc::logParam("decay","Decay",5,4000,500,"ms",0));
        addParam(mpc::floatParam("sustain","Sustain",0,100,75,"%",0));
        addParam(mpc::logParam("release","Release",5,4000,350,"ms",0));
        addParam(mpc::logParam("cutoff","Cutoff",30,18000,12000,"Hz",0));
        addParam(mpc::floatParam("pan","Pan",-100,100,0,"%",0));
        addParam(mpc::floatParam("volume","Volume",-48,0,-9,"dB",1));
        addProgram("BA Deep Bass",{0,8,550,80,250,6000,0,-9});
        addProgram("KY Crystal Keys",{1,4,650,45,450,15000,0,-9});
        addProgram("PD Velvet Pad",{2,350,1000,90,800,9000,0,-9});
        addProgram("PL Silver Pluck",{3,3,180,20,180,14000,0,-9});
        for(int p=0;p<patches;++p) for(int z=0;z<zones;++z) {
            std::vector<uint8_t> bytes;
            auto name=std::to_string(p)+"_"+std::to_string(roots[z])+".pcm";
            if(!mpc::readFile(mpc::contentPath("Samples/"+name),bytes) ||
                bytes.size()!=size_t(frames)*4) continue;
            auto& dst=bank[p][z]; dst.resize(frames*2);
            for(size_t i=0;i<dst.size();++i) {
                int x=int(bytes[i*2]) | (int(bytes[i*2+1])<<8);
                if(x>=32768) x-=65536;
                dst[i]=float(x)/32768.f;
            }
        }
    }
    void prepare(double sampleRate,int) override { sr=std::max(1.0,sampleRate); reset(); }
    void reset() override { voices={}; pedal=false; bend=0; bendRatio=1; counter=0; gain=0; }
    void process(mpc::ProcessContext& ctx) override {
        const float a=1.f/std::max(1.f,float(sr)*get(Attack)*.001f);
        const float d=1.f/std::max(1.f,float(sr)*get(Decay)*.001f);
        const float s=get(Sustain)*.01f;
        const float releaseFrames=std::max(1.f,float(sr)*get(Release)*.001f);
        const float coef=1.f-std::exp(-6.2831853f*std::min(get(Cutoff),float(sr)*.45f)/float(sr));
        const float pan=(get(Pan)+100.f)*.005f;
        const float pl=std::cos(pan*1.5707963f),pr=std::sin(pan*1.5707963f);
        const float target=std::pow(10.f,get(Volume)*.05f)*.25f;
        int event=0;
        for(int i=0;i<ctx.numFrames;++i) {
            while(event<ctx.numMidiEvents && ctx.midi[event].sampleOffset<=i) midi(ctx.midi[event++]);
            float l=0,r=0;
            for(auto& v:voices) {
                if(!v.stage) continue;
                const auto& pcm=bank[v.patch][v.zone];
                if(pcm.empty() || v.pos>=frames-1) { v.stage=0; continue; }
                const int k=int(v.pos); const float f=float(v.pos-k);
                const float x=pcm[2*k]+f*(pcm[2*k+2]-pcm[2*k]);
                const float y=pcm[2*k+1]+f*(pcm[2*k+3]-pcm[2*k+1]);
                v.pos+=v.step*bendRatio;
                if(v.stage==1) {v.env=std::min(1.f,v.env+a);if(v.env>=1) v.stage=2;}
                else if(v.stage==2) {v.env=std::max(s,v.env-d);if(v.env<=s)v.stage=3;}
                else if(v.stage==3) v.env=s;
                else {if(v.releaseStep==0) v.releaseStep=v.env/releaseFrames;v.env-=v.releaseStep;if(v.env<=0){v.stage=0;continue;}}
                const float fade=std::min(1.f,float(frames-1-k)/128.f);
                v.l+=coef*(x-v.l);v.r+=coef*(y-v.r);
                l+=v.l*v.env*v.vel*fade;r+=v.r*v.env*v.vel*fade;
            }
            gain+=.002f*(target-gain);
            ctx.outputs[0][i]=std::tanh(l*gain*pl);
            if(ctx.numOutputs>1)ctx.outputs[1][i]=std::tanh(r*gain*pr);
        }
    }
private:
    static mpc::PluginInfo info(){
        mpc::PluginInfo p; p.name="Liberty Infinity";p.vendor="Liberty Music Software";
        p.uniqueId=MPC_FOURCC('L','I','n','f');p.version=100;p.isInstrument=true;p.numInputs=0;p.numOutputs=2;p.tailSamples=0;return p;
    }
    void midi(const mpc::MidiEvent& e){
        if(e.isNoteOn()){
            const int p=std::max(0,std::min(patches-1,getInt(Sound)));
            int z=0;for(int j=1;j<zones;++j)if(std::abs(e.note()-roots[j])<std::abs(e.note()-roots[z]))z=j;
            if(bank[p][z].empty())return;
            Voice* v=&voices[0];
            for(auto& x:voices){if(!x.stage){v=&x;break;}if(x.age<v->age)v=&x;}
            *v=Voice{};v->note=e.note();v->patch=p;v->zone=z;v->vel=e.velocity()/127.f;
            v->step=rate/sr*std::pow(2.,(e.note()-roots[z])/12.);v->stage=1;v->held=true;v->age=++counter;
        }else if(e.isNoteOff()){
            for(auto& v:voices)if(v.stage && v.note==e.note()){v.held=false;if(!pedal&&v.stage!=4)v.stage=4;}
        }else if(e.isAllNotesOff()){
            for(auto& v:voices)v=Voice{};
            pedal=false;
        }else if(e.isController()&&e.controller()==64){
            pedal=e.controllerValue()>=64;
            if(!pedal)for(auto& v:voices)if(v.stage&&!v.held&&v.stage!=4)v.stage=4;
        }else if(e.isPitchBend()){
            bend=e.pitchBend();bendRatio=std::pow(2.,bend/(8192.*6.));
        }
    }
    std::array<std::array<std::vector<float>,zones>,patches> bank;
    std::array<Voice,32> voices{};
    double sr=44100,bendRatio=1;
    int bend=0;bool pedal=false;uint64_t counter=0;float gain=0;
};
MPC_PLUGIN_ENTRY(Infinity)
