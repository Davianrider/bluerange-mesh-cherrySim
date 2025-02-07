////////////////////////////////////////////////////////////////////////////////
// /****************************************************************************
// **
// ** Copyright (C) 2015-2022 M-Way Solutions GmbH
// ** Contact: https://www.blureange.io/licensing
// **
// ** This file is part of the Bluerange/FruityMesh implementation
// **
// ** $BR_BEGIN_LICENSE:GPL-EXCEPT$
// ** Commercial License Usage
// ** Licensees holding valid commercial Bluerange licenses may use this file in
// ** accordance with the commercial license agreement provided with the
// ** Software or, alternatively, in accordance with the terms contained in
// ** a written agreement between them and M-Way Solutions GmbH.
// ** For licensing terms and conditions see https://www.bluerange.io/terms-conditions. For further
// ** information use the contact form at https://www.bluerange.io/contact.
// **
// ** GNU General Public License Usage
// ** Alternatively, this file may be used under the terms of the GNU
// ** General Public License version 3 as published by the Free Software
// ** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
// ** included in the packaging of this file. Please review the following
// ** information to ensure the GNU General Public License requirements will
// ** be met: https://www.gnu.org/licenses/gpl-3.0.html.
// **
// ** $BR_END_LICENSE$
// **
// ****************************************************************************/
////////////////////////////////////////////////////////////////////////////////
#include "gtest/gtest.h"
#include "Utility.h"
#include "CherrySimTester.h"
#include "CherrySimUtils.h"
#include "MultiScheduler.h"
#include "BitMask.h"
#include "SlotStorage.h"
#include <set>

TEST(TestUtility, TestGetIndexForSerial) {
    //The original serial number range had 5 characters
    ASSERT_EQ(Utility::GetIndexForSerial("BBBBB"), 0);
    ASSERT_EQ(Utility::GetIndexForSerial("BBBBC"), 1);
    ASSERT_EQ(Utility::GetIndexForSerial("BBBB9"), 29);
    ASSERT_EQ(Utility::GetIndexForSerial("CDWXY"), 879859);
    ASSERT_EQ(Utility::GetIndexForSerial("QRSTV"), 10084066);
    ASSERT_EQ(Utility::GetIndexForSerial("WPMNB"), 14075400);
    ASSERT_EQ(Utility::GetIndexForSerial("12345"), 17625445);
    ASSERT_EQ(Utility::GetIndexForSerial("99999"), 24299999);

    //Old Asset serial numbers (A should be ignored and handled as a B)
    ASSERT_EQ(Utility::GetIndexForSerial("ABBBB"), 0);
    ASSERT_EQ(Utility::GetIndexForSerial("ABBBC"), 1);

    //The open source range for testing
    ASSERT_EQ(Utility::GetIndexForSerial("FMBBB"), 2673000);
    ASSERT_EQ(Utility::GetIndexForSerial("FM999"), 2699999);

    //The extended range has 7 characters, 5 character serial numbers can always be prefixed with BB if desired
    ASSERT_EQ(Utility::GetIndexForSerial("BB99999"), 24299999);
    ASSERT_EQ(Utility::GetIndexForSerial("BCBBBBB"), 24300000);
    ASSERT_EQ(Utility::GetIndexForSerial("BCBBBBC"), 24300001);
    ASSERT_EQ(Utility::GetIndexForSerial("H62Q56T"), UINT32_MAX);
}

TEST(TestUtility, TestGenerateBeaconSerialForIndex) {
    char buffer[128];
    Utility::GenerateBeaconSerialForIndex(0, buffer); ASSERT_STREQ(buffer, "BBBBB");
    Utility::GenerateBeaconSerialForIndex(1, buffer); ASSERT_STREQ(buffer, "BBBBC");
    Utility::GenerateBeaconSerialForIndex(29, buffer); ASSERT_STREQ(buffer, "BBBB9");
    Utility::GenerateBeaconSerialForIndex(879859, buffer); ASSERT_STREQ(buffer, "CDWXY");
    Utility::GenerateBeaconSerialForIndex(10084066, buffer); ASSERT_STREQ(buffer, "QRSTV");
    Utility::GenerateBeaconSerialForIndex(14075400, buffer); ASSERT_STREQ(buffer, "WPMNB");
    Utility::GenerateBeaconSerialForIndex(17625445, buffer); ASSERT_STREQ(buffer, "12345");
    Utility::GenerateBeaconSerialForIndex(24299999, buffer); ASSERT_STREQ(buffer, "99999");

    //The open source range used for testing
    Utility::GenerateBeaconSerialForIndex(2673000, buffer); ASSERT_STREQ(buffer, "FMBBB");
    Utility::GenerateBeaconSerialForIndex(2699999, buffer); ASSERT_STREQ(buffer, "FM999");

    //The serial numbers of the extended range (7 chars instead of 5)
    Utility::GenerateBeaconSerialForIndex(24300000, buffer); ASSERT_STREQ(buffer, "BCBBBBB"); //first
    Utility::GenerateBeaconSerialForIndex(24300001, buffer); ASSERT_STREQ(buffer, "BCBBBBC"); //second
    Utility::GenerateBeaconSerialForIndex(0x7FFFFFFFUL, buffer); ASSERT_STREQ(buffer, "D8PJQ8K"); //Last serial of the Mway range
    Utility::GenerateBeaconSerialForIndex(UINT32_MAX, buffer); ASSERT_STREQ(buffer, "H62Q56T"); //last

    //The following tests the Vendor range
    VendorSerial serial = {};
    serial.parts.vendorFlag = 1;
    ASSERT_EQ(serial.serialIndex, 0x80000000UL); //Make sure that the bitfields are properly ordered

    //Tests the serial numbers for vendor 0x0000
    serial.parts.vendorId = 0x0000;

    serial.parts.vendorSerialId = 0; //First serial number
    Utility::GenerateBeaconSerialForIndex(serial.serialIndex, buffer); ASSERT_STREQ(buffer, "D8PJQ8L");
    serial.parts.vendorSerialId = 1; //Second serial number
    Utility::GenerateBeaconSerialForIndex(serial.serialIndex, buffer); ASSERT_STREQ(buffer, "D8PJQ8M");
    serial.parts.vendorSerialId = 0x7FFF; //last serial number
    Utility::GenerateBeaconSerialForIndex(serial.serialIndex, buffer); ASSERT_STREQ(buffer, "D8PKYNT");

    //Tests the serial numbers for vendor 0x024D (Our M-Way range)
    serial.parts.vendorId = 0x024D;

    serial.parts.vendorSerialId = 0; //First serial number
    Utility::GenerateBeaconSerialForIndex(serial.serialIndex, buffer); ASSERT_STREQ(buffer, "D9HCK3N");
    serial.parts.vendorSerialId = 1; //Second serial number
    Utility::GenerateBeaconSerialForIndex(serial.serialIndex, buffer); ASSERT_STREQ(buffer, "D9HCK3P");
    serial.parts.vendorSerialId = 0x7FFF; //last serial number
    Utility::GenerateBeaconSerialForIndex(serial.serialIndex, buffer); ASSERT_STREQ(buffer, "D9HDSHW");

    //Make sure the bitfield is properly ordered
    serial.parts.vendorId = 0x0002;
    serial.parts.vendorSerialId = 3;
    ASSERT_EQ(serial.serialIndex, (1 << 31) | (0x2 << 15) | (3));
}

