
#pragma once

header test_dstAddr_t {
    macAddr_t dstAddr;
}

header test_srcAddr_t {
    macAddr_t srcAddr;
}

header test_etherType_t {
    bit<16> etherType;
}

struct padded_bool_t { bit<(8-1)>   pad1;  bool    b;   }

struct padded1_t     { bit<(8-1)>   pad1;  bit<1>  f1;  }
struct padded2_t     { bit<(8-2)>   pad2;  bit<2>  f2;  }
struct padded3_t     { bit<(8-3)>   pad3;  bit<3>  f3;  }
struct padded4_t     { bit<(8-4)>   pad4;  bit<4>  f4;  }
struct padded5_t     { bit<(8-5)>   pad5;  bit<5>  f5;  }
struct padded6_t     { bit<(8-6)>   pad6;  bit<6>  f6;  }
struct padded7_t     { bit<(8-7)>   pad7;  bit<7>  f7;  }
struct padded8_t     {                     bit<8>  f8;  }
struct padded9_t     { bit<(16-9)>  pad9;  bit<9>  f9;  }
struct padded10_t    { bit<(16-10)> pad10; bit<10> f10; }
struct padded11_t    { bit<(16-11)> pad11; bit<11> f11; }
struct padded12_t    { bit<(16-12)> pad12; bit<12> f12; }
struct padded13_t    { bit<(16-13)> pad13; bit<13> f13; }
struct padded14_t    { bit<(16-14)> pad14; bit<14> f14; }
struct padded15_t    { bit<(16-15)> pad15; bit<15> f15; }
struct padded16_t    {                     bit<16> f16; }
struct padded17_t    { bit<(32-17)> pad17; bit<17> f17; }
struct padded18_t    { bit<(32-18)> pad18; bit<18> f18; }
struct padded19_t    { bit<(32-19)> pad19; bit<19> f19; }
struct padded20_t    { bit<(32-20)> pad20; bit<20> f20; }
struct padded21_t    { bit<(32-21)> pad21; bit<21> f21; }
struct padded22_t    { bit<(32-22)> pad22; bit<22> f22; }
struct padded23_t    { bit<(32-23)> pad23; bit<23> f23; }
struct padded24_t    { bit<(32-24)> pad24; bit<24> f24; }
struct padded25_t    { bit<(32-25)> pad25; bit<25> f25; }
struct padded26_t    { bit<(32-26)> pad26; bit<26> f26; }
struct padded27_t    { bit<(32-27)> pad27; bit<27> f27; }
struct padded28_t    { bit<(32-28)> pad28; bit<28> f28; }
struct padded29_t    { bit<(32-29)> pad29; bit<29> f29; }
struct padded30_t    { bit<(32-30)> pad30; bit<30> f30; }
struct padded31_t    { bit<(32-31)> pad31; bit<31> f31; }
struct padded32_t    {                     bit<32> f32; }
struct padded33_t    { bit<(64-33)> pad33; bit<33> f33; }
struct padded34_t    { bit<(64-34)> pad34; bit<34> f34; }
struct padded35_t    { bit<(64-35)> pad35; bit<35> f35; }
struct padded36_t    { bit<(64-36)> pad36; bit<36> f36; }
struct padded37_t    { bit<(64-37)> pad37; bit<37> f37; }
struct padded38_t    { bit<(64-38)> pad38; bit<38> f38; }
struct padded39_t    { bit<(64-39)> pad39; bit<39> f39; }
struct padded40_t    { bit<(64-40)> pad40; bit<40> f40; }
struct padded41_t    { bit<(64-41)> pad41; bit<41> f41; }
struct padded42_t    { bit<(64-42)> pad42; bit<42> f42; }
struct padded43_t    { bit<(64-43)> pad43; bit<43> f43; }
struct padded44_t    { bit<(64-44)> pad44; bit<44> f44; }
struct padded45_t    { bit<(64-45)> pad45; bit<45> f45; }
struct padded46_t    { bit<(64-46)> pad46; bit<46> f46; }
struct padded47_t    { bit<(64-47)> pad47; bit<47> f47; }
struct padded48_t    { bit<(64-48)> pad48; bit<48> f48; }
struct padded49_t    { bit<(64-49)> pad49; bit<49> f49; }
struct padded50_t    { bit<(64-50)> pad50; bit<50> f50; }
struct padded51_t    { bit<(64-51)> pad51; bit<51> f51; }
struct padded52_t    { bit<(64-52)> pad52; bit<52> f52; }
struct padded53_t    { bit<(64-53)> pad53; bit<53> f53; }
struct padded54_t    { bit<(64-54)> pad54; bit<54> f54; }
struct padded55_t    { bit<(64-55)> pad55; bit<55> f55; }
struct padded56_t    { bit<(64-56)> pad56; bit<56> f56; }
struct padded57_t    { bit<(64-57)> pad57; bit<57> f57; }
struct padded58_t    { bit<(64-58)> pad58; bit<58> f58; }
struct padded59_t    { bit<(64-59)> pad59; bit<59> f59; }
struct padded60_t    { bit<(64-60)> pad60; bit<60> f60; }
struct padded61_t    { bit<(64-61)> pad61; bit<61> f61; }
struct padded62_t    { bit<(64-62)> pad62; bit<62> f62; }
struct padded63_t    { bit<(64-63)> pad63; bit<63> f63; }
struct padded64_t    {                     bit<64> f64; }

