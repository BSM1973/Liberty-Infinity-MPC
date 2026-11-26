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
static int pcmSelection=-1;
static int current=0,category=0,page=0,style=0,visible[500],visibleCount=0;
static constexpr int pageSize=6;
static bool favoritesOnly=false;
static NSMutableSet<NSString*> *favorites;
static NSMutableDictionary<NSString*,NSDictionary*> *userPresets;
static NSString *query=@"",*userName=nil;
static std::atomic<int> auditionCommand{0},auditionPlaying{0},midiFlash{0};
static const char* categories[]={"All","Bass","Keys","Pads","Plucks","Leads","Pianos","Strings","Brass","Arpeggios","Bells & Mallets","Atmospheres","Synth & FX"};
static NSString* patchName(int i){return [NSString stringWithUTF8String:factory[i].name];}
static const char* styles[]={"All styles","Electronic","Cinematic","Ambient","Pop / Keys","Retro"};
static bool matchesStyle(int i){
    int k=factory[i].kind,v=factory[i].variant;
    switch(style){
        case 1:return k==0||k==3||k==4||k==8||k==11;
        case 2:return k==2||k==6||k==7||k==10||k==11;
        case 3:return k==2||k==9||k==10||k==3;
        case 4:return k==1||k==5||k==3||k==6;
        case 5:return (k==0||k==1||k==4||k==7)&&v%3!=2;
        default:return true;
    }
}
static void persist(){NSUserDefaults *d=NSUserDefaults.standardUserDefaults;[d setObject:favorites.allObjects forKey:@"InfinityFavoritesV1"];[d setObject:userPresets forKey:@"InfinityUserPresetsV1"];}
#include "PresetBackup.h"
#include "PresetLibrary.h"
static void rebuild(){
    visibleCount=0;
    NSArray *names=activeLibrary()?libraryNames():nil;
    int count=names?int(names.count):factoryCount;
    for(int i=0;i<count;++i){
        int f=names?factoryIndex(activeLibrary()[@"presets"][names[i]]):i;if(f<0)continue;
        if(category&&std::string(factory[f].category)!=categories[category])continue;
        if(!matchesStyle(f)||(!names&&favoritesOnly&&![favorites containsObject:patchName(f)]))continue;
        NSString *hay=[NSString stringWithFormat:@"%@ %s",names?names[i]:patchName(f),factory[f].category];
        if(query.length&&[hay rangeOfString:query options:NSCaseInsensitiveSearch|NSDiacriticInsensitiveSearch].location==NSNotFound)continue;
        visible[visibleCount++]=i;
    }
    page=0;
}

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
static void browserButton(NSString *title, NSRect r, bool active=false){
    NSBezierPath *p=[NSBezierPath bezierPathWithRoundedRect:r xRadius:3 yRadius:3];[(active?selection():[NSColor colorWithWhite:.10 alpha:1])setFill];[p fill];[gold() setStroke];p.lineWidth=.7;[p stroke];
    text(title,NSInsetRect(r,3,2),11,ink(),true);
}
#include "MultisampleStudio.h"
static NSRect bounds(NSDictionary *c){return NSMakeRect([c[@"x"] doubleValue],[c[@"y"] doubleValue],[c[@"w"] doubleValue],[c[@"h"] doubleValue]);}
static void choose(int p){if(p<0||p>=factoryCount)return;pcmSelection=-1;auditionCommand.store(-1);current=p;userName=nil;synth->setProgram(p);}
static void chooseRow(int i){
    if(!activeLibrary()){choose(i);selectedLibraryRow=-1;return;}
    NSDictionary *record=libraryPreset(i);int f=factoryIndex(record);if(f<0)return;
    choose(f);NSDictionary *values=record[@"values"];
    for(int k=1;k<soundParameterCount;++k)synth->setNormalised(k,[values[[NSString stringWithUTF8String:synth->param(k).id.c_str()]] floatValue]);
    selectedLibraryRow=i;userName=libraryPresetName(i);
}
static const char *keyMap="awsedftgyhujkolp";
@interface Panel:NSView<NSSearchFieldDelegate> {int drag; CGFloat startY; float startValue; int mouseNote; NSSearchField *search;}
@end
@implementation Panel
-(BOOL)isFlipped{return YES;}
-(BOOL)acceptsFirstResponder{return YES;}
-(instancetype)initWithFrame:(NSRect)r{if((self=[super initWithFrame:r])){drag=-1;mouseNote=-1;
    search=[[NSSearchField alloc]initWithFrame:NSMakeRect(500,99,461,24)];search.placeholderString=@"Rechercher un preset";search.font=[NSFont systemFontOfSize:12];search.delegate=self;search.sendsSearchStringImmediately=YES;search.appearance=[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];[self addSubview:search];}return self;}
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
    NSString *shown=pcmSelection>=0?@"Extension PCM":collectionName();
    browserButton([NSString stringWithFormat:@"%@ ▾",shown],NSMakeRect(540,54,260,26),true);
    browserButton([NSString stringWithUTF8String:styles[style]],NSMakeRect(307,100,185,22),style!=0);
    for(int i=0;i<13;++i){
        NSRect r=NSMakeRect(307,128+i*13,185,13);
        if(i==category){[selection()setFill];NSRectFill(r);}
        text([NSString stringWithUTF8String:categories[i]],NSInsetRect(r,3,0),11,ink());
    }
    browserButton(activeLibrary()?@"Collection externe":favoritesOnly?@"Favoris actifs":@"Tous / Favoris",NSMakeRect(307,302,185,23),!activeLibrary()&&favoritesOnly);
    for(int row=0;row<pageSize;++row){int at=page*pageSize+row;if(at>=visibleCount)break;int i=visible[at];
        NSRect r=NSMakeRect(500,128+row*22,461,21);
        if(pcmSelection<0&&i==(activeLibrary()?selectedLibraryRow:current)){[selection()setFill];NSRectFill(r);}
        NSString *name=activeLibrary()?libraryPresetName(i):patchName(i);
        text(name,NSMakeRect(508,r.origin.y,403,21),15,ink());
        if(!activeLibrary())text([favorites containsObject:patchName(i)]?@"*":@"+",NSMakeRect(928,r.origin.y,28,21),15,gold(),true);
    }
    if(!visibleCount)text(@"Aucun résultat",NSMakeRect(505,159,450,25),15,ink(),true);
    browserButton(@"< Son",NSMakeRect(500,264,68,20));
    browserButton(@"Page -",NSMakeRect(574,264,68,20));
    text([NSString stringWithFormat:@"%d / %d | %d sons",page+1,std::max(1,(visibleCount+pageSize-1)/pageSize),visibleCount],NSMakeRect(645,266,169,18),11,ink(),true);
    browserButton(@"Page +",NSMakeRect(817,264,68,20));
    browserButton(@"Son >",NSMakeRect(891,264,70,20));
    browserButton(auditionPlaying.load()?@"Stop":@"Écouter",NSMakeRect(500,288,108,20),auditionPlaying.load()!=0);
    browserButton(@"Enregistrer",NSMakeRect(613,288,112,20));
    browserButton(@"Mes presets",NSMakeRect(730,288,112,20));
    browserButton(activeLibrary()?@"Factory 1":([favorites containsObject:patchName(current)]?@"Favori : oui":@"Favori : non"),NSMakeRect(847,288,114,20),!activeLibrary()&&[favorites containsObject:patchName(current)]);
    text(synth->loadedPatch()!=(pcmSelection>=0?pcmSelection:current)?@"Chargement du son...":(pcmSelection>=0?[NSString stringWithUTF8String:synth->displayText(0).c_str()]:(userName?:patchName(current))),NSMakeRect(500,312,461,19),14,ink(),true);
    [(midiFlash.load()>0?gold():[NSColor colorWithWhite:.16 alpha:1]) setFill];
    [[NSBezierPath bezierPathWithOvalInRect:NSMakeRect(1047,414,8,8)]fill];
    text(@"MIDI",NSMakeRect(1060,410,45,16),11,ink());
    text([NSString stringWithFormat:@"%d / %d voix",synth->activeVoiceCount(),synth->maxVoices()],NSMakeRect(1046,433,95,18),11,ink());
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
    if(NSPointInRect(p,NSMakeRect(540,54,260,26))){
        NSMenu *menu=[[NSMenu alloc]initWithTitle:@"Collections"];
        NSMenuItem *item=[[NSMenuItem alloc]initWithTitle:@"Factory 1 (100 presets)" action:@selector(selectCollection:) keyEquivalent:@""];item.target=self;item.tag=-1;item.state=collection==-1?NSControlStateValueOn:NSControlStateValueOff;[menu addItem:item];
        for(NSInteger j=0;j<(NSInteger)libraries.count;++j){
            NSDictionary *lib=libraries[j];NSString *title=[NSString stringWithFormat:@"%@ — %@ (%lu)",lib[@"name"],lib[@"vendor"],(unsigned long)[lib[@"presets"] count]];
            NSMenuItem *it=[[NSMenuItem alloc]initWithTitle:title action:@selector(selectCollection:) keyEquivalent:@""];it.target=self;it.tag=j;it.state=collection==j?NSControlStateValueOn:NSControlStateValueOff;[menu addItem:it];
        }
        for(int s=0;s<hd::expansionSlots;++s)if(auto* ex=synth->expansion(s)){
            NSString *title=[NSString stringWithUTF8String:ex->name.c_str()]?:@"Extension PCM";
            NSMenuItem *parent=[[NSMenuItem alloc]initWithTitle:title action:nil keyEquivalent:@""];
            NSMenu *sub=[[NSMenu alloc]initWithTitle:title];parent.submenu=sub;
            for(int j=0;j<hd::expansionCapacity;++j){int index=factoryCount+s*hd::expansionCapacity+j;
                if(auto* ep=synth->expansionPreset(index)){
                    NSMenuItem *it=[[NSMenuItem alloc]initWithTitle:[NSString stringWithUTF8String:ep->name.c_str()]?:@"Preset" action:@selector(selectPCM:) keyEquivalent:@""];
                    it.target=self;it.tag=index;[sub addItem:it];
                }
            }[menu addItem:parent];
        }
        [NSMenu popUpContextMenu:menu withEvent:e forView:self];return;
    }
    if(p.x>=307&&p.x<492&&p.y>=100&&p.y<122){
        NSMenu *menu=[[NSMenu alloc]initWithTitle:@"Style"];for(int j=0;j<6;++j){NSMenuItem *item=[[NSMenuItem alloc]initWithTitle:[NSString stringWithUTF8String:styles[j]] action:@selector(selectStyle:) keyEquivalent:@""];item.target=self;item.tag=j;item.state=j==style?NSControlStateValueOn:NSControlStateValueOff;[menu addItem:item];}[NSMenu popUpContextMenu:menu withEvent:e forView:self];return;
    }
    if(p.x>=307&&p.x<492&&p.y>=128&&p.y<297){category=int((p.y-128)/13);rebuild();return;}
    if(NSPointInRect(p,NSMakeRect(307,302,185,23))){if(!activeLibrary()){favoritesOnly=!favoritesOnly;rebuild();}return;}
    if(p.x>=500&&p.x<961&&p.y>=128&&p.y<260){int at=page*pageSize+int((p.y-128)/22);if(at<visibleCount){if(p.x>=920&&!activeLibrary())[self toggleFavorite:visible[at]];else chooseRow(visible[at]);}return;}
    if(p.y>=264&&p.y<284&&p.x>=500&&p.x<961){
        if(p.x<568)[self stepPreset:-1];else if(p.x>=574&&p.x<642)page=std::max(0,page-1);
        else if(p.x>=817&&p.x<885)page=std::min(std::max(0,(visibleCount-1)/pageSize),page+1);
        else if(p.x>=891)[self stepPreset:1];return;
    }
    if(p.y>=288&&p.y<308&&p.x>=500&&p.x<961){
        if(p.x<608&&synth->loadedPatch()==(pcmSelection>=0?pcmSelection:current))auditionCommand.store(auditionPlaying.load()?-1:1);
        else if(p.x>=613&&p.x<725&&pcmSelection<0)[self saveUserPreset];
        else if(p.x>=730&&p.x<842)[self showUserPresets:e];
        else if(p.x>=847){if(activeLibrary()){collection=-1;selectedLibraryRow=-1;category=style=0;query=@"";search.stringValue=@"";favoritesOnly=false;rebuild();chooseRow(current);}else [self toggleFavorite:current];}return;
    }
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
-(void)controlTextDidChange:(NSNotification*)n{if(n.object==search){query=[search.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];rebuild();[self setNeedsDisplay:YES];}}
-(void)selectStyle:(NSMenuItem*)item{style=int(item.tag);rebuild();[self setNeedsDisplay:YES];}
-(void)selectPCM:(NSMenuItem*)item{pcmSelection=int(item.tag);auditionCommand.store(-1);synth->setProgram(pcmSelection);[self setNeedsDisplay:YES];}
-(void)selectCollection:(NSMenuItem*)item{collection=item.tag;selectedLibraryRow=-1;category=style=0;query=@"";search.stringValue=@"";favoritesOnly=false;rebuild();if(visibleCount)chooseRow(visible[0]);[self setNeedsDisplay:YES];}
-(void)stepPreset:(int)delta{if(!visibleCount)return;int at=-1;for(int j=0;j<visibleCount;++j)if(visible[j]==(activeLibrary()?selectedLibraryRow:current)){at=j;break;}at=at<0?(delta>0?0:visibleCount-1):(at+delta+visibleCount)%visibleCount;chooseRow(visible[at]);page=at/pageSize;}
-(void)toggleFavorite:(int)i{NSString *name=patchName(i);if([favorites containsObject:name])[favorites removeObject:name];else [favorites addObject:name];persist();int old=page;rebuild();page=std::min(old,std::max(0,(visibleCount-1)/pageSize));}
-(void)saveUserPreset{
    NSAlert *a=[NSAlert new];a.messageText=@"Enregistrer un preset personnel";[a addButtonWithTitle:@"Enregistrer"];[a addButtonWithTitle:@"Annuler"];
    NSTextField *field=[[NSTextField alloc]initWithFrame:NSMakeRect(0,0,270,24)];field.stringValue=userName?:patchName(current);a.accessoryView=field;a.window.initialFirstResponder=field;
    if([a runModal]!=NSAlertFirstButtonReturn)return;
    NSString *name=[field.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];if(!name.length||name.length>80)return;
    if(userPresets[name]){NSAlert *confirm=[NSAlert new];confirm.messageText=@"Remplacer ce preset ?";[confirm addButtonWithTitle:@"Remplacer"];[confirm addButtonWithTitle:@"Annuler"];if([confirm runModal]!=NSAlertFirstButtonReturn)return;}
    NSMutableDictionary *values=[NSMutableDictionary new];for(int i=0;i<soundParameterCount;++i)values[[NSString stringWithUTF8String:synth->param(i).id.c_str()]]=@(synth->getNormalised(i));
    userPresets[name]=@{@"schema":@1,@"factory":patchName(current),@"values":values};userName=name;persist();
}
-(void)showUserPresets:(NSEvent*)e{
    NSMenu *menu=[[NSMenu alloc]initWithTitle:@"Mes presets"];NSArray *names=[userPresets.allKeys sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)];
    if(!names.count){NSMenuItem *empty=[[NSMenuItem alloc]initWithTitle:@"Aucun preset personnel" action:nil keyEquivalent:@""];[menu addItem:empty];}
    for(NSString *name in names){NSMenuItem *item=[[NSMenuItem alloc]initWithTitle:name action:@selector(loadUserPreset:) keyEquivalent:@""];item.target=self;item.representedObject=name;[menu addItem:item];}
    [NSMenu popUpContextMenu:menu withEvent:e forView:self];
}
-(void)loadUserPreset:(NSMenuItem*)item{
    NSString *name=item.representedObject;NSDictionary *record=userPresets[name];if(![record isKindOfClass:NSDictionary.class]||![record[@"schema"] isEqual:@1])return;
    NSDictionary *values=record[@"values"];if(![values isKindOfClass:NSDictionary.class])return;
    int p=-1;for(int i=0;i<factoryCount;++i)if([record[@"factory"] isEqual:patchName(i)]){p=i;break;}if(p<0)return;
    for(int i=1;i<soundParameterCount;++i){id v=values[[NSString stringWithUTF8String:synth->param(i).id.c_str()]];if(![v isKindOfClass:NSNumber.class]||!std::isfinite([v doubleValue])||[v doubleValue]<0||[v doubleValue]>1)return;}
    selectedLibraryRow=-1;choose(p);for(int i=1;i<soundParameterCount;++i)synth->setNormalised(i,[values[[NSString stringWithUTF8String:synth->param(i).id.c_str()]] floatValue]);userName=name;
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
    NSString *support=[libraryDirectory() stringByDeletingLastPathComponent];if(support)setenv("INFINITY_EXPANSION_ROOT",support.fileSystemRepresentation,1);
    synth=std::make_unique<Infinity>();synth->setProgram(0);
    NSUserDefaults *d=NSUserDefaults.standardUserDefaults;favorites=[NSMutableSet setWithArray:[d arrayForKey:@"InfinityFavoritesV1"]?:@[]];userPresets=[[d dictionaryForKey:@"InfinityUserPresetsV1"] mutableCopy]?:[NSMutableDictionary new];loadLibraries();rebuild();
    if(synth->loadedZones()!=factoryCount*5*4){NSAlert *a=[NSAlert new];a.messageText=@"Banque sonore incomplète";[a runModal];[NSApp terminate:nil];return;}
    if(MIDIClientCreate(CFSTR("Liberty Infinity"),nullptr,nullptr,&midiClient)==noErr && MIDIInputPortCreate(midiClient,CFSTR("Input"),midiRead,nullptr,&midiPort)==noErr){for(ItemCount i=0;i<MIDIGetNumberOfSources();++i)MIDIPortConnectSource(midiPort,MIDIGetSource(i),nullptr);}
    audio=[AVAudioEngine new];
    AVAudioFormat *hardware=[audio.outputNode inputFormatForBus:0];
    double sr=hardware.sampleRate>0?hardware.sampleRate:44100;
    AVAudioFormat *fmt=[[AVAudioFormat alloc]initStandardFormatWithSampleRate:sr channels:2];
    synth->prepare(sr,4096);
    AVAudioSourceNode *source=[[AVAudioSourceNode alloc]initWithFormat:fmt renderBlock:^OSStatus(BOOL *silence,const AudioTimeStamp *stamp,AVAudioFrameCount count,AudioBufferList *abl){
        (void)stamp;if(abl->mNumberBuffers<2){for(UInt32 i=0;i<abl->mNumberBuffers;i++)memset(abl->mBuffers[i].mData,0,abl->mBuffers[i].mDataByteSize);*silence=YES;return noErr;}
        mpc::MidiEvent events[134];int n=0;
        int ped=externalPedal.load(),bend=externalBend.load();if(ped!=lastPedal){lastPedal=ped;events[n++]=mpc::MidiEvent::cc(0,64,ped);}if(bend!=lastBend){lastBend=bend;mpc::MidiEvent e;e.size=3;e.data[0]=0xE0;e.data[1]=bend&127;e.data[2]=(bend>>7)&127;events[n++]=e;}
        for(int i=0;i<128;++i){int v=std::max(notes[i].load(std::memory_order_relaxed),external[i].load(std::memory_order_relaxed));if(bool(v)!=played[i]){played[i]=bool(v);events[n++]=v?mpc::MidiEvent::noteOn(0,i,v):mpc::MidiEvent::noteOff(0,i);}}
        if(n)midiFlash.store(int(sr*.15));else midiFlash.store(std::max(0,midiFlash.load()-int(count)));
        static int previewFrames=0;
        int command=auditionCommand.exchange(0);
        if(command||previewFrames>0){
            if(previewFrames>0&&(command||previewFrames<=int(count))){
                events[n++]=mpc::MidiEvent::noteOff(15,60);previewFrames=0;auditionPlaying.store(0);
            }
            if(command==1){events[n++]=mpc::MidiEvent::noteOn(15,60,95);previewFrames=int(sr*.85);auditionPlaying.store(1);}
            else if(previewFrames>0)previewFrames-=int(count);
        }
        float *out[]={static_cast<float*>(abl->mBuffers[0].mData),static_cast<float*>(abl->mBuffers[1].mData)};
        mpc::ProcessContext ctx;ctx.outputs=out;ctx.numOutputs=2;ctx.numFrames=count;ctx.midi=events;ctx.numMidiEvents=n;ctx.sampleRate=sr;
        synth->process(ctx);float l=0,r=0;for(unsigned i=0;i<count;++i){l=std::max(l,std::abs(out[0][i]));r=std::max(r,std::abs(out[1][i]));}
        peakL.store(std::max(l,peakL.load()*.85f));peakR.store(std::max(r,peakR.load()*.85f));*silence=NO;return noErr;
    }];
    [audio attachNode:source];[audio connect:source to:audio.mainMixerNode format:fmt];
    NSError *error=nil;if(![audio startAndReturnError:&error]){NSAlert *a=[NSAlert new];a.messageText=@"Audio indisponible";a.informativeText=error.localizedDescription?:@"Erreur audio";[a runModal];[NSApp terminate:nil];return;}
    NSRect screen=NSScreen.mainScreen.visibleFrame;CGFloat scale=std::min(CGFloat(1),std::min((screen.size.width-24)/1280,(screen.size.height-48)/720));
    self.window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,1280*scale,720*scale) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
    self.window.title=@"Liberty Infinity | Black Gold | Multisample Studio";Panel *v=[[Panel alloc]initWithFrame:NSMakeRect(0,0,1280*scale,720*scale)];self.window.contentView=v;[v setBounds:NSMakeRect(0,0,1280,720)];[self.window center];[self.window makeKeyAndOrderFront:nil];[self.window makeFirstResponder:v];
    self.timer=[NSTimer scheduledTimerWithTimeInterval:1./30 repeats:YES block:^(NSTimer *t){(void)t;[v setNeedsDisplay:YES];}];
    NSMenu *bar=[NSMenu new];NSMenuItem *appItem=[NSMenuItem new];[bar addItem:appItem];NSMenu *appMenu=[NSMenu new];[appItem setSubmenu:appMenu];[appMenu addItemWithTitle:@"Quitter Liberty Infinity" action:@selector(terminate:) keyEquivalent:@"q"];
    NSMenuItem *studioItem=[[NSMenuItem alloc]initWithTitle:@"Multisample" action:nil keyEquivalent:@""];[bar addItem:studioItem];NSMenu *studioMenu=[[NSMenu alloc]initWithTitle:@"Multisample"];studioItem.submenu=studioMenu;
    NSMenuItem *open=[[NSMenuItem alloc]initWithTitle:@"Ouvrir Multisample Studio" action:@selector(openStudio:) keyEquivalent:@"m"];open.target=self;[studioMenu addItem:open];
    NSMenuItem *backupItem=[[NSMenuItem alloc]initWithTitle:@"Sauvegarde" action:nil keyEquivalent:@""];[bar addItem:backupItem];
    NSMenu *backupMenu=[[NSMenu alloc]initWithTitle:@"Sauvegarde"];backupItem.submenu=backupMenu;
    NSMenuItem *save=[[NSMenuItem alloc]initWithTitle:@"Exporter favoris et presets…" action:@selector(exportBackup:) keyEquivalent:@""];save.target=self;[backupMenu addItem:save];
    NSMenuItem *restore=[[NSMenuItem alloc]initWithTitle:@"Restaurer favoris et presets…" action:@selector(restoreBackup:) keyEquivalent:@""];restore.target=self;[backupMenu addItem:restore];
    NSMenuItem *libItem=[[NSMenuItem alloc]initWithTitle:@"Collections" action:nil keyEquivalent:@""];[bar addItem:libItem];
    NSMenu *libMenu=[[NSMenu alloc]initWithTitle:@"Collections"];libItem.submenu=libMenu;
    NSMenuItem *imp=[[NSMenuItem alloc]initWithTitle:@"Installer une Library…" action:@selector(importLibrary:) keyEquivalent:@""];imp.target=self;[libMenu addItem:imp];
    NSMenuItem *exp=[[NSMenuItem alloc]initWithTitle:@"Créer une Library de presets…" action:@selector(exportLibrary:) keyEquivalent:@""];exp.target=self;[libMenu addItem:exp];
    NSApp.mainMenu=bar;
    [NSApp activateIgnoringOtherApps:YES];
}
-(void)backupMessage:(NSString*)message{NSAlert *a=[NSAlert new];a.messageText=message;[a runModal];}
-(void)importLibrary:(id)sender{
    (void)sender;NSOpenPanel *p=[NSOpenPanel openPanel];p.allowedFileTypes=@[@"infinitylibrary"];if([p runModal]!=NSModalResponseOK)return;
    NSDictionary *doc=readLibrary(p.URL);if(!doc){[self backupMessage:@"Library invalide ou incompatible avec Factory 1."];return;}
    if(libraries.count>=maxLibraries){[self backupMessage:@"Limite de 64 Libraries installées atteinte."];return;}
    NSString *identifier=libraryID(doc);for(NSDictionary *lib in libraries)if([libraryID(lib) isEqual:identifier]){[self backupMessage:@"Cette Library est déjà installée."];return;}
    NSString *dir=libraryDirectory();if(!dir){[self backupMessage:@"Dossier de collections indisponible."];return;}
    NSError *err=nil;NSFileManager *fm=NSFileManager.defaultManager;
    if(![fm createDirectoryAtPath:dir withIntermediateDirectories:YES attributes:nil error:&err]){[self backupMessage:err.localizedDescription];return;}
    NSURL *dest=[NSURL fileURLWithPath:[dir stringByAppendingPathComponent:[identifier stringByAppendingPathExtension:@"infinitylibrary"]]];
    if([fm fileExistsAtPath:dest.path]){[self backupMessage:@"Ce fichier de Library existe déjà."];return;}
    NSData *data=[NSJSONSerialization dataWithJSONObject:doc options:NSJSONWritingPrettyPrinted error:&err];
    if(!data||![data writeToURL:dest options:NSDataWritingWithoutOverwriting error:&err]){[self backupMessage:err.localizedDescription?:@"Installation impossible."];return;}
    [libraries addObject:doc];[self.window.contentView setNeedsDisplay:YES];[self backupMessage:@"Library installée. Choisissez-la dans le sélecteur Factory 1."];
}
-(void)exportLibrary:(id)sender{
    (void)sender;if(!userPresets.count||userPresets.count>maxLibraryPresets){[self backupMessage:@"Enregistrez entre 1 et 500 presets personnels avant de créer une Library."];return;}
    NSAlert *a=[NSAlert new];a.messageText=@"Créer une Library de presets";
    a.informativeText=@"Tous les presets personnels seront inclus. La Library utilise les sons de Factory 1, sans nouveaux WAV.";
    NSTextField *field=[[NSTextField alloc]initWithFrame:NSMakeRect(0,0,300,24)];field.placeholderString=@"Nom de la collection";a.accessoryView=field;a.window.initialFirstResponder=field;
    [a addButtonWithTitle:@"Créer"];[a addButtonWithTitle:@"Annuler"];if([a runModal]!=NSAlertFirstButtonReturn)return;
    NSString *name=[field.stringValue stringByTrimmingCharactersInSet:NSCharacterSet.whitespaceAndNewlineCharacterSet];
    NSDictionary *doc=libraryDocument(name,@"Liberty Music Software",userPresets);
    if(!validLibrary(doc)){[self backupMessage:@"Nom ou presets invalides."];return;}
    NSSavePanel *p=[NSSavePanel savePanel];p.allowedFileTypes=@[@"infinitylibrary"];p.nameFieldStringValue=@"Liberty-Presets.infinitylibrary";
    if([p runModal]!=NSModalResponseOK)return;
    NSError *err=nil;NSData *data=[NSJSONSerialization dataWithJSONObject:doc options:NSJSONWritingPrettyPrinted error:&err];
    if(data&&data.length<=maxLibraryBytes&&[data writeToURL:p.URL options:NSDataWritingAtomic error:&err])[self backupMessage:@"Library créée. Le fichier peut être distribué séparément."];
    else [self backupMessage:err.localizedDescription?:@"Export impossible."];
}
-(void)exportBackup:(id)sender{
    (void)sender;NSSavePanel *p=[NSSavePanel savePanel];p.nameFieldStringValue=@"Infinity-Presets.json";p.allowedFileTypes=@[@"json"];
    if([p runModal]!=NSModalResponseOK)return;
    NSError *err=nil;NSData *data=[NSJSONSerialization dataWithJSONObject:backupDocument() options:NSJSONWritingPrettyPrinted error:&err];
    if(data&&[data writeToURL:p.URL options:NSDataWritingAtomic error:&err])[self backupMessage:@"Sauvegarde enregistrée."];
    else [self backupMessage:err.localizedDescription?:@"Sauvegarde impossible."];
}
-(void)restoreBackup:(id)sender{
    (void)sender;NSOpenPanel *p=[NSOpenPanel openPanel];p.allowedFileTypes=@[@"json"];if([p runModal]!=NSModalResponseOK)return;
    NSNumber *size=nil;[p.URL getResourceValue:&size forKey:NSURLFileSizeKey error:nil];
    if(!size||size.unsignedLongLongValue>4*1024*1024){[self backupMessage:@"Fichier trop volumineux ou illisible."];return;}
    NSData *data=[NSData dataWithContentsOfURL:p.URL];id doc=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;
    if(!validPresetBackup(doc)){[self backupMessage:@"Sauvegarde invalide. Aucun réglage modifié."];return;}
    NSAlert *a=[NSAlert new];a.messageText=@"Remplacer les favoris et presets personnels ?";
    a.informativeText=@"Exportez d’abord vos réglages actuels si vous souhaitez les conserver.";
    [a addButtonWithTitle:@"Remplacer"];[a addButtonWithTitle:@"Annuler"];if([a runModal]!=NSAlertFirstButtonReturn)return;
    favorites=[NSMutableSet setWithArray:doc[@"favorites"]];userPresets=[doc[@"presets"] mutableCopy];userName=nil;persist();rebuild();[self.window.contentView setNeedsDisplay:YES];
    [self backupMessage:@"Sauvegarde restaurée."];
}
-(void)openStudio:(id)sender{(void)sender;showInfinityStudio();}
-(void)applicationDidResignActive:(NSNotification*)n{(void)n;for(auto &v:notes)v.store(0);}
-(BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)a{(void)a;return YES;}
-(void)applicationWillTerminate:(NSNotification*)n{(void)n;[self.timer invalidate];[audio stop];if(midiPort)MIDIPortDispose(midiPort);if(midiClient)MIDIClientDispose(midiClient);}
@end
int main(int argc,char** argv){@autoreleasepool{if(argc==2&&std::string(argv[1])=="--qc-backup")return backupSelfTest();if(argc==2&&std::string(argv[1])=="--qc-library")return librarySelfTest();NSApplication *app=NSApplication.sharedApplication;[app setActivationPolicy:NSApplicationActivationPolicyRegular];App *delegate=[App new];app.delegate=delegate;[app run];}return 0;}
