#include "mpc_plugin.h"
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <atomic>
#include "Factory.h"

namespace {
constexpr int roots[]={36,48,60,72,84};
constexpr int patches=factoryCount, zones=5, rate=24000, frames=rate*2;
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
    Infinity(): mpc::Plugin(info()) {
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
        if(!loadPackedBank()) for(int p=0;p<patches;++p) for(int z=0;z<zones;++z) {
            std::vector<uint8_t> bytes;
            auto name=std::to_string(p)+"_"+std::to_string(roots[z])+".pcm";
            if(!mpc::readFile(mpc::contentPath("Samples/"+name),bytes) ||
                bytes.size()!=size_t(frames)*4) continue;
            auto& dst=bank[p][z]; dst.resize(frames*2);
            for(size_t i=0;i<dst.size();++i) {
                int x=int(bytes[i*2]) | (int(bytes[i*2+1])<<8);
                if(x>=32768) x-=65536;
                dst[i]=static_cast<int16_t>(x);
            }
        }
    }
    int activeVoiceCount() const {return activeVoices.load(std::memory_order_relaxed);}
    bool usingPackedBank() const {return packedBank;}
    int loadedZones() const {int n=0;for(const auto& p:bank)for(const auto& z:p)n+=!z.empty();return n;}
    void prepare(double sampleRate,int) override { sr=std::clamp(sampleRate,8000.0,192000.0); for(auto& b:lines)b.assign(size_t(sr*2)+4,0); reset(); }
    void reset() override { activeVoices.store(0,std::memory_order_relaxed); voices={}; pedal=false; bend=0; bendRatio=1; counter=0; gain=0; ix=0; phase=0; master={}; low={}; high={}; for(auto& b:lines)std::fill(b.begin(),b.end(),0.f); }
    void process(mpc::ProcessContext& ctx) override {
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
        const float dm=get(P_delaymix)*.01f,ds=get(P_delaytime)*.001f*sr,df=get(P_feedback)*.01f,dmod=get(P_delaymod)*.01f;
        const float rm=get(P_revmix)*.01f,rp=get(P_revpre)*.001f*sr,rd=get(P_revdec)*.001f,rmod=get(P_revmod)*.01f;
        auto coefficient=[&](float hz){return 1.f-std::exp(-6.2831853f*std::min(hz,float(sr)*.45f)/float(sr));};
        const float dlo=coefficient(get(P_delaylo)),dhi=coefficient(get(P_delayhi)),rlo=coefficient(get(P_revlo)),rhi=coefficient(get(P_revhi));
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
                constexpr int start=rate*3/4, end=frames-2, xf=768;
                const bool loop=factory[v.patch].loop;
                if(loop&&v.pos>=end)v.pos=start+xf+std::fmod(v.pos-end,double(end-start-xf));
                if(pcm.empty() || v.pos>=frames-1) { v.stage=0; continue; }
                const int k=int(v.pos);
                auto sample=[&](double pos,int ch){int a=int(pos);float b=float(pos-a);return (pcm[2*a+ch]+b*(int(pcm[2*a+2+ch])-int(pcm[2*a+ch])))/32768.f;};
                float x=sample(v.pos,0),y=sample(v.pos,1);
                if(loop&&v.pos>=end-xf){const float w=float((v.pos-(end-xf))/xf);const double other=start+v.pos-(end-xf);x=x*(1-w)+sample(other,0)*w;y=y*(1-w)+sample(other,1)*w;}
                v.pos+=v.step*bendRatio;
                if(v.stage==1) {v.env=std::min(1.f,v.env+a);if(v.env>=1) v.stage=2;}
                else if(v.stage==2) {v.env=std::max(s,v.env-d);if(v.env<=s)v.stage=3;}
                else if(v.stage==3) v.env=s;
                else {if(v.releaseStep==0) v.releaseStep=v.env/releaseFrames;v.env-=v.releaseStep;if(v.env<=0){v.stage=0;continue;}}
                const float fade=loop?1.f:std::min(1.f,float(frames-1-k)/128.f);
                if(v.stage==4&&v.fs!=4){v.fs=4;v.fr=v.fe/frel;}
                if(v.fs==1){v.fe=std::min(1.f,v.fe+fa);if(v.fe>=1)v.fs=2;}
                else if(v.fs==2){v.fe=std::max(fsus,v.fe-fd);if(v.fe<=fsus)v.fs=3;}
                else if(v.fs==3)v.fe=fsus;else v.fe=std::max(0.f,v.fe-v.fr);
                const float g=std::tan(3.14159265f*std::clamp(fc*std::exp2(envAmt*v.fe*4),20.f,float(sr)*.45f)/float(sr));
                float xl=filterOn?v.fl.run(x,g,fq,0):x,yr=filterOn?v.fright.run(y,g,fq,0):y;
                float mid=(xl+yr)*.5f;xl=mid+(xl-mid)*(ampOn?spread:1);yr=mid+(yr-mid)*(ampOn?spread:1);
                const float ae=ampOn?v.env:std::min(1.f,float(v.pos)/128.f)*(v.stage==4?v.env:1.f);
                const float accent=ampOn?1+spike*std::max(0.f,1-float(v.pos)/2400.f):1;
                l+=xl*ae*v.vel*fade*accent;r+=yr*ae*v.vel*fade*accent;
            }
            if(mf){
                float* channels[]={&l,&r};for(int c=0;c<2;++c){
                    float v=*channels[c]; if(ms==0&&(mt==0||mt==1)){auto& state=master[c][0].s2;state+=(mg/(1+mg))*(v-state);v=mt==0?state:v-state;}
                    else {v=master[c][0].run(v,mg,mq,mt);if(ms==2)v=master[c][1].run(v,mg,mq,mt);}
                    *channels[c]=v;
                }
            }
            if(!lines[0].empty()){
                phase+=.7f/float(sr);if(phase>=1)phase-=1;
                float wave=std::sin(6.2831853f*phase);
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
                    const float f=std::min(.95f,std::pow(.001f,base/std::max(.1f,rd)));
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
    bool loadPackedBank(){
        const auto path=mpc::contentPath("Factory.pcm");
        std::FILE* file=std::fopen(path.c_str(),"rb");if(!file)return false;
        // All supported targets are little-endian. Reject bad/truncated packs before allocating.
        const unsigned char expected[]={ 'L','I','N','F','B','K','0','1',100,0,0,0,5,0,0,0,0xc0,0x5d,0,0,0x80,0xbb,0,0 };
        unsigned char header[sizeof(expected)]{};
        bool ok=std::fseek(file,0,SEEK_END)==0 && std::ftell(file)==long(sizeof(expected)+size_t(patches)*zones*frames*4)
            && std::fseek(file,0,SEEK_SET)==0 && std::fread(header,1,sizeof(header),file)==sizeof(header)
            && std::equal(std::begin(header),std::end(header),std::begin(expected));
        if(ok)for(auto& p:bank)for(auto& z:p){
            if(!ok)break;z.resize(frames*2);
            if(std::fread(z.data(),sizeof(int16_t),z.size(),file)!=z.size())ok=false;
        }
        std::fclose(file);
        if(!ok)for(auto& p:bank)for(auto& z:p)z.clear();
        packedBank=ok;return ok;
    }
    std::atomic<int> activeVoices{0};
    bool packedBank=false;
    static mpc::PluginInfo info(){
        mpc::PluginInfo p; p.name="Liberty Infinity";p.vendor="Liberty Music Software";
        p.uniqueId=MPC_FOURCC('L','I','n','f');p.version=500;p.isInstrument=true;p.numInputs=0;p.numOutputs=2;p.tailSamples=192000*8;return p;
    }
    void midi(const mpc::MidiEvent& e){
        if(e.isNoteOn()){
            const int p=std::max(0,std::min(patches-1,getInt(Sound)));
            int z=0;for(int j=1;j<zones;++j)if(std::abs(e.note()-roots[j])<std::abs(e.note()-roots[z]))z=j;
            if(bank[p][z].empty())return;
            Voice* v=&voices[0];
            for(auto& x:voices){if(!x.stage){v=&x;break;}if(x.age<v->age)v=&x;}
            *v=Voice{};v->note=e.note();v->channel=e.channel();v->patch=p;v->zone=z;v->vel=e.velocity()/127.f;
            v->step=rate/sr*std::pow(2.,(e.note()-roots[z])/12.);v->stage=1;v->held=true;v->age=++counter;
        }else if(e.isNoteOff()){
            for(auto& v:voices)if(v.stage && v.note==e.note() && v.channel==e.channel()){v.held=false;if((!pedal||v.channel==15)&&v.stage!=4)v.stage=4;}
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
    float read(int n,float samples) const {
        const auto& b=lines[n]; samples=std::clamp(samples,1.f,float(b.size()-2));
        size_t a=size_t(samples);float f=samples-a;
        size_t k=(ix+b.size()-a)%b.size(),j=(k+b.size()-1)%b.size();return b[k]+f*(b[j]-b[k]);
    }
    float tone(float x,int c,float lc,float hc){low[c]+=lc*(x-low[c]);x-=low[c];high[c]+=hc*(x-high[c]);return high[c];}
    std::array<std::array<SVF,2>,2> master{};
    std::array<float,4> low{},high{};
    std::array<std::vector<float>,6> lines;size_t ix=0;float phase=0;
    std::array<std::array<std::vector<int16_t>,zones>,patches> bank;
    std::array<Voice,32> voices{};
    double sr=44100,bendRatio=1;
    int bend=0;bool pedal=false;uint64_t counter=0;float gain=0;
};
#ifndef INFINITY_STANDALONE
MPC_PLUGIN_ENTRY(Infinity)
#endif
