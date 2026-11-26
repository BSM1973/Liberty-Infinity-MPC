#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"
#include <filesystem>
#include <iostream>
#include <unistd.h>
int main(){
    namespace fs=std::filesystem;char tmp[]="/tmp/infinity-lic-XXXXXX";char* dir=mkdtemp(tmp);if(!dir)return 1;
    struct Clean{fs::path p;~Clean(){std::error_code ec;fs::remove_all(p,ec);}} clean{dir};
    const std::string root=dir,id="12345678-1234-1234-1234-123456789abc",buyer="test_order";
    {std::ofstream f(root+"/Library.map");f<<"LINFEXP1\n";}
    {std::ofstream f(root+"/Library.pcm",std::ios::binary);f<<"LINFEX01test";}
    std::array<uint8_t,32> seed{},pub{},map{},pcm{};std::array<uint8_t,64> sk{},sig{};
    seed[0]=42;libertycrypto::crypto_eddsa_key_pair(sk.data(),pub.data(),seed.data());
    if(!hd::hashFile(root+"/Library.map",1024,map)||!hd::hashFile(root+"/Library.pcm",1024,pcm))return 2;
    auto m=hd::encodeHex(map.data(),32),p=hd::encodeHex(pcm.data(),32),message=hd::receiptMessage(id,buyer,m,p);
    libertycrypto::crypto_eddsa_sign(sig.data(),sk.data(),reinterpret_cast<const uint8_t*>(message.data()),message.size());
    auto receipt="LINFLIC2 "+id+" "+buyer+" "+m+" "+p+" "+hd::encodeHex(sig.data(),64)+"\n";
    {std::ofstream f(root+"/License.txt");f<<receipt;}
    if(!hd::verifyExpansionReceipt(root,id,pub,false))return 3;
    if(hd::verifyExpansionReceipt(root,id,hd::productionPublicKey,false))return 4;
    pub[0]^=1;if(hd::verifyExpansionReceipt(root,id,pub,false))return 5;pub[0]^=1;
    {std::ofstream f(root+"/Library.pcm",std::ios::app);f<<"tampered";}
    if(hd::verifyExpansionReceipt(root,id,pub,false))return 6;
    {std::ofstream f(root+"/License.txt");f<<"LINFLIC1 "<<id<<" DEMO\n";}
    if(!hd::verifyExpansionReceipt(root,id,pub,true)||hd::verifyExpansionReceipt(root,id,pub,false))return 7;
    std::cout<<"PASS signed receipt, wrong key, altered PCM and production DEMO rejection\n";
}
