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
#include <fstream>
#include <sstream>
#include <memory>
#include <limits>

namespace hd {
constexpr int rate=44100,frames=rate*2,zones=5,velocities=4,maxLayers=4;
constexpr int roots[]={36,48,60,72,84};
constexpr int taps=12,phases=128,bands=7;
constexpr size_t zoneBytes=size_t(frames)*3;
constexpr size_t patchBytes=zoneBytes*zones*velocities;
constexpr int headerSize=32;
constexpr uint64_t legacyBytes=headerSize+uint64_t(factoryCount)*patchBytes;
constexpr uint64_t maxBankBytes=2000000000ull; // portable signed long offsets on ARM32
constexpr size_t maxCacheBytes=48u*1024u*1024u; // decoded PCM per slot, excludes other DSP state
constexpr int maxSampleFrames=192000*120;
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
struct LayerSettings { int source=-1; float gain=0,pan=0,cents=0; };
struct Region { int start=0,end=frames-16,xf=0; bool custom=false; };
struct SampleRecord {
    uint64_t offset=0; int length=frames,channels=1,sampleRate=rate,root=60;
};
struct DecodedZone { std::shared_ptr<std::vector<int32_t>> pcm; SampleRecord info; };
struct Slot {
    std::atomic<int> state{0}; // free, writing, ready, playing
    int patch=-1,layers=0;
    std::array<std::array<DecodedZone,zones*velocities>,maxLayers> data;
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
    std::array<std::array<LayerSettings,maxLayers>,factoryCount> settings{};
    std::array<int,factoryCount> counts{};
    std::array<std::array<Region,zones*velocities>,factoryCount> regions{};
    uint64_t fileBytes=0;
    std::array<std::array<SampleRecord,zones*velocities>,factoryCount> records{};
    SampleRecord record(int source,int z,int v)const{
        auto rec=records[source][z*velocities+v];
        if(!rec.offset){rec.offset=headerSize+(uint64_t(source)*zones*velocities+z*velocities+v)*zoneBytes;rec.root=roots[z];}
        return rec;
    }
    bool checkBudget(int p)const{
        std::array<SampleRecord,maxLayers*zones*velocities> used{};int n=0;uint64_t bytes=0;
        int count=counts[p]?counts[p]:layerCount(p);
        for(int l=0;l<count;++l)for(int z=0;z<zones;++z)for(int v=0;v<velocities;++v){
            const auto rec=record(sourceFor(p,l),z,v);bool duplicate=false;
            for(int i=0;i<n;++i)if(used[i].offset==rec.offset){
                if(used[i].length!=rec.length||used[i].channels!=rec.channels)return false;
                duplicate=true;break;
            }
            if(!duplicate){used[n++]=rec;bytes+=uint64_t(rec.length)*rec.channels*sizeof(int32_t);if(bytes>maxCacheBytes)return false;}
        }
        return true;
    }
    bool readMap(){
        std::ifstream in(path+".map");
        if(!in){ std::FILE* f=std::fopen((path+".map").c_str(),"rb"); if(f){std::fclose(f);return false;} return fileBytes==legacyBytes; }
        std::string line; if(!std::getline(in,line)||(line!="LINFMAP1"&&line!="LINFMAP2"))return false;
        bool extended=line=="LINFMAP2";if(!extended&&fileBytes!=legacyBytes)return false;
        while(std::getline(in,line)){
            if(line.empty())continue;
            std::istringstream s(line); char type; int p,l,q; if(!(s>>type>>p>>l>>q)||p<0||p>=factoryCount)return false;
            if(type=='L'){
                float g,pan,cents; if(!(s>>g>>pan>>cents)||l!=counts[p]||l>=maxLayers||q<0||q>=factoryCount||
                    !std::isfinite(g)||g<0||g>2||!std::isfinite(pan)||pan<-1||pan>1||!std::isfinite(cents)||std::abs(cents)>1200)return false;
                settings[p][l]={q,g,pan,cents};++counts[p];
            }else if(type=='Z'){
                int start,end,xf; if(!(s>>start>>end>>xf)||l<0||l>=zones||q<0||q>=velocities||
                    start<0||end<32||end>frames||start>=end||xf<0||xf>(end-start)/3)return false;
                auto& r=regions[p][l*velocities+q];if(r.custom)return false;
                r={start,end,xf,true};
            }else if(type=='X'&&extended){
                uint64_t offset;int len,ch,sr,root,start,end,xf;
                if(!(s>>offset>>len>>ch>>sr>>root>>start>>end>>xf)||l<0||l>=zones||q<0||q>=velocities||
                    len<32||len>maxSampleFrames||(ch!=1&&ch!=2)||sr<8000||sr>192000||root<0||root>127||
                    start<0||end<32||end>len||start>=end||xf<0||xf>(end-start)/3||
                    offset<legacyBytes||offset>fileBytes||uint64_t(len)*ch*3>fileBytes-offset)return false;
                auto& r=regions[p][l*velocities+q];if(r.custom)return false;
                r={start,end,xf,true};records[p][l*velocities+q]={offset,len,ch,sr,root};
            }else return false;
            std::string extra;if(s>>extra)return false;
        }
        if(!in.eof())return false;
        for(int p=0;p<factoryCount;++p)if(!checkBudget(p))return false;
        return true;
    }
    std::array<std::array<std::array<float,taps>,phases>,bands> kernel{};
    static constexpr double pi=3.14159265358979323846;
    bool readSlot(Slot& s,int p){
        // The worker owns this slot exclusively. Release old data before allocating.
        s.data={};
        std::FILE* f=std::fopen(path.c_str(),"rb");if(!f)return false;
        constexpr size_t chunkSamples=16384;
        bool ok=true;int count=counts[p]?counts[p]:layerCount(p);
        try{
            std::vector<unsigned char> raw(chunkSamples*3);
            for(int l=0;l<count&&ok;++l)for(int j=0;j<zones*velocities&&ok;++j){
                auto& dst=s.data[l][j];dst.info=record(sourceFor(p,l),j/velocities,j%velocities);
                const auto rec=dst.info;
                for(int a=0;a<=l&&!dst.pcm;++a)for(int b=0;b<(a==l?j:zones*velocities);++b){
                    const auto& prev=s.data[a][b];
                    if(prev.info.offset==rec.offset&&prev.info.length==rec.length&&prev.info.channels==rec.channels){dst.pcm=prev.pcm;break;}
                }
                if(dst.pcm)continue;
                dst.pcm=std::make_shared<std::vector<int32_t>>(size_t(rec.length)*rec.channels);
                auto& data=*dst.pcm;ok=std::fseek(f,long(rec.offset),SEEK_SET)==0;
                for(size_t offset=0;ok&&offset<data.size();offset+=chunkSamples){
                    const size_t n=std::min(chunkSamples,data.size()-offset);
                    ok=std::fread(raw.data(),3,n,f)==n;
                    if(ok)for(size_t i=0;i<n;++i){
                        uint32_t x=uint32_t(raw[i*3])|(uint32_t(raw[i*3+1])<<8)|(uint32_t(raw[i*3+2])<<16);
                        data[offset+i]=x>=0x800000?int32_t(x)-0x1000000:int32_t(x);
                    }
                }
            }
        }catch(...){ok=false;}
        std::fclose(f);if(ok){s.patch=p;s.layers=count;}else s.data={};return ok;
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
        bool sizeOK=std::fseek(f,0,SEEK_END)==0;
        long size=sizeOK?std::ftell(f):-1;
        fileBytes=size>=0?uint64_t(size):0;
        valid=fileBytes>=legacyBytes&&fileBytes<=maxBankBytes
            &&std::fseek(f,0,SEEK_SET)==0&&std::fread(header,1,headerSize,f)==headerSize
            &&std::equal(std::begin(header),std::end(header),std::begin(expected));
        std::fclose(f);if(!valid||!readMap()){valid=false;return;}
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
    void requestPatch(int p){if(valid)request.store(p,std::memory_order_release);}
    bool select(int p){
        if(!valid)return false;
        requestPatch(p);
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
    int sourceFor(int p,int l)const{return counts[p]?settings[p][l].source:source(p,l);}
    bool customPatch(int p)const{return counts[p]!=0;}
    LayerSettings layerSettings(int p,int l)const{return settings[p][l];}
    Region region(int p,int l,int z,int v)const{return regions[sourceFor(p,l)][z*velocities+v];}
    int layers()const{return active>=0?slots[active].layers:0;}
    int band(double step)const{
        if(step<=1)return 0;
        return std::clamp(int(std::ceil(std::log2(step)/.75)),0,bands-1);
    }
    SampleRecord sampleRecord(int layer,int z,int v)const{return slots[active].data[layer][z*velocities+v].info;}
    float sample(int layer,int zone,int velocity,double pos,int b,int channel=0)const{
        const auto& z=slots[active].data[layer][zone*velocities+velocity];
        const auto& pcm=*z.pcm;const int len=z.info.length,ch=z.info.channels;
        channel=ch==1?0:channel;
        int i=int(pos);int phase=std::clamp(int((pos-i)*phases),0,phases-1);
        const auto& coeff=kernel[b][phase];float x=0;
        const int first=i-(taps/2-1);
        if(first>=0&&first+taps<=len){
            const int32_t* data=pcm.data()+size_t(first)*ch+channel;
            for(int t=0;t<taps;++t)x+=float(data[t*ch])*coeff[t];
        }else for(int t=0;t<taps;++t)x+=float(pcm[size_t(std::clamp(first+t,0,len-1))*ch+channel])*coeff[t];
        return x*(1.f/8388608.f);
    }
};
}
