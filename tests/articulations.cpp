#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <filesystem>
#include <iostream>
#include <unistd.h>
int main(){
    std::array<hd::LayerSettings,4> settings{};
    settings[0].role=settings[1].role=hd::RoundRobin;
    auto a=hd::articulationWeights(settings,2,70,0),b=hd::articulationWeights(settings,2,70,1);
    if(a[0]!=1||a[1]!=0||b[0]!=0||b[1]!=1)return 1;
    settings[0].role=hd::VelocityLow;settings[1].role=hd::VelocityHigh;
    auto low=hd::articulationWeights(settings,2,40,0),mid=hd::articulationWeights(settings,2,64,0),high=hd::articulationWeights(settings,2,90,0);
    if(low[0]!=1||low[1]!=0||mid[0]!=.5f||mid[1]!=.5f||high[0]!=0||high[1]!=1)return 2;
    namespace fs=std::filesystem;char pattern[]="/tmp/infinity-art-XXXXXX";char* dir=mkdtemp(pattern);if(!dir)return 3;
    struct Clean{fs::path p;~Clean(){std::error_code e;fs::remove_all(p,e);}} clean{dir};
    setenv("MPC_PLUGIN_FOLDER",dir,1);fs::create_directories(fs::path(dir)/"Content");
    auto path=mpc::contentPath("Factory.pcm");fs::create_directories(fs::path(path).parent_path());
    constexpr int n=8192;
    const unsigned char header[]={ 'L','I','N','F','H','D','0','1',100,0,0,0,5,0,0,0,4,0,0,0,0x44,0xac,0,0,0x88,0x58,1,0,24,0,0,0 };
    {std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(header),sizeof(header));f.seekp(hd::legacyBytes-1);f.put(0);
        for(int source=0;source<2;++source)for(int i=0;i<n;++i){int32_t x=int32_t(2000000*std::sin(i*6.283185307179586*(source?523.25113:261.625565)/44100));
            for(int k=0;k<3;++k)f.put(char((uint32_t(x)>>(k*8))&255));}if(!f)return 4;}
    auto map=[&](int role0,int role1){
        std::ofstream f(path+".map");f<<"LINFMAP2\nL 0 0 0 1 -1 0\nL 0 1 1 1 1 0\nA 0 0 "<<role0<<"\nA 0 1 "<<role1<<"\n";
        for(int source=0;source<2;++source)for(int z=0;z<5;++z)for(int v=0;v<4;++v)
            f<<"X "<<source<<" "<<z<<" "<<v<<" "<<hd::legacyBytes+uint64_t(source)*n*3<<" "<<n<<" 1 44100 60 0 "<<n<<" 0\n";
    };
    map(hd::RoundRobin,hd::RoundRobin);
    {Infinity s;if(!s.usingPackedBank())return 5;s.setProgram(0);s.prepare(44100,128);s.set(P_filteron,0);s.set(P_ampon,0);
        float l[128],r[128];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=44100;
        auto note=mpc::MidiEvent::noteOn(0,60,100);auto kill=mpc::MidiEvent::cc(0,120,0);
        for(int k=0;k<2;++k){ctx.midi=&note;ctx.numMidiEvents=1;double L=0,R=0;
            for(int b=0;b<20;++b){s.process(ctx);ctx.numMidiEvents=0;for(int i=0;i<128;++i){L+=l[i]*l[i];R+=r[i]*r[i];}}
            if(k==0?(L<1e-5||R>L*1e-6):(R<1e-5||L>R*1e-6))return 6;
            ctx.midi=&kill;ctx.numMidiEvents=1;s.process(ctx);
        }
    }
    map(hd::Normal,hd::ReleaseSample);
    {Infinity s;if(!s.usingPackedBank())return 7;s.setProgram(0);s.prepare(44100,128);s.set(P_filteron,0);s.set(P_ampon,0);
        float l[128],r[128];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=44100;
        auto note=mpc::MidiEvent::noteOn(0,60,100);auto off=mpc::MidiEvent::noteOff(0,60);
        ctx.midi=&note;ctx.numMidiEvents=1;double L=0,R=0;
        for(int b=0;b<80;++b){s.process(ctx);ctx.numMidiEvents=0;for(int i=0;i<128;++i){L+=l[i]*l[i];R+=r[i]*r[i];}}
        if(L<1e-5||R>L*1e-6||s.activeVoiceCount()!=1)return 8;
        auto pedalDown=mpc::MidiEvent::cc(0,64,127),pedalUp=mpc::MidiEvent::cc(0,64,0);
        ctx.midi=&pedalDown;ctx.numMidiEvents=1;s.process(ctx);
        ctx.midi=&off;ctx.numMidiEvents=1;R=0;
        for(int b=0;b<20;++b){s.process(ctx);ctx.numMidiEvents=0;for(float x:r)R+=x*x;}
        if(R>1e-8||s.activeVoiceCount()!=1)return 11;
        ctx.midi=&pedalUp;ctx.numMidiEvents=1;R=0;
        for(int b=0;b<80;++b){s.process(ctx);ctx.numMidiEvents=0;for(float x:r)R+=x*x;}
        if(R<1e-5||s.activeVoiceCount()!=0)return 9;
    }
    map(hd::RoundRobin,hd::Normal);{hd::Bank invalid(path);if(invalid.good())return 10;}
    std::cout<<"PASS round robin, velocity crossfade, release sample after one-shot, invalid roles rejected\n";
}
