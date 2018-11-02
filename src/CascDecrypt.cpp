/*****************************************************************************/
/* CascDecrypt.cpp                        Copyright (c) Ladislav Zezula 2015 */
/*---------------------------------------------------------------------------*/
/* Decryption functions for CascLib                                          */
/*---------------------------------------------------------------------------*/
/*   Date    Ver   Who  Comment                                              */
/* --------  ----  ---  -------                                              */
/* 31.10.15  1.00  Lad  The first version of CascDecrypt.cpp                 */
/*****************************************************************************/

#define __CASCLIB_SELF__
#include "CascLib.h"
#include "CascCommon.h"

//-----------------------------------------------------------------------------
// Local structures

#define CASC_EXTRA_KEYS 0x80

typedef struct _CASC_ENCRYPTION_KEY
{
    ULONGLONG KeyName;                  // "Name" of the key
    BYTE Key[0x10];                     // The key itself
} CASC_ENCRYPTION_KEY, *PCASC_ENCRYPTION_KEY;

typedef struct _CASC_SALSA20
{
    DWORD Key[0x10];
    DWORD dwRounds;

} CASC_SALSA20, *PCASC_SALSA20;

//-----------------------------------------------------------------------------
// Known encryption keys. See https://wowdev.wiki/CASC for updates

static const char * szKeyConstant16 = "expand 16-byte k";
static const char * szKeyConstant32 = "expand 32-byte k";

