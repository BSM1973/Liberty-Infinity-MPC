#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <cstdlib>
static void writeWav(const std::string& path,const std::vector<int16_t>& samples){
    std::ofstream f(path,std::ios::binary);auto u16=[&](uint16_t x){f.put(char(x&255));f.put(char(x>>8));};
    auto u32=[&](uint32_t x){u16(x&65535);u16(x>>16);};
    f.write("RIFF",4);u32(36+uint32_t(samples.size()*2));f.write("WAVEfmt ",8);u32(16);u16(1);u16(2);u32(48000);u32(192000);u16(4);u16(16);
    f.write("data",4);u32(uint32_t(samples.size()*2));for(int16_t s:samples)u16(uint16_t(s));
    if(!f)throw std::runtime_error("WAV write failed");
}
int main(){
    Infinity synth;
    if(!synth.usingPackedBank()||synth.loadedZones()!=2000)return 1;
    synth.prepare(48000,256);
    float l[256],r[256];float* out[]={l,r};
    mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=256;ctx.sampleRate=48000;
    auto load=[&](int p){
        synth.setProgram(p);synth.reset();ctx.numMidiEvents=0;
        for(int i=0;i<1500;++i){synth.process(ctx);if(synth.loadedPatch()==p)return true;std::this_thread::sleep_for(std::chrono::milliseconds(4));}
        return false;
    };
    std::ofstream csv("factory-levels.csv");if(!csv)return 11;csv<<"preset_id,name,rms_dbfs,peak_dbfs\n";
    const char* listen=std::getenv("INFINITY_LISTEN_DIR");if(listen)std::filesystem::create_directories(listen);
    for(int p=0;p<factoryCount;++p){
        double sum=0,peak=0;uint64_t count=0;
        if(!load(p)){std::cerr<<"Load timeout "<<p<<"\n";return 2;}
        for(int note:roots)for(int vel:{24,56,88,120}){
            synth.reset();auto on=mpc::MidiEvent::noteOn(0,note,vel);ctx.midi=&on;ctx.numMidiEvents=1;
            double energy=0;
            for(int b=0;b<48;++b){synth.process(ctx);ctx.numMidiEvents=0;for(int i=0;i<256;++i){
                if(!std::isfinite(l[i])||!std::isfinite(r[i])||std::abs(l[i])>1||std::abs(r[i])>1)return 3;
                energy+=l[i]*l[i]+r[i]*r[i];sum+=double(l[i])*l[i]+double(r[i])*r[i];count+=2;peak=std::max({peak,std::abs(double(l[i])),std::abs(double(r[i]))});}}
            if(energy<.000001){std::cerr<<"Silent "<<p<<" "<<note<<" "<<vel<<"\n";return 4;}
        }
        if(factory[p].loop){
            synth.reset();auto on=mpc::MidiEvent::noteOn(0,60,110);ctx.midi=&on;ctx.numMidiEvents=1;
            double energy=0;for(int b=0;b<520;++b){synth.process(ctx);ctx.numMidiEvents=0;if(b>470)for(float v:l){if(!std::isfinite(v))return 5;energy+=v*v;}}
            if(energy<.00001)return 6;
        }
        csv<<p<<","<<factory[p].name<<","<<std::fixed<<std::setprecision(3)<<10*std::log10(std::max(1e-20,sum/count))<<","<<20*std::log10(std::max(1e-10,peak))<<"\n";
        if(listen){
            std::vector<int16_t> recording;recording.reserve(48000*2*4);
            for(int vel:{40,80,120}){
                synth.reset();auto note=mpc::MidiEvent::noteOn(0,60,vel),off=mpc::MidiEvent::noteOff(0,60);
                ctx.midi=&note;ctx.numMidiEvents=1;
                for(int b=0;b<250;++b){
                    if(b==125){ctx.midi=&off;ctx.numMidiEvents=1;}
                    synth.process(ctx);ctx.numMidiEvents=0;
                    for(int i=0;i<256;++i){recording.push_back(int16_t(std::lround(std::clamp(l[i],-1.f,1.f)*32767)));recording.push_back(int16_t(std::lround(std::clamp(r[i],-1.f,1.f)*32767)));}
                }
            }
            writeWav(std::string(listen)+"/preset-"+std::to_string(p)+".wav",recording);
        }
        std::cout<<"PASS "<<p<<" "<<factory[p].name<<" ("<<hd::layerCount(p)<<" layers)\n";
    }
    if(!load(2))return 7;
    mpc::MidiEvent chord[64];for(int i=0;i<64;++i)chord[i]=mpc::MidiEvent::noteOn(0,36+i,100);
    ctx.midi=chord;ctx.numMidiEvents=64;synth.process(ctx);ctx.numMidiEvents=0;
    if(synth.activeVoiceCount()!=synth.maxVoices())return 8;
    auto start=std::chrono::steady_clock::now();
    for(int b=0;b<375;++b){synth.process(ctx);for(float x:l)if(!std::isfinite(x))return 9;}
    auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"64-note host stress, 2 seconds audio rendered in "<<ms<<" ms (not MPC hardware)\n";
    auto off=mpc::MidiEvent::cc(0,120,0);ctx.midi=&off;ctx.numMidiEvents=1;synth.process(ctx);if(synth.activeVoiceCount()!=0)return 10;
    std::cout<<"100 presets / 2000 zones / four velocity levels validated\n";
}
