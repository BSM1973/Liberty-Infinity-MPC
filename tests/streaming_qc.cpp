#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <unistd.h>
using Clock=std::chrono::steady_clock;
int main(){
    namespace fs=std::filesystem;char pattern[]="/tmp/infinity-qc-XXXXXX";char* dir=mkdtemp(pattern);if(!dir)return 1;
    struct Clean{fs::path p;~Clean(){std::error_code e;fs::remove_all(p,e);}} clean{dir};
    setenv("MPC_PLUGIN_FOLDER",dir,1);fs::create_directories(fs::path(dir)/"Content");
    auto path=mpc::contentPath("Factory.pcm");fs::create_directories(fs::path(path).parent_path());
    constexpr int n=44100*12, sources=4, N=128;constexpr double sr=48000;
    const unsigned char header[]={ 'L','I','N','F','H','D','0','1',100,0,0,0,5,0,0,0,4,0,0,0,0x44,0xac,0,0,0x88,0x58,1,0,24,0,0,0 };
    {std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(header),sizeof(header));f.seekp(hd::legacyBytes-1);f.put(0);
        for(int s=0;s<sources;++s)for(int i=0;i<n;++i)for(int ch=0;ch<2;++ch){
            double hz=(s+1)*110.+ch*55.;int32_t x=int32_t(1000000*std::sin(i*6.283185307179586*hz/44100));
            for(int k=0;k<3;++k)f.put(char((uint32_t(x)>>(k*8))&255));}if(!f)return 2;}
    std::cout<<"mode,seconds,voices,sources,active_pcm_bytes,page_bytes,mean_budget_pct,p95_budget_pct,p99_budget_pct,max_budget_pct,over_budget_blocks,missing_reads,request_drops,io_errors,lowest_block_energy\n";
    for(bool stream:{false,true}){
        {std::ofstream f(path+".map");f<<(stream?"LINFMAP3\n":"LINFMAP2\n");
            for(int s=0;s<sources;++s){f<<"L 0 "<<s<<" "<<s<<" 0.25 0 0\n";
                for(int z=0;z<5;++z)for(int v=0;v<4;++v)f<<"X "<<s<<" "<<z<<" "<<v<<" "<<hd::legacyBytes+uint64_t(s)*n*6<<" "<<n<<" 2 44100 60 44100 "<<n<<" 1764\n";}}
        Infinity synth;if(!synth.usingPackedBank())return 3;synth.prepare(sr,N);synth.setProgram(0);synth.set(P_ampon,0);synth.set(P_filteron,0);
        float l[N],r[N];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=N;ctx.sampleRate=sr;
        std::vector<mpc::MidiEvent> notes;for(int i=0;i<16;++i)notes.push_back(mpc::MidiEvent::noteOn(i,59+i%3,100));
        ctx.midi=notes.data();ctx.numMidiEvents=int(notes.size());
        constexpr int blocks=22500;std::vector<double> times;times.reserve(blocks);double sum=0,minEnergy=1e30;int over=0;
        auto deadline=Clock::now();const auto period=std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(N/sr));
        for(int b=0;b<blocks;++b){
            auto start=Clock::now();synth.process(ctx);ctx.numMidiEvents=0;
            double pct=std::chrono::duration<double>(Clock::now()-start).count()/(N/sr)*100;times.push_back(pct);sum+=pct;over+=pct>100;
            double e=0;for(int i=0;i<N;++i){if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 4;e+=double(l[i])*l[i]+double(r[i])*r[i];}
            if(b>200)minEnergy=std::min(minEnergy,e);
            deadline+=period;std::this_thread::sleep_until(deadline);
        }
        std::sort(times.begin(),times.end());
        std::cout<<(stream?"stream":"resident")<<",60,16,4,"<<synth.activePCMBytes()<<","<<synth.streamPageBytes()<<","<<sum/blocks<<","<<times[blocks*95/100]<<","<<times[blocks*99/100]<<","<<times.back()<<","<<over<<","<<synth.streamUnderruns()<<","<<synth.streamQueueDrops()<<","<<synth.streamIOErrors()<<","<<minEnergy<<std::endl;
        if(synth.activeVoiceCount()!=16||synth.streamIOErrors()||minEnergy<1e-9)return 5;
        // Missing reads and drops remain visible in report; experimental mode not certified.
    }
}