static CASC_ENCRYPTION_KEY CascKeys[] =
{
    // Key Name               Encryption key                                                                                         Seen in
    // ---------------------- ------------------------------------------------------------------------------------------------       -----------

    // Battle.net app
    { 0x2C547F26A2613E01ULL, { 0x37, 0xC5, 0x0C, 0x10, 0x2D, 0x4C, 0x9E, 0x3A, 0x5A, 0xC0, 0x69, 0xF0, 0x72, 0xB1, 0x41, 0x7D } },   // Battle.net App Alpha 1.5.0

    // Starcraft
//  { 0xD0CAE11366CEEA83ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? }},    // 1.12.3.2609 (build 45364)

    // Overwatch
    { 0xFB680CB6A8BF81F3ULL, { 0x62, 0xD9, 0x0E, 0xFA, 0x7F, 0x36, 0xD7, 0x1C, 0x39, 0x8A, 0xE2, 0xF1, 0xFE, 0x37, 0xBD, 0xB9 } },   // 0.8.0.24919_retailx64 (hardcoded)
    { 0x402CD9D8D6BFED98ULL, { 0xAE, 0xB0, 0xEA, 0xDE, 0xA4, 0x76, 0x12, 0xFE, 0x6C, 0x04, 0x1A, 0x03, 0x95, 0x8D, 0xF2, 0x41 } },   // 0.8.0.24919_retailx64 (hardcoded)
    { 0xDBD3371554F60306ULL, { 0x34, 0xE3, 0x97, 0xAC, 0xE6, 0xDD, 0x30, 0xEE, 0xFD, 0xC9, 0x8A, 0x2A, 0xB0, 0x93, 0xCD, 0x3C } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x11A9203C9881710AULL, { 0x2E, 0x2C, 0xB8, 0xC3, 0x97, 0xC2, 0xF2, 0x4E, 0xD0, 0xB5, 0xE4, 0x52, 0xF1, 0x8D, 0xC2, 0x67 } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0xA19C4F859F6EFA54ULL, { 0x01, 0x96, 0xCB, 0x6F, 0x5E, 0xCB, 0xAD, 0x7C, 0xB5, 0x28, 0x38, 0x91, 0xB9, 0x71, 0x2B, 0x4B } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x87AEBBC9C4E6B601ULL, { 0x68, 0x5E, 0x86, 0xC6, 0x06, 0x3D, 0xFD, 0xA6, 0xC9, 0xE8, 0x52, 0x98, 0x07, 0x6B, 0x3D, 0x42 } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0xDEE3A0521EFF6F03ULL, { 0xAD, 0x74, 0x0C, 0xE3, 0xFF, 0xFF, 0x92, 0x31, 0x46, 0x81, 0x26, 0x98, 0x57, 0x08, 0xE1, 0xB9 } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x8C9106108AA84F07ULL, { 0x53, 0xD8, 0x59, 0xDD, 0xA2, 0x63, 0x5A, 0x38, 0xDC, 0x32, 0xE7, 0x2B, 0x11, 0xB3, 0x2F, 0x29 } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x49166D358A34D815ULL, { 0x66, 0x78, 0x68, 0xCD, 0x94, 0xEA, 0x01, 0x35, 0xB9, 0xB1, 0x6C, 0x93, 0xB1, 0x12, 0x4A, 0xBA } },   // 0.8.0.24919_retailx64 (streamed from server)
    { 0x1463A87356778D14ULL, { 0x69, 0xBD, 0x2A, 0x78, 0xD0, 0x5C, 0x50, 0x3E, 0x93, 0x99, 0x49, 0x59, 0xB3, 0x0E, 0x5A, 0xEC } },   //  ? 1.0.3.0 (streamed from server) 
    { 0x5E152DE44DFBEE01ULL, { 0xE4, 0x5A, 0x17, 0x93, 0xB3, 0x7E, 0xE3, 0x1A, 0x8E, 0xB8, 0x5C, 0xEE, 0x0E, 0xEE, 0x1B, 0x68 } },   //  ? 1.0.3.0 (streamed from server) 
    { 0x9B1F39EE592CA415ULL, { 0x54, 0xA9, 0x9F, 0x08, 0x1C, 0xAD, 0x0D, 0x08, 0xF7, 0xE3, 0x36, 0xF4, 0x36, 0x8E, 0x89, 0x4C } },   //  ? 1.0.3.0 (streamed from server)
    { 0x24C8B75890AD5917ULL, { 0x31, 0x10, 0x0C, 0x00, 0xFD, 0xE0, 0xCE, 0x18, 0xBB, 0xB3, 0x3F, 0x3A, 0xC1, 0x5B, 0x30, 0x9F } },   //  ? 1.0.3.0 (included in game)
    { 0xEA658B75FDD4890FULL, { 0xDE, 0xC7, 0xA4, 0xE7, 0x21, 0xF4, 0x25, 0xD1, 0x33, 0x03, 0x98, 0x95, 0xC3, 0x60, 0x36, 0xF8 } },   //  ? 1.0.3.0 (included in game)
    { 0x026FDCDF8C5C7105ULL, { 0x8F, 0x41, 0x80, 0x9D, 0xA5, 0x53, 0x66, 0xAD, 0x41, 0x6D, 0x3C, 0x33, 0x74, 0x59, 0xEE, 0xE3 } },   // (included in game)
    { 0xCAE3FAC925F20402ULL, { 0x98, 0xB7, 0x8E, 0x87, 0x74, 0xBF, 0x27, 0x50, 0x93, 0xCB, 0x1B, 0x5F, 0xC7, 0x14, 0x51, 0x1B } },   // (included in game)
    { 0x061581CA8496C80CULL, { 0xDA, 0x2E, 0xF5, 0x05, 0x2D, 0xB9, 0x17, 0x38, 0x0B, 0x8A, 0xA6, 0xEF, 0x7A, 0x5F, 0x8E, 0x6A } },   // 
    { 0xBE2CB0FAD3698123ULL, { 0x90, 0x2A, 0x12, 0x85, 0x83, 0x6C, 0xE6, 0xDA, 0x58, 0x95, 0x02, 0x0D, 0xD6, 0x03, 0xB0, 0x65 } },   // 
    { 0x57A5A33B226B8E0AULL, { 0xFD, 0xFC, 0x35, 0xC9, 0x9B, 0x9D, 0xB1, 0x1A, 0x32, 0x62, 0x60, 0xCA, 0x24, 0x6A, 0xCB, 0x41 } },   // 1.1.0.0.30200 Ana
    { 0x42B9AB1AF5015920ULL, { 0xC6, 0x87, 0x78, 0x82, 0x3C, 0x96, 0x4C, 0x6F, 0x24, 0x7A, 0xCC, 0x0F, 0x4A, 0x25, 0x84, 0xF8 } },   // 1.2.0.1.30684 Summer Games
    { 0x4F0FE18E9FA1AC1AULL, { 0x89, 0x38, 0x1C, 0x74, 0x8F, 0x65, 0x31, 0xBB, 0xFC, 0xD9, 0x77, 0x53, 0xD0, 0x6C, 0xC3, 0xCD } },   // 1.2.0.1.30684
    { 0x7758B2CF1E4E3E1BULL, { 0x3D, 0xE6, 0x0D, 0x37, 0xC6, 0x64, 0x72, 0x35, 0x95, 0xF2, 0x7C, 0x5C, 0xDB, 0xF0, 0x8B, 0xFA } },   // 1.2.0.1.30684
    { 0xE5317801B3561125ULL, { 0x7D, 0xD0, 0x51, 0x19, 0x9F, 0x84, 0x01, 0xF9, 0x5E, 0x4C, 0x03, 0xC8, 0x84, 0xDC, 0xEA, 0x33 } },   // 1.4.0.2.32143 Halloween Terror
    { 0x16B866D7BA3A8036ULL, { 0x13, 0x95, 0xE8, 0x82, 0xBF, 0x25, 0xB4, 0x81, 0xF6, 0x1A, 0x4D, 0x62, 0x11, 0x41, 0xDA, 0x6E } },   // 1.4.1.0.31804 Bastion Blizzcon 2016 skin
    { 0x11131FFDA0D18D30ULL, { 0xC3, 0x2A, 0xD1, 0xB8, 0x25, 0x28, 0xE0, 0xA4, 0x56, 0x89, 0x7B, 0x3C, 0xE1, 0xC2, 0xD2, 0x7E } },   // 1.5.0.1.32795 Sombra
    { 0xCAC6B95B2724144AULL, { 0x73, 0xE4, 0xBE, 0xA1, 0x45, 0xDF, 0x2B, 0x89, 0xB6, 0x5A, 0xEF, 0x02, 0xF8, 0x3F, 0xA2, 0x60 } },   // 1.5.0.1.32795 Ecopoint: Antarctica
    { 0xB7DBC693758A5C36ULL, { 0xBC, 0x3A, 0x92, 0xBF, 0xE3, 0x02, 0x51, 0x8D, 0x91, 0xCC, 0x30, 0x79, 0x06, 0x71, 0xBF, 0x10 } },   // 1.5.0.1.32795 Genji Oni skin
    { 0x90CA73B2CDE3164BULL, { 0x5C, 0xBF, 0xF1, 0x1F, 0x22, 0x72, 0x0B, 0xAC, 0xC2, 0xAE, 0x6A, 0xAD, 0x8F, 0xE5, 0x33, 0x17 } },   // 1.6.1.0.33236 Oasis map
    { 0x6DD3212FB942714AULL, { 0xE0, 0x2C, 0x16, 0x43, 0x60, 0x2E, 0xC1, 0x6C, 0x3A, 0xE2, 0xA4, 0xD2, 0x54, 0xA0, 0x8F, 0xD9 } },   // 1.6.1.0.33236
    { 0x11DDB470ABCBA130ULL, { 0x66, 0x19, 0x87, 0x66, 0xB1, 0xC4, 0xAF, 0x75, 0x89, 0xEF, 0xD1, 0x3A, 0xD4, 0xDD, 0x66, 0x7A } },   // 1.6.1.0.33236 Winter Wonderland
    { 0x5BEF27EEE95E0B4BULL, { 0x36, 0xBC, 0xD2, 0xB5, 0x51, 0xFF, 0x1C, 0x84, 0xAA, 0x3A, 0x39, 0x94, 0xCC, 0xEB, 0x03, 0x3E } },   // 
    { 0x9359B46E49D2DA42ULL, { 0x17, 0x3D, 0x65, 0xE7, 0xFC, 0xAE, 0x29, 0x8A, 0x93, 0x63, 0xBD, 0x6A, 0xA1, 0x89, 0xF2, 0x00 } },   //               Diablo's 20th anniversary
    { 0x1A46302EF8896F34ULL, { 0x80, 0x29, 0xAD, 0x54, 0x51, 0xD4, 0xBC, 0x18, 0xE9, 0xD0, 0xF5, 0xAC, 0x44, 0x9D, 0xC0, 0x55 } },   // 1.7.0.2.34156 Year of the Rooster
    { 0x693529F7D40A064CULL, { 0xCE, 0x54, 0x87, 0x3C, 0x62, 0xDA, 0xA4, 0x8E, 0xFF, 0x27, 0xFC, 0xC0, 0x32, 0xBD, 0x07, 0xE3 } },   // 1.8.0.0.34470 CTF Maps
    { 0x388B85AEEDCB685DULL, { 0xD9, 0x26, 0xE6, 0x59, 0xD0, 0x4A, 0x09, 0x6B, 0x24, 0xC1, 0x91, 0x51, 0x07, 0x6D, 0x37, 0x9A } },   // 1.8.0.0.34470 Numbani Update (Doomfist teaser)
    { 0xE218F69AAC6C104DULL, { 0xF4, 0x3D, 0x12, 0xC9, 0x4A, 0x9A, 0x52, 0x84, 0x97, 0x97, 0x1F, 0x1C, 0xBE, 0x41, 0xAD, 0x4D } },   // 1.9.0.0.34986 Orisa
    { 0xF432F0425363F250ULL, { 0xBA, 0x69, 0xF2, 0xB3, 0x3C, 0x27, 0x68, 0xF5, 0xF2, 0x9B, 0xFE, 0x78, 0xA5, 0xA1, 0xFA, 0xD5 } },   // 1.10.0.0.35455 Uprising
    { 0x061D52F86830B35DULL, { 0xD7, 0x79, 0xF9, 0xC6, 0xCC, 0x9A, 0x4B, 0xE1, 0x03, 0xA4, 0xE9, 0x0A, 0x73, 0x38, 0xF7, 0x93 } },   // 1.10.0.0.35455 D.Va Officer Skin (HotS Nexus Challenge 2)
    { 0x1275C84CF113EF65ULL, { 0xCF, 0x58, 0xB6, 0x93, 0x3E, 0xAF, 0x98, 0xAF, 0x53, 0xE7, 0x6F, 0x84, 0x26, 0xCC, 0x7E, 0x6C } },   // 
    { 0xD9C7C7AC0F14C868ULL, { 0x3A, 0xFD, 0xF6, 0x8E, 0x3A, 0x5D, 0x63, 0xBA, 0xBA, 0x1E, 0x68, 0x21, 0x88, 0x3F, 0x06, 0x7D } },   // 
    { 0xBD4E42661A432951ULL, { 0x6D, 0xE8, 0xE2, 0x8C, 0x85, 0x11, 0x64, 0x4D, 0x55, 0x95, 0xFC, 0x45, 0xE5, 0x35, 0x14, 0x72 } },   // 1.11.0.0.36376 Anniversary event
    { 0xC43CB14355249451ULL, { 0x0E, 0xA2, 0xB4, 0x4F, 0x96, 0xA2, 0x69, 0xA3, 0x86, 0x85, 0x6D, 0x04, 0x9A, 0x3D, 0xEC, 0x86 } },   // 1.12.0.0.37104 Horizon Lunar Colony
    { 0xE6D914F8E4744953ULL, { 0xC8, 0x47, 0x7C, 0x28, 0x9D, 0xCE, 0x66, 0xD9, 0x13, 0x65, 0x07, 0xA3, 0x3A, 0xA3, 0x33, 0x01 } },   // 1.13.0.0.37646 Doomfist
    { 0x5694C503F8C80178ULL, { 0x7F, 0x4C, 0xF1, 0xC1, 0xFB, 0xBA, 0xD9, 0x2B, 0x18, 0x43, 0x36, 0xD6, 0x77, 0xEB, 0xF9, 0x37 } },   // 1.13.0.0.37646 Doomfist
    { 0x21DBFD65F3E54269ULL, { 0xAB, 0x58, 0x0C, 0x38, 0x37, 0xCA, 0xF8, 0xA4, 0x61, 0xF2, 0x43, 0xA5, 0x66, 0xB2, 0xAE, 0x4D } },   // 1.13.0.0.37646 Summer Games 2017
//  { 0x27ABA5F88DD8D078ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },     // 1.13.0.0.37646
    { 0x21E1F90E71D33C71ULL, { 0x32, 0x87, 0x42, 0x33, 0x91, 0x62, 0xB3, 0x26, 0x76, 0xC8, 0x03, 0xC2, 0x25, 0x59, 0x31, 0xA6 } },   // 1.14.1.0.39083 Deathmatch
    { 0xD9CB055BCDD40B6EULL, { 0x49, 0xFB, 0x44, 0x77, 0xA4, 0xA0, 0x82, 0x53, 0x27, 0xE9, 0xA7, 0x36, 0x82, 0xBE, 0xCD, 0x0C } },   // 1.15.0.0.????? Junkertown
    { 0x8175CE3C694C6659ULL, { 0xE3, 0xF3, 0xFA, 0x77, 0x26, 0xC7, 0x0D, 0x26, 0xAE, 0x13, 0x0D, 0x96, 0x9D, 0xDD, 0xF3, 0x99 } },   // 1.16.0.0.40011 Halloween 2017
    { 0xB8DE51690075435AULL, { 0xC0, 0x7E, 0x92, 0x60, 0xBB, 0x71, 0x12, 0x17, 0xE7, 0xDE, 0x6F, 0xED, 0x91, 0x1F, 0x42, 0x96 } },   // 1.16.0.0.????? Winston Blizzcon 2017 skin
    { 0xF6CF23955B5D437DULL, { 0xAE, 0xBA, 0x22, 0x73, 0x28, 0xA5, 0xB0, 0xAA, 0x9F, 0x51, 0xDA, 0xE3, 0xF6, 0xA7, 0xDF, 0xE4 } },   // 1.17.0.2.41350 Moira
    { 0x0E4D9426F2891F5CULL, { 0x9F, 0xF0, 0x64, 0xC3, 0x8B, 0xE5, 0x2C, 0xCD, 0xF7, 0x37, 0x48, 0x18, 0x0F, 0x62, 0x82, 0x05 } },   // 1.18.1.2.42076 Winter Wonderland 2017
    { 0x9240BA6A2A0CF684ULL, { 0xDF, 0x2E, 0x37, 0xD7, 0x8B, 0x43, 0x10, 0x8F, 0xA6, 0x24, 0x20, 0x68, 0xB7, 0x0D, 0x1F, 0x65 } },   // 1.19.1.3.42563 Overwatch League
    { 0x82297FBAB7F5EB80ULL, { 0xB5, 0x34, 0xC2, 0x09, 0x65, 0x85, 0x2F, 0xB1, 0x5A, 0xEC, 0xAC, 0x17, 0xE3, 0x81, 0xB4, 0x17 } },   // 1.19.1.3.42563 Jan 2017 Lootbox Update
    { 0x9ADF00AA1A174A69ULL, { 0x9A, 0x4A, 0xC8, 0x99, 0x26, 0x1A, 0x2F, 0x1C, 0x69, 0x69, 0xF3, 0x93, 0x97, 0xC3, 0x58, 0xE7 } },   // 1.19.1.3.42563 Blizzard World
    { 0xCFA05AA76B49F881ULL, { 0x52, 0x6D, 0xDD, 0xEF, 0x19, 0xBF, 0x37, 0x3C, 0x25, 0xB6, 0x29, 0xA3, 0x34, 0xCD, 0x72, 0x37 } },   // 1.19.1.3.42563 WoW's Battle For Azeroth Preorder
    { 0x493455579DA0B18EULL, { 0xC0, 0xBA, 0xBF, 0x72, 0xAD, 0x2C, 0x05, 0xDF, 0xC1, 0x40, 0x17, 0xD1, 0xAD, 0xBF, 0x59, 0x77 } },   // 1.19.3.1.43036 Inaugural Season Spray/Icon ??
    { 0x6362C5AD65DAE686ULL, { 0x62, 0xF6, 0x03, 0xD5, 0x39, 0x0F, 0x76, 0x3E, 0xD5, 0x17, 0x73, 0xF0, 0x16, 0x4F, 0xED, 0xB5 } },   // 1.19.3.1.43036 White/Gray OWL Skins ??
    { 0x8162E5313A9C135DULL, { 0xF4, 0x07, 0x83, 0x4D, 0x95, 0x21, 0x58, 0x7C, 0x50, 0x12, 0xB0, 0xA5, 0x9D, 0x7E, 0x06, 0x4B } },   // 1.20.0.2.43435 Lunar New Year 2018 (Dog) / Ayutthaya / Comp CTF
//  { 0x68EAE8FDC008C381ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 1.20.0.2.43435
    { 0xF412C6327C4BF091ULL, { 0x6F, 0xAF, 0xC6, 0x48, 0xCB, 0xF1, 0xC2, 0x11, 0x5B, 0x76, 0x95, 0x93, 0xC1, 0x70, 0xE7, 0x32 } },   // 1.20.0.2.43435 SC2 20th Anniversary (Kerrigan Skin)

    // Streamed WoW keys                        
    { 0xFA505078126ACB3EULL, { 0xBD, 0xC5, 0x18, 0x62, 0xAB, 0xED, 0x79, 0xB2, 0xDE, 0x48, 0xC8, 0xE7, 0xE6, 0x6C, 0x62, 0x00 } },   // 15  WOW-20740patch7.0.1_Beta  <not used between 7.0 and 7.3>
    { 0xFF813F7D062AC0BCULL, { 0xAA, 0x0B, 0x5C, 0x77, 0xF0, 0x88, 0xCC, 0xC2, 0xD3, 0x90, 0x49, 0xBD, 0x26, 0x7F, 0x06, 0x6D } },   // 25  WOW-20740patch7.0.1_Beta  <not used between 7.0 and 7.3>
    { 0xD1E9B5EDF9283668ULL, { 0x8E, 0x4A, 0x25, 0x79, 0x89, 0x4E, 0x38, 0xB4, 0xAB, 0x90, 0x58, 0xBA, 0x5C, 0x73, 0x28, 0xEE } },   // 39  WOW-20740patch7.0.1_Beta  Enchanted Torch pet
    { 0xB76729641141CB34ULL, { 0x98, 0x49, 0xD1, 0xAA, 0x7B, 0x1F, 0xD0, 0x98, 0x19, 0xC5, 0xC6, 0x62, 0x83, 0xA3, 0x26, 0xEC } },   // 40  WOW-20740patch7.0.1_Beta  Enchanted Pen pet
    { 0xFFB9469FF16E6BF8ULL, { 0xD5, 0x14, 0xBD, 0x19, 0x09, 0xA9, 0xE5, 0xDC, 0x87, 0x03, 0xF4, 0xB8, 0xBB, 0x1D, 0xFD, 0x9A } },   // 41  WOW-20740patch7.0.1_Beta  <not used between 7.0 and 7.3>
    { 0x23C5B5DF837A226CULL, { 0x14, 0x06, 0xE2, 0xD8, 0x73, 0xB6, 0xFC, 0x99, 0x21, 0x7A, 0x18, 0x08, 0x81, 0xDA, 0x8D, 0x62 } },   // 42  WOW-20740patch7.0.1_Beta  Enchanted Cauldron pet
//  { 0x3AE403EF40AC3037ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 51  WOW-21249patch7.0.3_Beta  <not used between 7.0 and 7.3>
    { 0xE2854509C471C554ULL, { 0x43, 0x32, 0x65, 0xF0, 0xCD, 0xEB, 0x2F, 0x4E, 0x65, 0xC0, 0xEE, 0x70, 0x08, 0x71, 0x4D, 0x9E } },   // 52  WOW-21249patch7.0.3_Beta  Warcraft movie items
    { 0x8EE2CB82178C995AULL, { 0xDA, 0x6A, 0xFC, 0x98, 0x9E, 0xD6, 0xCA, 0xD2, 0x79, 0x88, 0x59, 0x92, 0xC0, 0x37, 0xA8, 0xEE } },   // 55  WOW-21531patch7.0.3_Beta  BlizzCon 2016 Murlocs
    { 0x5813810F4EC9B005ULL, { 0x01, 0xBE, 0x8B, 0x43, 0x14, 0x2D, 0xD9, 0x9A, 0x9E, 0x69, 0x0F, 0xAD, 0x28, 0x8B, 0x60, 0x82 } },   // 56  WOW-21531patch7.0.3_Beta  Fel Kitten
    { 0x7F9E217166ED43EAULL, { 0x05, 0xFC, 0x92, 0x7B, 0x9F, 0x4F, 0x5B, 0x05, 0x56, 0x81, 0x42, 0x91, 0x2A, 0x05, 0x2B, 0x0F } },   // 57  WOW-21531patch7.0.3_Beta  Legion music <not used in 7.3>
    { 0xC4A8D364D23793F7ULL, { 0xD1, 0xAC, 0x20, 0xFD, 0x14, 0x95, 0x7F, 0xAB, 0xC2, 0x71, 0x96, 0xE9, 0xF6, 0xE7, 0x02, 0x4A } },   // 58  WOW-21691patch7.0.3_Beta  Demon Hunter #1 cinematic (legion_dh1)
    { 0x40A234AEBCF2C6E5ULL, { 0xC6, 0xC5, 0xF6, 0xC7, 0xF7, 0x35, 0xD7, 0xD9, 0x4C, 0x87, 0x26, 0x7F, 0xA4, 0x99, 0x4D, 0x45 } },   // 59  WOW-21691patch7.0.3_Beta  Demon Hunter #2 cinematic (legion_dh2)
    { 0x9CF7DFCFCBCE4AE5ULL, { 0x72, 0xA9, 0x7A, 0x24, 0xA9, 0x98, 0xE3, 0xA5, 0x50, 0x0F, 0x38, 0x71, 0xF3, 0x76, 0x28, 0xC0 } },   // 60  WOW-21691patch7.0.3_Beta  Val'sharah #1 cinematic (legion_val_yd)
    { 0x4E4BDECAB8485B4FULL, { 0x38, 0x32, 0xD7, 0xC4, 0x2A, 0xAC, 0x92, 0x68, 0xF0, 0x0B, 0xE7, 0xB6, 0xB4, 0x8E, 0xC9, 0xAF } },   // 61  WOW-21691patch7.0.3_Beta  Val'sharah #2 cinematic (legion_val_yx)
    { 0x94A50AC54EFF70E4ULL, { 0xC2, 0x50, 0x1A, 0x72, 0x65, 0x4B, 0x96, 0xF8, 0x63, 0x50, 0xC5, 0xA9, 0x27, 0x96, 0x2F, 0x7A } },   // 62  WOW-21691patch7.0.3_Beta  Sylvanas warchief cinematic (legion_org_vs)
    { 0xBA973B0E01DE1C2CULL, { 0xD8, 0x3B, 0xBC, 0xB4, 0x6C, 0xC4, 0x38, 0xB1, 0x7A, 0x48, 0xE7, 0x6C, 0x4F, 0x56, 0x54, 0xA3 } },   // 63  WOW-21691patch7.0.3_Beta  Stormheim Sylvanas vs Greymane cinematic (legion_sth)
    { 0x494A6F8E8E108BEFULL, { 0xF0, 0xFD, 0xE1, 0xD2, 0x9B, 0x27, 0x4F, 0x6E, 0x7D, 0xBD, 0xB7, 0xFF, 0x81, 0x5F, 0xE9, 0x10 } },   // 64  WOW-21691patch7.0.3_Beta  Harbingers Gul'dan video (legion_hrb_g)
    { 0x918D6DD0C3849002ULL, { 0x85, 0x70, 0x90, 0xD9, 0x26, 0xBB, 0x28, 0xAE, 0xDA, 0x4B, 0xF0, 0x28, 0xCA, 0xCC, 0x4B, 0xA3 } },   // 65  WOW-21691patch7.0.3_Beta  Harbingers Khadgar video (legion_hrb_k)
    { 0x0B5F6957915ADDCAULL, { 0x4D, 0xD0, 0xDC, 0x82, 0xB1, 0x01, 0xC8, 0x0A, 0xBA, 0xC0, 0xA4, 0xD5, 0x7E, 0x67, 0xF8, 0x59 } },   // 66  WOW-21691patch7.0.3_Beta  Harbingers Illidan video (legion_hrb_i)
    { 0x794F25C6CD8AB62BULL, { 0x76, 0x58, 0x3B, 0xDA, 0xCD, 0x52, 0x57, 0xA3, 0xF7, 0x3D, 0x15, 0x98, 0xA2, 0xCA, 0x2D, 0x99 } },   // 67  WOW-21846patch7.0.3_Beta  Suramar cinematic (legion_su_i)
    { 0xA9633A54C1673D21ULL, { 0x1F, 0x8D, 0x46, 0x7F, 0x5D, 0x6D, 0x41, 0x1F, 0x8A, 0x54, 0x8B, 0x63, 0x29, 0xA5, 0x08, 0x7E } },   // 68  WOW-21846patch7.0.3_Beta  legion_su_r cinematic
    { 0x5E5D896B3E163DEAULL, { 0x8A, 0xCE, 0x8D, 0xB1, 0x69, 0xE2, 0xF9, 0x8A, 0xC3, 0x6A, 0xD5, 0x2C, 0x08, 0x8E, 0x77, 0xC1 } },   // 69  WOW-21846patch7.0.3_Beta  Broken Shore intro cinematic (legion_bs_i)
    { 0x0EBE36B5010DFD7FULL, { 0x9A, 0x89, 0xCC, 0x7E, 0x3A, 0xCB, 0x29, 0xCF, 0x14, 0xC6, 0x0B, 0xC1, 0x3B, 0x1E, 0x46, 0x16 } },   // 70  WOW-21846patch7.0.3_Beta  Alliance Broken Shore cinematic (legion_bs_a)
    { 0x01E828CFFA450C0FULL, { 0x97, 0x2B, 0x6E, 0x74, 0x42, 0x0E, 0xC5, 0x19, 0xE6, 0xF9, 0xD9, 0x7D, 0x59, 0x4A, 0xA3, 0x7C } },   // 71  WOW-21846patch7.0.3_Beta  Horde Broken Shore cinematic (legion_bs_h)
    { 0x4A7BD170FE18E6AEULL, { 0xAB, 0x55, 0xAE, 0x1B, 0xF0, 0xC7, 0xC5, 0x19, 0xAF, 0xF0, 0x28, 0xC1, 0x56, 0x10, 0xA4, 0x5B } },   // 72  WOW-21846patch7.0.3_Beta  Khadgar & Light's Heart cinematic (legion_iq_lv)
    { 0x69549CB975E87C4FULL, { 0x7B, 0x6F, 0xA3, 0x82, 0xE1, 0xFA, 0xD1, 0x46, 0x5C, 0x85, 0x1E, 0x3F, 0x47, 0x34, 0xA1, 0xB3 } },   // 73  WOW-21846patch7.0.3_Beta  legion_iq_id cinematic
    { 0x460C92C372B2A166ULL, { 0x94, 0x6D, 0x56, 0x59, 0xF2, 0xFA, 0xF3, 0x27, 0xC0, 0xB7, 0xEC, 0x82, 0x8B, 0x74, 0x8A, 0xDB } },   // 74  WOW-21952patch7.0.3_Beta  Stormheim Alliance cinematic (legion_g_a_sth)
    { 0x8165D801CCA11962ULL, { 0xCD, 0x0C, 0x0F, 0xFA, 0xAD, 0x93, 0x63, 0xEC, 0x14, 0xDD, 0x25, 0xEC, 0xDD, 0x2A, 0x5B, 0x62 } },   // 75  WOW-21952patch7.0.3_Beta  Stormheim Horde cinematic (legion_g_h_sth)
    { 0xA3F1C999090ADAC9ULL, { 0xB7, 0x2F, 0xEF, 0x4A, 0x01, 0x48, 0x8A, 0x88, 0xFF, 0x02, 0x28, 0x0A, 0xA0, 0x7A, 0x92, 0xBB } },   // 81  WOW-22578patch7.1.0_PTR   Firecat Mount
//  { 0x18AFDF5191923610ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 82  WOW-22578patch7.1.0_PTR   <not used between 7.1 and 7.3>
//  { 0x3C258426058FBD93ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 91  WOW-23436patch7.2.0_PTR   <not used between 7.2 and 7.3>
    { 0x094E9A0474876B98ULL, { 0xE5, 0x33, 0xBB, 0x6D, 0x65, 0x72, 0x7A, 0x58, 0x32, 0x68, 0x0D, 0x62, 0x0B, 0x0B, 0xC1, 0x0B } },   // 92  WOW-23910patch7.2.5_PTR   shadowstalkerpanthermount, shadowstalkerpantherpet
    { 0x3DB25CB86A40335EULL, { 0x02, 0x99, 0x0B, 0x12, 0x26, 0x0C, 0x1E, 0x9F, 0xDD, 0x73, 0xFE, 0x47, 0xCB, 0xAB, 0x70, 0x24 } },   // 93  WOW-23789patch7.2.0_PTR   legion_72_ots
    { 0x0DCD81945F4B4686ULL, { 0x1B, 0x78, 0x9B, 0x87, 0xFB, 0x3C, 0x92, 0x38, 0xD5, 0x28, 0x99, 0x7B, 0xFA, 0xB4, 0x41, 0x86 } },   // 94  WOW-23789patch7.2.0_PTR   legion_72_tst
    { 0x486A2A3A2803BE89ULL, { 0x32, 0x67, 0x9E, 0xA7, 0xB0, 0xF9, 0x9E, 0xBF, 0x4F, 0xA1, 0x70, 0xE8, 0x47, 0xEA, 0x43, 0x9A } },   // 95  WOW-23789patch7.2.0_PTR   legion_72_ars
    { 0x71F69446AD848E06ULL, { 0xE7, 0x9A, 0xEB, 0x88, 0xB1, 0x50, 0x9F, 0x62, 0x8F, 0x38, 0x20, 0x82, 0x01, 0x74, 0x1C, 0x30 } },   // 97  WOW-24473patch7.3.0_PTR   BlizzCon 2017 Mounts (AllianceShipMount and HordeZeppelinMount)
    { 0x211FCD1265A928E9ULL, { 0xA7, 0x36, 0xFB, 0xF5, 0x8D, 0x58, 0x7B, 0x39, 0x72, 0xCE, 0x15, 0x4A, 0x86, 0xAE, 0x45, 0x40 } },   // 98  WOW-24473patch7.3.0_PTR   "Shadow" fox pet (store)
    { 0x0ADC9E327E42E98CULL, { 0x01, 0x7B, 0x34, 0x72, 0xC1, 0xDE, 0xE3, 0x04, 0xFA, 0x0B, 0x2F, 0xF8, 0xE5, 0x3F, 0xF7, 0xD6 } },   // 99  WOW-23910patch7.2.5_PTR   legion_72_tsf
    { 0xBAE9F621B60174F1ULL, { 0x38, 0xC3, 0xFB, 0x39, 0xB4, 0x97, 0x17, 0x60, 0xB4, 0xB9, 0x82, 0xFE, 0x9F, 0x09, 0x50, 0x14 } },   // 100 WOW-24727patch7.3.0_PTR   Rejection of the Gift cinematic (legion_73_agi)
    { 0x34DE1EEADC97115EULL, { 0x2E, 0x3A, 0x53, 0xD5, 0x9A, 0x49, 0x1E, 0x5C, 0xD1, 0x73, 0xF3, 0x37, 0xF7, 0xCD, 0x8C, 0x61 } },   // 101 WOW-24727patch7.3.0_PTR   Resurrection of Alleria Windrunner cinematic (legion_73_avt)
    { 0xE07E107F1390A3DFULL, { 0x29, 0x0D, 0x27, 0xB0, 0xE8, 0x71, 0xF8, 0xC5, 0xB1, 0x4A, 0x14, 0xE5, 0x14, 0xD0, 0xF0, 0xD9 } },   // 102 WOW-25079patch7.3.2_PTR   Tottle battle pet, Raptor mount, Horse mount (104 files)
    { 0x32690BF74DE12530ULL, { 0xA2, 0x55, 0x62, 0x10, 0xAE, 0x54, 0x22, 0xE6, 0xD6, 0x1E, 0xDA, 0xAF, 0x12, 0x2C, 0xB6, 0x37 } },   // 103 WOW-24781patch7.3.0_PTR   legion_73_pan
    { 0xBF3734B1DCB04696ULL, { 0x48, 0x94, 0x61, 0x23, 0x05, 0x0B, 0x00, 0xA7, 0xEF, 0xB1, 0xC0, 0x29, 0xEE, 0x6C, 0xC4, 0x38 } },   // 104 WOW-25079patch7.3.2_PTR   legion_73_afn
    { 0x74F4F78002A5A1BEULL, { 0xC1, 0x4E, 0xEC, 0x8D, 0x5A, 0xEE, 0xF9, 0x3F, 0xA8, 0x11, 0xD4, 0x50, 0xB4, 0xE4, 0x6E, 0x91 } },   // 105 WOW-25079patch7.3.2_PTR   SilithusPhase01 map
//  { 0x423F07656CA27D23ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 107 WOW-25600patch7.3.5_PTR   bltestmap
//  { 0x0691678F83E8A75DULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 108 WOW-25600patch7.3.5_PTR   filedataid 1782602-1782603
//  { 0x324498590F550556ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 109 WOW-25600patch7.3.5_PTR   filedataid 1782615-1782619
//  { 0xC02C78F40BEF5998ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 110 WOW-25600patch7.3.5_PTR   test/testtexture.blp (fdid 1782613)
//  { 0x47011412CCAAB541ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 111 WOW-25600patch7.3.5_PTR   unused in 25600
//  { 0x23B6F5764CE2DDD6ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 112 WOW-25600patch7.3.5_PTR   unused in 25600
//  { 0x8E00C6F405873583ULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 113 WOW-25600patch7.3.5_PTR   filedataid 1783470-1783472
    { 0x78482170E4CFD4A6ULL, { 0x76, 0x85, 0x40, 0xC2, 0x0A, 0x5B, 0x15, 0x35, 0x83, 0xAD, 0x7F, 0x53, 0x13, 0x0C, 0x58, 0xFE } },   // 114 WOW-25600patch7.3.5_PTR   Magni Bronzebeard VO
    { 0xB1EB52A64BFAF7BFULL, { 0x45, 0x81, 0x33, 0xAA, 0x43, 0x94, 0x9A, 0x14, 0x16, 0x32, 0xC4, 0xF8, 0x59, 0x6D, 0xE2, 0xB0 } },   // 115 WOW-25600patch7.3.5_PTR   dogmount, 50 files
    { 0xFC6F20EE98D208F6ULL, { 0x57, 0x79, 0x0E, 0x48, 0xD3, 0x55, 0x00, 0xE7, 0x0D, 0xF8, 0x12, 0x59, 0x4F, 0x50, 0x7B, 0xE7 } },   // 117 WOW-25632patch7.3.5_PTR   shop stuff
    { 0x402CFABF2020D9B7ULL, { 0x67, 0x19, 0x7B, 0xCD, 0x9D, 0x0E, 0xF0, 0xC4, 0x08, 0x53, 0x78, 0xFA, 0xA6, 0x9A, 0x32, 0x64 } },   // 118 WOW-25678patch7.3.5_PTR   filedataid 1854762
    { 0x6FA0420E902B4FBEULL, { 0x27, 0xB7, 0x50, 0x18, 0x4E, 0x53, 0x29, 0xC4, 0xE4, 0x45, 0x5C, 0xBD, 0x3E, 0x1F, 0xD5, 0xAB } },   // 119 WOW-25744patch7.3.5_PTR   legion_735_epa / legion_735_eph
    { 0x1076074F2B350A2DULL, { 0x88, 0xBF, 0x0C, 0xD0, 0xD5, 0xBA, 0x15, 0x9A, 0xE7, 0xCB, 0x91, 0x6A, 0xFB, 0xE1, 0x38, 0x65 } },   // 121 WOW-26287patch8.0.1_Beta  skiff
    { 0x816F00C1322CDF52ULL, { 0x6F, 0x83, 0x22, 0x99, 0xA7, 0x57, 0x89, 0x57, 0xEE, 0x86, 0xB7, 0xF9, 0xF1, 0x5B, 0x01, 0x88 } },   // 122 WOW-26287patch8.0.1_Beta  snowkid
    { 0xDDD295C82E60DB3CULL, { 0x34, 0x29, 0xCC, 0x59, 0x27, 0xD1, 0x62, 0x97, 0x65, 0x97, 0x4F, 0xD9, 0xAF, 0xAB, 0x75, 0x80 } },   // 123 WOW-26287patch8.0.1_Beta  redbird
    { 0x83E96F07F259F799ULL, { 0x91, 0xF7, 0xD0, 0xE7, 0xA0, 0x2C, 0xDE, 0x0D, 0xE0, 0xBD, 0x36, 0x7F, 0xAB, 0xCB, 0x8A, 0x6E } },   // 124 WOW-26522patch8.0.1_Beta  BlizzCon 2018 (Alliance and Horde banners and cloaks)
    { 0xC1E5D7408A7D4484ULL, { 0xA7, 0xD8, 0x8E, 0x52, 0x74, 0x9F, 0xA5, 0x45, 0x9D, 0x64, 0x45, 0x23, 0xF8, 0x35, 0x96, 0x51 } },   // 226 WOW-26871patch8.0.1_Beta  Sylvanas Warbringer cinematic
    { 0xE46276EB9E1A9854ULL, { 0xCC, 0xCA, 0x36, 0xE3, 0x02, 0xF9, 0x45, 0x9B, 0x1D, 0x60, 0x52, 0x6A, 0x31, 0xBE, 0x77, 0xC8 } },   // 227 WOW-26871patch8.0.1_Beta  ltc_a, ltc_h and ltt cinematics
    { 0xD245B671DD78648CULL, { 0x19, 0xDC, 0xB4, 0xD4, 0x5A, 0x65, 0x8B, 0x54, 0x35, 0x1D, 0xB7, 0xDD, 0xC8, 0x1D, 0xE7, 0x9E } },   // 228 WOW-26871patch8.0.1_Beta  stz, zia, kta, jnm & ja cinematics
    { 0x4C596E12D36DDFC3ULL, { 0xB8, 0x73, 0x19, 0x26, 0x38, 0x94, 0x99, 0xCB, 0xD4, 0xAD, 0xBF, 0x50, 0x06, 0xCA, 0x03, 0x91 } },   // 229 WOW-26871patch8.0.1_Beta  bar cinematic
    { 0x0C9ABD5081C06411ULL, { 0x25, 0xA7, 0x7C, 0xD8, 0x00, 0x19, 0x7E, 0xE6, 0xA3, 0x2D, 0xD6, 0x3F, 0x04, 0xE1, 0x15, 0xFA } },   // 230 WOW-26871patch8.0.1_Beta  zcf cinematic
    { 0x3C6243057F3D9B24ULL, { 0x58, 0xAE, 0x3E, 0x06, 0x42, 0x10, 0xE3, 0xED, 0xF9, 0xC1, 0x25, 0x9C, 0xDE, 0x91, 0x4C, 0x5D } },   // 231 WOW-26871patch8.0.1_Beta  ktf cinematic
    { 0x7827FBE24427E27DULL, { 0x34, 0xA4, 0x32, 0x04, 0x20, 0x73, 0xCD, 0x0B, 0x51, 0x62, 0x70, 0x68, 0xD2, 0xE0, 0xBD, 0x3E } },   // 232 WOW-26871patch8.0.1_Beta  rot cinematic
//  { 0x5DD92EE32BBF9ABDULL, { 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x??, 0x?? } },   // 234 WOW-27004patch8.0.1_Subm  filedataid 2238294
};
                                                                                                          
