#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <iostream>
int main(int argc,char** argv){
    if(argc!=2)return 1;hd::Bank b(argv[1]);if(!b.good())return 2;
    for(int p:{68,70,72,78}){
        b.requestPatch(p);for(int i=0;i<2000&&b.loadedPatch()!=p;++i){b.select(p);std::this_thread::sleep_for(std::chrono::milliseconds(2));}
        if(b.loadedPatch()!=p||b.layers()!=1)return 3;
        for(int v:{0,3}){
            auto rec=b.sampleRecord(0,2,v);if(rec.offset<hd::legacyBytes||rec.length<64)return 4;
            double e=0;for(int i=0;i<8192&&i<rec.length-16;++i){float x=b.sample(0,2,v,i,0);if(!std::isfinite(x))return 5;e+=x*x;}
            if(e<1e-8)return 6;
        }
        std::cout<<"PASS recorded patch "<<p<<"\n";
    }
}
