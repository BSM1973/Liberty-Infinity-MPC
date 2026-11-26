#pragma once
// Included in namespace hd, after the standard headers.
#include "LicensePolicy.h"
inline bool decodeHex(const std::string& text,uint8_t* out,size_t n){
    if(text.size()!=2*n)return false;
    auto digit=[](char c)->int{return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    for(size_t i=0;i<n;++i){int a=digit(text[2*i]),b=digit(text[2*i+1]);if(a<0||b<0)return false;out[i]=uint8_t(a*16+b);}
    return true;
}
inline std::string encodeHex(const uint8_t* bytes,size_t n){
    constexpr char digits[]="0123456789abcdef";std::string s(2*n,'0');
    for(size_t i=0;i<n;++i){s[2*i]=digits[bytes[i]>>4];s[2*i+1]=digits[bytes[i]&15];}return s;
}
inline bool hashFile(const std::string& path,uint64_t limit,std::array<uint8_t,32>& digest){
    std::FILE* file=std::fopen(path.c_str(),"rb");if(!file)return false;
    libertycrypto::crypto_blake2b_ctx context;libertycrypto::crypto_blake2b_init(&context,digest.size());
    std::array<uint8_t,16384> bytes{};uint64_t total=0;bool ok=true;
    while(size_t n=std::fread(bytes.data(),1,bytes.size(),file)){
        total+=n;if(total>limit){ok=false;break;}
        libertycrypto::crypto_blake2b_update(&context,bytes.data(),n);
    }
    if(std::ferror(file))ok=false;std::fclose(file);
    libertycrypto::crypto_blake2b_final(&context,digest.data());return ok;
}
inline std::string receiptMessage(const std::string& id,const std::string& buyer,const std::string& map,const std::string& pcm){
    return "LINFLIC2\n"+id+"\n"+buyer+"\n"+map+"\n"+pcm+"\n";
}
inline bool validBuyer(const std::string& s){
    if(s.empty()||s.size()>64)return false;
    for(char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'))return false;
    return true;
}
inline bool verifyExpansionReceipt(const std::string& dir,const std::string& id,const std::array<uint8_t,32>& key,bool demo){
    std::ifstream in(dir+"/License.txt",std::ios::binary|std::ios::ate);
    if(!in||in.tellg()<0||in.tellg()>1024)return false;in.seekg(0);
    std::string version,received;if(!(in>>version>>received)||received!=id)return false;
    if(version=="LINFLIC1"){std::string mode,extra;return demo&&(in>>mode)&&mode=="DEMO"&&!(in>>extra);}
    if(version!="LINFLIC2"||std::all_of(key.begin(),key.end(),[](uint8_t b){return b==0;}))return false;
    std::string buyer,map,pcm,signature,extra;
    if(!(in>>buyer>>map>>pcm>>signature)||!validBuyer(buyer)||(in>>extra))return false;
    std::array<uint8_t,64> sig{};std::array<uint8_t,32> mapHash{},pcmHash{},actual{};
    if(!decodeHex(signature,sig.data(),sig.size())||!decodeHex(map,mapHash.data(),32)||!decodeHex(pcm,pcmHash.data(),32))return false;
    auto message=receiptMessage(id,buyer,map,pcm);
    if(libertycrypto::crypto_eddsa_check(sig.data(),key.data(),reinterpret_cast<const uint8_t*>(message.data()),message.size())!=0)return false;
    // Bounded streaming digest; no bank allocation. Performed only during plugin construction.
    return hashFile(dir+"/Library.map",8u*1024u*1024u,actual)&&actual==mapHash&&
           hashFile(dir+"/Library.pcm",maxBankBytes,actual)&&actual==pcmHash;
}