TEST(TestUtility, TestByteToAsciiHex) {
    ASSERT_EQ(Utility::ByteToAsciiHex(0xAB), 0x4241);
    ASSERT_EQ(Utility::ByteToAsciiHex(0x9A), 0x4139);
    ASSERT_EQ(Utility::ByteToAsciiHex(0x12), 0x3231);
}

TEST(TestUtility, TestByteFromAsciiHex) {
    ASSERT_EQ(Utility::ByteFromAsciiHex("01234567", 8), 0x67452301);
    ASSERT_EQ(Utility::ByteFromAsciiHex("89ABCDEF", 8), 0xEFCDAB89);
    ASSERT_EQ(Utility::ByteFromAsciiHex("89abcdef", 8), 0xEFCDAB89);
}

TEST(TestUtility, TestIsPowerOfTwo) {
    for (u32 i = 0; i < 32; i++) {
        ASSERT_TRUE(Utility::IsPowerOfTwo(1ul << i));
    }
    ASSERT_FALSE(Utility::IsPowerOfTwo(3));
    ASSERT_FALSE(Utility::IsPowerOfTwo(5));
    ASSERT_FALSE(Utility::IsPowerOfTwo(6));
    ASSERT_FALSE(Utility::IsPowerOfTwo(7));
    ASSERT_FALSE(Utility::IsPowerOfTwo(9));
    ASSERT_FALSE(Utility::IsPowerOfTwo(10));
    ASSERT_FALSE(Utility::IsPowerOfTwo(11));
    for (u32 i = 3; i < 31; i++) {
        ASSERT_FALSE(Utility::IsPowerOfTwo((1ul << i) - 1));
        ASSERT_FALSE(Utility::IsPowerOfTwo((1ul << i) + 1));
    }
}

TEST(TestUtility, TestGetVersionStringFromInt) {
    char buffer[128];
    Utility::GetVersionStringFromInt(0         , buffer); ASSERT_STREQ(buffer, "0.0.0");
    Utility::GetVersionStringFromInt(1         , buffer); ASSERT_STREQ(buffer, "0.0.1");
    Utility::GetVersionStringFromInt(10        , buffer); ASSERT_STREQ(buffer, "0.0.10");
    Utility::GetVersionStringFromInt(283       , buffer); ASSERT_STREQ(buffer, "0.0.283");
    Utility::GetVersionStringFromInt(3029      , buffer); ASSERT_STREQ(buffer, "0.0.3029");
    Utility::GetVersionStringFromInt(12092     , buffer); ASSERT_STREQ(buffer, "0.1.2092");
    Utility::GetVersionStringFromInt(392348    , buffer); ASSERT_STREQ(buffer, "0.39.2348");
    Utility::GetVersionStringFromInt(9938472   , buffer); ASSERT_STREQ(buffer, "0.993.8472");
    Utility::GetVersionStringFromInt(49284902  , buffer); ASSERT_STREQ(buffer, "4.928.4902");
    Utility::GetVersionStringFromInt(389283203 , buffer); ASSERT_STREQ(buffer, "38.928.3203");
    Utility::GetVersionStringFromInt(4294967295, buffer); ASSERT_STREQ(buffer, "429.496.7295"); //Highest possible version
}

TEST(TestUtility, TestGetRandomInteger) {
    CherrySimTesterConfig testerConfig = CherrySimTester::CreateDefaultTesterConfiguration();
    SimConfiguration simConfig = CherrySimTester::CreateDefaultSimConfiguration();
    simConfig.terminalId = 0;
    //testerConfig.verbose = true;
    simConfig.nodeConfigName.insert({ "prod_sink_nrf52", 1});
    simConfig.nodeConfigName.insert({ "prod_mesh_nrf52", 1});
    CherrySimTester tester = CherrySimTester(testerConfig, simConfig);
    tester.Start();
    NodeIndexSetter setter(0);

    std::set<u32> randoms;
    int amountOfClashes = 0;
    for (int i = 0; i < 10000; i++)
    {
        u32 rand = Utility::GetRandomInteger();
        if (randoms.count(rand) != 0)
        {
            amountOfClashes++;
        }
        else
        {
            randoms.insert(rand);
        }
    }
    //Yes I know its random, but think about it for a second. We generate 10000 random 32 bit integers (4.294.967.296 different values). 
    //Having more than 10 clashes would be surprising, having more than 100 definetly means there is something wrong with the random function.
    ASSERT_TRUE(amountOfClashes < 100);
}

