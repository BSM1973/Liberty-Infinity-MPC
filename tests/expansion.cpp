#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <filesystem>
#include <iostream>
#include <unistd.h>
int main(){
    namespace fs=std::filesystem;char pattern[]="/tmp/infinity-exp-XXXXXX";char* temp=mkdtemp(pattern);if(!temp)return 1;
    struct Clean{fs::path p;~Clean(){std::error_code e;fs::remove_all(p,e);}} clean{temp};
    setenv("MPC_PLUGIN_FOLDER",temp,1);auto base=fs::path(mpc::contentPath("Factory.pcm")).parent_path();auto dir=base/"Expansions/01";fs::create_directories(dir);
    const unsigned char head[]={ 'L','I','N','F','H','D','0','1',100,0,0,0,5,0,0,0,4,0,0,0,0x44,0xac,0,0,0x88,0x58,1,0,24,0,0,0 };
    {std::ofstream f(base/"Factory.pcm",std::ios::binary);f.write((const char*)head,32);f.seekp(hd::legacyBytes-1);f.put(0);}
    const std::string id="12345678-1234-1234-1234-123456789abc";
    {std::ofstream f(dir/"Library.pcm",std::ios::binary);f.write("LINFEX01",8);for(int i=0;i<44100;++i){
        int x=int(3000000*std::sin(i*6.28318530718*261.6256/44100));for(int ch=0;ch<2;++ch)for(int k=0;k<3;++k)f.put(char((uint32_t(ch?x/2:x)>>(8*k))&255));}}
    {std::ofstream f(dir/"Library.map");f<<"LINFEXP1\nI "<<id<<"\nN \"Test Collection\"\nP 0 \"Test Stereo\"\nC 0 6\nL 0 0 1 0 0 0\n";for(int z=0;z<5;++z)for(int v=0;v<4;++v)f<<"X 0 0 "<<z<<" "<<v<<" 8 44100 2 44100 60 0 44100 0\n";
     f<<"P 2 \"Test Stereo Second\"\nC 2 6\nL 2 0 1 0 0 0\n";for(int z=0;z<5;++z)for(int v=0;v<4;++v)f<<"X 2 0 "<<z<<" "<<v<<" 8 44100 2 44100 60 0 44100 0\n";}
    {std::ofstream f(dir/"License.txt");f<<"LINFLIC1 "<<id<<" DEMO\n";}
    Infinity s;if(!s.usingPackedBank()||!s.expansionPreset(100)||s.expansionPreset(101)||s.numPrograms()!=500)return 2;
    if(s.browserCount(0,0)!=100||s.browserCount(1,6)!=2||s.browserCount(1,2)!=0||s.browserPatch(1,6,1)!=100)return 10;
    s.prepare(48000,128);s.set(P_library,1);s.set(P_category,6);s.set(P_ampon,0);s.set(P_filteron,0);
    float l[128],r[128];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=48000;
    for(int i=0;i<1500&&s.loadedPatch()!=100;++i){s.process(ctx);std::this_thread::sleep_for(std::chrono::milliseconds(2));}if(s.loadedPatch()!=100)return 3;
    if(s.getInt(Sound)!=100)return 11;s.set(P_preview,1);double preview=0;
    for(int i=0;i<30;++i){s.process(ctx);for(float x:l)preview+=x*x;}if(preview<1e-5)return 12;
    // Audition remains armed after the 0.85s note, without looping the note.
    for(int i=0;i<900;++i)s.process(ctx);
    if(!s.getBool(P_preview)||s.activeVoiceCount()!=0)return 30;
    s.set(P_browse,2);
    for(int i=0;i<1500&&s.loadedPatch()!=102;++i){s.process(ctx);std::this_thread::sleep_for(std::chrono::milliseconds(2));}
    if(s.loadedPatch()!=102||s.getInt(Sound)!=102||!s.getBool(P_preview))return 31;
    double second=0;for(int i=0;i<30;++i){s.process(ctx);for(float x:l)second+=x*x;}
    if(second<1e-5)return 32;
    // Rapid selection changes, then disable before/after load, no forced reactivation.
    s.set(P_browse,1);s.process(ctx);s.set(P_browse,2);s.process(ctx);s.set(P_browse,1);s.process(ctx);
    if(!s.getBool(P_preview))return 33;
    for(int i=0;i<1500&&s.loadedPatch()!=100;++i){s.process(ctx);std::this_thread::sleep_for(std::chrono::milliseconds(2));}
    if(s.loadedPatch()!=100||!s.getBool(P_preview))return 34;
    s.set(P_preview,0);for(int i=0;i<900;++i)s.process(ctx);
    if(s.getBool(P_preview)||s.activeVoiceCount()!=0)return 35;
    s.set(P_browse,2);
    for(int i=0;i<1500&&s.loadedPatch()!=102;++i){s.process(ctx);std::this_thread::sleep_for(std::chrono::milliseconds(2));}
    s.process(ctx);if(s.activeVoiceCount()!=0||s.getBool(P_preview))return 36;
    s.set(P_browse,1);
    for(int i=0;i<1500&&s.loadedPatch()!=100;++i){s.process(ctx);std::this_thread::sleep_for(std::chrono::milliseconds(2));}
    if(s.loadedPatch()!=100)return 37;
    std::cout<<"PASS persistent preview mode, automatic next preset, no looping, rapid changes, disabled stays silent\n";
    auto on=mpc::MidiEvent::noteOn(0,60,100);ctx.midi=&on;ctx.numMidiEvents=1;double energy=0;
    for(int i=0;i<30;++i){s.process(ctx);ctx.numMidiEvents=0;for(float x:l)energy+=x*x;}if(energy<1e-5)return 4;
    auto state=s.internalSaveState();if(!s.internalLoadState(state.data(),state.size())||s.getInt(Sound)!=100)return 5;
    std::vector<uint8_t> bad(36,'x');s.loadExtraState(bad.data(),bad.size());s.process(ctx);for(float x:l)if(x!=0)return 6;
    s.setProgram(0);if(s.getInt(Sound)!=0)return 7;
    // All 100 Factory names reachable in the unfiltered views and in their category.
    int all[100]={},filtered[100]={};
    for(int view=0;view<mainBrowserPageCount;++view)for(int row=1;row<=8;++row){
        int p=mainBrowserPatch(view,row);if(p<0)continue;if(p>=100)return 20;
        int cat=mainBrowserPages[view].category;
        if(cat){if(factory[p].kind+1!=cat)return 21;++filtered[p];}else ++all[p];
        s.set(P_mainView,float(view));s.set(P_mainPick,float(row));s.process(ctx);
        if(s.getInt(Sound)!=p||s.getInt(P_mainPick)!=0)return 22;
    }
    for(int p=0;p<100;++p)if(all[p]!=1||filtered[p]!=1)return 23;
    s.setProgram(0);s.set(P_mainView,1);s.process(ctx);if(s.getInt(Sound)!=0)return 24;
    s.set(P_mainPick,1);s.process(ctx);if(s.getInt(Sound)!=8)return 25;
    auto mainState=s.internalSaveState();s.set(P_mainView,0);s.setProgram(0);
    if(!s.internalLoadState(mainState.data(),mainState.size()))return 26;
    s.process(ctx);if(s.getInt(Sound)!=8)return 27;
    std::cout<<"PASS inline browser: all Factory/category rows, page-only navigation, row selection and state\n";
    fs::remove(dir/"License.txt");{Infinity no;if(no.expansionPreset(100))return 8;no.prepare(48000,128);no.setProgram(100);no.process(ctx);for(float x:l)if(x!=0)return 9;}
    std::cout<<"PASS independent PCM, collection/category browser, preview, stable state and missing entitlement rejection\n";
}
