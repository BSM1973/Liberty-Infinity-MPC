#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <unistd.h>
static void wav(const std::string& path,const std::vector<int16_t>& samples){
    std::ofstream f(path,std::ios::binary);auto u16=[&](uint16_t x){f.put(char(x&255));f.put(char(x>>8));};
    auto u32=[&](uint32_t x){u16(x&65535);u16(x>>16);};
    f.write("RIFF",4);u32(36+uint32_t(samples.size()*2));f.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(48000);u32(192000);u16(4);u16(16);
    f.write("data",4);u32(uint32_t(samples.size()*2));for(auto s:samples)u16(uint16_t(s));
    if(!f)throw std::runtime_error("WAV write failed");
}
int main(int argc,char** argv){
    if(argc!=3)return 1;namespace fs=std::filesystem;char pattern[]="/tmp/infinity-premium-XXXXXX";char* dir=mkdtemp(pattern);if(!dir)return 2;
    struct Clean{fs::path p;~Clean(){std::error_code e;fs::remove_all(p,e);}} clean{dir};
    setenv("MPC_PLUGIN_FOLDER",dir,1);auto content=fs::path(dir)/"Content",slot=content/"Expansions/01";
    fs::create_directories(slot);fs::create_symlink(fs::absolute(argv[1]),content/"Factory.pcm");
    for(auto name:{"Library.pcm","Library.map","License.txt"})fs::create_symlink(fs::absolute(fs::path(argv[2])/name),slot/name);
    Infinity s;if(!s.usingPackedBank()||s.browserCount(1,0)!=100)return 3;
    s.prepare(48000,128);
    float l[128],r[128];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=48000;
    const char* listening=std::getenv("INFINITY_CINEMATIC_LISTEN_DIR");if(listening)fs::create_directories(listening);
    std::cout<<"preset,name,category,rms_dbfs,peak_dbfs,pcm_bytes,late_rms_dbfs,max_step\n";
    for(int p=100;p<200;++p){
        s.setProgram(p);s.reset();ctx.numMidiEvents=0;
        for(int i=0;i<2000&&s.loadedPatch()!=p;++i){s.process(ctx);std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        if(s.loadedPatch()!=p)return 4;
        double e=0,peak=0,late=0,maxStep=0;size_t samples=0,lateN=0;std::vector<int16_t> recording;
        for(int vel:{40,100}){
            float previousL=0,previousR=0;
            s.reset();
            auto on=mpc::MidiEvent::noteOn(0,60,vel),off=mpc::MidiEvent::noteOff(0,60);
            ctx.midi=&on;ctx.numMidiEvents=1;
            for(int b=0;b<1500;++b){
                if(b==1125){ctx.midi=&off;ctx.numMidiEvents=1;}
                s.process(ctx);ctx.numMidiEvents=0;
                for(int k=0;k<128;++k){
                    maxStep=std::max({maxStep,double(std::abs(l[k]-previousL)),double(std::abs(r[k]-previousR))});previousL=l[k];previousR=r[k];
                    for(float x:{l[k],r[k]}){if(!std::isfinite(x))return 5;e+=double(x)*x;peak=std::max(peak,double(std::abs(x)));++samples;
                        if(b>=900&&b<1125){late+=double(x)*x;++lateN;}
                        if(listening)recording.push_back(int16_t(std::lround(std::clamp(x,-1.f,1.f)*32767)));
                    }
                }
            }s.reset();
        }
        if(e<1e-7||peak>=1||s.activePCMBytes()>hd::maxCacheBytes)return 6;
        if(listening)wav(std::string(listening)+"/preset-"+std::to_string(p-100)+".wav",recording);
        auto* ep=s.expansionPreset(p);if(!ep)return 7;
        if(ep->regions[0][2*hd::velocities+3].xf>0&&late<1e-6){std::cerr<<"Silent loop "<<p<<"\n";return 8;}
        std::cout<<p-100<<","<<std::quoted(ep->name)<<","<<ep->category<<","<<10*std::log10(e/samples)<<","<<20*std::log10(peak)<<","<<s.activePCMBytes()<<","<<10*std::log10(std::max(1e-20,late/std::max<size_t>(1,lateN)))<<","<<maxStep<<"\n";
    }
}