TEST(TestUtility, TestCRC) {
    char data[] = "This is my data and I shall check if its valid or not. Deep-sea fishes are scary!";
    const u32 len = strlen(data);
    ASSERT_EQ(Utility::CalculateCrc8 ((u8*)data, len), 37);
    ASSERT_EQ(Utility::CalculateCrc16((u8*)data, len, nullptr), 58061);
    constexpr u32 expectedCrc32 = 2744420781;
    ASSERT_EQ(Utility::CalculateCrc32((u8*)data, len), expectedCrc32);

    //Calculate the crc32 of only half the data...
    u32 partialCrc32 = Utility::CalculateCrc32((u8*)data, len / 2);
    ASSERT_EQ(partialCrc32, 3310278043);
    //...and then use the partial Crc32 to calculate the full expectedCrc32 with the second half of the data.
    ASSERT_EQ(Utility::CalculateCrc32((u8*)data + len / 2, len / 2 + 1, partialCrc32), expectedCrc32);

    data[40] = 'A'; //Just change something.
    ASSERT_EQ(Utility::CalculateCrc8 ((u8*)data, len), 70);
    ASSERT_EQ(Utility::CalculateCrc16((u8*)data, len, nullptr), 55639);
    ASSERT_EQ(Utility::CalculateCrc32((u8*)data, len), 1322553117);
}

TEST(TestUtility, TestFindLast) {
    char data[] = "This string has many sheeps! The reason for this is that sheeps are cool. sheeps? sheeps! And apples.";
    ASSERT_STREQ(Utility::FindLast(data, "sheep"), "sheeps! And apples.");
    ASSERT_EQ(Utility::FindLast(data, "wolf"), nullptr);
}

TEST(TestUtility, TestAes128BlockEncrypt) {
    Aes128Block msgBlock = {
        0x00, 0x10, 0x20, 0x30,
        0x0A, 0x1A, 0x2A, 0x3A,
        0x0B, 0x1B, 0x2B, 0x3B,
        0x0C, 0x1C, 0x2C, 0x3C,
    };

    Aes128Block key = {
        0xA0, 0xB0, 0xC0, 0xD0,
        0xAA, 0xBA, 0xCA, 0xDA,
        0xAB, 0xBB, 0xCB, 0xDB,
        0xAC, 0xBC, 0xCC, 0xDC,
    };

    Aes128Block encrypted;

    Utility::Aes128BlockEncrypt(&msgBlock, &key, &encrypted);

    ASSERT_EQ(encrypted.data[0 ], 0x2B);
    ASSERT_EQ(encrypted.data[1 ], 0x62);
    ASSERT_EQ(encrypted.data[2 ], 0x7A);
    ASSERT_EQ(encrypted.data[3 ], 0x78);
    ASSERT_EQ(encrypted.data[4 ], 0x3C);
    ASSERT_EQ(encrypted.data[5 ], 0x0E);
    ASSERT_EQ(encrypted.data[6 ], 0xB9);
    ASSERT_EQ(encrypted.data[7 ], 0xBC);
    ASSERT_EQ(encrypted.data[8 ], 0x57);
    ASSERT_EQ(encrypted.data[9 ], 0x8D);
    ASSERT_EQ(encrypted.data[10], 0x7F);
    ASSERT_EQ(encrypted.data[11], 0x69);
    ASSERT_EQ(encrypted.data[12], 0x36);
    ASSERT_EQ(encrypted.data[13], 0xD4);
    ASSERT_EQ(encrypted.data[14], 0x23);
    ASSERT_EQ(encrypted.data[15], 0xC6);
}

TEST(TestUtility, TestXorWords) {
    u32 src1[]    = {  100,  1000, 100000, 324543, 23491291, 20, 1 };
    u32 src2[]    = { 2919, 13282,     10,  10492,    12245, 20, 2 };
    u32 correct[] = { 2819, 12298, 100010, 318275, 23485710,  0, 3 };
    u32 out[sizeof(src1) / sizeof(*src1)];

    Utility::XorWords(src1, src2, sizeof(src1) / sizeof(*src1), out);

    for (size_t i = 0; i < sizeof(src1) / sizeof(*src1); i++)
    {
        ASSERT_EQ(correct[i], out[i]);
    }
}


TEST(TestUtility, TestXorBytes) {
    u8 src1[]    = { 100, 200, 20,   0,  1,  4, 1, 100 };
    u8 src2[]    = {  50,  27, 10, 100, 25, 13, 2, 100 };
    u8 correct[] = {  86, 211, 30, 100, 24,  9, 3,   0 };
    u8 out[sizeof(src1) / sizeof(*src1)];

    Utility::XorBytes(src1, src2, sizeof(src1) / sizeof(*src1), out);

    for (size_t i = 0; i < sizeof(src1) / sizeof(*src1); i++)
    {
        ASSERT_EQ(correct[i], out[i]);
    }
}

TEST(TestUtility, TestSwapBytes)
{
    {
        //With unswapped middle
        u8 toSwap[] = { 100, 10, 20, 50, 20 };
        u8 swapped[] = { 20, 50, 20, 10, 100 };

        Utility::SwapBytes(toSwap, 5);

        for (size_t i = 0; i < sizeof(toSwap) / sizeof(*toSwap); i++)
        {
            ASSERT_EQ(toSwap[i], swapped[i]);
        }
    }

    {
        //With swapped middle
        u8 toSwap[] = { 100, 10, 20,  50 };
        u8 swapped[] = { 50, 20, 10, 100 };

        Utility::SwapBytes(toSwap, 4);

        for (int i = 0; i < 4; i++)
        {
            ASSERT_EQ(toSwap[i], swapped[i]);
        }
    }
}


TEST(TestUtility, TestCompareMem)
{
    for (int i = 0; i <= 255; i++)
    {
        u8 *data = new u8[i + 1];
        CheckedMemset(data, i, i + 1);
        ASSERT_TRUE (Utility::CompareMem((u8)i,       data, i + 1));
        ASSERT_FALSE(Utility::CompareMem((u8)(i + 1), data, i + 1));
        delete[] data;
    }
}

TEST(TestUtility, TestToUpperCase)
{
    char data [1024];

    strcpy(data, "apple");  Utility::ToUpperCase(data); ASSERT_STREQ(data, "APPLE");
    strcpy(data, "APPLE");  Utility::ToUpperCase(data); ASSERT_STREQ(data, "APPLE");
    strcpy(data, "BaNaNa"); Utility::ToUpperCase(data); ASSERT_STREQ(data, "BANANA");
    strcpy(data, "9 kiwi"); Utility::ToUpperCase(data); ASSERT_STREQ(data, "9 KIWI");
}