//-----------------------------------------------------------------------------
// Local functions

static DWORD Rol32(DWORD dwValue, DWORD dwRolCount)
{
    return (dwValue << dwRolCount) | (dwValue >> (32 - dwRolCount));
}

static void Initialize(PCASC_SALSA20 pState, LPBYTE pbKey, DWORD cbKeyLength, LPBYTE pbVector)
{
    const char * szConstants = (cbKeyLength == 32) ? szKeyConstant32 : szKeyConstant16;
    DWORD KeyIndex = cbKeyLength - 0x10;

    memset(pState, 0, sizeof(CASC_SALSA20));
    pState->Key[0]  = *(PDWORD)(szConstants + 0x00);
    pState->Key[1]  = *(PDWORD)(pbKey + 0x00);
    pState->Key[2]  = *(PDWORD)(pbKey + 0x04);
    pState->Key[3]  = *(PDWORD)(pbKey + 0x08);
    pState->Key[4]  = *(PDWORD)(pbKey + 0x0C);
    pState->Key[5]  = *(PDWORD)(szConstants + 0x04);
    pState->Key[6]  = *(PDWORD)(pbVector + 0x00);
    pState->Key[7]  = *(PDWORD)(pbVector + 0x04);
    pState->Key[8]  = 0;
    pState->Key[9]  = 0;
    pState->Key[10] = *(PDWORD)(szConstants + 0x08);
    pState->Key[11] = *(PDWORD)(pbKey + KeyIndex + 0x00);
    pState->Key[12] = *(PDWORD)(pbKey + KeyIndex + 0x04);
    pState->Key[13] = *(PDWORD)(pbKey + KeyIndex + 0x08);
    pState->Key[14] = *(PDWORD)(pbKey + KeyIndex + 0x0C);
    pState->Key[15] = *(PDWORD)(szConstants + 0x0C);

    pState->dwRounds = 20;
}

