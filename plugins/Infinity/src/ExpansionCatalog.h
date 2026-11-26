#pragma once
// Immutable catalog, constructed off the audio thread. Four fixed slots,
// 100 patches each. Each collection has independent compact PCM.
constexpr int expansionSlots=4,expansionCapacity=100,totalPatches=factoryCount+expansionSlots*expansionCapacity;
struct ExpansionPreset {
    std::string name;int layers=0,category=0;
    float trim=1.f;bool trimSet=false;
    std::array<LayerSettings,maxLayers> settings{};
    std::array<std::array<SampleRecord,zones*velocities>,maxLayers> records{};
    std::array<std::array<Region,zones*velocities>,maxLayers> regions{};
};
struct Expansion {
    std::string id,name,path;
    std::array<std::unique_ptr<ExpansionPreset>,expansionCapacity> presets;
};
class ExpansionCatalog {
    std::array<std::unique_ptr<Expansion>,expansionSlots> slots;
    static bool uuid(const std::string& id){
        if(id.size()!=36)return false;
        for(size_t i=0;i<id.size();++i){char c=id[i];if(i==8||i==13||i==18||i==23){if(c!='-')return false;}
            else if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;}
        return true;
    }
    static bool nameOK(const std::string& s){if(s.empty()||s.size()>80)return false;for(unsigned char c:s)if(c<32||c==127)return false;return true;}
    static std::unique_ptr<Expansion> load(const std::string& dir){
        auto ex=std::make_unique<Expansion>();ex->path=dir+"/Library.pcm";
        std::FILE* f=std::fopen(ex->path.c_str(),"rb");if(!f)return {};
        char magic[8];bool ok=std::fread(magic,1,8,f)==8&&std::memcmp(magic,"LINFEX01",8)==0&&std::fseek(f,0,SEEK_END)==0;
        long size=ok?std::ftell(f):-1;std::fclose(f);
        if(size<8||uint64_t(size)>maxBankBytes)return {};
        std::ifstream in(dir+"/Library.map");std::string line;
        if(!std::getline(in,line)||line!="LINFEXP1")return {};
        size_t lines=0;
        while(std::getline(in,line)){
            if(++lines>10000||line.size()>512)return {};
            std::istringstream s(line);char op;if(!(s>>op))continue;int p,l,z,v;
            if(op=='I'){if(!ex->id.empty()||!(s>>ex->id)||!uuid(ex->id))return {};}
            else if(op=='N'){if(!ex->name.empty()||!(s>>std::quoted(ex->name))||!nameOK(ex->name))return {};}
            else {
                if(!(s>>p)||p<0||p>=expansionCapacity)return {};
                if(op=='P'){
                    if(ex->presets[p])return {};auto q=std::make_unique<ExpansionPreset>();
                    if(!(s>>std::quoted(q->name))||!nameOK(q->name))return {};
                    ex->presets[p]=std::move(q);
                }else if(op=='G'){
                    auto* q=ex->presets[p].get();float db;
                    if(!q||q->trimSet||!(s>>db)||!std::isfinite(db)||db< -12.f||db>12.f)return {};
                    q->trim=std::pow(10.f,db/20.f);q->trimSet=true;
                }else if(op=='C'){
                    auto* q=ex->presets[p].get();if(!q||!(s>>q->category)||q->category<0||q->category>12)return {};
                }else{
                    auto* q=ex->presets[p].get();if(!q||!(s>>l)||l<0||l>=maxLayers)return {};
                    if(op=='L'){
                        float g,pan,cents;int role;
                        if(l!=q->layers||!(s>>g>>pan>>cents>>role)||!std::isfinite(g)||g<0||g>2||
                           !std::isfinite(pan)||std::abs(pan)>1||!std::isfinite(cents)||std::abs(cents)>1200||role<0||role>4)return {};
                        q->settings[l]={l,g,pan,cents,role};++q->layers;
                    }else if(op=='X'){
                        uint64_t off;int n,ch,sr,root,start,end,xf;
                        if(l>=q->layers||!(s>>z>>v>>off>>n>>ch>>sr>>root>>start>>end>>xf)||z<0||z>=zones||v<0||v>=velocities||
                           n<32||n>maxSampleFrames||(ch!=1&&ch!=2)||sr<8000||sr>192000||root<0||root>127||
                           start<0||end<32||start>=end||end>n||xf<0||xf>(end-start)/3||
                           off<8||off>uint64_t(size)||uint64_t(n)*ch*3>uint64_t(size)-off)return {};
                        auto& rec=q->records[l][z*velocities+v];if(rec.offset)return {};
                        rec={off,n,ch,sr,root,-1};q->regions[l][z*velocities+v]={start,end,xf,true};
                    }else return {};
                }
            }
            std::string extra;if(s>>extra)return {};
        }
        if(!in.eof()||ex->id.empty()||ex->name.empty())return {};
        if(!verifyExpansionReceipt(dir,ex->id,productionPublicKey,allowUnsignedDemo))return {};
        bool any=false;
        for(auto& q:ex->presets)if(q){
            any=true;if(!q->layers)return {};
            std::array<int,5> roles{};std::array<SampleRecord,maxLayers*zones*velocities> used{};int count=0;uint64_t bytes=0;
            for(int l=0;l<q->layers;++l){
                ++roles[q->settings[l].role];
                for(auto rec:q->records[l]){
                    if(!rec.offset)return {};bool dup=false;
                    for(int j=0;j<count;++j)if(used[j].offset==rec.offset){if(used[j].length!=rec.length||used[j].channels!=rec.channels)return {};dup=true;break;}
                    if(!dup){used[count++]=rec;bytes+=uint64_t(rec.length)*rec.channels*4;if(bytes>maxCacheBytes)return {};}
                }
            }
            if(roles[RoundRobin]==1||roles[VelocityLow]!=roles[VelocityHigh]||roles[VelocityLow]>1||roles[ReleaseSample]>1||roles[ReleaseSample]==q->layers)return {};
        }
        if(!any)return {};return ex;
    }
public:
    explicit ExpansionCatalog(const std::string& root){
        for(int i=0;i<expansionSlots;++i){
            auto ex=load(root+"/Expansions/0"+std::to_string(i+1));
            if(ex){bool duplicate=false;for(auto& old:slots)if(old&&old->id==ex->id)duplicate=true;if(!duplicate)slots[i]=std::move(ex);}
        }
    }
    const Expansion* collection(int slot)const{return slot>=0&&slot<expansionSlots?slots[slot].get():nullptr;}
    const Expansion* collectionFor(int p)const{return p>=factoryCount&&p<totalPatches?collection((p-factoryCount)/expansionCapacity):nullptr;}
    const ExpansionPreset* preset(int p)const{auto* ex=collectionFor(p);return ex?ex->presets[(p-factoryCount)%expansionCapacity].get():nullptr;}
    static std::string label(int p){return "LIB "+std::to_string(1+(p-factoryCount)/expansionCapacity)+" / "+std::to_string(1+(p-factoryCount)%expansionCapacity);}
};
