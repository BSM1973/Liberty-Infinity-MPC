#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <filesystem>
#include <iostream>
#include <iomanip>
#include <unistd.h>
using Clock=std::chrono::steady_clock;
int main(){
    namespace fs=std::filesystem;
    char pattern[]="/tmp/infinity-stream-XXXXXX";char* dir=mkdtemp(pattern);if(!dir)return 1;
    struct Clean{fs::path p;~Clean(){std::error_code e;fs::remove_all(p,e);}} clean{dir};
    setenv("MPC_PLUGIN_FOLDER",dir,1);fs::create_directories(fs::path(dir)/"Content");
    const std::string path=mpc::contentPath("Factory.pcm");fs::create_directories(fs::path(path).parent_path());
    constexpr int n=44100*30;
    const unsigned char header[]={ 'L','I','N','F','H','D','0','1',100,0,0,0,5,0,0,0,4,0,0,0,0x44,0xac,0,0,0x88,0x58,1,0,24,0,0,0 };
    {std::ofstream f(path,std::ios::binary);f.write(reinterpret_cast<const char*>(header),sizeof(header));f.seekp(hd::legacyBytes-1);f.put(0);
        for(int i=0;i<n;++i){int32_t a=int32_t(2000000*std::sin(i*6.283185307179586*261.625565/44100)),b=int32_t(1000000*std::sin(i*6.283185307179586*523.25113/44100));
            for(auto x:{a,b})for(int k=0;k<3;++k)f.put(char((uint32_t(x)>>(k*8))&255));}if(!f)return 2;}
    auto map=[&](bool stream){
        std::ofstream f(path+".map");f<<(stream?"LINFMAP3\n":"LINFMAP2\n")<<"L 0 0 0 1 0 0\n";
        for(int z=0;z<5;++z)for(int v=0;v<4;++v)f<<"X 0 "<<z<<" "<<v<<" "<<hd::legacyBytes<<" "<<n<<" 2 44100 60 0 "<<n<<" 0\n";
    };
    map(false);hd::Bank resident(path);if(!resident.good())return 3;
    map(true);
    {hd::Bank streamed(path);if(!streamed.good()||!streamed.streamPageBytes())return 4;
        if(streamed.activePCMBytes()>=resident.activePCMBytes())return 5;
        // Compare exact interpolated values at distant positions, including a page boundary.
        for(double pos:{37.,16382.25,4096.*19-1.5,44100.*25+123.75}){
            float target=resident.sample(0,2,2,pos,0,1);bool equal=false;
            for(int j=0;j<500;++j){
                streamed.beginBlock();streamed.prefetch(0,2,2,pos);
                float x=streamed.sample(0,2,2,pos,0,1);
                if(std::abs(x-target)<1e-7f){equal=true;break;}
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
            if(!equal)return 6;
        }
        if(streamed.streamIOErrors())return 7;
    }
    std::cout<<"mode,voices,pcm_active_bytes,stream_page_bytes,mean_budget_pct,p95_budget_pct,p99_budget_pct,max_budget_pct,over_budget_blocks,missing_channel_reads,request_drops,io_errors\n";
    for(bool streaming:{false,true}){
        map(streaming);Infinity synth;if(!synth.usingPackedBank())return 8;
        constexpr int N=128;constexpr double sr=48000;synth.prepare(sr,N);synth.setProgram(0);
        float l[N],r[N];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=N;ctx.sampleRate=sr;
        for(int voices:{8,16,32,64}){
            if(voices>synth.maxVoices())continue;
            synth.reset();synth.set(P_ampon,0);synth.set(P_filteron,0);
            std::vector<mpc::MidiEvent> notes;
            for(int i=0;i<voices;++i)notes.push_back(mpc::MidiEvent::noteOn(i%16,59+i%5,100));
            ctx.midi=notes.data();ctx.numMidiEvents=voices;
            uint32_t missing=synth.streamUnderruns(),drops=synth.streamQueueDrops(),errors=synth.streamIOErrors();
            std::vector<double> times;times.reserve(750);double sum=0;int over=0;double energy=0;
            auto deadline=Clock::now();const auto period=std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(N/sr));
            for(int b=0;b<750;++b){
                auto start=Clock::now();synth.process(ctx);ctx.numMidiEvents=0;
                double pct=std::chrono::duration<double>(Clock::now()-start).count()/(N/sr)*100;
                times.push_back(pct);sum+=pct;over+=pct>100;
                for(int i=0;i<N;++i){if(!std::isfinite(l[i])||!std::isfinite(r[i]))return 9;if(b>200)energy+=l[i]*l[i]+r[i]*r[i];}
                deadline+=period;std::this_thread::sleep_until(deadline);
            }
            if(synth.activeVoiceCount()!=voices||energy<1e-5||synth.streamIOErrors()!=errors)return 10;
            std::sort(times.begin(),times.end());
            std::cout<<(streaming?"stream":"resident")<<","<<voices<<","<<synth.activePCMBytes()<<","<<synth.streamPageBytes()<<","<<std::fixed<<std::setprecision(3)<<sum/times.size()<<","<<times[712]<<","<<times[742]<<","<<times.back()<<","<<over<<","<<synth.streamUnderruns()-missing<<","<<synth.streamQueueDrops()-drops<<","<<synth.streamIOErrors()-errors<<"\n";
        }
    }
}
