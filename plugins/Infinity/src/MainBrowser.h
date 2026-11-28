#pragma once
// Generated from Factory.h for the native inline browser; fixed Factory indices.
struct MainBrowserPage { int category; const char* label; int rows[8]; };
static constexpr MainBrowserPage mainBrowserPages[]={
    {0,"Toutes  1/13",{0,1,2,3,4,5,6,7}},
    {0,"Toutes  2/13",{8,9,10,11,12,13,14,15}},
    {0,"Toutes  3/13",{16,17,18,19,20,21,22,23}},
    {0,"Toutes  4/13",{24,25,26,27,28,29,30,31}},
    {0,"Toutes  5/13",{32,33,34,35,36,37,38,39}},
    {0,"Toutes  6/13",{40,41,42,43,44,45,46,47}},
    {0,"Toutes  7/13",{48,49,50,51,52,53,54,55}},
    {0,"Toutes  8/13",{56,57,58,59,60,61,62,63}},
    {0,"Toutes  9/13",{64,65,66,67,68,69,70,71}},
    {0,"Toutes  10/13",{72,73,74,75,76,77,78,79}},
    {0,"Toutes  11/13",{80,81,82,83,84,85,86,87}},
    {0,"Toutes  12/13",{88,89,90,91,92,93,94,95}},
    {0,"Toutes  13/13",{96,97,98,99,-1,-1,-1,-1}},
    {1,"Bass  1/2",{0,4,5,6,7,8,9,10}},
    {1,"Bass  2/2",{11,12,13,14,15,16,17,-1}},
    {2,"Keys  1/1",{1,18,19,20,21,22,23,24}},
    {3,"Pads  1/2",{2,25,26,27,28,29,30,31}},
    {3,"Pads  2/2",{32,33,34,35,36,37,38,-1}},
    {4,"Plucks  1/2",{3,39,40,41,42,43,44,45}},
    {4,"Plucks  2/2",{46,47,48,49,-1,-1,-1,-1}},
    {5,"Leads  1/2",{50,51,52,53,54,55,56,57}},
    {5,"Leads  2/2",{58,59,60,61,-1,-1,-1,-1}},
    {6,"Pianos  1/1",{62,63,64,65,66,67,68,69}},
    {7,"Strings  1/1",{70,71,72,73,74,-1,-1,-1}},
    {8,"Brass  1/1",{75,76,77,78,79,-1,-1,-1}},
    {9,"Arpeggios  1/1",{80,81,82,83,84,-1,-1,-1}},
    {10,"Bells & Mallets  1/1",{85,86,87,88,89,-1,-1,-1}},
    {11,"Atmospheres  1/1",{90,91,92,93,94,-1,-1,-1}},
    {12,"Synth & FX  1/1",{95,96,97,98,99,-1,-1,-1}}
};
static constexpr int mainBrowserPageCount=sizeof(mainBrowserPages)/sizeof(mainBrowserPages[0]);
inline int mainBrowserPatch(int page,int row){
    return page>=0&&page<mainBrowserPageCount&&row>=1&&row<=8?mainBrowserPages[page].rows[row-1]:-1;
}
