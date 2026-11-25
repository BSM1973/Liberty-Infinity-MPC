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
    {std::ofstream f(dir/"Library.map");f<<"LINFEXP1\nI "<<id<<"\nN \"Test Collection\"\nP 0 \"Test Stereo\"\nL 0 0 1 0 0 0\n";for(int z=0;z<5;++z)for(int v=0;v<4;++v)f<<"X 0 0 "<<z<<" "<<v<<" 8 44100 2 44100 60 0 44100 0\n";}
    {std::ofstream f(dir/"License.txt");f<<"LINFLIC1 "<<id<<" DEMO\n";}
    Infinity s;if(!s.usingPackedBank()||!s.expansionPreset(100)||s.expansionPreset(101)||s.numPrograms()!=500)return 2;
    s.prepare(48000,128);s.setProgram(100);s.set(P_ampon,0);s.set(P_filteron,0);
    float l[128],r[128];float* out[]={l,r};mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=128;ctx.sampleRate=48000;
    for(int i=0;i<1500&&s.loadedPatch()!=100;++i){s.process(ctx);std::this_thread::sleep_for(std::chrono::milliseconds(2));}if(s.loadedPatch()!=100)return 3;
    auto on=mpc::MidiEvent::noteOn(0,60,100);ctx.midi=&on;ctx.numMidiEvents=1;double energy=0;
    for(int i=0;i<30;++i){s.process(ctx);ctx.numMidiEvents=0;for(float x:l)energy+=x*x;}if(energy<1e-5)return 4;
    auto state=s.internalSaveState();if(!s.internalLoadState(state.data(),state.size())||s.getInt(Sound)!=100)return 5;
    std::vector<uint8_t> bad(36,'x');s.loadExtraState(bad.data(),bad.size());s.process(ctx);for(float x:l)if(x!=0)return 6;
    s.setProgram(0);if(s.getInt(Sound)!=0)return 7;
    fs::remove(dir/"License.txt");{Infinity no;if(no.expansionPreset(100))return 8;no.prepare(48000,128);no.setProgram(100);no.process(ctx);for(float x:l)if(x!=0)return 9;}
    std::cout<<"PASS independent PCM, stable 100+400 slots, audio, state identity, missing entitlement rejection\n";
}
