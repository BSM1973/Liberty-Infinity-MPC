#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <filesystem>
#include <iostream>
#include <unistd.h>
int main(){
    namespace fs=std::filesystem;
    char pattern[]="/tmp/infinity-hd-XXXXXX";char *dir=mkdtemp(pattern);if(!dir)return 1;
    struct Clean{fs::path p;~Clean(){std::error_code e;fs::remove_all(p,e);}} clean{dir};
    setenv("MPC_PLUGIN_FOLDER",dir,1);fs::create_directories(fs::path(dir)/"Content");
    const std::string path=mpc::contentPath("Factory.pcm");
    fs::create_directories(fs::path(path).parent_path());
    constexpr int n=44100*3;
    const unsigned char header[]={ 'L','I','N','F','H','D','0','1',100,0,0,0,5,0,0,0,4,0,0,0,0x44,0xac,0,0,0x88,0x58,1,0,24,0,0,0 };
    {std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(header),sizeof(header));f.seekp(hd::legacyBytes-1);f.put(0);
        // Stereo source with different signals, 3 seconds at 44.1k.
        for(int i=0;i<n;++i){int32_t a=int32_t(2000000*std::sin(i*6.283185307179586*261.625565/44100));
            int32_t b=int32_t(1000000*std::sin(i*6.283185307179586*523.25113/44100));
            for(auto x:{a,b})for(int k=0;k<3;++k)f.put(char((uint32_t(x)>>(k*8))&255));}if(!f)return 2;}
    {std::ofstream f(path+".map");f<<"LINFMAP2\nL 0 0 0 1 0 0\n";for(int z=0;z<5;++z)for(int v=0;v<4;++v)f<<"X 0 "<<z<<" "<<v<<" "<<hd::legacyBytes<<" "<<n<<" 2 44100 60 0 "<<n<<" 0\n";}
    {hd::Bank bank(path);if(!bank.good())return 3;auto r=bank.sampleRecord(0,2,2);if(r.channels!=2||r.length!=n||r.root!=60)return 4;
        if(std::abs(bank.sample(0,2,2,37,0)-bank.sample(0,2,2,37,0,1))<.01f)return 5;}
    {Infinity synth;if(!synth.usingPackedBank())return 6;synth.setProgram(0);synth.prepare(44100,128);
        synth.set(P_ampon,0);synth.set(P_filteron,0);
        float l[128],r[128];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=44100;
        auto on=mpc::MidiEvent::noteOn(0,60,100);ctx.midi=&on;ctx.numMidiEvents=1;
        double energy=0,diff=0;
        for(int b=0;b<1100;++b){synth.process(ctx);ctx.numMidiEvents=0;
            for(int i=0;i<128;++i){if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 7;if(b>720&&b<1000){energy+=l[i]*l[i]+r[i]*r[i];diff+=(l[i]-r[i])*(l[i]-r[i]);}}}
        if(energy<1e-5||diff<1e-5||synth.activeVoiceCount()!=0)return 8;
    }
    {std::ofstream f(path+".map");f<<"LINFMAP2\nX 0 0 0 "<<hd::legacyBytes<<" "<<n*10<<" 2 44100 60 0 "<<n<<" 0\n";}
    {hd::Bank bad(path);if(bad.good())return 9;}
    std::cout<<"PASS stereo HD: independent channels, 3-second playback, end-of-file, invalid offsets rejected\n";
}
