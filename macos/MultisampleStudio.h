#pragma once
#include "NativeBankExport.h"
// Included by Main.mm after the shared colours. Offline editor, never called from render().
@interface InfinityWave : NSView
@property(nonatomic) AVAudioFramePosition total;
@property(nonatomic) AVAudioFramePosition first;
@property(nonatomic) AVAudioFramePosition last;
@property(nonatomic) BOOL loopEnabled;
@property(nonatomic,copy) void (^changed)(AVAudioFramePosition,AVAudioFramePosition);
-(BOOL)loadURL:(NSURL*)url error:(NSError**)error;
@end
@implementation InfinityWave {
    std::array<float,1024> lo,hi;
    AVAudioFramePosition anchor;
}
-(BOOL)isFlipped{return YES;}
-(BOOL)loadURL:(NSURL*)url error:(NSError**)error{
    self.total=0;lo.fill(0);hi.fill(0);[self setNeedsDisplay:YES];
    AVAudioFile *file=[[AVAudioFile alloc]initForReading:url commonFormat:AVAudioPCMFormatFloat32 interleaved:NO error:error];
    if(!file||file.length<64||file.processingFormat.channelCount<1||file.processingFormat.channelCount>2)return NO;
    // Keep the drawing bounded and avoid allocating the whole WAV.
    if(file.length>file.processingFormat.sampleRate*120)return NO;
    AVAudioPCMBuffer *buf=[[AVAudioPCMBuffer alloc]initWithPCMFormat:file.processingFormat frameCapacity:8192];
    if(!buf)return NO;
    AVAudioFramePosition offset=0;
    while(offset<file.length){
        if(![file readIntoBuffer:buf error:error]||!buf.frameLength)return NO;
        for(AVAudioFrameCount j=0;j<buf.frameLength;++j){
            size_t k=std::min(size_t(1023),size_t((offset+j)*1024/file.length));
            for(AVAudioChannelCount c=0;c<file.processingFormat.channelCount;++c){
                float x=buf.floatChannelData[c][j];if(!std::isfinite(x))return NO;
                lo[k]=std::min(lo[k],x);hi[k]=std::max(hi[k],x);
            }
        }
        offset+=buf.frameLength;
    }
    self.total=file.length;self.first=0;self.last=file.length;[self setNeedsDisplay:YES];return YES;
}
-(void)drawRect:(NSRect)dirty{
    (void)dirty;[[NSColor colorWithWhite:.075 alpha:1]setFill];NSRectFill(self.bounds);
    if(!self.total)return;
    CGFloat w=NSWidth(self.bounds),h=NSHeight(self.bounds);
    if(self.loopEnabled){[selection() setFill];NSRectFill(NSMakeRect(w*self.first/self.total,0,w*(self.last-self.first)/self.total,h));}
    NSBezierPath *p=[NSBezierPath bezierPath];[gold() setStroke];p.lineWidth=.75;
    for(int i=0;i<1024;++i){CGFloat x=(i+.5)*w/1024;
        [p moveToPoint:NSMakePoint(x,h*.5-std::clamp(hi[i],-1.f,1.f)*h*.46)];
        [p lineToPoint:NSMakePoint(x,h*.5-std::clamp(lo[i],-1.f,1.f)*h*.46)];}
    [p stroke];
}
-(AVAudioFramePosition)frameAt:(NSEvent*)e{
    double x=[self convertPoint:e.locationInWindow fromView:nil].x/std::max(1.,double(NSWidth(self.bounds)));
    return AVAudioFramePosition(std::clamp(x,0.,1.)*self.total);
}
-(void)mouseDown:(NSEvent*)e{if(self.total)anchor=std::min(self.total-1,[self frameAt:e]);}
-(void)mouseDragged:(NSEvent*)e{
    if(!self.total)return;auto f=[self frameAt:e];self.first=std::min(anchor,f);self.last=std::max(anchor,f);
    if(self.last<=self.first)self.last=self.first+1;
    self.loopEnabled=YES;[self setNeedsDisplay:YES];if(self.changed)self.changed(self.first,self.last);
}
@end