TEST(TestUtility, TestConfigurableBackOff)
{
    bool result = false;

    u32 backOffIvsDs[] = { SEC_TO_DS(30), SEC_TO_DS(1 * 60), SEC_TO_DS(2 * 60), SEC_TO_DS(30 * 60) };

    //appTimer changed from 200 to 499, Interval counting was stated at 200
    //Should NOT TRIGGER as we did not approach the first interval yet
    result = Utility::ShouldBackOffIvTrigger(499, 300, 200, backOffIvsDs, sizeof(backOffIvsDs));
    ASSERT_EQ(result, 0);

    //appTimer changed from 200 to 500, Interval counting was stated at 200
    //Should TRIGGER as we exactly hit the first interval of 300
    result = Utility::ShouldBackOffIvTrigger(500, 300, 200, backOffIvsDs, sizeof(backOffIvsDs));
    ASSERT_EQ(result, 1);

    //appTimer changed from 201 to 501, Interval counting was stated at 200
    //Should TRIGGER as we exceeded the first interval of 300
    result = Utility::ShouldBackOffIvTrigger(501, 300, 200, backOffIvsDs, sizeof(backOffIvsDs));
    ASSERT_EQ(result, 1);

    //appTimer changed from 600 to 700, Interval counting was stated at 200
    //Should NOT TRIGGER as the first interval should only trigger once we are at 200 startTime + 300 at which the interval should trigger
    result = Utility::ShouldBackOffIvTrigger(700, 100, 200, backOffIvsDs, sizeof(backOffIvsDs));
    ASSERT_EQ(result, 0);

    //appTimer changed from 600 to 700
    //Should NOT TRIGGER as the second interval at 600 was already hit the last time when the timer was at 600
    result = Utility::ShouldBackOffIvTrigger(700, 100, 0, backOffIvsDs, sizeof(backOffIvsDs));
    ASSERT_EQ(result, 0);

    //appTimer changed from 500 to 700
    //Should TRIGGER as the second interval at 600 is hit inbetween
    result = Utility::ShouldBackOffIvTrigger(700, 200, 0, backOffIvsDs, sizeof(backOffIvsDs));
    ASSERT_EQ(result, 1);

    u32 passedTimeDs = 2;
    u32 startTimeDs = Utility::INVALID_BACKOFF_START_TIME;

    u32 TEST_START_TIME = 500;

    //We simulate an appTimer that always increases by 2 deciseconds
    //It should trigger at some points, but only once at these points
    for (u32 appTimerDs = 0; appTimerDs < SEC_TO_DS(30 * 60) + 1000; appTimerDs += passedTimeDs)
    {

        if (appTimerDs == TEST_START_TIME) startTimeDs = appTimerDs;

        result = Utility::ShouldBackOffIvTrigger(appTimerDs, passedTimeDs, startTimeDs, backOffIvsDs, sizeof(backOffIvsDs));

        if (appTimerDs == TEST_START_TIME + SEC_TO_DS(30)) ASSERT_EQ(result, 1);
        else if (appTimerDs == TEST_START_TIME + SEC_TO_DS(1 * 60)) ASSERT_EQ(result, 1);
        else if (appTimerDs == TEST_START_TIME + SEC_TO_DS(2 * 60)) ASSERT_EQ(result, 1);
        else if (appTimerDs == TEST_START_TIME + SEC_TO_DS(30 * 60)) ASSERT_EQ(result, 1);
        else ASSERT_EQ(result, 0);

        //printf("AppTimerDs %u, PassedTimeDs %u, Trigger %u" EOL, appTimerDs, passedTimeDs, result);
    }
}

// Sample string (without NULL terminator) for which hash was calcualted using online tools
static const char text[6] = {'F', 'r', 'i', 'e', 'n', 'd'}; // Friend
u8 ecdsa_hash[] =
{
    0xac, 0xd8, 0xf6, 0x64, 0x40, 0x16, 0x25, 0x11,
    0xef, 0xe9, 0xb2, 0x4a, 0x26, 0x77, 0xba, 0xb2,
    0x5, 0x18, 0xf4, 0x4d, 0x6f, 0x23, 0x3b, 0x8,
    0xe9, 0x38, 0x7d, 0x97, 0x47, 0x60, 0x1f, 0x18,
};

// Test vectors
// These were generated using python script which can be found in comment section of BR-1291.
// There are 3 different keys/signature.
u8 ecdsa_private_key[][32] =
{
    {
        0xac, 0x34, 0xd6, 0x73, 0x6a, 0x9c, 0x5c, 0xf5,
        0xce, 0x47, 0xbd, 0x9a, 0x7b, 0x3f, 0x8e, 0xb6,
        0x16, 0x3, 0x94, 0x77, 0x4b, 0xe6, 0x2f, 0x98,
        0x44, 0x95, 0x1, 0xe0, 0x82, 0xf9, 0xcc, 0x41,
    },
    {
        0x92, 0xff, 0x6f, 0x13, 0xa9, 0x5c, 0x9e, 0x98,
        0xd5, 0xfd, 0xf, 0x19, 0x21, 0xe5, 0x71, 0x53,
        0x7a, 0x1f, 0x21, 0xd3, 0xae, 0xb9, 0x9d, 0x2f,
        0x72, 0x1f, 0xae, 0x23, 0x5b, 0x9a, 0x46, 0xa8,
    },
    {
        0x90, 0xd2, 0xcb, 0xc, 0x83, 0xb5, 0xb4, 0x3f,
        0x5d, 0x61, 0xc9, 0xda, 0xc1, 0x83, 0xc9, 0xd,
        0x8, 0x5f, 0xfa, 0xbc, 0x72, 0xfa, 0x70, 0x2,
        0x2d, 0x42, 0x7, 0xa3, 0xd, 0xab, 0xd4, 0x2f,
    }
};