header bits1_t   { bit<1>  f1;  }
header bits2_t   { bit<2>  f2;  }
header bits3_t   { bit<3>  f3;  }
header bits4_t   { bit<4>  f4;  }
header bits5_t   { bit<5>  f5;  }
header bits6_t   { bit<6>  f6;  }
header bits7_t   { bit<7>  f7;  }
header bits8_t   { bit<8>  f8;  }
header bits9_t   { bit<9>  f9;  }
header bits10_t  { bit<10> f10;  }
header bits11_t  { bit<11> f11;  }
header bits12_t  { bit<12> f12;  }
header bits13_t  { bit<13> f13;  }
header bits14_t  { bit<14> f14;  }
header bits15_t  { bit<15> f15;  }
header bits16_t  { bit<16> f16;  }
header bits17_t  { bit<17> f17;  }
header bits18_t  { bit<18> f18;  }
header bits19_t  { bit<19> f19;  }
header bits20_t  { bit<20> f20;  }
header bits21_t  { bit<21> f21;  }
header bits22_t  { bit<22> f22;  }
header bits23_t  { bit<23> f23;  }
header bits24_t  { bit<24> f24;  }
header bits25_t  { bit<25> f25;  }
header bits26_t  { bit<26> f26;  }
header bits27_t  { bit<27> f27;  }
header bits28_t  { bit<28> f28;  }
header bits29_t  { bit<29> f29;  }
header bits30_t  { bit<30> f30;  }
header bits31_t  { bit<31> f31;  }
header bits32_t  { bit<32> f32;  }
header bits33_t  { bit<33> f33;  }
header bits34_t  { bit<34> f34;  }
header bits35_t  { bit<35> f35;  }
header bits36_t  { bit<36> f36;  }
header bits37_t  { bit<37> f37;  }
header bits38_t  { bit<38> f38;  }
header bits39_t  { bit<39> f39;  }
header bits40_t  { bit<40> f40;  }
header bits41_t  { bit<41> f41;  }
header bits42_t  { bit<42> f42;  }
header bits43_t  { bit<43> f43;  }
header bits44_t  { bit<44> f44;  }
header bits45_t  { bit<45> f45;  }
header bits46_t  { bit<46> f46;  }
header bits47_t  { bit<47> f47;  }
header bits48_t  { bit<48> f48;  }
header bits49_t  { bit<49> f49;  }
header bits50_t  { bit<50> f50;  }
header bits51_t  { bit<51> f51;  }
header bits52_t  { bit<52> f52;  }
header bits53_t  { bit<53> f53;  }
header bits54_t  { bit<54> f54;  }
header bits55_t  { bit<55> f55;  }
header bits56_t  { bit<56> f56;  }
header bits57_t  { bit<57> f57;  }
header bits58_t  { bit<58> f58;  }
header bits59_t  { bit<59> f59;  }
header bits60_t  { bit<60> f60;  }
header bits61_t  { bit<61> f61;  }
header bits62_t  { bit<62> f62;  }
header bits63_t  { bit<63> f63;  }
header bits64_t  { bit<64> f64;  }

header bool_t    { bool    b;  }

