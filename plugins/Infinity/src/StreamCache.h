#pragma once
// Included inside namespace hd after SampleRecord. SPSC requests: one render
// producer, one disk worker consumer. Fixed four-way page cache, no RT allocation.
class StreamCache {
public:
    static constexpr int pageFrames=4096, attackFrames=16384, guard=taps;
#if defined(__arm__) && !defined(__aarch64__)
    static constexpr unsigned pageCount=128;
#else
    static constexpr unsigned pageCount=256;
#endif
    static constexpr unsigned ways=8, sets=pageCount/ways;
    static constexpr unsigned pageBits=13, pageMask=(1u<<pageBits)-1;
    static constexpr unsigned queueSize=2048, recentSize=4096;
private:
    struct Page {
        std::atomic<unsigned> state{0}; // 0 empty, 1 worker writes, 2 ready, 3 render reads
        std::atomic<uint32_t> key{0},last{0};
        std::array<int32_t,(pageFrames+2*guard)*2> pcm;
    };
    struct Recent{uint32_t key=0,epoch=0;};
    std::unique_ptr<Page[]> pages;
    std::array<uint32_t,queueSize> queue{};
    std::array<std::array<Recent,4>,recentSize/4> recent{}; // producer only
    std::atomic<uint32_t> head{0},tail{0},epoch{1},misses{0},drops{0},errors{0};
    static unsigned hash(uint32_t x){x^=x>>16;x*=0x7feb352du;x^=x>>15;x*=0x846ca68bu;return x^(x>>16);}
    static uint32_t makeKey(int id,int page){return (uint32_t(id+1)<<pageBits)|uint32_t(page);}
    static unsigned setFor(uint32_t key){return (hash(key)&(sets-1))*ways;}
    void enqueue(uint32_t key)noexcept{
        const uint32_t e=epoch.load(std::memory_order_relaxed);
        auto& group=recent[hash(key)&(recentSize/4-1)];Recent* entry=nullptr;
        for(auto& r:group){if(r.key==key&&r.epoch==e)return;if(r.epoch!=e&&!entry)entry=&r;}
        // Bound requests to four distinct keys per bucket per audio block,
        // rather than repeatedly evicting the deduplication record.
        if(!entry){drops.fetch_add(1,std::memory_order_relaxed);return;}
        *entry={key,e};
        for(unsigned w=0;w<ways;++w){auto& p=pages[setFor(key)+w];
            if(p.state.load(std::memory_order_acquire)>=2&&p.key.load(std::memory_order_relaxed)==key){p.last.store(e,std::memory_order_relaxed);return;}}
        const uint32_t h=head.load(std::memory_order_relaxed),t=tail.load(std::memory_order_acquire);
        if(h-t>=queueSize){drops.fetch_add(1,std::memory_order_relaxed);return;}
        queue[h&(queueSize-1)]=key;head.store(h+1,std::memory_order_release);
    }
    bool fill(Page& p,std::FILE* f,const SampleRecord& rec,int page){
        const int first=page*pageFrames-guard;
        const int begin=std::clamp(first,0,rec.length-1),end=std::min(rec.length,first+pageFrames+2*guard);
        if(end<=begin)return false;
        std::array<unsigned char,(pageFrames+2*guard)*2*3> raw;
        const size_t count=size_t(end-begin)*rec.channels;
        if(std::fseek(f,long(rec.offset+uint64_t(begin)*rec.channels*3),SEEK_SET)!=0||
           std::fread(raw.data(),3,count,f)!=count)return false;
        for(int i=0;i<pageFrames+2*guard;++i){
            const size_t src=size_t(std::clamp(first+i,begin,end-1)-begin)*rec.channels;
            for(int ch=0;ch<rec.channels;++ch){
                const size_t k=(src+ch)*3;uint32_t x=uint32_t(raw[k])|(uint32_t(raw[k+1])<<8)|(uint32_t(raw[k+2])<<16);
                p.pcm[size_t(i)*rec.channels+ch]=x>=0x800000?int32_t(x)-0x1000000:int32_t(x);
            }
        }
        return true;
    }
public:
    bool enable(){try{pages.reset(new Page[pageCount]);return true;}catch(...){return false;}}
    bool enabled()const noexcept{return bool(pages);}
    size_t bytes()const noexcept{return pages?sizeof(Page)*pageCount:0;}
    uint32_t underruns()const noexcept{return misses.load(std::memory_order_relaxed);}
    uint32_t queueDrops()const noexcept{return drops.load(std::memory_order_relaxed);}
    uint32_t ioErrors()const noexcept{return errors.load(std::memory_order_relaxed);}
    void beginBlock()noexcept{if(pages)epoch.fetch_add(1,std::memory_order_relaxed);}
    void prefetch(const SampleRecord& rec,double pos)noexcept{
        if(!pages||rec.streamId<0||pos<0||pos>=rec.length)return;
        int page=int(pos)/pageFrames;
        for(int n=0;n<=3;++n){
            int q=page+n;
            if(q*pageFrames>=rec.length)break;
            if((q+1)*pageFrames+guard<=attackFrames)continue;
            enqueue(makeKey(rec.streamId,q));
        }
    }
    float sample(const SampleRecord& rec,int i,int channel,const std::array<float,taps>& coeff)noexcept{
        const int page=i/pageFrames;const uint32_t key=makeKey(rec.streamId,page);
        const unsigned set=setFor(key);
        for(unsigned w=0;w<ways;++w){
            auto& p=pages[set+w];
            if(p.key.load(std::memory_order_relaxed)!=key)continue;
            unsigned expected=2;
            if(!p.state.compare_exchange_strong(expected,3,std::memory_order_acq_rel))continue;
            if(p.key.load(std::memory_order_relaxed)!=key){p.state.store(2,std::memory_order_release);continue;}
            const int first=i-page*pageFrames+guard-(taps/2-1);
            const int32_t* data=p.pcm.data()+size_t(first)*rec.channels+channel;
            float value=0;for(int t=0;t<taps;++t)value+=float(data[t*rec.channels])*coeff[t];
            p.last.store(epoch.load(std::memory_order_relaxed),std::memory_order_relaxed);
            p.state.store(2,std::memory_order_release);return value*(1.f/8388608.f);
        }
        misses.fetch_add(1,std::memory_order_relaxed);enqueue(key);return 0.f;
    }
    // Sole disk worker. Prioritise FIFO requests from audible voices before patch loads.
    template<class Records> void service(std::FILE* file,const Records& records,unsigned limit=128){
        if(!pages)return;
        while(limit--){
            const uint32_t t=tail.load(std::memory_order_relaxed);
            if(t==head.load(std::memory_order_acquire))break;
            const uint32_t key=queue[t&(queueSize-1)];tail.store(t+1,std::memory_order_release);
            const unsigned id=(key>>pageBits)-1,page=key&pageMask;
            if(id>=records.size()||!file){errors.fetch_add(1,std::memory_order_relaxed);continue;}
            const unsigned set=setFor(key);bool present=false;
            for(unsigned w=0;w<ways;++w){auto& p=pages[set+w];unsigned state=p.state.load(std::memory_order_acquire);
                if(state>=2&&p.key.load(std::memory_order_relaxed)==key){present=true;break;}}
            if(present)continue;
            int victim=-1;uint32_t age=0,now=epoch.load(std::memory_order_relaxed);
            for(unsigned w=0;w<ways;++w){
                auto& p=pages[set+w];unsigned state=p.state.load(std::memory_order_acquire);
                if(state==0){victim=int(set+w);break;}
                uint32_t a=now-p.last.load(std::memory_order_relaxed);
                if(state==2&&a>2&&(victim<0||a>age)){victim=int(set+w);age=a;}
            }
            if(victim<0)continue;
            auto& p=pages[victim];unsigned expected=p.state.load(std::memory_order_relaxed);
            if(expected!=0&&expected!=2)continue;
            if(!p.state.compare_exchange_strong(expected,1,std::memory_order_acq_rel))continue;
            const bool ok=fill(p,file,records[id],int(page));
            if(ok){p.key.store(key,std::memory_order_relaxed);p.last.store(now,std::memory_order_relaxed);}
            else errors.fetch_add(1,std::memory_order_relaxed);
            p.state.store(ok?2:0,std::memory_order_release);
        }
    }
};
