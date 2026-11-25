#pragma once
#include "Factory.h"
#include <array>
#include <atomic>
#include <vector>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <string>

namespace hd {
constexpr int rate=44100,frames=rate*2,zones=5,velocities=4,maxLayers=4;
constexpr int roots[]={36,48,60,72,84};
constexpr int taps=12,phases=128,bands=7;
constexpr size_t zoneBytes=size_t(frames)*3;
constexpr size_t patchBytes=zoneBytes*zones*velocities;
constexpr int headerSize=32;
inline int layerCount(int p){
    switch(factory[p].kind){case 2:return 4;case 4:return 3;case 6:case 10:return 3;case 0:case 3:case 7:case 11:return 2;default:return 1;}
}
inline int source(int p,int layer){
    if(layer==0)return p;
    // Reuse distinct recorded/rendered sources from the same category.
    int q=p;
    for(int n=0;n<layer;++n)do{q=(q+1)%factoryCount;}while(factory[q].kind!=factory[p].kind);
    return q;
}
struct Slot {
    std::atomic<int> state{0}; // free, writing, ready, playing
    int patch=-1,layers=0;
    std::array<std::vector<int32_t>,maxLayers> pcm;
};
class Bank {
    std::string path;
    std::array<Slot,2> slots;
    std::atomic<int> request{0};
    std::atomic<int> loaded{-1};
    std::atomic<bool> quit{false};
    std::thread worker;
    int active=-1; // accessed only on the audio thread after construction
    bool valid=false;
    std::array<std::array<std::array<float,taps>,phases>,bands> kernel{};
    static constexpr double pi=3.14159265358979323846;
    bool readSlot(Slot& s,int p){
        std::FILE* f=std::fopen(path.c_str(),"rb");if(!f)return false;
        std::vector<unsigned char> raw(patchBytes);
        bool ok=true;int count=layerCount(p);
        for(int l=0;l<count&&ok;++l){
            auto& d=s.pcm[l];d.resize(size_t(zones)*velocities*frames);
            ok=std::fseek(f,long(headerSize+size_t(source(p,l))*patchBytes),SEEK_SET)==0
                &&std::fread(raw.data(),1,raw.size(),f)==raw.size();
            if(ok)for(size_t i=0;i<d.size();++i){
                uint32_t x=uint32_t(raw[i*3])|(uint32_t(raw[i*3+1])<<8)|(uint32_t(raw[i*3+2])<<16);
                d[i]=x>=0x800000?int32_t(x)-0x1000000:int32_t(x);
            }
        }
        std::fclose(f);if(ok){s.patch=p;s.layers=count;}return ok;
    }
    void run(){
        while(!quit.load(std::memory_order_acquire)){
            const int p=request.load(std::memory_order_acquire);
            if(p!=loaded.load(std::memory_order_acquire)){
                bool found=false;
                for(auto& s:slots){
                    int expected=2;
                    // Claim a ready slot before reading or changing its non-atomic contents.
                    if(s.state.compare_exchange_strong(expected,1,std::memory_order_acq_rel)){
                        if(s.patch==p){found=true;s.state.store(2,std::memory_order_release);}
                        else s.state.store(0,std::memory_order_release);
                    }
                }
                if(!found)for(auto& s:slots){
                    int expected=0;
                    if(!s.state.compare_exchange_strong(expected,1,std::memory_order_acq_rel))continue;
                    bool ok=readSlot(s,p);
                    s.state.store(ok?2:0,std::memory_order_release);
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
public:
    explicit Bank(std::string file):path(std::move(file)){
        std::FILE* f=std::fopen(path.c_str(),"rb");if(!f)return;
        const unsigned char expected[]={ 'L','I','N','F','H','D','0','1',100,0,0,0,5,0,0,0,4,0,0,0,0x44,0xac,0,0,0x88,0x58,1,0,24,0,0,0 };
        unsigned char header[headerSize]{};
        valid=std::fseek(f,0,SEEK_END)==0&&std::ftell(f)==long(headerSize+size_t(factoryCount)*patchBytes)
            &&std::fseek(f,0,SEEK_SET)==0&&std::fread(header,1,headerSize,f)==headerSize
            &&std::equal(std::begin(header),std::end(header),std::begin(expected));
        std::fclose(f);if(!valid)return;
        for(int b=0;b<bands;++b)for(int p=0;p<phases;++p){
            const double cut=.94*std::pow(.5,b*.75),frac=double(p)/phases;double sum=0;
            for(int t=0;t<taps;++t){
                double x=t-(taps/2-1)-frac;
                double sinc=std::abs(x)<1e-9?cut:std::sin(pi*x*cut)/(pi*x);
                double u=double(t)/(taps-1),window=.42-.5*std::cos(2*pi*u)+.08*std::cos(4*pi*u);
                kernel[b][p][t]=float(sinc*window);sum+=kernel[b][p][t];
            }
            for(auto& x:kernel[b][p])x=float(x/sum);
        }
        if(!readSlot(slots[0],0)){valid=false;return;}
        slots[0].state.store(3,std::memory_order_release);active=0;loaded.store(0);
        worker=std::thread([this]{run();});
    }
    ~Bank(){quit.store(true,std::memory_order_release);if(worker.joinable())worker.join();}
    bool good()const{return valid;}
    int loadedPatch()const{return loaded.load(std::memory_order_acquire);}
    // Only called from process. Slot ownership prevents any concurrent vector mutation.
    bool select(int p){
        if(!valid)return false;
        request.store(p,std::memory_order_release);
        if(active>=0&&slots[active].patch==p)return false;
        for(int i=0;i<2;++i){
            int expected=2;
            if(!slots[i].state.compare_exchange_strong(expected,3,std::memory_order_acq_rel))continue;
            if(slots[i].patch!=p){slots[i].state.store(2,std::memory_order_release);continue;}
            int old=active;active=i;loaded.store(p,std::memory_order_release);
            if(old>=0)slots[old].state.store(0,std::memory_order_release);
            return true;
        }
        return false;
    }
    int layers()const{return active>=0?slots[active].layers:0;}
    int band(double step)const{
        if(step<=1)return 0;
        return std::clamp(int(std::ceil(std::log2(step)/.75)),0,bands-1);
    }
    float sample(int layer,int zone,int velocity,double pos,int b)const{
        const auto& pcm=slots[active].pcm[layer];
        const size_t base=(size_t(zone)*velocities+velocity)*frames;
        int i=int(pos);int phase=std::clamp(int((pos-i)*phases),0,phases-1);
        const auto& coeff=kernel[b][phase];float x=0;
        for(int t=0;t<taps;++t)x+=float(pcm[base+std::clamp(i+t-(taps/2-1),0,frames-1)])*coeff[t];
        return x*(1.f/8388608.f);
    }
};
}