@interface InfinityKeys:NSView
@property(nonatomic) NSInteger low;
@property(nonatomic) NSInteger high;
@property(nonatomic) NSInteger root;
@end
@implementation InfinityKeys
-(BOOL)isFlipped{return YES;}
-(void)drawRect:(NSRect)dirty{
    (void)dirty;CGFloat w=NSWidth(self.bounds)/128.,h=NSHeight(self.bounds);
    for(int n=0;n<128;++n){int pc=n%12;bool black=pc==1||pc==3||pc==6||pc==8||pc==10;
        NSColor *c=n==self.root?gold():n>=self.low&&n<=self.high?selection():[NSColor colorWithWhite:black?.15:.65 alpha:1];
        [c setFill];NSRectFill(NSMakeRect(n*w,0,std::max(.5,double(w-.6)),h));
    }
}
@end

@interface InfinityStudio:NSView
-(void)openWavs:(id)sender;
-(NSMutableDictionary*)studioLayerRecord;
-(NSMutableDictionary*)studioZoneRecord;
-(NSMutableArray*)studioZoneRecords;
@end
@implementation InfinityStudio {
    NSPopUpButton *layerMenu,*zoneMenu;
    NSTextField *patch,*source,*gainField,*panField,*tune,*root,*keyLo,*keyHi,*velLo,*velHi,*loopStart,*loopEnd,*status;
    NSButton *loop,*normalize,*mute,*solo;
    InfinityWave *wave;
    InfinityKeys *keys;
    NSMutableArray<NSMutableDictionary*> *layers;
    AVAudioPlayer *preview;
    BOOL exporting;
    NSInteger selectedLayer,selectedZone;
}
-(BOOL)isFlipped{return YES;}
-(NSTextField*)label:(NSString*)title x:(CGFloat)x y:(CGFloat)y w:(CGFloat)w{
    NSTextField *v=[NSTextField labelWithString:title];v.frame=NSMakeRect(x,y,w,20);v.textColor=gold();v.font=[NSFont systemFontOfSize:12];[self addSubview:v];return v;
}
-(NSTextField*)field:(NSString*)title x:(CGFloat)x y:(CGFloat)y w:(CGFloat)w{
    [self label:title x:x y:y w:w];NSTextField *v=[[NSTextField alloc]initWithFrame:NSMakeRect(x,y+20,w,24)];
    v.target=self;v.action=@selector(commitFields:);[self addSubview:v];return v;
}
-(NSButton*)button:(NSString*)title x:(CGFloat)x y:(CGFloat)y w:(CGFloat)w action:(SEL)action{
    NSButton *b=[NSButton buttonWithTitle:title target:self action:action];b.frame=NSMakeRect(x,y,w,30);[self addSubview:b];return b;
}
-(NSButton*)check:(NSString*)title x:(CGFloat)x y:(CGFloat)y w:(CGFloat)w{
    NSButton *b=[NSButton checkboxWithTitle:title target:self action:@selector(commitFields:)];b.frame=NSMakeRect(x,y,w,24);[self addSubview:b];return b;
}
-(instancetype)initWithFrame:(NSRect)frame{
    if(!(self=[super initWithFrame:frame]))return nil;
    self.appearance=[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];self.wantsLayer=YES;self.layer.backgroundColor=[NSColor colorWithWhite:.10 alpha:1].CGColor;
    layers=[NSMutableArray new];for(int i=0;i<4;++i)[layers addObject:[@{@"source":@(i),@"gain_db":@(-6),@"pan":@0,@"cents":@0,@"normalize":@YES,@"mute":@NO,@"solo":@NO,@"zones":[NSMutableArray new]} mutableCopy]];
    selectedLayer=0;selectedZone=-1;
    NSTextField *title=[self label:@"LIBERTY INFINITY  |  MULTISAMPLE STUDIO" x:16 y:12 w:800];title.font=[NSFont boldSystemFontOfSize:18];
    patch=[self field:@"Preset (0–99)" x:16 y:40 w:112];patch.integerValue=0;
    layerMenu=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(140,60,118,26) pullsDown:NO];
    [layerMenu addItemsWithTitles:@[@"Couche 1",@"Couche 2",@"Couche 3",@"Couche 4"]];layerMenu.target=self;layerMenu.action=@selector(changeLayer:);[self addSubview:layerMenu];
    source=[self field:@"Source (0–99)" x:270 y:40 w:112];gainField=[self field:@"Gain (dB)" x:394 y:40 w:100];
    panField=[self field:@"Pan (−1 à 1)" x:506 y:40 w:100];tune=[self field:@"Accord (cents)" x:618 y:40 w:104];
    mute=[self check:@"Mute" x:736 y:49 w:70];solo=[self check:@"Solo" x:736 y:73 w:70];normalize=[self check:@"Normaliser" x:808 y:60 w:108];
    [self button:@"Ajouter WAV" x:16 y:100 w:128 action:@selector(openWavs:)];
    zoneMenu=[[NSPopUpButton alloc]initWithFrame:NSMakeRect(150,102,424,28) pullsDown:NO];zoneMenu.target=self;zoneMenu.action=@selector(changeZone:);[self addSubview:zoneMenu];
    [self button:@"Retirer" x:580 y:100 w:96 action:@selector(removeZone:)];
    [self button:@"Écouter" x:682 y:100 w:102 action:@selector(play:)];
    [self button:@"Stop" x:790 y:100 w:102 action:@selector(stop:)];
    wave=[[InfinityWave alloc]initWithFrame:NSMakeRect(16,140,876,166)];[self addSubview:wave];
    [self label:@"Glisser dans la forme d’onde pour définir la boucle (frames WAV)" x:16 y:309 w:850];
    loop=[self check:@"Boucle" x:16 y:337 w:90];loopStart=[self field:@"Début" x:110 y:330 w:128];loopEnd=[self field:@"Fin exclusive" x:248 y:330 w:128];
    root=[self field:@"Note MIDI (−1 = auto)" x:386 y:330 w:145];keyLo=[self field:@"Note min." x:541 y:330 w:78];keyHi=[self field:@"Note max." x:629 y:330 w:78];
    velLo=[self field:@"Vél. min." x:717 y:330 w:78];velHi=[self field:@"Vél. max." x:805 y:330 w:87];
    keys=[[InfinityKeys alloc]initWithFrame:NSMakeRect(16,385,876,35)];[self addSubview:keys];
    [self label:@"Mapping : notes MIDI 0–127 | export sur 5 notes × 4 vélocités" x:16 y:424 w:870];
    [self button:@"Enregistrer JSON" x:16 y:452 w:165 action:@selector(saveManifest:)];
    [self button:@"Ouvrir JSON" x:188 y:452 w:140 action:@selector(openManifest:)];
    [self button:@"Exporter banque MPC" x:335 y:452 w:200 action:@selector(exportBank:)];
    status=[self label:@"Déposez des fichiers WAV sur cette fenêtre." x:16 y:492 w:876];status.maximumNumberOfLines=3;status.frame=NSMakeRect(16,492,876,58);
    __weak InfinityStudio *weakSelf=self;wave.changed=^(AVAudioFramePosition a,AVAudioFramePosition b){[weakSelf setLoopA:a b:b];};
    [self registerForDraggedTypes:@[NSPasteboardTypeFileURL]];[self showLayer];return self;
}
-(NSMutableDictionary*)studioLayerRecord{return layers[selectedLayer];}
-(NSMutableArray*)studioZoneRecords{return [self studioLayerRecord][@"zones"];}
-(NSMutableDictionary*)studioZoneRecord{NSMutableArray *z=[self studioZoneRecords];return selectedZone>=0&&selectedZone<(NSInteger)z.count?z[selectedZone]:nil;}
-(void)message:(NSString*)s{status.stringValue=s?:@"";}
-(void)setLoopA:(AVAudioFramePosition)a b:(AVAudioFramePosition)b{if(![self studioZoneRecord])return;loop.state=NSControlStateValueOn;loopStart.integerValue=a;loopEnd.integerValue=b;[self commitFields:nil];}
-(void)commitFields:(id)sender{
    (void)sender;NSMutableDictionary *l=[self studioLayerRecord];l[@"source"]=@(std::clamp(source.integerValue,NSInteger(0),NSInteger(99)));
    l[@"gain_db"]=@(std::clamp(gainField.doubleValue,-60.,6.));l[@"pan"]=@(std::clamp(panField.doubleValue,-1.,1.));l[@"cents"]=@(std::clamp(tune.doubleValue,-1200.,1200.));
    l[@"normalize"]=@(normalize.state==NSControlStateValueOn);l[@"mute"]=@(mute.state==NSControlStateValueOn);l[@"solo"]=@(solo.state==NSControlStateValueOn);
    source.integerValue=[l[@"source"] integerValue];gainField.doubleValue=[l[@"gain_db"] doubleValue];panField.doubleValue=[l[@"pan"] doubleValue];tune.doubleValue=[l[@"cents"] doubleValue];
    patch.integerValue=std::clamp(patch.integerValue,NSInteger(0),NSInteger(99));
    NSMutableDictionary *z=[self studioZoneRecord];if(!z)return;
    NSInteger k=std::clamp(keyLo.integerValue,NSInteger(0),NSInteger(127)),v=std::clamp(velLo.integerValue,NSInteger(1),NSInteger(127));
    keyLo.integerValue=k;keyHi.integerValue=std::clamp(keyHi.integerValue,k,NSInteger(127));velLo.integerValue=v;velHi.integerValue=std::clamp(velHi.integerValue,v,NSInteger(127));
    root.integerValue=std::clamp(root.integerValue,NSInteger(-1),NSInteger(127));
    z[@"key_min"]=@(keyLo.integerValue);z[@"key_max"]=@(keyHi.integerValue);z[@"vel_min"]=@(velLo.integerValue);z[@"vel_max"]=@(velHi.integerValue);
    if(root.integerValue>=0)z[@"root"]=@(root.integerValue);else [z removeObjectForKey:@"root"];
    if(wave.total>0&&loop.state==NSControlStateValueOn){
        auto a=std::clamp(AVAudioFramePosition(loopStart.integerValue),AVAudioFramePosition(0),wave.total-1);
        auto b=std::clamp(AVAudioFramePosition(loopEnd.integerValue),a+1,wave.total);
        loopStart.integerValue=a;loopEnd.integerValue=b;z[@"loop"]=@[@(a),@(b)];wave.first=a;wave.last=b;
    }else if(loop.state!=NSControlStateValueOn)[z removeObjectForKey:@"loop"];
    wave.loopEnabled=loop.state==NSControlStateValueOn;[wave setNeedsDisplay:YES];keys.low=keyLo.integerValue;keys.high=keyHi.integerValue;keys.root=root.integerValue;[keys setNeedsDisplay:YES];
}
-(void)showLayer{
    NSDictionary *l=[self studioLayerRecord];source.integerValue=[l[@"source"] integerValue];gainField.doubleValue=[l[@"gain_db"] doubleValue];panField.doubleValue=[l[@"pan"] doubleValue];tune.doubleValue=[l[@"cents"] doubleValue];
    normalize.state=[l[@"normalize"] boolValue]?NSControlStateValueOn:NSControlStateValueOff;mute.state=[l[@"mute"] boolValue]?NSControlStateValueOn:NSControlStateValueOff;solo.state=[l[@"solo"] boolValue]?NSControlStateValueOn:NSControlStateValueOff;
    [zoneMenu removeAllItems];for(NSDictionary *z in [self studioZoneRecords])[zoneMenu addItemWithTitle:[NSString stringWithFormat:@"%ld — %@",(long)zoneMenu.numberOfItems+1,[z[@"wav"] lastPathComponent]]];
    selectedZone=[self studioZoneRecords].count?0:-1;[self showZone];
}
-(void)showZone{
    [preview stop];preview=nil;NSDictionary *z=[self studioZoneRecord];BOOL exists=z!=nil;
    for(NSControl *c in @[root,keyLo,keyHi,velLo,velHi,loop,loopStart,loopEnd])c.enabled=exists;
    if(!exists){wave.total=0;[wave setNeedsDisplay:YES];keys.low=1;keys.high=0;keys.root=-1;[keys setNeedsDisplay:YES];return;}
    [zoneMenu selectItemAtIndex:selectedZone];NSError *err=nil;
    BOOL ok=[wave loadURL:[NSURL fileURLWithPath:z[@"wav"]] error:&err];
    root.integerValue=z[@"root"]?[z[@"root"] integerValue]:-1;keyLo.integerValue=[z[@"key_min"] integerValue];keyHi.integerValue=[z[@"key_max"] integerValue];
    velLo.integerValue=[z[@"vel_min"] integerValue];velHi.integerValue=[z[@"vel_max"] integerValue];
    NSArray *bounds=z[@"loop"];loop.state=bounds?NSControlStateValueOn:NSControlStateValueOff;loopStart.integerValue=bounds?[bounds[0] integerValue]:0;loopEnd.integerValue=bounds?[bounds[1] integerValue]:wave.total;
    wave.first=std::clamp(AVAudioFramePosition(loopStart.integerValue),AVAudioFramePosition(0),wave.total);wave.last=std::clamp(AVAudioFramePosition(loopEnd.integerValue),wave.first,wave.total);wave.loopEnabled=bounds!=nil;[wave setNeedsDisplay:YES];
    keys.low=keyLo.integerValue;keys.high=keyHi.integerValue;keys.root=root.integerValue;[keys setNeedsDisplay:YES];
    [self message:ok?[NSString stringWithFormat:@"%@ | %lld frames",[z[@"wav"] lastPathComponent],(long long)wave.total]:@"WAV indisponible ou non pris en charge (maximum 120 s pour l’aperçu)."];
}
-(void)changeLayer:(id)sender{(void)sender;[self commitFields:nil];selectedLayer=layerMenu.indexOfSelectedItem;[self showLayer];}
-(void)changeZone:(id)sender{(void)sender;[self commitFields:nil];selectedZone=zoneMenu.indexOfSelectedItem;[self showZone];}
-(void)addURLs:(NSArray<NSURL*>*)urls{
    [self commitFields:nil];NSUInteger added=0;
    for(NSURL *url in urls){
        if(!url.isFileURL||![url.pathExtension.lowercaseString isEqual:@"wav"]||[self studioZoneRecords].count>=128)continue;
        AVAudioFile *f=[[AVAudioFile alloc]initForReading:url error:nil];if(!f||f.length<64||f.processingFormat.channelCount>2||f.length>120*f.processingFormat.sampleRate)continue;
        [[self studioZoneRecords] addObject:[@{@"wav":url.path,@"key_min":@0,@"key_max":@127,@"vel_min":@1,@"vel_max":@127} mutableCopy]];++added;
    }
    [self showLayer];if(added){selectedZone=[self studioZoneRecords].count-1;[self showZone];}else [self message:@"Aucun WAV valide ajouté."];
}
-(void)openWavs:(id)sender{(void)sender;NSOpenPanel *p=[NSOpenPanel openPanel];p.allowsMultipleSelection=YES;p.allowedFileTypes=@[@"wav"];if([p runModal]==NSModalResponseOK)[self addURLs:p.URLs];}
-(NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender{(void)sender;return NSDragOperationCopy;}
-(BOOL)performDragOperation:(id<NSDraggingInfo>)sender{
    NSArray *urls=[sender.draggingPasteboard readObjectsForClasses:@[NSURL.class] options:@{NSPasteboardURLReadingFileURLsOnlyKey:@YES}];if(!urls.count)return NO;[self addURLs:urls];return YES;
}
-(void)removeZone:(id)sender{(void)sender;if([self studioZoneRecord]){[[self studioZoneRecords] removeObjectAtIndex:selectedZone];[self showLayer];}}
-(void)play:(id)sender{(void)sender;[self stop:nil];NSDictionary *z=[self studioZoneRecord];if(!z)return;NSError *err=nil;preview=[[AVAudioPlayer alloc]initWithContentsOfURL:[NSURL fileURLWithPath:z[@"wav"]] error:&err];if(![preview play])[self message:err.localizedDescription?:@"Préécoute impossible."];}
-(void)stop:(id)sender{(void)sender;[preview stop];preview=nil;}
-(void)viewWillMoveToWindow:(NSWindow*)newWindow{if(!newWindow)[self stop:nil];[super viewWillMoveToWindow:newWindow];}
-(NSDictionary*)manifest{
    [self commitFields:nil];NSMutableArray *sources=[NSMutableArray new],*active=[NSMutableArray new];NSMutableSet *used=[NSMutableSet new];BOOL anySolo=NO;
    for(NSDictionary *l in layers)if([l[@"zones"] count]&&[l[@"solo"] boolValue]&&![l[@"mute"] boolValue])anySolo=YES;
    for(NSDictionary *l in layers){
        if(![l[@"zones"] count])continue;NSNumber *q=l[@"source"];if([used containsObject:q]){[self message:@"Deux couches utilisent le même emplacement source."];return nil;}[used addObject:q];
        [sources addObject:@{@"source":q,@"normalize":l[@"normalize"],@"zones":l[@"zones"]}];
        if(![l[@"mute"] boolValue]&&(!anySolo||[l[@"solo"] boolValue]))[active addObject:@{@"source":q,@"gain_db":l[@"gain_db"],@"pan":l[@"pan"],@"cents":l[@"cents"]}];
    }
    if(!active.count){[self message:@"Au moins une couche non muette avec des WAV est nécessaire."];return nil;}
    return @{@"sources":sources,@"patches":@[@{@"patch":@(patch.integerValue),@"layers":active}]};
}
-(void)saveManifest:(id)sender{
    (void)sender;[self commitFields:nil];NSSavePanel *p=[NSSavePanel savePanel];p.nameFieldStringValue=@"Infinity-Studio.json";p.allowedFileTypes=@[@"json"];
    if([p runModal]!=NSModalResponseOK)return;
    NSDictionary *doc=@{@"studio":@1,@"patch":@(patch.integerValue),@"layers":layers};NSError *err=nil;
    NSData *data=[NSJSONSerialization dataWithJSONObject:doc options:NSJSONWritingPrettyPrinted error:&err];
    if(data&&[data writeToURL:p.URL options:NSDataWritingAtomic error:&err])[self message:@"Projet enregistré (références WAV absolues)."];else [self message:err.localizedDescription];
}
-(void)openManifest:(id)sender{
    (void)sender;NSOpenPanel *p=[NSOpenPanel openPanel];p.allowedFileTypes=@[@"json"];if([p runModal]!=NSModalResponseOK)return;
    NSData *data=[NSData dataWithContentsOfURL:p.URL];id doc=data?[NSJSONSerialization JSONObjectWithData:data options:NSJSONReadingMutableContainers error:nil]:nil;
    if(![doc isKindOfClass:NSDictionary.class]||![doc[@"studio"] isEqual:@1]||![doc[@"patch"] isKindOfClass:NSNumber.class]||![doc[@"layers"] isKindOfClass:NSMutableArray.class]||[doc[@"layers"] count]!=4){[self message:@"Projet Studio invalide."];return;}
    for(id l in doc[@"layers"]){
        if(![l isKindOfClass:NSMutableDictionary.class]||![l[@"zones"] isKindOfClass:NSMutableArray.class]||[l[@"zones"] count]>128){[self message:@"Couche invalide."];return;}
        for(NSString *k in @[@"source",@"gain_db",@"pan",@"cents",@"normalize",@"mute",@"solo"])if(![l[k] isKindOfClass:NSNumber.class]){[self message:@"Réglages invalides."];return;}
        for(id z in l[@"zones"]){
            if(![z isKindOfClass:NSMutableDictionary.class]||![z[@"wav"] isKindOfClass:NSString.class]){[self message:@"Zone invalide."];return;}
            for(NSString *k in @[@"key_min",@"key_max",@"vel_min",@"vel_max"])if(![z[k] isKindOfClass:NSNumber.class]){[self message:@"Mapping invalide."];return;}
            if(z[@"root"]&&![z[@"root"] isKindOfClass:NSNumber.class]){[self message:@"Note invalide."];return;}
            id b=z[@"loop"];if(b&&(![b isKindOfClass:NSArray.class]||[b count]!=2||![b[0] isKindOfClass:NSNumber.class]||![b[1] isKindOfClass:NSNumber.class])){[self message:@"Boucle invalide."];return;}
        }
    }
    layers=doc[@"layers"];patch.integerValue=[doc[@"patch"] integerValue];selectedLayer=0;[layerMenu selectItemAtIndex:0];[self showLayer];
}
-(void)exportBank:(id)sender{
    (void)sender;if(exporting){[self message:@"Export déjà en cours."];return;}
    NSDictionary *manifest=[self manifest];if(!manifest)return;
    NSData *snapshot=[NSJSONSerialization dataWithJSONObject:manifest options:0 error:nil];
    if(!snapshot){[self message:@"Projet invalide."];return;}
    NSDictionary *immutable=[NSJSONSerialization JSONObjectWithData:snapshot options:0 error:nil];
    NSOpenPanel *dest=[NSOpenPanel openPanel];dest.canChooseFiles=NO;dest.canChooseDirectories=YES;dest.canCreateDirectories=YES;
    dest.message=@"Dossier de destination vide pour Factory.pcm et Factory.pcm.map";
    if([dest runModal]!=NSModalResponseOK)return;
    NSString *folder=dest.URL.path,*base=[NSBundle.mainBundle.resourcePath stringByAppendingPathComponent:@"Content/Factory.pcm"];
    exporting=YES;[self message:@"Export stéréo HD en cours…"];
    __weak InfinityStudio *weakSelf=self;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED,0),^{@autoreleasepool{
        NSString *failure=studioHD::exportBank(immutable,base,folder);
        dispatch_async(dispatch_get_main_queue(),^{InfinityStudio *s=weakSelf;if(!s)return;s->exporting=NO;
            if(!failure)[s message:@"Export HD réussi. Arrêtez le MPC avant installation des deux fichiers."];
            else { [s message:@"Export échoué. Banque Factory intacte."];NSAlert *a=[NSAlert new];a.messageText=@"Export impossible";a.informativeText=failure;[a runModal];}
        });
    }});
}
@end

static NSWindow *infinityStudioWindow;
static void showInfinityStudio(){
    if(!infinityStudioWindow){
        infinityStudioWindow=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,908,558) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable backing:NSBackingStoreBuffered defer:NO];
        infinityStudioWindow.releasedWhenClosed=NO;infinityStudioWindow.title=@"Liberty Infinity — Multisample Studio";
        infinityStudioWindow.contentView=[[InfinityStudio alloc]initWithFrame:NSMakeRect(0,0,908,558)];[infinityStudioWindow center];
    }
    [infinityStudioWindow makeKeyAndOrderFront:nil];
}