header pbits1_t   { bit<(8-1)>   pad1;  bit<1>  f1;  }
header pbits2_t   { bit<(8-2)>   pad2;  bit<2>  f2;  }
header pbits3_t   { bit<(8-3)>   pad3;  bit<3>  f3;  }
header pbits4_t   { bit<(8-4)>   pad4;  bit<4>  f4;  }
header pbits5_t   { bit<(8-5)>   pad5;  bit<5>  f5;  }
header pbits6_t   { bit<(8-6)>   pad6;  bit<6>  f6;  }
header pbits7_t   { bit<(8-7)>   pad7;  bit<7>  f7;  }
// header pbits8_t   {                     bit<8>  f8;  }
header pbits9_t   { bit<(16-9)>  pad9;  bit<9>  f9;  }
header pbits10_t  { bit<(16-10)> pad10; bit<10> f10; }
header pbits11_t  { bit<(16-11)> pad11; bit<11> f11; }
header pbits12_t  { bit<(16-12)> pad12; bit<12> f12; }
header pbits13_t  { bit<(16-13)> pad13; bit<13> f13; }
header pbits14_t  { bit<(16-14)> pad14; bit<14> f14; }
header pbits15_t  { bit<(16-15)> pad15; bit<15> f15; }
// header pbits16_t  {                     bit<16> f16; }
header pbits17_t  { bit<(32-17)> pad17; bit<17> f17; }
header pbits18_t  { bit<(32-18)> pad18; bit<18> f18; }
header pbits19_t  { bit<(32-19)> pad19; bit<19> f19; }
header pbits20_t  { bit<(32-20)> pad20; bit<20> f20; }
header pbits21_t  { bit<(32-21)> pad21; bit<21> f21; }
header pbits22_t  { bit<(32-22)> pad22; bit<22> f22; }
header pbits23_t  { bit<(32-23)> pad23; bit<23> f23; }
header pbits24_t  { bit<(32-24)> pad24; bit<24> f24; }
header pbits25_t  { bit<(32-25)> pad25; bit<25> f25; }
header pbits26_t  { bit<(32-26)> pad26; bit<26> f26; }
header pbits27_t  { bit<(32-27)> pad27; bit<27> f27; }
header pbits28_t  { bit<(32-28)> pad28; bit<28> f28; }
header pbits29_t  { bit<(32-29)> pad29; bit<29> f29; }
header pbits30_t  { bit<(32-30)> pad30; bit<30> f30; }
header pbits31_t  { bit<(32-31)> pad31; bit<31> f31; }
// header pbits32_t  {                     bit<32> f32; }
header pbits33_t  { bit<(64-33)> pad33; bit<33> f33; }
header pbits34_t  { bit<(64-34)> pad34; bit<34> f34; }
header pbits35_t  { bit<(64-35)> pad35; bit<35> f35; }
header pbits36_t  { bit<(64-36)> pad36; bit<36> f36; }
header pbits37_t  { bit<(64-37)> pad37; bit<37> f37; }
header pbits38_t  { bit<(64-38)> pad38; bit<38> f38; }
header pbits39_t  { bit<(64-39)> pad39; bit<39> f39; }
header pbits40_t  { bit<(64-40)> pad40; bit<40> f40; }
header pbits41_t  { bit<(64-41)> pad41; bit<41> f41; }
header pbits42_t  { bit<(64-42)> pad42; bit<42> f42; }
header pbits43_t  { bit<(64-43)> pad43; bit<43> f43; }
header pbits44_t  { bit<(64-44)> pad44; bit<44> f44; }
header pbits45_t  { bit<(64-45)> pad45; bit<45> f45; }
header pbits46_t  { bit<(64-46)> pad46; bit<46> f46; }
header pbits47_t  { bit<(64-47)> pad47; bit<47> f47; }
header pbits48_t  { bit<(64-48)> pad48; bit<48> f48; }
header pbits49_t  { bit<(64-49)> pad49; bit<49> f49; }
header pbits50_t  { bit<(64-50)> pad50; bit<50> f50; }
header pbits51_t  { bit<(64-51)> pad51; bit<51> f51; }
header pbits52_t  { bit<(64-52)> pad52; bit<52> f52; }
header pbits53_t  { bit<(64-53)> pad53; bit<53> f53; }
header pbits54_t  { bit<(64-54)> pad54; bit<54> f54; }
header pbits55_t  { bit<(64-55)> pad55; bit<55> f55; }
header pbits56_t  { bit<(64-56)> pad56; bit<56> f56; }
header pbits57_t  { bit<(64-57)> pad57; bit<57> f57; }
header pbits58_t  { bit<(64-58)> pad58; bit<58> f58; }
header pbits59_t  { bit<(64-59)> pad59; bit<59> f59; }
header pbits60_t  { bit<(64-60)> pad60; bit<60> f60; }
header pbits61_t  { bit<(64-61)> pad61; bit<61> f61; }
header pbits62_t  { bit<(64-62)> pad62; bit<62> f62; }
header pbits63_t  { bit<(64-63)> pad63; bit<63> f63; }
// header pbits64_t  {                     bit<64> f64; }