u8 ecdsa_public_key[][64] = 
{
    {
        0xc6, 0xed, 0x4, 0x94, 0x57, 0x81, 0x7c, 0xe8,
        0x91, 0xc3, 0x81, 0x13, 0xc9, 0xc7, 0xb6, 0x64,
        0xbc, 0x3c, 0xd2, 0x55, 0xb7, 0xe5, 0xd9, 0x6f,
        0x8e, 0xae, 0x8a, 0xf8, 0xa3, 0xe6, 0x5f, 0xa6,
        0x27, 0xc0, 0x1a, 0x55, 0x2f, 0xdb, 0x70, 0x5b,
        0x8b, 0xa, 0xce, 0x2b, 0x33, 0x21, 0xac, 0xc,
        0xd2, 0x72, 0x2d, 0x63, 0x1b, 0x84, 0x1, 0xcd,
        0xad, 0x10, 0x19, 0xe3, 0x90, 0xd7, 0xbf, 0x64,
    },
    {
        0x15, 0xe, 0x46, 0x1a, 0x6c, 0xfd, 0x8c, 0x80,
        0x18, 0x3a, 0xe5, 0xa5, 0x26, 0xe9, 0xd9, 0x2c,
        0xc, 0xb4, 0xc9, 0x1f, 0x1c, 0xcb, 0xb9, 0xde,
        0xe1, 0x6b, 0x84, 0x5b, 0x29, 0x38, 0x89, 0x84,
        0xf2, 0xd8, 0x85, 0x10, 0x1d, 0xe2, 0x54, 0x27,
        0xcf, 0x8f, 0x95, 0xbe, 0xa8, 0x9c, 0x35, 0x7e,
        0x45, 0x59, 0x38, 0x4a, 0x5c, 0x34, 0x55, 0xf4,
        0x98, 0x60, 0xd2, 0xf6, 0xb5, 0x4, 0x36, 0xd5,
    },
    {
        0xd6, 0x35, 0x49, 0x71, 0x2b, 0x68, 0xba, 0x90,
        0x3d, 0x8a, 0xa6, 0xfe, 0x9f, 0x8f, 0xf2, 0x99,
        0x3c, 0x58, 0xea, 0xeb, 0x61, 0x85, 0x6b, 0x20,
        0x5, 0x32, 0x6d, 0xb2, 0xc6, 0x8f, 0xd7, 0x2c,
        0x43, 0x1b, 0x8d, 0x76, 0xb6, 0x5e, 0x91, 0x11,
        0x8c, 0xfa, 0x61, 0xf1, 0x3a, 0x0, 0xbb, 0xf7,
        0x7f, 0x0, 0xc3, 0x9b, 0x22, 0x17, 0x95, 0x85,
        0x22, 0x7d, 0xca, 0x99, 0x75, 0xd5, 0x73, 0xff,
    }

};

u8 ecdsa_signature[][64] =
{
    {
        0x40, 0xe2, 0xae, 0xa2, 0xea, 0x81, 0xd6, 0x35,
        0x11, 0xa, 0xe9, 0x55, 0x7a, 0xa0, 0xad, 0x85,
        0xf8, 0xf7, 0x76, 0xec, 0x4e, 0x4b, 0xea, 0xb0,
        0xf8, 0x91, 0xe5, 0xbb, 0x76, 0x8, 0x16, 0x6a,
        0x30, 0xcd, 0xd5, 0x9, 0x65, 0x5c, 0xac, 0xa2,
        0x61, 0x85, 0xc6, 0xab, 0x66, 0x17, 0xaa, 0xc6,
        0x5a, 0xcb, 0x93, 0x7, 0x8a, 0xc2, 0x2d, 0x54,
        0x4f, 0xc5, 0xd6, 0x40, 0x6e, 0xef, 0x6d, 0x16,
    },
    {
        0x6d, 0xd1, 0xc8, 0xa4, 0xe4, 0xe8, 0x6e, 0xb4,
        0x52, 0x73, 0xca, 0x31, 0xc2, 0x40, 0x6f, 0x35,
        0xfb, 0xed, 0xb, 0xf6, 0x2, 0x2b, 0x4f, 0xeb,
        0x8f, 0x53, 0xdf, 0x91, 0x8d, 0xf1, 0x6, 0xb2,
        0xc6, 0xa2, 0x47, 0x8b, 0x2, 0x38, 0xe2, 0xeb,
        0xf3, 0x9, 0x6d, 0xa2, 0x90, 0x43, 0x6c, 0xa7,
        0x10, 0x66, 0xcc, 0x74, 0xf6, 0x4d, 0x2d, 0x45,
        0x7e, 0xde, 0xc8, 0xa8, 0xe6, 0x8, 0x58, 0x25,
    },
    {
        0x5d, 0x63, 0x71, 0x82, 0x86, 0xb6, 0xfd, 0xf8,
        0x19, 0x36, 0x95, 0xc4, 0xf2, 0x62, 0x74, 0x49,
        0xa8, 0x5d, 0x66, 0x72, 0x62, 0xd8, 0x25, 0x98,
        0xb4, 0x45, 0xaa, 0x6a, 0x61, 0xfd, 0xde, 0x81,
        0xf, 0xc7, 0xfe, 0xf5, 0x27, 0xab, 0xc8, 0x93,
        0x44, 0x12, 0xeb, 0x6a, 0x41, 0x14, 0x38, 0xc1,
        0xe6, 0x26, 0x7a, 0xd, 0x33, 0x89, 0x30, 0xd5,
        0xc4, 0x87, 0xe9, 0xfc, 0x1b, 0x1f, 0x47, 0x66,
    }
};

TEST(TestUtility, TestECDSAHash)
{
    u8 hash[32];
    Utility::SHA256HashCalculate((u8*)text, sizeof(text), hash);
    for (size_t j = 0; j < sizeof(hash); j++)
    {
        ASSERT_EQ(ecdsa_hash[j], hash[j]);
    }
}

