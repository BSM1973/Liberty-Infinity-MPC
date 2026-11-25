#include "mpc_plugin.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <atomic>
#include "Factory.h"
#include "HDBank.h"

namespace {
using hd::roots;
constexpr int patches=factoryCount,zones=hd::zones,rate=hd::rate,frames=hd::frames;
#if defined(__arm__) && !defined(__aarch64__)
constexpr int voiceLimit=24;
#elif defined(__aarch64__) && !defined(__APPLE__)
constexpr int voiceLimit=32;
#else
constexpr int voiceLimit=64;
#endif
struct SVF {
    float s1=0,s2=0;
    float run(float x,float g,float k,int type) {
        const float v1=(s1+g*(x-s2))/(1+g*(g+k)), v2=s2+g*v1;
        s1=2*v1-s1; s2=2*v2-s2;
        if(type==1)return x-k*v1-v2;
        if(type==2)return v1;
        if(type==3)return x-k*v1;
        return v2;
    }
};
struct Voice {
    int note=-1, patch=0, zone=0, channel=0;
    double pos=0, step=1;
    int velocityLayer=0,layers=1; bool custom=false; std::array<hd::Region,4> region{}; std::array<double,4> ratio{}; std::array<double,4> layerPos{},layerStep{};
    std::array<int,4> layerBand{};
    std::array<float,4> layerLeft{},layerRight{};
    float vel=0, env=0, l=0, r=0, releaseStep=0;
    int stage=0;
    float fe=0,fr=0; int fs=1; SVF fl,fright;
    bool held=false;
    uint64_t age=0;
};
enum Param { Sound, Attack, Decay, Sustain, Release, Cutoff, Pan, Volume, P_filteron, P_fenv, P_reso, P_fatk, P_fdec, P_fsus, P_frel, P_ampon, P_spike, P_spread, P_masteron, P_mastertype, P_masterslope, P_mastercut, P_masterres, P_delayon, P_delaytype, P_delaymix, P_delaytime, P_feedback, P_delaymod, P_delaylo, P_delayhi, P_revon, P_revtype, P_revmix, P_revpre, P_revdec, P_revmod, P_revlo, P_revhi };
}