static int Decrypt(PCASC_SALSA20 pState, LPBYTE pbOutBuffer, LPBYTE pbInBuffer, size_t cbInBuffer)
{
    LPBYTE pbXorValue;
    DWORD KeyMirror[0x10];
    DWORD XorValue[0x10];
    DWORD BlockSize;
    DWORD i;

    // Repeat until we have data to read
    while(cbInBuffer > 0)
    {
        // Create the copy of the key
        memcpy(KeyMirror, pState->Key, sizeof(KeyMirror));

        // Shuffle the key
        for(i = 0; i < pState->dwRounds; i += 2)
        {
            KeyMirror[0x04] ^= Rol32((KeyMirror[0x00] + KeyMirror[0x0C]), 0x07);
            KeyMirror[0x08] ^= Rol32((KeyMirror[0x04] + KeyMirror[0x00]), 0x09);
            KeyMirror[0x0C] ^= Rol32((KeyMirror[0x08] + KeyMirror[0x04]), 0x0D);
            KeyMirror[0x00] ^= Rol32((KeyMirror[0x0C] + KeyMirror[0x08]), 0x12);

            KeyMirror[0x09] ^= Rol32((KeyMirror[0x05] + KeyMirror[0x01]), 0x07);
            KeyMirror[0x0D] ^= Rol32((KeyMirror[0x09] + KeyMirror[0x05]), 0x09);
            KeyMirror[0x01] ^= Rol32((KeyMirror[0x0D] + KeyMirror[0x09]), 0x0D);
            KeyMirror[0x05] ^= Rol32((KeyMirror[0x01] + KeyMirror[0x0D]), 0x12);

            KeyMirror[0x0E] ^= Rol32((KeyMirror[0x0A] + KeyMirror[0x06]), 0x07);
            KeyMirror[0x02] ^= Rol32((KeyMirror[0x0E] + KeyMirror[0x0A]), 0x09);
            KeyMirror[0x06] ^= Rol32((KeyMirror[0x02] + KeyMirror[0x0E]), 0x0D);
            KeyMirror[0x0A] ^= Rol32((KeyMirror[0x06] + KeyMirror[0x02]), 0x12);

            KeyMirror[0x03] ^= Rol32((KeyMirror[0x0F] + KeyMirror[0x0B]), 0x07);
            KeyMirror[0x07] ^= Rol32((KeyMirror[0x03] + KeyMirror[0x0F]), 0x09);
            KeyMirror[0x0B] ^= Rol32((KeyMirror[0x07] + KeyMirror[0x03]), 0x0D);
            KeyMirror[0x0F] ^= Rol32((KeyMirror[0x0B] + KeyMirror[0x07]), 0x12);

            KeyMirror[0x01] ^= Rol32((KeyMirror[0x00] + KeyMirror[0x03]), 0x07);
            KeyMirror[0x02] ^= Rol32((KeyMirror[0x01] + KeyMirror[0x00]), 0x09);
            KeyMirror[0x03] ^= Rol32((KeyMirror[0x02] + KeyMirror[0x01]), 0x0D);
            KeyMirror[0x00] ^= Rol32((KeyMirror[0x03] + KeyMirror[0x02]), 0x12);

            KeyMirror[0x06] ^= Rol32((KeyMirror[0x05] + KeyMirror[0x04]), 0x07);
            KeyMirror[0x07] ^= Rol32((KeyMirror[0x06] + KeyMirror[0x05]), 0x09);
            KeyMirror[0x04] ^= Rol32((KeyMirror[0x07] + KeyMirror[0x06]), 0x0D);
            KeyMirror[0x05] ^= Rol32((KeyMirror[0x04] + KeyMirror[0x07]), 0x12);

            KeyMirror[0x0B] ^= Rol32((KeyMirror[0x0A] + KeyMirror[0x09]), 0x07);
            KeyMirror[0x08] ^= Rol32((KeyMirror[0x0B] + KeyMirror[0x0A]), 0x09);
            KeyMirror[0x09] ^= Rol32((KeyMirror[0x08] + KeyMirror[0x0B]), 0x0D);
            KeyMirror[0x0A] ^= Rol32((KeyMirror[0x09] + KeyMirror[0x08]), 0x12);

            KeyMirror[0x0C] ^= Rol32((KeyMirror[0x0F] + KeyMirror[0x0E]), 0x07);
            KeyMirror[0x0D] ^= Rol32((KeyMirror[0x0C] + KeyMirror[0x0F]), 0x09);
            KeyMirror[0x0E] ^= Rol32((KeyMirror[0x0D] + KeyMirror[0x0C]), 0x0D);
            KeyMirror[0x0F] ^= Rol32((KeyMirror[0x0E] + KeyMirror[0x0D]), 0x12);
        }

        // Set the number of remaining bytes
        pbXorValue = (LPBYTE)XorValue;
        BlockSize = (DWORD)CASCLIB_MIN(cbInBuffer, 0x40);

        // Prepare the XOR constants
        for(i = 0; i < 16; i++)
        {
            XorValue[i] = KeyMirror[i] + pState->Key[i];
        }

        // Decrypt the block
        for(i = 0; i < BlockSize; i++)
        {
            pbOutBuffer[i] = pbInBuffer[i] ^ pbXorValue[i];
        }

        pState->Key[8] = pState->Key[8] + 1;
        if(pState->Key[8] == 0)
            pState->Key[9] = pState->Key[9] + 1;

        // Adjust buffers
        pbOutBuffer += BlockSize;
        pbInBuffer += BlockSize;
        cbInBuffer -= BlockSize;
    }

    return ERROR_SUCCESS;
}

