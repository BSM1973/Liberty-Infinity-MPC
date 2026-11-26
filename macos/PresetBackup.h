#pragma once
// Included after synth, patchName and persist declarations.
static bool knownFactory(id name){
    if(![name isKindOfClass:NSString.class])return false;
    for(int i=0;i<factoryCount;++i)if([name isEqual:patchName(i)])return true;
    return false;
}
static bool validPresetBackup(id doc){
    if(![doc isKindOfClass:NSDictionary.class]||![doc[@"schema"] isEqual:@1]||![doc[@"product"] isEqual:@"Liberty Infinity"])return false;
    id fav=doc[@"favorites"], presets=doc[@"presets"];
    if(![fav isKindOfClass:NSArray.class]||[fav count]>factoryCount||![presets isKindOfClass:NSDictionary.class]||[presets count]>2000)return false;
    for(id name in fav)if(!knownFactory(name))return false;
    for(id name in presets){
        if(![name isKindOfClass:NSString.class]||![name length]||[name length]>80)return false;
        id rec=presets[name];
        if(![rec isKindOfClass:NSDictionary.class]||![rec[@"schema"] isEqual:@1]||!knownFactory(rec[@"factory"]))return false;
        id values=rec[@"values"];
        if(![values isKindOfClass:NSDictionary.class]||[values count]!=NSUInteger(soundParameterCount))return false;
        for(int i=0;i<soundParameterCount;++i){
            id v=values[[NSString stringWithUTF8String:synth->param(i).id.c_str()]];
            if(![v isKindOfClass:NSNumber.class]||!std::isfinite([v doubleValue])||[v doubleValue]<0||[v doubleValue]>1)return false;
        }
    }
    return true;
}
static NSDictionary* backupDocument(){
    return @{@"schema":@1,@"product":@"Liberty Infinity",@"favorites":favorites.allObjects,@"presets":userPresets};
}
static int backupSelfTest(){
    NSString *res=NSBundle.mainBundle.resourcePath;
    if(!res)return 20;setenv("MPC_PLUGIN_FOLDER",res.fileSystemRepresentation,1);
    synth=std::make_unique<Infinity>();if(!synth->usingPackedBank())return 21;
    synth->setProgram(3);synth->setNormalised(1,.25f);
    NSMutableDictionary *values=[NSMutableDictionary new];
    for(int i=0;i<soundParameterCount;++i)values[[NSString stringWithUTF8String:synth->param(i).id.c_str()]]=@(synth->getNormalised(i));
    favorites=[NSMutableSet setWithObject:patchName(3)];userPresets=[@{@"Épreuve de sauvegarde":@{@"schema":@1,@"factory":patchName(3),@"values":values}} mutableCopy];
    NSData *bytes=[NSJSONSerialization dataWithJSONObject:backupDocument() options:0 error:nil];
    id restored=bytes?[NSJSONSerialization JSONObjectWithData:bytes options:0 error:nil]:nil;
    if(!validPresetBackup(restored)||![restored[@"presets"] isEqual:userPresets]||![restored[@"favorites"] containsObject:patchName(3)])return 22;
    synth->setProgram(0);
    NSDictionary *v=restored[@"presets"][@"Épreuve de sauvegarde"][@"values"];
    synth->setProgram(3);
    for(int i=1;i<soundParameterCount;++i)synth->setNormalised(i,[v[[NSString stringWithUTF8String:synth->param(i).id.c_str()]] floatValue]);
    if(std::abs(synth->getNormalised(1)-.25f)>1e-5f)return 23;
    if(validPresetBackup(@{@"schema":@1,@"product":@"Liberty Infinity",@"favorites":@[@"inconnu"],@"presets":@{}}))return 24;
    if(validPresetBackup(@{@"schema":@1,@"product":@"Liberty Infinity",@"favorites":@[],@"presets":@{@"invalide":@{@"schema":@1,@"factory":patchName(3),@"values":@{}}}}))return 25;
    synth.reset();std::puts("PASS macOS preset/favorite JSON round-trip, UTF-8 and invalid backup rejection");return 0;
}