class Infinity final: public mpc::Plugin {
public:
    Infinity(): mpc::Plugin(info()), bank(mpc::contentPath("Factory.pcm")) {
        std::vector<std::string> choices; for(const auto& p:factory) choices.emplace_back(p.name);
        addParam(mpc::choiceParam("sound","Sound",choices,0));
        addParam(mpc::logParam("attack","Attack",2,2000,10,"ms",0));
        addParam(mpc::logParam("decay","Decay",5,4000,500,"ms",0));
        addParam(mpc::floatParam("sustain","Sustain",0,100,75,"%",0));
        addParam(mpc::logParam("release","Release",5,4000,350,"ms",0));
        addParam(mpc::logParam("cutoff","Cutoff",30,18000,12000,"Hz",0));
        addParam(mpc::floatParam("pan","Pan",-100,100,0,"%",0));
        addParam(mpc::floatParam("volume","Volume",-48,0,-9,"dB",1));
        addParam(mpc::boolParam("filteron","Filter On",true));
        addParam(mpc::floatParam("fenv","Filter Env",-100,100,0,"%",0));
        addParam(mpc::floatParam("reso","Resonance",0,95,0,"%",0));
        addParam(mpc::logParam("fatk","F Attack",2,2000,10,"ms",0));
        addParam(mpc::logParam("fdec","F Decay",5,4000,500,"ms",0));
        addParam(mpc::floatParam("fsus","F Sustain",0,100,75,"%",0));
        addParam(mpc::logParam("frel","F Release",5,4000,350,"ms",0));
        addParam(mpc::boolParam("ampon","Amp On",true));
        addParam(mpc::floatParam("spike","Spike",0,100,0,"%",0));
        addParam(mpc::floatParam("spread","Spread",0,100,100,"%",0));
        addParam(mpc::boolParam("masteron","Master On",false));
        addParam(mpc::choiceParam("mastertype","Master Type",{"LP","HP","BP","Notch"},0));
        addParam(mpc::choiceParam("masterslope","Slope",{"6","12","24"},1));
        addParam(mpc::logParam("mastercut","Master Cut",30,18000,14000,"Hz",0));
        addParam(mpc::floatParam("masterres","Master Res",0,95,0,"%",0));
        addParam(mpc::boolParam("delayon","Delay On",false));
        addParam(mpc::choiceParam("delaytype","Delay Type",{"Mono","Stereo","Cross","Ping Pong"},1));
        addParam(mpc::floatParam("delaymix","Delay Mix",0,100,20,"%",0));
        addParam(mpc::logParam("delaytime","Time",10,1500,350,"ms",0));
        addParam(mpc::floatParam("feedback","Feedback",0,90,30,"%",0));
        addParam(mpc::floatParam("delaymod","Delay Mod",0,100,0,"%",0));
        addParam(mpc::logParam("delaylo","Delay Lo",20,4000,100,"Hz",0));
        addParam(mpc::logParam("delayhi","Delay Hi",500,18000,8000,"Hz",0));
        addParam(mpc::boolParam("revon","Reverb On",false));
        addParam(mpc::choiceParam("revtype","Reverb Type",{"Room","Hall","Plate","Space"},1));
        addParam(mpc::floatParam("revmix","Reverb Mix",0,100,20,"%",0));
        addParam(mpc::floatParam("revpre","Pre Delay",0,200,20,"ms",0));
        addParam(mpc::logParam("revdec","Rev Decay",100,7000,1800,"ms",0));
        addParam(mpc::floatParam("revmod","Rev Mod",0,100,0,"%",0));
        addParam(mpc::logParam("revlo","Rev Lo",20,4000,100,"Hz",0));
        addParam(mpc::logParam("revhi","Rev Hi",500,18000,8000,"Hz",0));
        for(int p=0;p<patches;++p){
            const auto& q=factory[p];const int k=q.kind,v=q.variant;
            const float a=k==2||k==10?220.f+v*20:k==6?85.f:5.f;
            const float d=k==3||k==9?200.f+v*25:600.f+v*30;
            const float su=q.loop?85.f:k==1||k==5?45.f:18.f;
            const float rel=k==2||k==10?850.f:240.f+v*15;
            const float cut=k==0?6500.f:11000.f+v*180;
            addProgram(q.name,{float(p),a,d,su,rel,cut,0,-9,1,0,0,10,500,75,350,1,0,100,0,0,1,14000,0,0,1,20,350,30,0,100,8000,0,1,20,20,1800,0,100,8000});
        }
    }
    int activeVoiceCount() const {return activeVoices.load(std::memory_order_relaxed);}
    int maxVoices() const {return voiceLimit;}
    int loadedPatch()const{return bank.loadedPatch();}
    bool usingPackedBank() const {return bank.good();}
    int loadedZones() const {return bank.good()?patches*zones*hd::velocities:0;}
    void prepare(double sampleRate,int) override { sr=std::clamp(sampleRate,8000.0,192000.0); for(auto& b:lines)b.assign(size_t(sr*2)+4,0); reset(); }
    void reset() override { pending={};requested=-1;switchGain=1.f;activeVoices.store(0,std::memory_order_relaxed); voices={}; pedal=false; bend=0; bendRatio=1; counter=0; gain=0; ix=0; phase=0; master={}; low={}; high={}; for(auto& b:lines)std::fill(b.begin(),b.end(),0.f); }
    void process(mpc::ProcessContext& ctx) override {
        const int desired=std::clamp(getInt(Sound),0,patches-1);
        if(desired!=requested){requested=desired;pending={};}
        bank.requestPatch(desired);
        if(bank.loadedPatch()!=desired && switchGain<=0.f && bank.select(desired)){voices={};activeVoices.store(0,std::memory_order_relaxed);}
        const bool waiting=bank.loadedPatch()!=desired;
        if(!waiting)for(int ch=0;ch<16;++ch)for(int note=0;note<128;++note)if(pending[ch][note]){
            startVoice(ch,note,pending[ch][note],desired);pending[ch][note]=0;
        }
        const float switchStep=1.f/std::max(1.f,float(sr)*.005f);
        const float a=1.f/std::max(1.f,float(sr)*get(Attack)*.001f);
        const float d=1.f/std::max(1.f,float(sr)*get(Decay)*.001f);
        const float s=get(Sustain)*.01f;
        const float releaseFrames=std::max(1.f,float(sr)*get(Release)*.001f);
        const bool filterOn=getBool(P_filteron),ampOn=getBool(P_ampon);
        const float fa=1.f/std::max(1.f,float(sr)*get(P_fatk)*.001f),fd=1.f/std::max(1.f,float(sr)*get(P_fdec)*.001f);
        const float fsus=get(P_fsus)*.01f, frel=std::max(1.f,float(sr)*get(P_frel)*.001f);
        const float envAmt=get(P_fenv)*.01f, fc=get(Cutoff), fq=2.f-1.85f*get(P_reso)/95.f;
        const float spike=get(P_spike)*.01f,spread=get(P_spread)*.01f;
        const bool mf=getBool(P_masteron),de=getBool(P_delayon),rv=getBool(P_revon);
        const int mt=getInt(P_mastertype),ms=getInt(P_masterslope),dt=getInt(P_delaytype),rt=getInt(P_revtype);
        const float mg=std::tan(3.14159265f*std::min(get(P_mastercut),float(sr)*.45f)/float(sr)),mq=2.f-1.85f*get(P_masterres)/95.f;
        const float filterG=std::tan(3.14159265f*std::clamp(fc,20.f,float(sr)*.45f)/float(sr));
        const float dm=get(P_delaymix)*.01f,ds=get(P_delaytime)*.001f*sr,df=get(P_feedback)*.01f,dmod=get(P_delaymod)*.01f;
        const float rm=get(P_revmix)*.01f,rp=get(P_revpre)*.001f*sr,rd=get(P_revdec)*.001f,rmod=get(P_revmod)*.01f;
        auto coefficient=[&](float hz){return 1.f-std::exp(-6.2831853f*std::min(hz,float(sr)*.45f)/float(sr));};
        const float dlo=coefficient(get(P_delaylo)),dhi=coefficient(get(P_delayhi)),rlo=coefficient(get(P_revlo)),rhi=coefficient(get(P_revhi));
        const float revFeedback=std::min(.95f,std::pow(.001f,(.032f+rt*.019f)/std::max(.1f,rd)));
        const float pan=(get(Pan)+100.f)*.005f;
        const float pl=std::cos(pan*1.5707963f),pr=std::sin(pan*1.5707963f);
        const float target=std::pow(10.f,get(Volume)*.05f)*.25f;
        int event=0;
        for(int i=0;i<ctx.numFrames;++i) {
            while(event<ctx.numMidiEvents && ctx.midi[event].sampleOffset<=i) midi(ctx.midi[event++],desired,waiting);
            float l=0,r=0;
            for(auto& v:voices) {
                if(!v.stage) continue;
                constexpr int start=rate*3/4,end=frames-16,xf=rate/25;
                const bool loop=factory[v.patch].loop;
                if(!v.custom&&!loop&&v.pos>=end){v.stage=0;continue;}
                if(!v.custom&&loop&&v.pos>=end)v.pos=start+xf+std::fmod(v.pos-end,double(end-start-xf));
                const int k=v.custom?0:int(v.pos);
                float x=0,y=0; bool any=false;
                for(int layer=0;layer<v.layers;++layer){
                    double& pos=v.layerPos[layer];
                    const double step=v.layerStep[layer];
                    const auto region=v.region[layer];
                    const int ls=v.custom?region.start:start,le=v.custom?region.end:end,lx=v.custom?region.xf:xf;
                    const bool looping=v.custom?lx>0:loop;
                    if(looping&&pos>=le)pos=ls+lx+std::fmod(pos-le,double(le-ls-lx));
                    if(pos>=le)continue;
                    any=true;
                    int band=v.layerBand[layer];
                    float value=bank.sample(layer,v.zone,v.velocityLayer,pos,band);
                    if(looping&&pos>=le-lx){float w=float((pos-(le-lx))/lx);w=w*w*(3-2*w);
                        value=value*(1-w)+bank.sample(layer,v.zone,v.velocityLayer,ls+pos-(le-lx),band)*w;}
                    if(v.custom&&!looping)value*=std::min(1.f,float(le-pos)/128.f);
                    pos+=step;
                    x+=value*v.layerLeft[layer];y+=value*v.layerRight[layer];
                }
                if(v.custom&&!any){v.stage=0;continue;}
                v.pos+=v.step*bendRatio;
                if(v.custom)v.pos=std::min(v.pos,double(frames)*2); // Bound elapsed position for sustained custom loops.
                if(v.stage==1) {v.env=std::min(1.f,v.env+a);if(v.env>=1) v.stage=2;}
                else if(v.stage==2) {v.env=std::max(s,v.env-d);if(v.env<=s)v.stage=3;}
                else if(v.stage==3) v.env=s;
                else {if(v.releaseStep==0) v.releaseStep=v.env/releaseFrames;v.env-=v.releaseStep;if(v.env<=0){v.stage=0;continue;}}
                const float fade=v.custom||loop?1.f:std::min(1.f,float(frames-1-k)/128.f);
                if(v.stage==4&&v.fs!=4){v.fs=4;v.fr=v.fe/frel;}
                if(v.fs==1){v.fe=std::min(1.f,v.fe+fa);if(v.fe>=1)v.fs=2;}
                else if(v.fs==2){v.fe=std::max(fsus,v.fe-fd);if(v.fe<=fsus)v.fs=3;}
                else if(v.fs==3)v.fe=fsus;else v.fe=std::max(0.f,v.fe-v.fr);
                const float g=envAmt==0?filterG:std::tan(3.14159265f*std::clamp(fc*std::exp2(envAmt*v.fe*4),20.f,float(sr)*.45f)/float(sr));
                float xl=filterOn?v.fl.run(x,g,fq,0):x,yr=filterOn?v.fright.run(y,g,fq,0):y;
                float mid=(xl+yr)*.5f;xl=mid+(xl-mid)*(ampOn?spread:1);yr=mid+(yr-mid)*(ampOn?spread:1);
                const float ae=ampOn?v.env:std::min(1.f,float(v.pos)/128.f)*(v.stage==4?v.env:1.f);
                const float accent=ampOn?1+spike*std::max(0.f,1-float(v.pos)/2400.f):1;
                l+=xl*ae*v.vel*fade*accent;r+=yr*ae*v.vel*fade*accent;
            }
            switchGain=waiting?std::max(0.f,switchGain-switchStep):std::min(1.f,switchGain+switchStep);
            l*=switchGain;r*=switchGain;
            if(mf){
                float* channels[]={&l,&r};for(int c=0;c<2;++c){
                    float v=*channels[c]; if(ms==0&&(mt==0||mt==1)){auto& state=master[c][0].s2;state+=(mg/(1+mg))*(v-state);v=mt==0?state:v-state;}
                    else {v=master[c][0].run(v,mg,mq,mt);if(ms==2)v=master[c][1].run(v,mg,mq,mt);}
                    *channels[c]=v;
                }
            }
            if(!lines[0].empty()){
                phase+=.7f/float(sr);if(phase>=1)phase-=1;
                float wave=(de&&dmod!=0)||(rv&&rmod!=0)?std::sin(6.2831853f*phase):0.f;
                if(de){
                    float dl=read(0,ds+wave*dmod*sr*.003f),dr=read(1,ds*(dt==1?1.12f:1)-wave*dmod*sr*.003f);
                    dl=tone(dl,0,dlo,dhi);dr=tone(dr,1,dlo,dhi);
                    float inL=l,inR=r;if(dt==0)inL=inR=(l+r)*.5f;if(dt==3)inR=0;
                    lines[0][ix]=inL+(dt>=2?dr:dl)*df;lines[1][ix]=inR+(dt>=2?dl:dr)*df;
                    l=l*(1-dm)+dl*dm;r=r*(1-dm)+dr*dm;
                } else lines[0][ix]=lines[1][ix]=0;
                lines[2][ix]=l;lines[3][ix]=r;
                if(rv){
                    const float base=.032f+rt*.019f;
                    float a=read(4,sr*base+wave*rmod*sr*.002f),b=read(5,sr*(base+.011f)-wave*rmod*sr*.002f);
                    a=tone(a,2,rlo,rhi);b=tone(b,3,rlo,rhi);
                    const float f=revFeedback;
                    lines[4][ix]=(rp<1?l:read(2,rp))+.65f*f*a+.3f*f*b;
                    lines[5][ix]=(rp<1?r:read(3,rp))+.65f*f*b-.3f*f*a;
                    l=l*(1-rm)+a*rm;r=r*(1-rm)+b*rm;
                }else lines[4][ix]=lines[5][ix]=0;
                if(++ix>=lines[0].size())ix=0;
            }
            gain+=.002f*(target-gain);
            ctx.outputs[0][i]=std::tanh(l*gain*pl);
            if(ctx.numOutputs>1)ctx.outputs[1][i]=std::tanh(r*gain*pr);
        }
        int active=0;for(const auto& v:voices)active+=v.stage!=0;
        activeVoices.store(active,std::memory_order_relaxed);
    }
private:
    std::atomic<int> activeVoices{0};
    static mpc::PluginInfo info(){
        mpc::PluginInfo p; p.name="Liberty Infinity";p.vendor="Liberty Music Software";
        p.uniqueId=MPC_FOURCC('L','I','n','f');p.version=900;p.isInstrument=true;p.numInputs=0;p.numOutputs=2;p.tailSamples=192000*8;return p;
    }
    void updateSteps(Voice& v){
        for(int l=0;l<v.layers;++l){
            const double ratio=v.custom?v.ratio[l]:1.0+(l==1?.002:l==2?-.003:l==3?.005:0);
            v.layerStep[l]=v.step*bendRatio*ratio;
            v.layerBand[l]=bank.band(v.layerStep[l]);
        }
    }
    void startVoice(int ch,int note,int vel,int p){
        if(bank.loadedPatch()!=p||vel<=0)return;
        int z=0;for(int j=1;j<zones;++j)if(std::abs(note-roots[j])<std::abs(note-roots[z]))z=j;
        Voice* v=&voices[0];
        for(auto& x:voices){
            if(!x.stage){v=&x;break;}
            if((x.stage==4&&v->stage!=4)||(x.stage==4)==(v->stage==4)&&x.age<v->age)v=&x;
        }
        *v=Voice{};v->note=note;v->channel=ch;v->patch=p;v->zone=z;v->vel=vel/127.f;
        v->velocityLayer=std::min(3,(vel-1)/32);v->layers=bank.layers();const bool mappedPatch=bank.customPatch(p);v->custom=mappedPatch;
        v->step=rate/sr*std::pow(2.,(note-roots[z])/12.);v->stage=1;v->held=true;v->age=++counter;
        for(int l=0;l<v->layers;++l){
            float pan=v->layers==1?.5f:l==0?.5f:l==1?.15f:l==2?.85f:.65f;
            float weight=(l==0?1.f:.45f)/(1.f+.45f*(v->layers-1));
            v->ratio[l]=1.0+(l==1?.002:l==2?-.003:l==3?.005:0);
            if(mappedPatch){
                auto s=bank.layerSettings(p,l);
                pan=(s.pan+1.f)*.5f;weight=s.gain;
                v->ratio[l]=std::pow(2.,s.cents/1200.);
            }
            v->region[l]=bank.region(p,l,z,v->velocityLayer);
            if(v->region[l].custom)v->custom=true;
            else v->region[l]={rate*3/4,frames-16,factory[p].loop?rate/25:0,false};
            v->layerLeft[l]=weight*(1-pan)*2;v->layerRight[l]=weight*pan*2;
        }
        updateSteps(*v);
    }
    void midi(const mpc::MidiEvent& e,int desired,bool waiting){
        if(e.isNoteOn()){
            if(waiting)pending[e.channel()][e.note()]=uint8_t(e.velocity());
            else startVoice(e.channel(),e.note(),e.velocity(),desired);
        }else if(e.isNoteOff()){
            pending[e.channel()][e.note()]=0;
            for(auto& v:voices)if(v.stage && v.note==e.note() && v.channel==e.channel()){v.held=false;if((!pedal||v.channel==15)&&v.stage!=4)v.stage=4;}
        }else if(e.isAllNotesOff()){
            pending={};for(auto& v:voices)v=Voice{};
            pedal=false;
        }else if(e.isController()&&e.controller()==64){
            pedal=e.controllerValue()>=64;
            if(!pedal)for(auto& v:voices)if(v.stage&&!v.held&&v.stage!=4)v.stage=4;
        }else if(e.isPitchBend()){
            bend=e.pitchBend();bendRatio=std::pow(2.,bend/(8192.*6.));
            for(auto& v:voices)if(v.stage)updateSteps(v);
        }
    }
    float read(int n,float samples) const {
        const auto& b=lines[n]; samples=std::clamp(samples,1.f,float(b.size()-2));
        size_t a=size_t(samples);float f=samples-a;
        size_t k=(ix+b.size()-a)%b.size(),j=(k+b.size()-1)%b.size();return b[k]+f*(b[j]-b[k]);
    }
    float tone(float x,int c,float lc,float hc){low[c]+=lc*(x-low[c]);x-=low[c];high[c]+=hc*(x-high[c]);return high[c];}
    std::array<std::array<SVF,2>,2> master{};
    std::array<float,4> low{},high{};
    std::array<std::vector<float>,6> lines;size_t ix=0;float phase=0;
    hd::Bank bank;
    std::array<Voice,voiceLimit> voices{};
    double sr=44100,bendRatio=1;
    std::array<std::array<uint8_t,128>,16> pending{};
    int requested=-1;float switchGain=1.f;
    int bend=0;bool pedal=false;uint64_t counter=0;float gain=0;
};
#ifndef INFINITY_STANDALONE
MPC_PLUGIN_ENTRY(Infinity)
#endif