TEST(TestUtility, TestECDSASign)
{
    u8 signature[64];

    for (u8 i = 0; i < 3; i++)
    {
        ErrorType ret = Utility::ECDSASecp256r1Sign(ecdsa_private_key[i], ecdsa_hash, sizeof(ecdsa_hash), signature);
        ASSERT_EQ(ret, ErrorType::SUCCESS);
        ret = Utility::ECDSASecp256r1Verify(ecdsa_public_key[i], ecdsa_hash, sizeof(ecdsa_hash), signature);
        ASSERT_EQ(ret, ErrorType::SUCCESS);
    }
}

TEST(TestUtility, TestECDSAVerify)
{
    for (u8 i = 0; i < 3; i++)
    {
        ErrorType ret = Utility::ECDSASecp256r1Verify(ecdsa_public_key[i], ecdsa_hash, sizeof(ecdsa_hash), ecdsa_signature[i]);
        ASSERT_EQ(ret, ErrorType::SUCCESS);
    }
}

TEST(TestUtility, TestMultischeduler)
{
    MultiScheduler<u8, 10> ms;
    ASSERT_FALSE(ms.isEventReady());
    ms.addEvent(17, 4, 0, EventTimeType::RELATIVE);
    ms.addEvent(18, 5, 0, EventTimeType::RELATIVE);
    ms.addEvent(19, 7, 0, EventTimeType::RELATIVE);
    ms.addEvent(20, 7, 0, EventTimeType::RELATIVE);
    ms.addEvent(21, 3, 0, EventTimeType::RELATIVE);

    // The internal state should now be:
    // 21 3
    // 17 4
    // 18 5
    // 19 7
    // 20 7

    ms.advanceTime(1);
    ASSERT_FALSE(ms.isEventReady());
    ms.advanceTime(1);
    ASSERT_FALSE(ms.isEventReady());
    ms.advanceTime(1);
    ASSERT_TRUE(ms.isEventReady());
    ASSERT_EQ(ms.getAndReenter(), 21);
    ASSERT_FALSE(ms.isEventReady());
    ms.advanceTime(1);
    ASSERT_TRUE(ms.isEventReady());
    ASSERT_EQ(ms.getAndReenter(), 17);
    ASSERT_FALSE(ms.isEventReady());
    ms.advanceTime(1);
    ASSERT_TRUE(ms.isEventReady());
    ASSERT_EQ(ms.getAndReenter(), 18);
    ASSERT_FALSE(ms.isEventReady());
    ms.advanceTime(1);
    ASSERT_TRUE(ms.isEventReady());
    ASSERT_EQ(ms.getAndReenter(), 21);
    ASSERT_FALSE(ms.isEventReady());
    ms.advanceTime(1);
    ASSERT_TRUE(ms.isEventReady());
    ASSERT_EQ(ms.getAndReenter(), 19);
    ASSERT_TRUE(ms.isEventReady());
    ASSERT_EQ(ms.getAndReenter(), 20);
    ASSERT_FALSE(ms.isEventReady());
    ms.advanceTime(1);
    ASSERT_TRUE(ms.isEventReady());
    ASSERT_EQ(ms.getAndReenter(), 17);
    ASSERT_FALSE(ms.isEventReady());

    // Let the scheduler run a bit...
    for (u32 i = 0; i < 1000; i++)
    {
        ms.advanceTime(1);
        while (ms.isEventReady())
        {
            ms.getAndReenter();
        }
    }
    // ... and then make sure that we still get all 5 numbers from time to time.
    std::set<u32> set;
    for (u32 i = 0; i < 8; i++)
    {
        ms.advanceTime(1);
        while (ms.isEventReady())
        {
            set.insert(ms.getAndReenter());
        }
    }
    ASSERT_EQ(set.count(17), 1);
    ASSERT_EQ(set.count(18), 1);
    ASSERT_EQ(set.count(19), 1);
    ASSERT_EQ(set.count(20), 1);
    ASSERT_EQ(set.count(21), 1);

    // Remove two events...
    ms.removeEvent(18);
    ms.removeEvent(21);
    // ... and make sure they don't occure anymore.
    set.clear();
    for (u32 i = 0; i < 8; i++)
    {
        ms.advanceTime(1);
        while (ms.isEventReady())
        {
            set.insert(ms.getAndReenter());
        }
    }
    ASSERT_EQ(set.count(17), 1);
    ASSERT_EQ(set.count(18), 0);
    ASSERT_EQ(set.count(19), 1);
    ASSERT_EQ(set.count(20), 1);
    ASSERT_EQ(set.count(21), 0);

    {
        MultiScheduler<u8, 3> ms;
        Exceptions::DisableDebugBreakOnException disabler;
        ASSERT_THROW(ms.addEvent(9, 0, 0, EventTimeType::RELATIVE), IllegalArgumentException);
        ms.addEvent(0, 1, 0, EventTimeType::RELATIVE);
        ms.addEvent(1, 1, 0, EventTimeType::RELATIVE);
        ms.addEvent(2, 1, 0, EventTimeType::RELATIVE);
        ASSERT_THROW(ms.addEvent(10, 17, 0, EventTimeType::RELATIVE), BufferTooSmallException);
    }
    {
        
        MultiScheduler<u8, 3> ms;
        Exceptions::DisableDebugBreakOnException disabler;
        Exceptions::ExceptionDisabler<BufferTooSmallException> btse;
        Exceptions::ExceptionDisabler<IllegalArgumentException> iae;
        // Make sure it doesn't crash.
        ms.addEvent( 9,  0, 0, EventTimeType::RELATIVE);
        ms.addEvent( 0,  1, 0, EventTimeType::RELATIVE);
        ms.addEvent( 1,  1, 0, EventTimeType::RELATIVE);
        ms.addEvent( 2,  1, 0, EventTimeType::RELATIVE);
        ms.addEvent(10, 17, 0, EventTimeType::RELATIVE);
    }
}

constexpr u32 numBits = 20;
void checkEquality(const BitMask<numBits>& bm, const bool* checkArr)
{
    for (u32 i = 0; i < numBits; i++)
    {
        ASSERT_EQ(bm.get(i), checkArr[i]);
    }
}

