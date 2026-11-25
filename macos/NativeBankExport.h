#pragma once
#include <stdexcept>
#include <locale>
#include <iomanip>
#include <unistd.h>

namespace studioHD {
struct WAV { uint64_t offset=0;int length=0,ch=0,sr=0,root=0,start=0,end=0,xf=0,kl=0,kh=127,vl=1,vh=127; };
inline void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
inline int field(NSDictionary *d,NSString *k,int lo,int hi){
    id v=d[k];require([v isKindOfClass:NSNumber.class],"Valeur numérique manquante.");
    double x=[v doubleValue];require(std::isfinite(x)&&x>=lo&&x<=hi&&x==std::floor(x),"Valeur entière hors limites.");return int(x);
}
inline double number(NSDictionary *d,NSString *k,double lo,double hi){
    id v=d[k];require([v isKindOfClass:NSNumber.class],"Valeur numérique manquante.");
    double x=[v doubleValue];require(std::isfinite(x)&&x>=lo&&x<=hi,"Valeur hors limites.");return x;
}
inline int detect(const std::vector<float>& a,int sr){
    const int stride=std::max(1,sr/8000);std::vector<double> x;
    for(size_t i=0;i<a.size();i+=stride){double sum=0;int n=0;for(size_t j=i;j<a.size()&&j<i+size_t(stride);++j){sum+=a[j];++n;}x.push_back(sum/n);}
    require(x.size()>256,"Note automatique incertaine : indiquez la note MIDI.");
    double mean=0;for(double v:x)mean+=v;mean/=x.size();for(double& v:x)v-=mean;
    const double fs=double(sr)/stride;const int low=std::max(2,int(fs/2100)),high=std::min(int(x.size()/3),int(fs/30));
    require(high>low+2,"Note automatique incertaine : indiquez la note MIDI.");
    std::vector<double> corr(high+1);
    for(int lag=low-1;lag<=high;++lag){
        double ab=0,aa=0,bb=0;for(size_t i=0;i+size_t(lag)<x.size();++i){double a=x[i],b=x[i+lag];ab+=a*b;aa+=a*a;bb+=b*b;}
        corr[lag]=aa>1e-12&&bb>1e-12?ab/std::sqrt(aa*bb):0.;
    }
    double best=0;for(int i=low;i<high;++i)if(corr[i]>corr[i-1]&&corr[i]>=corr[i+1])best=std::max(best,corr[i]);
    require(best>.65,"Note automatique incertaine : indiquez la note MIDI.");
    for(int i=low;i<high;++i)if(corr[i]>corr[i-1]&&corr[i]>=corr[i+1]&&corr[i]>=.92*best){
        double denom=corr[i-1]-2*corr[i]+corr[i+1];
        double lag=i+(std::abs(denom)>1e-9?.5*(corr[i-1]-corr[i+1])/denom:0.);
        int root=int(std::lround(69+12*std::log2(fs/lag/440.)));require(root>=0&&root<=127,"Note détectée hors limites.");return root;
    }
    throw std::runtime_error("Note automatique incertaine.");
}
inline AVAudioFile *open(NSString *path){
    require([path isKindOfClass:NSString.class],"Chemin WAV invalide.");
    AVAudioFile *f=[[AVAudioFile alloc]initForReading:[NSURL fileURLWithPath:path] commonFormat:AVAudioPCMFormatFloat32 interleaved:NO error:nil];
    require(f!=nil,"Impossible d’ouvrir un WAV.");return f;
}
inline void read(AVAudioFile *f,AVAudioPCMBuffer *buf){
    require([f readIntoBuffer:buf error:nil]&&buf.frameLength>0,"Lecture WAV interrompue.");
}
inline WAV describe(NSDictionary *z,double& peak){
    AVAudioFile *f=open(z[@"wav"]);auto fmt=f.processingFormat;
    WAV a;require(f.length>=64&&f.length<=hd::maxSampleFrames&&fmt.sampleRate>=8000&&fmt.sampleRate<=192000&&f.length<=fmt.sampleRate*120&&fmt.channelCount>=1&&fmt.channelCount<=2,"WAV non pris en charge (mono/stéréo, 8–192 kHz, 120 s maximum).");
    a.length=int(f.length);a.ch=int(fmt.channelCount);a.sr=int(std::lround(fmt.sampleRate));a.end=a.length;
    a.kl=field(z,@"key_min",0,127);a.kh=field(z,@"key_max",a.kl,127);a.vl=field(z,@"vel_min",1,127);a.vh=field(z,@"vel_max",a.vl,127);
    if(z[@"root"])a.root=field(z,@"root",0,127);
    NSArray *loop=z[@"loop"];if(loop){
        require([loop isKindOfClass:NSArray.class]&&loop.count==2,"Boucle invalide.");
        NSDictionary *bounds=@{@"start":loop[0],@"end":loop[1]};
        a.start=field(bounds,@"start",0,a.length-1);a.end=field(bounds,@"end",a.start+96,a.length);
        a.xf=std::min(a.sr/25,(a.end-a.start)/3);
    }
    AVAudioPCMBuffer *buf=[[AVAudioPCMBuffer alloc]initWithPCMFormat:fmt frameCapacity:8192];require(buf!=nil,"Tampon WAV indisponible.");
    std::array<std::vector<float>,2> pitch;double localPeak=0;AVAudioFramePosition done=0;
    while(done<f.length){
        read(f,buf);for(AVAudioFrameCount i=0;i<buf.frameLength;++i)for(int ch=0;ch<a.ch;++ch){
            float value=buf.floatChannelData[ch][i];require(std::isfinite(value),"WAV non fini.");localPeak=std::max(localPeak,std::abs(double(value)));
            if(!z[@"root"]&&done+i<a.sr)pitch[ch].push_back(value);
        }done+=buf.frameLength;
    }
    require(localPeak>1e-6,"WAV silencieux.");peak=std::max(peak,localPeak);
    if(!z[@"root"]){double energy[2]={0,0};for(int ch=0;ch<a.ch;++ch)for(float v:pitch[ch])energy[ch]+=double(v)*v;a.root=detect(pitch[energy[1]>energy[0]?1:0],a.sr);}
    return a;
}
inline void append(std::FILE* out,NSDictionary *z,WAV& a,double scale){
    AVAudioFile *f=open(z[@"wav"]);auto fmt=f.processingFormat;
    require(f.length==a.length&&fmt.channelCount==a.ch&&std::lround(fmt.sampleRate)==a.sr,"Le WAV a changé pendant l’export.");
    long offset=std::ftell(out);require(offset>=0&&uint64_t(offset)+uint64_t(a.length)*a.ch*3<=hd::maxBankBytes,"Banque trop grande pour MPC ARM32.");
    a.offset=uint64_t(offset);AVAudioPCMBuffer *buf=[[AVAudioPCMBuffer alloc]initWithPCMFormat:fmt frameCapacity:8192];require(buf!=nil,"Tampon WAV indisponible.");
    std::vector<unsigned char> raw(8192*a.ch*3);AVAudioFramePosition done=0;
    while(done<f.length){
        read(f,buf);size_t k=0;for(AVAudioFrameCount i=0;i<buf.frameLength;++i)for(int ch=0;ch<a.ch;++ch){
            double value=buf.floatChannelData[ch][i];require(std::isfinite(value),"WAV non fini.");
            int32_t sample=int32_t(std::llround(std::clamp(value*scale,-1.,1.-1./8388608)*8388608));
            uint32_t u=uint32_t(sample);raw[k++]=u&255;raw[k++]=(u>>8)&255;raw[k++]=(u>>16)&255;
        }require(std::fwrite(raw.data(),1,k,out)==k,"Écriture PCM impossible.");done+=buf.frameLength;
    }
}
inline NSString *exportBank(NSDictionary *manifest,NSString *base,NSString *dest,bool streaming=false){
    NSFileManager *fm=NSFileManager.defaultManager;NSString *out=[dest stringByAppendingPathComponent:@"Factory.pcm"],*map=[out stringByAppendingString:@".map"];
    NSString *tmp=[dest stringByAppendingPathComponent:[@".infinity-" stringByAppendingString:NSUUID.UUID.UUIDString]];
    NSString *bank=[tmp stringByAppendingPathComponent:@"Factory.pcm"],*mapping=[bank stringByAppendingString:@".map"];
    NSError *error=nil;
    if([fm fileExistsAtPath:out]||[fm fileExistsAtPath:map])return @"La destination contient déjà une banque. Choisissez un dossier vide.";
    if(![fm createDirectoryAtPath:tmp withIntermediateDirectories:NO attributes:nil error:&error])return error.localizedDescription?:@"Dossier temporaire impossible.";
    NSString *failure=nil;
    try{
        require([fm copyItemAtPath:base toPath:bank error:nil],"Copie de la banque Factory impossible.");
        std::unique_ptr<std::FILE,decltype(&std::fclose)> f(std::fopen(bank.fileSystemRepresentation,"r+b"),&std::fclose);
        require(bool(f)&&std::fseek(f.get(),0,SEEK_END)==0&&std::ftell(f.get())==long(hd::legacyBytes),"La base doit être la banque Factory originale.");
        std::ostringstream records;records.imbue(std::locale::classic());records<<std::setprecision(10)<<(streaming?"LINFMAP3\n":"LINFMAP2\n");
        for(NSDictionary *p in manifest[@"patches"]){int patch=field(p,@"patch",0,99),i=0;
            for(NSDictionary *l in p[@"layers"]){
                require(i<4,"Quatre couches maximum.");records<<"L "<<patch<<" "<<i++<<" "<<field(l,@"source",0,99)<<" "<<std::pow(10.,number(l,@"gain_db",-60,6)/20.)<<" "<<number(l,@"pan",-1,1)<<" "<<number(l,@"cents",-1200,1200)<<"\n";
            }require(i>0,"Aucune couche active.");
        }
        for(NSDictionary *s in manifest[@"sources"]){
            int source=field(s,@"source",0,99);NSArray *zones=s[@"zones"];require(zones.count>0&&zones.count<=128,"Nombre de zones invalide.");
            std::vector<WAV> samples;double peak=0;
            for(NSDictionary *z in zones)samples.push_back(describe(z,peak));
            const double scale=[s[@"normalize"] boolValue]?std::min(4.,.891250938/peak):1.;
            // Resolve each grid cell first; write only referenced samples, once.
            std::array<int,20> chosen{};chosen.fill(-1);
            for(int i=0;i<5;++i)for(int j=0;j<4;++j){
                const int note=hd::roots[i],vel=16+j*32;int best=1000,idx=-1;
                for(int k=0;k<int(samples.size());++k){const auto& a=samples[k];int dist=std::abs(a.root-note);
                    if(note>=a.kl&&note<=a.kh&&vel>=a.vl&&vel<=a.vh&&dist<best){best=dist;idx=k;}}
                require(idx>=0,"Mapping incomplet : les cinq notes et quatre vélocités doivent être couvertes.");chosen[i*4+j]=idx;
            }
            for(int i=0;i<5;++i)for(int j=0;j<4;++j){
                int k=chosen[i*4+j];auto& a=samples[k];if(!a.offset)append(f.get(),zones[k],a,scale);
                records<<"X "<<source<<" "<<i<<" "<<j<<" "<<a.offset<<" "<<a.length<<" "<<a.ch<<" "<<a.sr<<" "<<a.root<<" "<<a.start<<" "<<a.end<<" "<<a.xf<<"\n";
            }
        }
        require(std::fflush(f.get())==0,"Écriture PCM impossible.");auto raw=f.release();require(std::fclose(raw)==0,"Fermeture PCM impossible.");
        std::ofstream side(mapping.fileSystemRepresentation);side<<records.str();side.close();require(bool(side),"Écriture du mapping impossible.");
        // Validate bounds and all 100 patch cache budgets with the actual engine.
        {hd::Bank check(bank.fileSystemRepresentation);require(check.good(),"Banque invalide ou plafond de 48 Mio par cache dépassé.");}
        require(::link(bank.fileSystemRepresentation,out.fileSystemRepresentation)==0,"Impossible de publier la banque (fichier existant ou stockage incompatible).");
        if(::link(mapping.fileSystemRepresentation,map.fileSystemRepresentation)!=0){::unlink(out.fileSystemRepresentation);throw std::runtime_error("Impossible de publier le mapping.");}
    }catch(const std::exception& e){failure=[NSString stringWithUTF8String:e.what()]?:@"Erreur d’export.";}
    catch(...){failure=@"Erreur inconnue d’export.";}
    [fm removeItemAtPath:tmp error:nil];return failure;
}
}
