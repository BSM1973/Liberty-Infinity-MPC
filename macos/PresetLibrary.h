#pragma once
// Only parameter libraries based on immutable Factory 1; no audio file or executable payload.
static constexpr NSUInteger maxLibraryPresets=500,maxLibraries=64,maxLibraryBytes=4*1024*1024;
static NSMutableArray<NSDictionary*> *libraries;
static NSInteger collection=-1,selectedLibraryRow=-1;
static bool validLibrary(id doc){
    if(![doc isKindOfClass:NSDictionary.class]||![doc[@"schema"] isEqual:@1]||
       ![doc[@"product"] isEqual:@"Liberty Infinity"]||![doc[@"base"] isEqual:@"Factory 1"])return false;
    id identifier=doc[@"id"],name=doc[@"name"],vendor=doc[@"vendor"],presets=doc[@"presets"];
    if(![identifier isKindOfClass:NSString.class]||![[NSUUID alloc]initWithUUIDString:identifier]||
       ![name isKindOfClass:NSString.class]||![name length]||[name length]>60||
       ![vendor isKindOfClass:NSString.class]||![vendor length]||[vendor length]>80||
       ![presets isKindOfClass:NSDictionary.class]||![presets count]||[presets count]>maxLibraryPresets)return false;
    if([name caseInsensitiveCompare:@"Factory 1"]==NSOrderedSame)return false;
    return validPresetBackup(@{@"schema":@1,@"product":@"Liberty Infinity",@"favorites":@[],@"presets":presets});
}
static NSString *libraryDirectory(){
    NSURL *url=[NSFileManager.defaultManager URLsForDirectory:NSApplicationSupportDirectory inDomains:NSUserDomainMask].firstObject;
    return [[url.path stringByAppendingPathComponent:@"Liberty Music Software/Liberty Infinity"] stringByAppendingPathComponent:@"Libraries"];
}
static NSDictionary *readLibrary(NSURL *url){
    NSNumber *size=nil;[url getResourceValue:&size forKey:NSURLFileSizeKey error:nil];
    if(!size||size.unsignedLongLongValue>maxLibraryBytes)return nil;
    NSData *data=[NSData dataWithContentsOfURL:url];if(!data||data.length>maxLibraryBytes)return nil;
    id doc=[NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
    return validLibrary(doc)?doc:nil;
}
static NSString *libraryID(NSDictionary *doc){return [[[NSUUID alloc]initWithUUIDString:doc[@"id"]] UUIDString];}
static NSDictionary *libraryDocument(NSString *name,NSString *vendor,NSDictionary *presets){
    return @{@"schema":@1,@"product":@"Liberty Infinity",@"base":@"Factory 1",@"id":NSUUID.UUID.UUIDString,@"name":name,@"vendor":vendor,@"presets":presets};
}
static NSDictionary *activeLibrary(){return collection>=0&&collection<(NSInteger)libraries.count?libraries[collection]:nil;}
static NSArray<NSString*> *libraryNames(){return [activeLibrary()[@"presets"] allKeys]?[ [activeLibrary()[@"presets"] allKeys] sortedArrayUsingSelector:@selector(localizedCaseInsensitiveCompare:)]:@[];}
static NSDictionary *libraryPreset(int row){
    NSArray *names=libraryNames();return row>=0&&row<(int)names.count?activeLibrary()[@"presets"][names[row]]:nil;
}
static NSString *libraryPresetName(int row){NSArray *names=libraryNames();return row>=0&&row<(int)names.count?names[row]:@"";}
static int factoryIndex(NSDictionary *record){for(int i=0;i<factoryCount;++i)if([record[@"factory"] isEqual:patchName(i)])return i;return -1;}
static NSString *collectionName(){return activeLibrary()?activeLibrary()[@"name"]:@"Factory 1";}
static void loadLibraries(){
    libraries=[NSMutableArray new];NSMutableSet *ids=[NSMutableSet new];
    NSString *dir=libraryDirectory();if(!dir)return;
    NSArray *files=[[NSFileManager.defaultManager contentsOfDirectoryAtPath:dir error:nil] sortedArrayUsingSelector:@selector(compare:)];
    for(NSString *name in files){
        if(libraries.count>=maxLibraries)break;if(![name.pathExtension.lowercaseString isEqual:@"infinitylibrary"])continue;
        NSDictionary *doc=readLibrary([NSURL fileURLWithPath:[dir stringByAppendingPathComponent:name]]);
        if(doc&&![ids containsObject:libraryID(doc)]){[libraries addObject:doc];[ids addObject:libraryID(doc)];}
    }
}
static int librarySelfTest(){
    if(backupSelfTest())return 30;
    NSString *res=NSBundle.mainBundle.resourcePath;setenv("MPC_PLUGIN_FOLDER",res.fileSystemRepresentation,1);
    synth=std::make_unique<Infinity>();if(!synth->usingPackedBank())return 31;
    NSDictionary *doc=libraryDocument(@"Collection Épreuve",@"Liberty Music Software",userPresets);
    NSData *bytes=[NSJSONSerialization dataWithJSONObject:doc options:0 error:nil];
    id result=bytes?[NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil]:nil;
    if(!validLibrary(result)||![result isEqual:doc])return 32;
    libraries=[NSMutableArray arrayWithObject:result];collection=0;
    if(libraryNames().count!=1||factoryIndex(libraryPreset(0))!=3)return 33;
    auto bad=[doc mutableCopy];bad[@"base"]=@"Factory 2";if(validLibrary(bad))return 34;
    bad=[doc mutableCopy];bad[@"name"]=@"Factory 1";if(validLibrary(bad))return 35;
    bad=[doc mutableCopy];bad[@"id"]=@"../bad";if(validLibrary(bad))return 36;
    collection=-1;synth.reset();std::puts("PASS Factory 1 library schema, Unicode round-trip and invalid collection rejection");return 0;
}