TEST(TestUtility, TestBitMask)
{
    BitMask<numBits> bm;
    bool checkArr[numBits] = {};
#define setBit(n, val) { bm.set((n), (val)); checkArr[(n)] = (val); }
    checkEquality(bm, checkArr);
    setBit(8, true);
    checkEquality(bm, checkArr);
    setBit(17, true);
    checkEquality(bm, checkArr);
    setBit(4, true);
    checkEquality(bm, checkArr);

    MersenneTwister rnd(1);
    for (u32 i = 0; i < 1000; i++)
    {
        u32 index = rnd.NextU32(0, numBits - 1);
        setBit(index, !checkArr[index]);
        checkEquality(bm, checkArr);
    }

    {
        Exceptions::DisableDebugBreakOnException disabler;
        ASSERT_THROW(bm.get(20), IllegalArgumentException);
        ASSERT_THROW(bm.set(20, true), IllegalArgumentException);
    }

    {
        Exceptions::DisableDebugBreakOnException disabler;
        Exceptions::ExceptionDisabler<IllegalArgumentException> iae;
        ASSERT_FALSE(bm.get(20));
        bm.set(20, true); // Make sure it doesn't crash.
    }

    for (u32 i = 0; i < numBits; i++)
    {
        bm.set(i, false);
    }
    bm.set(0, true);
    bm.set(9, true);
    bm.set(18, true);
    ASSERT_EQ(bm.getNumberBytes(), 3);
    ASSERT_EQ(bm.getRaw()[0], 1);
    ASSERT_EQ(bm.getRaw()[1], 2);
    ASSERT_EQ(bm.getRaw()[2], 4);
#undef setBit
}

void fillSlot(SlotStorage<20, 512>& slotStorage, u32 slot, u32 size, u32 offset)
{
    u8* s = slotStorage.get(slot);
    for (u32 i = 0; i < size; i++)
    {
        s[i] = u8((i + offset) % 256);
    }
}

void checkSlotIntegrity(SlotStorage<20, 512>& slotStorage, u32 slot, u32 size, u32 offset)
{
    u8* s = slotStorage.get(slot);
    for (u32 i = 0; i < size; i++)
    {
        const u8 expectedValue = u8((i + offset) % 256);
        const u8 actualValue = s[i];
        ASSERT_EQ(actualValue, expectedValue);
    }
}

