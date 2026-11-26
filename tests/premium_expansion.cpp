#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <unistd.h>
int main(int argc,char** argv){
    if(argc!=3)return 1;namespace fs=std::filesystem;char pattern[]="/tmp/infinity-premium-XXXXXX";char* dir=mkdtemp(pattern);if(!dir)return 2;
    struct Clean{fs::path p;~Clean(){std::error_code e;fs::remove_all(p,e);}} clean{dir};
    setenv("MPC_PLUGIN_FOLDER",dir,1);auto content=fs::path(dir)/"Content",slot=content/"Expansions/01";
    fs::create_directories(slot);fs::create_symlink(fs::absolute(argv[1]),content/"Factory.pcm");
    for(auto name:{"Library.pcm","Library.map","License.txt"})fs::create_symlink(fs::absolute(fs::path(argv[2])/name),slot/name);
    Infinity s;if(!s.usingPackedBank()||s.browserCount(1,0)!=100)return 3;
    s.prepare(48000,128);
    float l[128],r[128];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=48000;
    std::cout<<"preset,name,category,rms_dbfs,peak_dbfs,pcm_bytes\n";
    for(int p=100;p<200;++p){
        s.setProgram(p);s.reset();ctx.numMidiEvents=0;
        for(int i=0;i<2000&&s.loadedPatch()!=p;++i){s.process(ctx);std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        if(s.loadedPatch()!=p)return 4;
        double e=0,peak=0;size_t samples=0;
        for(int vel:{40,100}){
            auto on=mpc::MidiEvent::noteOn(0,60,vel),off=mpc::MidiEvent::noteOff(0,60);
            ctx.midi=&on;ctx.numMidiEvents=1;
            for(int b=0;b<192;++b){
                if(b==96){ctx.midi=&off;ctx.numMidiEvents=1;}
                s.process(ctx);ctx.numMidiEvents=0;
                for(int k=0;k<128;++k)for(float x:{l[k],r[k]}){if(!std::isfinite(x))return 5;e+=double(x)*x;peak=std::max(peak,double(std::abs(x)));++samples;}
            }s.reset();
        }
        if(e<1e-7||peak>=1||s.activePCMBytes()>hd::maxCacheBytes)return 6;
        auto* ep=s.expansionPreset(p);if(!ep)return 7;
        std::cout<<p-100<<","<<std::quoted(ep->name)<<","<<ep->category<<","<<10*std::log10(e/samples)<<","<<20*std::log10(peak)<<","<<s.activePCMBytes()<<"\n";
    }
}