static int Decrypt_Salsa20(LPBYTE pbOutBuffer, LPBYTE pbInBuffer, size_t cbInBuffer, LPBYTE pbKey, DWORD cbKeySize, LPBYTE pbVector)
{
    CASC_SALSA20 SalsaState;

    Initialize(&SalsaState, pbKey, cbKeySize, pbVector);
    return Decrypt(&SalsaState, pbOutBuffer, pbInBuffer, cbInBuffer);
}

//-----------------------------------------------------------------------------
// Public functions

int CascLoadEncryptionKeys(TCascStorage * hs)
{
    size_t nKeyCount = (sizeof(CascKeys) / sizeof(CASC_ENCRYPTION_KEY));
    size_t nMaxItems = nKeyCount + CASC_EXTRA_KEYS;
    int nError;

    // Create fast map of KeyName -> Key
    hs->pEncryptionKeys = Map_Create(nMaxItems, sizeof(ULONGLONG), FIELD_OFFSET(CASC_ENCRYPTION_KEY, KeyName));
    if (hs->pEncryptionKeys == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    // Insert all static keys
    for (size_t i = 0; i < nKeyCount; i++)
        Map_InsertObject(hs->pEncryptionKeys, &CascKeys[i], &CascKeys[i].KeyName);

    // Create array for extra keys
    nError = hs->ExtraKeysList.Create<CASC_ENCRYPTION_KEY>(CASC_EXTRA_KEYS);
    return nError;
}

LPBYTE CascFindKey(TCascStorage * hs, ULONGLONG KeyName)
{
    PCASC_ENCRYPTION_KEY pKey;

    pKey = (PCASC_ENCRYPTION_KEY)Map_FindObject(hs->pEncryptionKeys, &KeyName);
    return (pKey != NULL) ? pKey->Key : NULL;
}

int CascDecrypt(TCascStorage * hs, LPBYTE pbOutBuffer, PDWORD pcbOutBuffer, LPBYTE pbInBuffer, DWORD cbInBuffer, DWORD dwFrameIndex)
{
    ULONGLONG KeyName = 0;
    LPBYTE pbBufferEnd = pbInBuffer + cbInBuffer;
    LPBYTE pbKey;
    DWORD KeyNameSize;
    DWORD dwShift = 0;
    DWORD IVSize;
    BYTE Vector[0x08];
    BYTE EncryptionType;
    int nError;

    // Verify and retrieve the key name size
    if(pbInBuffer >= pbBufferEnd)
        return ERROR_FILE_CORRUPT;
    if(pbInBuffer[0] != 0 && pbInBuffer[0] != 8)
        return ERROR_NOT_SUPPORTED;
    KeyNameSize = *pbInBuffer++;

    // Copy the key name
    if((pbInBuffer + KeyNameSize) >= pbBufferEnd)
        return ERROR_FILE_CORRUPT;
    memcpy(&KeyName, pbInBuffer, KeyNameSize);
    pbInBuffer += KeyNameSize;

    // Verify and retrieve the Vector size
    if(pbInBuffer >= pbBufferEnd)
        return ERROR_FILE_CORRUPT;
    if(pbInBuffer[0] != 4 && pbInBuffer[0] != 8)
        return ERROR_NOT_SUPPORTED;
    IVSize = *pbInBuffer++;

    // Copy the initialization vector
    if((pbInBuffer + IVSize) >= pbBufferEnd)
        return ERROR_FILE_CORRUPT;
    memset(Vector, 0, sizeof(Vector));
    memcpy(Vector, pbInBuffer, IVSize);
    pbInBuffer += IVSize;

    // Verify and retrieve the encryption type
    if(pbInBuffer >= pbBufferEnd)
        return ERROR_FILE_CORRUPT;
    if(pbInBuffer[0] != 'S' && pbInBuffer[0] != 'A')
        return ERROR_NOT_SUPPORTED;
    EncryptionType = *pbInBuffer++;

    // Do we have enough space in the output buffer?
    if((DWORD)(pbBufferEnd - pbInBuffer) > pcbOutBuffer[0])
        return ERROR_INSUFFICIENT_BUFFER;

    // Check if we know the key
    pbKey = CascFindKey(hs, KeyName);
    if(pbKey == NULL)
        return ERROR_FILE_ENCRYPTED;

    // Shuffle the Vector with the block index
    // Note that there's no point to go beyond 32 bits, unless the file has
    // more than 0xFFFFFFFF frames.
    for(size_t i = 0; i < sizeof(dwFrameIndex); i++)
    {
        Vector[i] = Vector[i] ^ (BYTE)((dwFrameIndex >> dwShift) & 0xFF);
        dwShift += 8;
    }

    // Perform the decryption-specific action
    switch(EncryptionType)
    {
        case 'S':   // Salsa20
            nError = Decrypt_Salsa20(pbOutBuffer, pbInBuffer, (pbBufferEnd - pbInBuffer), pbKey, 0x10, Vector);
            if(nError != ERROR_SUCCESS)
                return nError;

            // Supply the size of the output buffer
            pcbOutBuffer[0] = (DWORD)(pbBufferEnd - pbInBuffer);
            return ERROR_SUCCESS;

//      case 'A':   
//          return ERROR_NOT_SUPPORTED;
    }

    assert(false);
    return ERROR_NOT_SUPPORTED;
}

int CascDirectCopy(LPBYTE pbOutBuffer, PDWORD pcbOutBuffer, LPBYTE pbInBuffer, DWORD cbInBuffer)
{
    // Check the buffer size
    if((cbInBuffer - 1) > pcbOutBuffer[0])
        return ERROR_INSUFFICIENT_BUFFER;

    // Copy the data
    memcpy(pbOutBuffer, pbInBuffer, cbInBuffer);
    pcbOutBuffer[0] = cbInBuffer;
    return ERROR_SUCCESS;
}

bool WINAPI CascAddEncryptionKey(HANDLE hStorage, ULONGLONG KeyName, LPBYTE Key)
{
    PCASC_ENCRYPTION_KEY pEncKey;
    TCascStorage * hs;

    // Validate the storage handle
    hs = IsValidCascStorageHandle(hStorage);
    if (hs == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return false;
    }

    // Don't allow more than CASC_EXTRA_KEYS keys
    if (hs->ExtraKeysList.ItemCount() >= CASC_EXTRA_KEYS)
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return false;
    }

    // Insert the key to the array
    pEncKey = (PCASC_ENCRYPTION_KEY)hs->ExtraKeysList.Insert(NULL, 1);
    if (pEncKey == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return false;
    }

    // Fill the key
    memcpy(pEncKey->Key, Key, sizeof(pEncKey->Key));
    pEncKey->KeyName = KeyName;

    // Also insert the key to the map
    return Map_InsertObject(hs->pEncryptionKeys, pEncKey, &pEncKey->KeyName);
}
