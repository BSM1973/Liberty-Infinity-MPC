#import <Cocoa/Cocoa.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMIDI/CoreMIDI.h>
#include <atomic>
#include <memory>
#define INFINITY_STANDALONE
#include "../plugins/Infinity/src/Infinity.cpp"

static std::unique_ptr<Infinity> synth;
static std::atomic<int> notes[128];
static std::atomic<float> peakL{0},peakR{0};
static bool played[128];
static AVAudioEngine *audio;
static NSArray *controls;
static NSImage *panel;
static NSDictionary<NSString*,NSImage*> *faces;
static int current=0,category=0,page=0,visible[100],visibleCount=0;
static const char* categories[]={"All","Bass","Keys","Pads","Plucks","Leads","Pianos","Strings","Brass","Arpeggios","Bells & Mallets","Atmospheres","Synth & FX"};
static NSString* patchName(int i){return [NSString stringWithUTF8String:factory[i].name];}
static void rebuild(){visibleCount=0;for(int i=0;i<factoryCount;++i)if(category==0||std::string(factory[i].category)==categories[category])visible[visibleCount++]=i;page=0;}
static MIDIClientRef midiClient=0;static MIDIPortRef midiPort=0;
static std::atomic<int> external[128],externalPedal{0},externalBend{8192};
static int lastPedal=0,lastBend=8192;
static void midiRead(const MIDIPacketList* list,void*,void*){
    const MIDIPacket* p=&list->packet[0];
    for(UInt32 n=0;n<list->numPackets;++n,p=MIDIPacketNext(p)){
        unsigned i=0;uint8_t status=0;
        while(i<p->length){uint8_t b=p->data[i];if(b>=0xF8){++i;continue;}if(b&128){status=b;++i;if(status>=0xF0){status=0;break;}}
            if(!status)break;int kind=status&0xF0, count=(kind==0xC0||kind==0xD0)?1:2;if(i+count>p->length)break;
            int a=p->data[i];if(a&128){status=0;continue;}int v=count==2?p->data[i+1]:0;if(v&128){++i;status=0;continue;}i+=count;
            if(kind==0x90||kind==0x80)external[a].store(kind==0x90?v:0);
            else if(kind==0xB0&&a==64)externalPedal.store(v);
            else if(kind==0xB0&&(a==120||a==123)){for(auto& x:external)x.store(0);externalPedal.store(0);}
            else if(kind==0xE0)externalBend.store(a|(v<<7));
        }
    }
}
static NSColor *ink(){return [NSColor colorWithWhite:.98 alpha:1];}
static NSColor *gold(){return [NSColor colorWithSRGBRed:.86 green:.70 blue:.39 alpha:1];}
static NSColor *selection(){return [NSColor colorWithSRGBRed:.34 green:.25 blue:.12 alpha:1];}
static void text(NSString *s,NSRect r,CGFloat size,NSColor *c,bool center=false){
    NSMutableParagraphStyle *p=[NSMutableParagraphStyle new];p.alignment=center?NSTextAlignmentCenter:NSTextAlignmentLeft;
    [s drawInRect:r withAttributes:@{NSFontAttributeName:[NSFont systemFontOfSize:size weight:NSFontWeightMedium],NSForegroundColorAttributeName:c,NSParagraphStyleAttributeName:p}];
}
static NSRect bounds(NSDictionary *c){return NSMakeRect([c[@"x"] doubleValue],[c[@"y"] doubleValue],[c[@"w"] doubleValue],[c[@"h"] doubleValue]);}
static void choose(int p){if(p<0||p>=factoryCount)return;current=p;synth->setProgram(p);}
static const char *keyMap="awsedftgyhujkolp";
@interface Panel:NSView {int drag; CGFloat startY; float startValue; int mouseNote;}
@end
@implementation Panel
-(BOOL)isFlipped{return YES;}
-(BOOL)acceptsFirstResponder{return YES;}
-(instancetype)initWithFrame:(NSRect)r{if((self=[super initWithFrame:r])){drag=-1;mouseNote=-1;}return self;}
-(void)drawRect:(NSRect)dirty {
    (void)dirty;
    [panel drawInRect:NSMakeRect(0,0,1280,628) fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
    for(NSDictionary *c in controls){
        int i=[c[@"i"] intValue];NSRect r=bounds(c);if(i==0)continue;
        NSString *kind=c[@"kind"];NSDictionary *face=c[@"face"];
        CGFloat fw=[face[@"w"] doubleValue],fh=[face[@"h"] doubleValue],fy=[face[@"y"] doubleValue];
        NSRect dst=NSMakeRect(NSMidX(r)-fw/2,r.origin.y+fy,fw,fh);
        NSString *value=[NSString stringWithUTF8String:synth->displayText(i).c_str()]?:@"";
        NSString *asset=face[@"asset"];
        if([kind isEqualToString:@"toggle"]){
            NSString *key=[asset stringByAppendingString:synth->getBool(i)?@"_on.png":@"_off.png"];
            [faces[key] drawInRect:dst fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
        }else if([kind isEqualToString:@"selector"]){
            [faces[asset] drawInRect:dst fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
            text(value,NSMakeRect(dst.origin.x+2,dst.origin.y+8,fw-12,16),10,ink(),true);
        }else{
            int frame=std::clamp(int(std::round(synth->getNormalised(i)*126)),0,126);
            NSRect src=NSMakeRect(0,(126-frame)*fh,fw,fh);
            [faces[asset] drawInRect:dst fromRect:src operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
        }
        text(c[@"name"],NSMakeRect(r.origin.x,r.origin.y+[face[@"labelY"] doubleValue],r.size.width,14),12,ink(),true);
        if(![kind isEqualToString:@"selector"])text(value,NSMakeRect(r.origin.x,r.origin.y+[face[@"valueY"] doubleValue],r.size.width,11),10,ink(),true);
    }
    text(@"CATEGORY",NSMakeRect(315,99,175,24),17,gold());
    text(@"FACTORY PRESETS",NSMakeRect(505,99,445,24),17,gold());
    for(int i=0;i<13;++i){
        NSRect r=NSMakeRect(307,124+i*13,185,13);
        if(i==category){[selection()setFill];NSRectFill(r);}
        text([NSString stringWithUTF8String:categories[i]],r,11,ink());
    }
    for(int row=0;row<7;++row){int at=page*7+row;if(at>=visibleCount)break;int i=visible[at];
        NSRect r=NSMakeRect(500,124+row*23,461,22);
        if(i==current){[selection()setFill];NSRectFill(r);}
        text(patchName(i),NSInsetRect(r,6,0),16,ink());
    }
    text(@"< PREV",NSMakeRect(505,286,80,20),12,ink(),true);
    text([NSString stringWithFormat:@"%d / %d",page+1,std::max(1,(visibleCount+6)/7)],NSMakeRect(605,286,250,20),12,ink(),true);
    text(@"NEXT >",NSMakeRect(865,286,80,20),12,ink(),true);
    text(patchName(current),NSMakeRect(310,307,660,21),17,ink(),true);
    for(int c=0;c<2;++c){
        float p=c?peakR.load():peakL.load();
        int n=std::clamp(int((20*std::log10(std::max(p,.0001f))+54)/6),0,9);
        for(int j=0;j<9;++j){
            [(j<n?gold():[NSColor colorWithWhite:.16 alpha:1]) setFill];
            [[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(1117+c*23,475-j*7,14,5) xRadius:2 yRadius:2] fill];
        }
    }
    [[NSColor colorWithWhite:.13 alpha:1]setFill];NSRectFill(NSMakeRect(0,628,1280,92));
    text(@"CLAVIER DE TEST  |  A W S E D F T G Y H U J K O L P",NSMakeRect(16,630,1248,20),13,NSColor.whiteColor);
    for(int i=0;i<16;++i){
        NSRect r=NSMakeRect(16+i*78,653,76,62);
        [(notes[60+i].load()?selection():[NSColor colorWithWhite:.20 alpha:1])setFill];
        [[NSBezierPath bezierPathWithRoundedRect:r xRadius:4 yRadius:4]fill];
        text([NSString stringWithFormat:@"%c",keyMap[i]],NSMakeRect(r.origin.x,676,76,22),17,ink(),true);
    }
}
-(void)mouseDown:(NSEvent*)e{
    NSPoint p=[self convertPoint:e.locationInWindow fromView:nil];[self.window makeFirstResponder:self];
    if(p.y>=653&&p.y<715&&p.x>=16&&p.x<1264){mouseNote=60+int((p.x-16)/78);notes[mouseNote].store(100);return;}
    if(p.x>=307&&p.x<492&&p.y>=124&&p.y<293){category=int((p.y-124)/13);rebuild();return;}
    if(p.x>500&&p.x<961&&p.y>=124&&p.y<285){int at=page*7+int((p.y-124)/23);if(at<visibleCount)choose(visible[at]);return;}
    if(p.y>=286&&p.y<308){if(p.x>505&&p.x<585)page=std::max(0,page-1);if(p.x>865&&p.x<945)page=std::min((visibleCount-1)/7,page+1);return;}
    for(NSDictionary *c in controls){int i=[c[@"i"] intValue];if(!i||!NSPointInRect(p,bounds(c)))continue;
        if(i==8||i==15||i==18||i==23||i==31){synth->set(i,!synth->getBool(i));return;}
        if(i==19||i==20||i==24||i==32){
            NSMenu *menu=[[NSMenu alloc]initWithTitle:@"Select"];
            const auto &choices=synth->param(i).choices;
            for(int j=0;j<int(choices.size());++j){
                NSMenuItem *item=[[NSMenuItem alloc]initWithTitle:[NSString stringWithUTF8String:choices[j].c_str()] action:@selector(selectMode:) keyEquivalent:@""];
                item.target=self;item.tag=i*100+j;item.state=j==synth->getInt(i)?NSControlStateValueOn:NSControlStateValueOff;[menu addItem:item];
            }
            [NSMenu popUpContextMenu:menu withEvent:e forView:self];return;
        }
        drag=i;startY=p.y;startValue=synth->getNormalised(i);return;
    }
}
-(void)selectMode:(NSMenuItem*)item{synth->set(int(item.tag/100),int(item.tag%100));[self setNeedsDisplay:YES];}
-(void)mouseDragged:(NSEvent*)e{if(drag>=0){NSPoint p=[self convertPoint:e.locationInWindow fromView:nil];synth->setNormalised(drag,std::clamp(startValue+float(startY-p.y)/150.f,0.f,1.f));}}
-(void)mouseUp:(NSEvent*)e{(void)e;drag=-1;if(mouseNote>=0){notes[mouseNote].store(0);mouseNote=-1;}}
-(int)noteFor:(NSEvent*)e{NSString *s=e.charactersIgnoringModifiers.lowercaseString;if(s.length!=1)return -1;unichar c=[s characterAtIndex:0];for(int i=0;i<16;++i)if(c==keyMap[i])return 60+i;return -1;}
-(void)keyDown:(NSEvent*)e{int n=[self noteFor:e];if(n>=0){notes[n].store(100);return;}[super keyDown:e];}
-(void)keyUp:(NSEvent*)e{int n=[self noteFor:e];if(n>=0)notes[n].store(0);}
@end

@interface App:NSObject<NSApplicationDelegate>
@property(strong) NSWindow *window;
@property(strong) NSTimer *timer;
@end
@implementation App
-(void)applicationDidFinishLaunching:(NSNotification*)notification{
    (void)notification;
    NSString *res=NSBundle.mainBundle.resourcePath;
    setenv("MPC_PLUGIN_FOLDER",res.fileSystemRepresentation,1);
    NSData *data=[NSData dataWithContentsOfFile:[res stringByAppendingPathComponent:@"layout.json"]];
    NSDictionary *layout=[NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    controls=layout[@"controls"];panel=[[NSImage alloc]initWithContentsOfFile:[res stringByAppendingPathComponent:@"retro.png"]];
    NSMutableDictionary<NSString*,NSImage*> *loaded=[NSMutableDictionary new];
    for(NSString *name in @[@"gold_knob_36.png",@"gold_knob_40.png",@"power_36_on.png",@"power_36_off.png",@"power_40_on.png",@"power_40_off.png",@"selector_60.png"]){
        NSImage *img=[[NSImage alloc]initWithContentsOfFile:[res stringByAppendingPathComponent:name]];
        if(img)loaded[name]=img;
    }
    faces=[loaded copy];
    if(!controls||!panel||faces.count!=7){NSLog(@"Missing resources");[NSApp terminate:nil];return;}
    synth=std::make_unique<Infinity>();synth->setProgram(0);rebuild();
    if(synth->loadedZones()!=factoryCount*5){NSAlert *a=[NSAlert new];a.messageText=@"Banque sonore incomplète";[a runModal];[NSApp terminate:nil];return;}
    if(MIDIClientCreate(CFSTR("Liberty Infinity"),nullptr,nullptr,&midiClient)==noErr && MIDIInputPortCreate(midiClient,CFSTR("Input"),midiRead,nullptr,&midiPort)==noErr){for(ItemCount i=0;i<MIDIGetNumberOfSources();++i)MIDIPortConnectSource(midiPort,MIDIGetSource(i),nullptr);}
    audio=[AVAudioEngine new];
    AVAudioFormat *hardware=[audio.outputNode inputFormatForBus:0];
    double sr=hardware.sampleRate>0?hardware.sampleRate:44100;
    AVAudioFormat *fmt=[[AVAudioFormat alloc]initStandardFormatWithSampleRate:sr channels:2];
    synth->prepare(sr,4096);
    AVAudioSourceNode *source=[[AVAudioSourceNode alloc]initWithFormat:fmt renderBlock:^OSStatus(BOOL *silence,const AudioTimeStamp *stamp,AVAudioFrameCount count,AudioBufferList *abl){
        (void)stamp;if(abl->mNumberBuffers<2){for(UInt32 i=0;i<abl->mNumberBuffers;i++)memset(abl->mBuffers[i].mData,0,abl->mBuffers[i].mDataByteSize);*silence=YES;return noErr;}
        mpc::MidiEvent events[130];int n=0;
        int ped=externalPedal.load(),bend=externalBend.load();if(ped!=lastPedal){lastPedal=ped;events[n++]=mpc::MidiEvent::cc(0,64,ped);}if(bend!=lastBend){lastBend=bend;mpc::MidiEvent e;e.size=3;e.data[0]=0xE0;e.data[1]=bend&127;e.data[2]=(bend>>7)&127;events[n++]=e;}
        for(int i=0;i<128;++i){int v=std::max(notes[i].load(std::memory_order_relaxed),external[i].load(std::memory_order_relaxed));if(bool(v)!=played[i]){played[i]=bool(v);events[n++]=v?mpc::MidiEvent::noteOn(0,i,v):mpc::MidiEvent::noteOff(0,i);}}
        float *out[]={static_cast<float*>(abl->mBuffers[0].mData),static_cast<float*>(abl->mBuffers[1].mData)};
        mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=count;ctx.midi=events;ctx.numMidiEvents=n;ctx.sampleRate=sr;
        synth->process(ctx);float l=0,r=0;for(unsigned i=0;i<count;++i){l=std::max(l,std::abs(out[0][i]));r=std::max(r,std::abs(out[1][i]));}
        peakL.store(std::max(l,peakL.load()*.85f));peakR.store(std::max(r,peakR.load()*.85f));*silence=NO;return noErr;
    }];
    [audio attachNode:source];[audio connect:source to:audio.mainMixerNode format:fmt];
    NSError *error=nil;if(![audio startAndReturnError:&error]){NSAlert *a=[NSAlert new];a.messageText=@"Audio indisponible";a.informativeText=error.localizedDescription?:@"Erreur audio";[a runModal];[NSApp terminate:nil];return;}
    NSRect screen=NSScreen.mainScreen.visibleFrame;CGFloat scale=std::min(CGFloat(1),std::min((screen.size.width-24)/1280,(screen.size.height-48)/720));
    self.window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,1280*scale,720*scale) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
    self.window.title=@"Liberty Infinity | Black Gold | MPC Test";Panel *v=[[Panel alloc]initWithFrame:NSMakeRect(0,0,1280*scale,720*scale)];self.window.contentView=v;[v setBounds:NSMakeRect(0,0,1280,720)];[self.window center];[self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:v];
    self.timer=[NSTimer scheduledTimerWithTimeInterval:1./30 repeats:YES block:^(NSTimer *t){(void)t;[v setNeedsDisplay:YES];}];
    [NSApp activateIgnoringOtherApps:YES];
}
-(void)applicationDidResignActive:(NSNotification*)n{(void)n;for(auto &v:notes)v.store(0);}
-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)a{(void)a;return YES;}
-(void)applicationWillTerminate:(NSNotification*)n{(void)n;[self.timer invalidate];[audio stop];if(midiPort)MIDIPortDispose(midiPort);if(midiClient)MIDIClientDispose(midiClient);}
@end
int main(){@autoreleasepool{NSApplication *app=NSApplication.sharedApplication;[app setActivationPolicy:NSApplicationActivationPolicyRegular];App *delegate=[App new];app.delegate=delegate;[app run];}return 0;}