TEST(TestUtility, TestSlotStorage)
{
    SlotStorage<20, 512> slotStorage;
    ASSERT_FALSE(slotStorage.isSlotRegistered(0));
    ASSERT_FALSE(slotStorage.isSlotRegistered(1));
    ASSERT_FALSE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_EQ(slotStorage.spaceLeft(), 512);
    ASSERT_EQ(slotStorage.spaceUsed(), 0);

    ASSERT_TRUE(slotStorage.isEnoughSpaceLeftForSlotWithSize(512));
    ASSERT_FALSE(slotStorage.isEnoughSpaceLeftForSlotWithSize(513));

    slotStorage.registerSlot(0, 64);
    ASSERT_TRUE(slotStorage.isSlotRegistered(0));
    ASSERT_FALSE(slotStorage.isSlotRegistered(1));
    ASSERT_FALSE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_EQ(slotStorage.spaceLeft(), 448);
    ASSERT_EQ(slotStorage.spaceUsed(), 64);
    fillSlot(slotStorage, 0, 64, 10);

    slotStorage.registerSlot(1, 32);
    ASSERT_TRUE(slotStorage.isSlotRegistered(0));
    ASSERT_TRUE(slotStorage.isSlotRegistered(1));
    ASSERT_FALSE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_EQ(slotStorage.spaceLeft(), 416);
    ASSERT_EQ(slotStorage.spaceUsed(), 96);
    fillSlot(slotStorage, 1, 32, 17);

    slotStorage.registerSlot(2, 128);
    ASSERT_TRUE(slotStorage.isSlotRegistered(0));
    ASSERT_TRUE(slotStorage.isSlotRegistered(1));
    ASSERT_TRUE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_EQ(slotStorage.spaceLeft(), 288);
    ASSERT_EQ(slotStorage.spaceUsed(), 224);
    fillSlot(slotStorage, 2, 128, 5);

    checkSlotIntegrity(slotStorage, 0,  64, 10);
    checkSlotIntegrity(slotStorage, 1,  32, 17);
    checkSlotIntegrity(slotStorage, 2, 128,  5);

    slotStorage.unregisterSlot(1);
    ASSERT_TRUE(slotStorage.isSlotRegistered(0));
    ASSERT_FALSE(slotStorage.isSlotRegistered(1));
    ASSERT_TRUE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_EQ(slotStorage.spaceLeft(), 320);
    ASSERT_EQ(slotStorage.spaceUsed(), 192);
    checkSlotIntegrity(slotStorage, 0,  64, 10);
    checkSlotIntegrity(slotStorage, 2, 128,  5);

    slotStorage.registerSlot(1, 17);
    checkSlotIntegrity(slotStorage, 0, 64, 10);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    fillSlot(slotStorage, 1, 17, 133);
    ASSERT_TRUE(slotStorage.isSlotRegistered(0));
    ASSERT_TRUE(slotStorage.isSlotRegistered(1));
    ASSERT_TRUE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_EQ(slotStorage.spaceLeft(), 303);
    ASSERT_EQ(slotStorage.spaceUsed(), 209);
    checkSlotIntegrity(slotStorage, 0, 64, 10);
    checkSlotIntegrity(slotStorage, 1, 17, 133);
    checkSlotIntegrity(slotStorage, 2, 128, 5);

    {
        Exceptions::DisableDebugBreakOnException disabler;
        ASSERT_THROW(slotStorage.registerSlot(5, 304), BufferTooSmallException);
        ASSERT_TRUE(slotStorage.isSlotRegistered(0));
        ASSERT_TRUE(slotStorage.isSlotRegistered(1));
        ASSERT_TRUE(slotStorage.isSlotRegistered(2));
        ASSERT_FALSE(slotStorage.isSlotRegistered(3));
        ASSERT_FALSE(slotStorage.isSlotRegistered(5));
        ASSERT_EQ(slotStorage.spaceLeft(), 303);
        ASSERT_EQ(slotStorage.spaceUsed(), 209);
        checkSlotIntegrity(slotStorage, 0, 64, 10);
        checkSlotIntegrity(slotStorage, 1, 17, 133);
        checkSlotIntegrity(slotStorage, 2, 128, 5);
    }

    slotStorage.registerSlot(5, 303);
    ASSERT_TRUE(slotStorage.isSlotRegistered(0));
    ASSERT_TRUE(slotStorage.isSlotRegistered(1));
    ASSERT_TRUE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_TRUE(slotStorage.isSlotRegistered(5));
    ASSERT_EQ(slotStorage.spaceLeft(), 0);
    ASSERT_EQ(slotStorage.spaceUsed(), 512);
    checkSlotIntegrity(slotStorage, 0, 64, 10);
    checkSlotIntegrity(slotStorage, 1, 17, 133);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    fillSlot(slotStorage, 5, 303, 29);
    checkSlotIntegrity(slotStorage, 0, 64, 10);
    checkSlotIntegrity(slotStorage, 1, 17, 133);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    checkSlotIntegrity(slotStorage, 5, 303, 29);

    slotStorage.unregisterSlot(0);
    ASSERT_FALSE(slotStorage.isSlotRegistered(0));
    ASSERT_TRUE(slotStorage.isSlotRegistered(1));
    ASSERT_TRUE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_TRUE(slotStorage.isSlotRegistered(5));
    ASSERT_EQ(slotStorage.spaceLeft(), 64);
    ASSERT_EQ(slotStorage.spaceUsed(), 448);
    checkSlotIntegrity(slotStorage, 1, 17, 133);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    checkSlotIntegrity(slotStorage, 5, 303, 29);

    // Registering slot 1 again, with same size. The contents should be cleared
    slotStorage.registerSlot(1, 17);
    ASSERT_FALSE(slotStorage.isSlotRegistered(0));
    ASSERT_TRUE(slotStorage.isSlotRegistered(1));
    ASSERT_TRUE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_TRUE(slotStorage.isSlotRegistered(5));
    ASSERT_EQ(slotStorage.spaceLeft(), 64);
    ASSERT_EQ(slotStorage.spaceUsed(), 448);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    checkSlotIntegrity(slotStorage, 5, 303, 29);
    fillSlot(slotStorage, 1, 17, 63);
    checkSlotIntegrity(slotStorage, 1, 17, 63);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    checkSlotIntegrity(slotStorage, 5, 303, 29);

    // Registering slot 1 again, with smaller size. The contents should be cleared
    slotStorage.registerSlot(1, 15);
    ASSERT_FALSE(slotStorage.isSlotRegistered(0));
    ASSERT_TRUE(slotStorage.isSlotRegistered(1));
    ASSERT_TRUE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_TRUE(slotStorage.isSlotRegistered(5));
    ASSERT_EQ(slotStorage.spaceLeft(), 66);
    ASSERT_EQ(slotStorage.spaceUsed(), 446);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    checkSlotIntegrity(slotStorage, 5, 303, 29);
    fillSlot(slotStorage, 1, 15, 52);
    checkSlotIntegrity(slotStorage, 1, 15, 52);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    checkSlotIntegrity(slotStorage, 5, 303, 29);

    // Registering slot 1 again, with smaller size. The contents should be cleared
    slotStorage.registerSlot(1, 81);
    ASSERT_FALSE(slotStorage.isSlotRegistered(0));
    ASSERT_TRUE(slotStorage.isSlotRegistered(1));
    ASSERT_TRUE(slotStorage.isSlotRegistered(2));
    ASSERT_FALSE(slotStorage.isSlotRegistered(3));
    ASSERT_TRUE(slotStorage.isSlotRegistered(5));
    ASSERT_EQ(slotStorage.spaceLeft(), 0);
    ASSERT_EQ(slotStorage.spaceUsed(), 512);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    checkSlotIntegrity(slotStorage, 5, 303, 29);
    fillSlot(slotStorage, 1, 81, 93);
    checkSlotIntegrity(slotStorage, 1, 81, 93);
    checkSlotIntegrity(slotStorage, 2, 128, 5);
    checkSlotIntegrity(slotStorage, 5, 303, 29);
}

TEST(TestUtility, TestSlotStorageErrorCases)
{
    {
        SlotStorage<20, 512> slotStorage;
        Exceptions::DisableDebugBreakOnException disabler;
        ASSERT_THROW(slotStorage.get(0), IllegalArgumentException); // Slot is empty.
        ASSERT_THROW(slotStorage.get(20), IllegalArgumentException); // Slot doesn't exist.
        ASSERT_THROW(slotStorage.isSlotRegistered(20), IllegalArgumentException); // Slot doesn't exist.
        ASSERT_THROW(slotStorage.getSizeOfSlot(0), IllegalArgumentException); // Slot is empty.
        slotStorage.registerSlot(0, 512);
        ASSERT_THROW(slotStorage.registerSlot(0, 513), BufferTooSmallException);
        ASSERT_THROW(slotStorage.registerSlot(1, 1), BufferTooSmallException);
    }
    {
        SlotStorage<20, 512> slotStorage;
        Exceptions::DisableDebugBreakOnException disabler;
        Exceptions::ExceptionDisabler<IllegalArgumentException> iae;
        ASSERT_EQ(slotStorage.get(0), nullptr); // Slot is empty.
        ASSERT_EQ(slotStorage.get(20), nullptr); // Slot doesn't exist.
        ASSERT_EQ(slotStorage.isSlotRegistered(20), false); // Slot doesn't exist.
        ASSERT_EQ(slotStorage.getSizeOfSlot(0), 0xFFFF); // Slot is empty.
        slotStorage.registerSlot(0, 512);
        Exceptions::ExceptionDisabler<BufferTooSmallException> btse;
        ASSERT_EQ(slotStorage.registerSlot(0, 513), nullptr);
        ASSERT_EQ(slotStorage.getSizeOfSlot(0), 512);
        ASSERT_EQ(slotStorage.registerSlot(1, 1), nullptr);
    }
}