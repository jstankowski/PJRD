/*
    SPDX-FileCopyrightText: 2024-2025 Jakub Stankowski <jakub.stankowski@put.poznan.pl>
    SPDX-License-Identifier: BSD-3-Clause
*/

#include "xJPEG_Arithm.h"
#include "xJPEG_EntropyUtils.h"

namespace PMBB_NAMESPACE::JPEG {

//=====================================================================================================================================================================================
// xArithmCoreModel
//=====================================================================================================================================================================================
const uint8 xArithmCoreModel::c_NextState[2 * c_NumProbIdx][2] = //[m_State][BinValue]
{
  {   2,   3 },
  {   2,   3 },
  {   4,  28 },
  {  29,   5 },
  {   6,  32 },
  {  33,   7 },
  {   8,  36 },
  {  37,   9 },
  {  10,  40 },
  {  41,  11 },
  {  12,  46 },
  {  47,  13 },
  {  14,  50 },
  {  51,  15 },
  {  16,  56 },
  {  57,  17 },
  {  18,  60 },
  {  61,  19 },
  {  20,  66 },
  {  67,  21 },
  {  22,  70 },
  {  71,  23 },
  {  24,  18 },
  {  19,  25 },
  {  26,  20 },
  {  21,  27 },
  {  26,  24 },
  {  25,  27 },
  {  30,  31 },
  {  30,  31 },
  {  32,  72 },
  {  73,  33 },
  {  34,  76 },
  {  77,  35 },
  {  36,  78 },
  {  79,  37 },
  {  38,  80 },
  {  81,  39 },
  {  40,  84 },
  {  85,  41 },
  {  42,  86 },
  {  87,  43 },
  {  44,  90 },
  {  91,  45 },
  {  46,  92 },
  {  93,  47 },
  {  48,  96 },
  {  97,  49 },
  {  50,  98 },
  {  99,  51 },
  {  52, 102 },
  { 103,  53 },
  {  54, 104 },
  { 105,  55 },
  {  56, 108 },
  { 109,  57 },
  {  58, 112 },
  { 113,  59 },
  {  60, 114 },
  { 115,  61 },
  {  62, 118 },
  { 119,  63 },
  {  64, 120 },
  { 121,  65 },
  {  66, 124 },
  { 125,  67 },
  {  68, 126 },
  { 127,  69 },
  {  70,  64 },
  {  65,  71 },
  {  18,  66 },
  {  67,  19 },
  {  74,  75 },
  {  74,  75 },
  {  76, 128 },
  { 129,  77 },
  {  78, 130 },
  { 131,  79 },
  {  80, 134 },
  { 135,  81 },
  {  82, 136 },
  { 137,  83 },
  {  84, 138 },
  { 139,  85 },
  {  86, 140 },
  { 141,  87 },
  {  88, 144 },
  { 145,  89 },
  {  90, 146 },
  { 147,  91 },
  {  92, 148 },
  { 149,  93 },
  {  94, 150 },
  { 151,  95 },
  {  96, 154 },
  { 155,  97 },
  {  98, 156 },
  { 157,  99 },
  { 100, 158 },
  { 159, 101 },
  { 102,  96 },
  {  97, 103 },
  { 104, 100 },
  { 101, 105 },
  { 106, 100 },
  { 101, 107 },
  { 108, 102 },
  { 103, 109 },
  { 110, 104 },
  { 105, 111 },
  { 112, 106 },
  { 107, 113 },
  { 114, 108 },
  { 109, 115 },
  { 116, 110 },
  { 111, 117 },
  { 118, 112 },
  { 113, 119 },
  { 120, 114 },
  { 115, 121 },
  { 122, 116 },
  { 117, 123 },
  { 124, 118 },
  { 119, 125 },
  { 126, 122 },
  { 123, 127 },
  {  64, 122 },
  { 123,  65 },
  { 130, 131 },
  { 130, 131 },
  { 132, 160 },
  { 161, 133 },
  { 134, 162 },
  { 163, 135 },
  { 136, 164 },
  { 165, 137 },
  { 138, 166 },
  { 167, 139 },
  { 140, 168 },
  { 169, 141 },
  { 142, 172 },
  { 173, 143 },
  { 144, 174 },
  { 175, 145 },
  { 146, 174 },
  { 175, 147 },
  { 148, 144 },
  { 145, 149 },
  { 150, 144 },
  { 145, 151 },
  { 152, 148 },
  { 149, 153 },
  { 154, 148 },
  { 149, 155 },
  { 156, 150 },
  { 151, 157 },
  { 158, 154 },
  { 155, 159 },
  {  96, 154 },
  { 155,  97 },
  { 162, 161 },
  { 160, 163 },
  { 164, 176 },
  { 177, 165 },
  { 166, 178 },
  { 179, 167 },
  { 168, 180 },
  { 181, 169 },
  { 170, 182 },
  { 183, 171 },
  { 172, 184 },
  { 185, 173 },
  { 174, 186 },
  { 187, 175 },
  { 142, 172 },
  { 173, 143 },
  { 178, 177 },
  { 176, 179 },
  { 180, 190 },
  { 191, 181 },
  { 182, 192 },
  { 193, 183 },
  { 184, 194 },
  { 195, 185 },
  { 186, 198 },
  { 199, 187 },
  { 188, 198 },
  { 199, 189 },
  { 172, 186 },
  { 187, 173 },
  { 192, 191 },
  { 190, 193 },
  { 194, 202 },
  { 203, 195 },
  { 196, 204 },
  { 205, 197 },
  { 198, 206 },
  { 207, 199 },
  { 200, 208 },
  { 209, 201 },
  { 186, 198 },
  { 199, 187 },
  { 204, 210 },
  { 211, 205 },
  { 206, 212 },
  { 213, 207 },
  { 208, 214 },
  { 215, 209 },
  { 198, 206 },
  { 207, 199 },
  { 212, 211 },
  { 210, 213 },
  { 214, 216 },
  { 217, 215 },
  { 206, 218 },
  { 219, 207 },
  { 218, 220 },
  { 221, 219 },
  { 214, 222 },
  { 223, 215 },
  { 222, 221 },
  { 220, 223 },
  { 218, 224 },
  { 225, 219 },
  { 222, 225 },
  { 224, 223 },
};

#if PMBB_JPEG_ARITHM_AS_IN_STANDARD
const uint8 xArithmCoreModel::c_NextIndex[c_NumProbIdx][3] =
{
  //NextIndexLPS, NextIndexMPS, SwitchMPS
  {   1,   1, 1 }, //   0
  {  14,   2, 0 }, //   1
  {  16,   3, 0 }, //   2
  {  18,   4, 0 }, //   3
  {  20,   5, 0 }, //   4
  {  23,   6, 0 }, //   5
  {  25,   7, 0 }, //   6
  {  28,   8, 0 }, //   7
  {  30,   9, 0 }, //   8
  {  33,  10, 0 }, //   9
  {  35,  11, 0 }, //  10
  {   9,  12, 0 }, //  11
  {  10,  13, 0 }, //  12
  {  12,  13, 0 }, //  13
  {  15,  15, 1 }, //  14
  {  36,  16, 0 }, //  15
  {  38,  17, 0 }, //  16
  {  39,  18, 0 }, //  17
  {  40,  19, 0 }, //  18
  {  42,  20, 0 }, //  19
  {  43,  21, 0 }, //  20
  {  45,  22, 0 }, //  21
  {  46,  23, 0 }, //  22
  {  48,  24, 0 }, //  23
  {  49,  25, 0 }, //  24
  {  51,  26, 0 }, //  25
  {  52,  27, 0 }, //  26
  {  54,  28, 0 }, //  27
  {  56,  29, 0 }, //  28
  {  57,  30, 0 }, //  29
  {  59,  31, 0 }, //  30
  {  60,  32, 0 }, //  31
  {  62,  33, 0 }, //  32
  {  63,  34, 0 }, //  33
  {  32,  35, 0 }, //  34
  {  33,   9, 0 }, //  35
  {  37,  37, 1 }, //  36
  {  64,  38, 0 }, //  37
  {  65,  39, 0 }, //  38
  {  67,  40, 0 }, //  39
  {  68,  41, 0 }, //  40
  {  69,  42, 0 }, //  41
  {  70,  43, 0 }, //  42
  {  72,  44, 0 }, //  43
  {  73,  45, 0 }, //  44
  {  74,  46, 0 }, //  45
  {  75,  47, 0 }, //  46
  {  77,  48, 0 }, //  47
  {  78,  49, 0 }, //  48
  {  79,  50, 0 }, //  49
  {  48,  51, 0 }, //  50
  {  50,  52, 0 }, //  51
  {  50,  53, 0 }, //  52
  {  51,  54, 0 }, //  53
  {  52,  55, 0 }, //  54
  {  53,  56, 0 }, //  55
  {  54,  57, 0 }, //  56
  {  55,  58, 0 }, //  57
  {  56,  59, 0 }, //  58
  {  57,  60, 0 }, //  59
  {  58,  61, 0 }, //  60
  {  59,  62, 0 }, //  61
  {  61,  63, 0 }, //  62
  {  61,  32, 0 }, //  63
  {  65,  65, 1 }, //  64
  {  80,  66, 0 }, //  65
  {  81,  67, 0 }, //  66
  {  82,  68, 0 }, //  67
  {  83,  69, 0 }, //  68
  {  84,  70, 0 }, //  69
  {  86,  71, 0 }, //  70
  {  87,  72, 0 }, //  71
  {  87,  73, 0 }, //  72
  {  72,  74, 0 }, //  73
  {  72,  75, 0 }, //  74
  {  74,  76, 0 }, //  75
  {  74,  77, 0 }, //  76
  {  75,  78, 0 }, //  77
  {  77,  79, 0 }, //  78
  {  77,  48, 0 }, //  79
  {  80,  81, 1 }, //  80
  {  88,  82, 0 }, //  81
  {  89,  83, 0 }, //  82
  {  90,  84, 0 }, //  83
  {  91,  85, 0 }, //  84
  {  92,  86, 0 }, //  85
  {  93,  87, 0 }, //  86
  {  86,  71, 0 }, //  87
  {  88,  89, 1 }, //  88
  {  95,  90, 0 }, //  89
  {  96,  91, 0 }, //  90
  {  97,  92, 0 }, //  91
  {  99,  93, 0 }, //  92
  {  99,  94, 0 }, //  93
  {  93,  86, 0 }, //  94
  {  95,  96, 1 }, //  95
  { 101,  97, 0 }, //  96
  { 102,  98, 0 }, //  97
  { 103,  99, 0 }, //  98
  { 104, 100, 0 }, //  99
  {  99,  93, 0 }, // 100
  { 105, 102, 0 }, // 101
  { 106, 103, 0 }, // 102
  { 107, 104, 0 }, // 103
  { 103,  99, 0 }, // 104
  { 105, 106, 1 }, // 105
  { 108, 107, 0 }, // 106
  { 109, 103, 0 }, // 107
  { 110, 109, 0 }, // 108
  { 111, 107, 0 }, // 109
  { 110, 111, 1 }, // 110
  { 112, 109, 0 }, // 111
  { 112, 111, 1 }, // 112
};

void xArithmCoreModel::updateStdMPS()
{
  uint32 ProbIdx     = getProbIdx();
  uint32 NextProbIdx = c_NextIndex[ProbIdx][1];
  setProbIdx(NextProbIdx);
}
void xArithmCoreModel::updateStdLPS()
{
  uint32 ProbIdx   = getProbIdx();
  uint32 SwitchMPS = c_NextIndex[ProbIdx][2];
  if(SwitchMPS)
  {
    uint32 MPS    = getMPS();
    uint32 NewMPS = 1 - MPS;
    setMPS(NewMPS);
  }
  uint32 NextProbIdx = c_NextIndex[ProbIdx][0];
  setProbIdx(NextProbIdx);
}
#endif

//=====================================================================================================================================================================================
// xArithmCoreCmn
//=====================================================================================================================================================================================
const uint16 xArithmCoreCmn::c_Qe[] =
{
  0x5a1d, //   0
  0x2586, //   1
  0x1114, //   2
  0x080b, //   3
  0x03d8, //   4
  0x01da, //   5
  0x00e5, //   6
  0x006f, //   7
  0x0036, //   8
  0x001a, //   9
  0x000d, //  10
  0x0006, //  11
  0x0003, //  12
  0x0001, //  13
  0x5a7f, //  14
  0x3f25, //  15
  0x2cf2, //  16
  0x207c, //  17
  0x17b9, //  18
  0x1182, //  19
  0x0cef, //  20
  0x09a1, //  21
  0x072f, //  22
  0x055c, //  23
  0x0406, //  24
  0x0303, //  25
  0x0240, //  26
  0x01b1, //  27
  0x0144, //  28
  0x00f5, //  29
  0x00b7, //  30
  0x008a, //  31
  0x0068, //  32
  0x004e, //  33
  0x003b, //  34
  0x002c, //  35
  0x5ae1, //  36
  0x484c, //  37
  0x3a0d, //  38
  0x2ef1, //  39
  0x261f, //  40
  0x1f33, //  41
  0x19a8, //  42
  0x1518, //  43
  0x1177, //  44
  0x0e74, //  45
  0x0bfb, //  46
  0x09f8, //  47
  0x0861, //  48
  0x0706, //  49
  0x05cd, //  50
  0x04de, //  51
  0x040f, //  52
  0x0363, //  53
  0x02d4, //  54
  0x025c, //  55
  0x01f8, //  56
  0x01a4, //  57
  0x0160, //  58
  0x0125, //  59
  0x00f6, //  60
  0x00cb, //  61
  0x00ab, //  62
  0x008f, //  63
  0x5b12, //  64
  0x4d04, //  65
  0x412c, //  66
  0x37d8, //  67
  0x2fe8, //  68
  0x293c, //  69
  0x2379, //  70
  0x1edf, //  71
  0x1aa9, //  72
  0x174e, //  73
  0x1424, //  74
  0x119c, //  75
  0x0f6b, //  76
  0x0d51, //  77
  0x0bb6, //  78
  0x0a40, //  79
  0x5832, //  80
  0x4d1c, //  81
  0x438e, //  82
  0x3bdd, //  83
  0x34ee, //  84
  0x2eae, //  85
  0x299a, //  86
  0x2516, //  87
  0x5570, //  88
  0x4ca9, //  89
  0x44d9, //  90
  0x3e22, //  91
  0x3824, //  92
  0x32b4, //  93
  0x2e17, //  94
  0x56a8, //  95
  0x4f46, //  96
  0x47e5, //  97
  0x41cf, //  98
  0x3c3d, //  99
  0x375e, // 100
  0x5231, // 101
  0x4c0f, // 102
  0x4639, // 103
  0x415e, // 104
  0x5627, // 105
  0x50e7, // 106
  0x4b85, // 107
  0x5597, // 108
  0x504f, // 109
  0x5a10, // 110
  0x5522, // 111
  0x59eb, // 112
};

const uint32 xArithmCoreCmn::c_EntropyBits[] =
{
  0x08000, //   0 Bin==MPS -->  32768 ( 1.00000)
  0x08000, //   0 Bin!=MPS -->  32768 ( 1.00000)
  0x02a9a, //   1 Bin==MPS -->  10906 ( 0.33281)
  0x123bc, //   1 Bin!=MPS -->  74684 ( 2.27919)
  0x0122e, //   2 Bin==MPS -->   4654 ( 0.14204)
  0x1b519, //   2 Bin!=MPS --> 111897 ( 3.41483)
  0x00857, //   3 Bin==MPS -->   2135 ( 0.06516)
  0x24027, //   3 Bin!=MPS --> 147495 ( 4.50118)
  0x003f0, //   4 Bin==MPS -->   1008 ( 0.03077)
  0x2c880, //   4 Bin!=MPS --> 182400 ( 5.56639)
  0x001e3, //   5 Bin==MPS -->    483 ( 0.01474)
  0x34f62, //   5 Bin!=MPS --> 216930 ( 6.62017)
  0x000e9, //   6 Bin==MPS -->    233 ( 0.00710)
  0x3d5b9, //   6 Bin!=MPS --> 251321 ( 7.66971)
  0x00071, //   7 Bin==MPS -->    113 ( 0.00344)
  0x45b75, //   7 Bin!=MPS --> 285557 ( 8.71449)
  0x00037, //   8 Bin==MPS -->     55 ( 0.00167)
  0x4e084, //   8 Bin!=MPS --> 319620 ( 9.75402)
  0x0001a, //   9 Bin==MPS -->     26 ( 0.00080)
  0x5677c, //   9 Bin!=MPS --> 354172 (10.80847)
  0x0000d, //  10 Bin==MPS -->     13 ( 0.00040)
  0x5e77c, //  10 Bin!=MPS --> 386940 (11.80847)
  0x00006, //  11 Bin==MPS -->      6 ( 0.00019)
  0x67644, //  11 Bin!=MPS --> 423492 (12.92395)
  0x00003, //  12 Bin==MPS -->      3 ( 0.00009)
  0x6f644, //  12 Bin!=MPS --> 456260 (13.92395)
  0x00001, //  13 Bin==MPS -->      1 ( 0.00003)
  0x7c124, //  13 Bin!=MPS --> 508196 (15.50891)
  0x07ed7, //  14 Bin==MPS -->  32471 ( 0.99093)
  0x0812b, //  14 Bin!=MPS -->  33067 ( 1.00913)
  0x04e9c, //  15 Bin==MPS -->  20124 ( 0.61414)
  0x0c3a0, //  15 Bin!=MPS -->  50080 ( 1.52832)
  0x03454, //  16 Bin==MPS -->  13396 ( 0.40882)
  0x10268, //  16 Bin!=MPS -->  66152 ( 2.01881)
  0x02446, //  17 Bin==MPS -->   9286 ( 0.28340)
  0x13e5e, //  17 Bin!=MPS -->  81502 ( 2.48723)
  0x019c5, //  18 Bin==MPS -->   6597 ( 0.20132)
  0x17869, //  18 Bin!=MPS -->  96361 ( 2.94072)
  0x012aa, //  19 Bin==MPS -->   4778 ( 0.14580)
  0x1b082, //  19 Bin!=MPS --> 110722 ( 3.37898)
  0x00d9a, //  20 Bin==MPS -->   3482 ( 0.10626)
  0x1e86e, //  20 Bin!=MPS --> 125038 ( 3.81586)
  0x00a08, //  21 Bin==MPS -->   2568 ( 0.07836)
  0x21eeb, //  21 Bin!=MPS --> 138987 ( 4.24154)
  0x0076e, //  22 Bin==MPS -->   1902 ( 0.05805)
  0x25505, //  22 Bin!=MPS --> 152837 ( 4.66420)
  0x00584, //  23 Bin==MPS -->   1412 ( 0.04309)
  0x28b1e, //  23 Bin!=MPS --> 166686 ( 5.08684)
  0x00420, //  24 Bin==MPS -->   1056 ( 0.03223)
  0x2c010, //  24 Bin!=MPS --> 180240 ( 5.50048)
  0x00314, //  25 Bin==MPS -->    788 ( 0.02405)
  0x2f58c, //  25 Bin!=MPS --> 193932 ( 5.91832)
  0x0024c, //  26 Bin==MPS -->    588 ( 0.01793)
  0x32b64, //  26 Bin!=MPS --> 207716 ( 6.33898)
  0x001b9, //  27 Bin==MPS -->    441 ( 0.01346)
  0x36016, //  27 Bin!=MPS --> 221206 ( 6.75069)
  0x0014a, //  28 Bin==MPS -->    330 ( 0.01006)
  0x395a4, //  28 Bin!=MPS --> 234916 ( 7.16906)
  0x000f9, //  29 Bin==MPS -->    249 ( 0.00760)
  0x3c940, //  29 Bin!=MPS --> 248128 ( 7.57227)
  0x000ba, //  30 Bin==MPS -->    186 ( 0.00567)
  0x3ff21, //  30 Bin!=MPS --> 261921 ( 7.99321)
  0x0008c, //  31 Bin==MPS -->    140 ( 0.00428)
  0x43340, //  31 Bin!=MPS --> 275264 ( 8.40038)
  0x0006a, //  32 Bin==MPS -->    106 ( 0.00322)
  0x4677c, //  32 Bin!=MPS --> 288636 ( 8.80847)
  0x0004f, //  33 Bin==MPS -->     79 ( 0.00242)
  0x49c9c, //  33 Bin!=MPS --> 302236 ( 9.22351)
  0x0003c, //  34 Bin==MPS -->     60 ( 0.00183)
  0x4d029, //  34 Bin!=MPS --> 315433 ( 9.62627)
  0x0002d, //  35 Bin==MPS -->     45 ( 0.00136)
  0x50655, //  35 Bin!=MPS --> 329301 (10.04948)
  0x07f9d, //  36 Bin==MPS -->  32669 ( 0.99697)
  0x08063, //  36 Bin!=MPS -->  32867 ( 1.00304)
  0x05d64, //  37 Bin==MPS -->  23908 ( 0.72960)
  0x0aaa1, //  37 Bin!=MPS -->  43681 ( 1.33305)
  0x046df, //  38 Bin==MPS -->  18143 ( 0.55367)
  0x0d328, //  38 Bin!=MPS -->  54056 ( 1.64967)
  0x03709, //  39 Bin==MPS -->  14089 ( 0.42997)
  0x0fa62, //  39 Bin!=MPS -->  64098 ( 1.95612)
  0x02b5d, //  40 Bin==MPS -->  11101 ( 0.33879)
  0x120d1, //  40 Bin!=MPS -->  73937 ( 2.25639)
  0x022b2, //  41 Bin==MPS -->   8882 ( 0.27106)
  0x145d2, //  41 Bin!=MPS -->  83410 ( 2.54547)
  0x01c09, //  42 Bin==MPS -->   7177 ( 0.21903)
  0x169f1, //  42 Bin!=MPS -->  92657 ( 2.82767)
  0x016bb, //  43 Bin==MPS -->   5819 ( 0.17757)
  0x18e1a, //  43 Bin!=MPS --> 101914 ( 3.11017)
  0x0129d, //  44 Bin==MPS -->   4765 ( 0.14542)
  0x1b0f7, //  44 Bin!=MPS --> 110839 ( 3.38253)
  0x00f44, //  45 Bin==MPS -->   3908 ( 0.11928)
  0x1d3eb, //  45 Bin!=MPS --> 119787 ( 3.65560)
  0x00c90, //  46 Bin==MPS -->   3216 ( 0.09816)
  0x1f691, //  46 Bin!=MPS --> 128657 ( 3.92630)
  0x00a65, //  47 Bin==MPS -->   2661 ( 0.08120)
  0x21883, //  47 Bin!=MPS --> 137347 ( 4.19150)
  0x008b2, //  48 Bin==MPS -->   2226 ( 0.06794)
  0x23898, //  48 Bin!=MPS --> 145560 ( 4.44215)
  0x00743, //  49 Bin==MPS -->   1859 ( 0.05673)
  0x2592f, //  49 Bin!=MPS --> 153903 ( 4.69673)
  0x005fa, //  50 Bin==MPS -->   1530 ( 0.04669)
  0x27c80, //  50 Bin!=MPS --> 162944 ( 4.97266)
  0x00500, //  51 Bin==MPS -->   1280 ( 0.03908)
  0x29ce8, //  51 Bin!=MPS --> 171240 ( 5.22582)
  0x00429, //  52 Bin==MPS -->   1065 ( 0.03251)
  0x2be74, //  52 Bin!=MPS --> 179828 ( 5.48793)
  0x00377, //  53 Bin==MPS -->    887 ( 0.02708)
  0x2dfe0, //  53 Bin!=MPS --> 188384 ( 5.74902)
  0x002e4, //  54 Bin==MPS -->    740 ( 0.02258)
  0x30129, //  54 Bin!=MPS --> 196905 ( 6.00906)
  0x00268, //  55 Bin==MPS -->    616 ( 0.01881)
  0x322a0, //  55 Bin!=MPS --> 205472 ( 6.27050)
  0x00202, //  56 Bin==MPS -->    514 ( 0.01568)
  0x3440c, //  56 Bin!=MPS --> 214028 ( 6.53163)
  0x001ac, //  57 Bin==MPS -->    428 ( 0.01305)
  0x365b8, //  57 Bin!=MPS --> 222648 ( 6.79466)
  0x00166, //  58 Bin==MPS -->    358 ( 0.01093)
  0x38655, //  58 Bin!=MPS --> 230997 ( 7.04948)
  0x0012a, //  59 Bin==MPS -->    298 ( 0.00909)
  0x3a836, //  59 Bin!=MPS --> 239670 ( 7.31415)
  0x000fa, //  60 Bin==MPS -->    250 ( 0.00763)
  0x3c880, //  60 Bin!=MPS --> 247936 ( 7.56639)
  0x000ce, //  61 Bin==MPS -->    206 ( 0.00629)
  0x3ebfa, //  61 Bin!=MPS --> 257018 ( 7.84357)
  0x000ae, //  62 Bin==MPS -->    174 ( 0.00530)
  0x40ba8, //  62 Bin!=MPS --> 265128 ( 8.09106)
  0x00091, //  63 Bin==MPS -->    145 ( 0.00443)
  0x42cad, //  63 Bin!=MPS --> 273581 ( 8.34904)
  0x08000, //  64 Bin==MPS -->  32768 ( 1.00000)
  0x08000, //  64 Bin!=MPS -->  32768 ( 1.00000)
  0x0657f, //  65 Bin==MPS -->  25983 ( 0.79295)
  0x09ef4, //  65 Bin!=MPS -->  40692 ( 1.24183)
  0x051c8, //  66 Bin==MPS -->  20936 ( 0.63893)
  0x0bdca, //  66 Bin!=MPS -->  48586 ( 1.48273)
  0x0439d, //  67 Bin==MPS -->  17309 ( 0.52823)
  0x0da51, //  67 Bin!=MPS -->  55889 ( 1.70559)
  0x0385c, //  68 Bin==MPS -->  14428 ( 0.44030)
  0x0f6a0, //  68 Bin!=MPS -->  63136 ( 1.92677)
  0x02f67, //  69 Bin==MPS -->  12135 ( 0.37032)
  0x11252, //  69 Bin!=MPS -->  70226 ( 2.14313)
  0x02800, //  70 Bin==MPS -->  10240 ( 0.31250)
  0x12e1d, //  70 Bin!=MPS -->  77341 ( 2.36027)
  0x0224c, //  71 Bin==MPS -->   8780 ( 0.26793)
  0x147c6, //  71 Bin!=MPS -->  83910 ( 2.56072)
  0x01d3a, //  72 Bin==MPS -->   7482 ( 0.22832)
  0x162db, //  72 Bin!=MPS -->  90843 ( 2.77230)
  0x01948, //  73 Bin==MPS -->   6472 ( 0.19752)
  0x17bb2, //  73 Bin!=MPS -->  97202 ( 2.96636)
  0x015a4, //  74 Bin==MPS -->   5540 ( 0.16906)
  0x196a4, //  74 Bin!=MPS --> 104100 ( 3.17687)
  0x012c7, //  75 Bin==MPS -->   4807 ( 0.14669)
  0x1af71, //  75 Bin!=MPS --> 110449 ( 3.37064)
  0x01055, //  76 Bin==MPS -->   4181 ( 0.12760)
  0x1c7fc, //  76 Bin!=MPS --> 116732 ( 3.56237)
  0x00e05, //  77 Bin==MPS -->   3589 ( 0.10953)
  0x1e30b, //  77 Bin!=MPS --> 123659 ( 3.77378)
  0x00c46, //  78 Bin==MPS -->   3142 ( 0.09588)
  0x1fac5, //  78 Bin!=MPS --> 129733 ( 3.95912)
  0x00ab2, //  79 Bin==MPS -->   2738 ( 0.08356)
  0x21360, //  79 Bin!=MPS --> 136032 ( 4.15136)
  0x07a43, //  80 Bin==MPS -->  31299 ( 0.95516)
  0x085ec, //  80 Bin!=MPS -->  34284 ( 1.04628)
  0x065a9, //  81 Bin==MPS -->  26025 ( 0.79423)
  0x09ebb, //  81 Bin!=MPS -->  40635 ( 1.24007)
  0x05595, //  82 Bin==MPS -->  21909 ( 0.66862)
  0x0b729, //  82 Bin!=MPS -->  46889 ( 1.43092)
  0x04996, //  83 Bin==MPS -->  18838 ( 0.57490)
  0x0cd7b, //  83 Bin!=MPS -->  52603 ( 1.60531)
  0x03f67, //  84 Bin==MPS -->  16231 ( 0.49532)
  0x0e436, //  84 Bin!=MPS -->  58422 ( 1.78290)
  0x036ae, //  85 Bin==MPS -->  13998 ( 0.42718)
  0x0fb6a, //  85 Bin!=MPS -->  64362 ( 1.96419)
  0x02fe2, //  86 Bin==MPS -->  12258 ( 0.37408)
  0x110af, //  86 Bin!=MPS -->  69807 ( 2.13034)
  0x02a0b, //  87 Bin==MPS -->  10763 ( 0.32846)
  0x125e7, //  87 Bin!=MPS -->  75239 ( 2.29611)
  0x074eb, //  88 Bin==MPS -->  29931 ( 0.91342)
  0x08bca, //  88 Bin!=MPS -->  35786 ( 1.09211)
  0x064e0, //  89 Bin==MPS -->  25824 ( 0.78808)
  0x09fcf, //  89 Bin!=MPS -->  40911 ( 1.24850)
  0x057ae, //  90 Bin==MPS -->  22446 ( 0.68499)
  0x0b3a8, //  90 Bin!=MPS -->  45992 ( 1.40357)
  0x04d0c, //  91 Bin==MPS -->  19724 ( 0.60192)
  0x0c69c, //  91 Bin!=MPS -->  50844 ( 1.55163)
  0x0440c, //  92 Bin==MPS -->  17420 ( 0.53163)
  0x0d956, //  92 Bin!=MPS -->  55638 ( 1.69794)
  0x03c3f, //  93 Bin==MPS -->  15423 ( 0.47068)
  0x0ec26, //  93 Bin!=MPS -->  60454 ( 1.84491)
  0x035e0, //  94 Bin==MPS -->  13792 ( 0.42091)
  0x0fdc4, //  94 Bin!=MPS -->  64964 ( 1.98253)
  0x07742, //  95 Bin==MPS -->  30530 ( 0.93172)
  0x0892d, //  95 Bin!=MPS -->  35117 ( 1.07168)
  0x06982, //  96 Bin==MPS -->  27010 ( 0.82427)
  0x0999e, //  96 Bin!=MPS -->  39326 ( 1.20014)
  0x05cb7, //  97 Bin==MPS -->  23735 ( 0.72433)
  0x0aba9, //  97 Bin!=MPS -->  43945 ( 1.34110)
  0x052ca, //  98 Bin==MPS -->  21194 ( 0.64680)
  0x0bbff, //  98 Bin!=MPS -->  48127 ( 1.46870)
  0x04a27, //  99 Bin==MPS -->  18983 ( 0.57933)
  0x0cc54, //  99 Bin!=MPS -->  52308 ( 1.59630)
  0x042eb, // 100 Bin==MPS -->  17131 ( 0.52280)
  0x0dbe6, // 100 Bin!=MPS -->  56294 ( 1.71795)
  0x06ed2, // 101 Bin==MPS -->  28370 ( 0.86579)
  0x092f1, // 101 Bin!=MPS -->  37617 ( 1.14799)
  0x063d3, // 102 Bin==MPS -->  25555 ( 0.77987)
  0x0a143, // 102 Bin!=MPS -->  41283 ( 1.25987)
  0x059ef, // 103 Bin==MPS -->  23023 ( 0.70261)
  0x0b001, // 103 Bin!=MPS -->  45057 ( 1.37504)
  0x05217, // 104 Bin==MPS -->  21015 ( 0.64134)
  0x0bd3d, // 104 Bin!=MPS -->  48445 ( 1.47841)
  0x0764a, // 105 Bin==MPS -->  30282 ( 0.92412)
  0x08a40, // 105 Bin!=MPS -->  35392 ( 1.08009)
  0x06c74, // 106 Bin==MPS -->  27764 ( 0.84730)
  0x095dd, // 106 Bin!=MPS -->  38365 ( 1.17080)
  0x062e3, // 107 Bin==MPS -->  25315 ( 0.77256)
  0x0a294, // 107 Bin!=MPS -->  41620 ( 1.27013)
  0x07535, // 108 Bin==MPS -->  30005 ( 0.91569)
  0x08b76, // 108 Bin!=MPS -->  35702 ( 1.08954)
  0x06b60, // 109 Bin==MPS -->  27488 ( 0.83886)
  0x09739, // 109 Bin!=MPS -->  38713 ( 1.18143)
  0x07df8, // 110 Bin==MPS -->  32248 ( 0.98412)
  0x0820e, // 110 Bin!=MPS -->  33294 ( 1.01605)
  0x07456, // 111 Bin==MPS -->  29782 ( 0.90888)
  0x08c73, // 111 Bin!=MPS -->  35955 ( 1.09727)
  0x07dae, // 112 Bin==MPS -->  32174 ( 0.98186)
  0x0825a, // 112 Bin!=MPS -->  33370 ( 1.01837)
};

//=====================================================================================================================================================================================
//xArithmCoreDec
//=====================================================================================================================================================================================

void xArithmCoreDec::start()
{
  //init registers 
  m_A = 0x10000;
  m_C = 0;
  //fetch first 2 bytes
  xByteIn();
  m_C <<= 8;
  xByteIn();
  m_C <<= 8;
  m_CT = 0;
}
void xArithmCoreDec::finish()
{
  //nothing to finich
}
#if PMBB_JPEG_ARITHM_AS_IN_STANDARD
uint32 xArithmCoreDec::decodeBinMP_as_in_standard(xArithmCoreModel& CtxModel)
{
  const uint32 MPS     = CtxModel.getMPS    ();
  const uint32 ProbIdx = CtxModel.getProbIdx();
  const uint32 Qe      = c_Qe[ProbIdx];

  m_A -= Qe;

  uint32 D  = std::numeric_limits<uint32>::max();
  uint32 CX = m_C >> 16;

  if(CX < m_A)
  {
    if(m_A < 0x8000)
    {
      D = xCond_MPS_exchange(CtxModel);
      xRenorm();
    }
    else
    {
      D = MPS;
    }
  }
  else
  {
    D = xCond_LPS_exchange(CtxModel);
    xRenorm();
  }

  return D;
}
uint32 xArithmCoreDec::xCond_LPS_exchange(xArithmCoreModel& CtxModel)
{
  const uint32 MPS     = CtxModel.getMPS    ();
  const uint32 ProbIdx = CtxModel.getProbIdx();
  const uint32 Qe      = c_Qe[ProbIdx];

  uint32 D = std::numeric_limits<uint32>::max();

  uint32 CX = m_C >> 16;

  if(m_A < Qe)
  {
    D = MPS;
    CX -= m_A;
    m_A = Qe;
    CtxModel.updateStdMPS();
  }
  else
  {
    D = 1 - MPS;
    CX -= m_A;
    m_A = Qe;
    CtxModel.updateStdLPS();
  }
  m_C = (m_C & 0x0000FFFF) | (CX << 16);
  return D;
}
uint32 xArithmCoreDec::xCond_MPS_exchange(xArithmCoreModel& CtxModel)
{
  const uint32 MPS     = CtxModel.getMPS    ();
  const uint32 ProbIdx = CtxModel.getProbIdx();
  const uint32 Qe      = c_Qe[ProbIdx];

  uint32 D = std::numeric_limits<uint32>::max();

  if(m_A < Qe)
  {
    D = 1 - MPS;
    CtxModel.updateStdLPS();
  }
  else
  {
    D = MPS;
    CtxModel.updateStdMPS();
  }
  return D;
}
#endif //PMBB_JPEG_ARITHM_AS_IN_STANDARD
uint32 xArithmCoreDec::decodeBinMP(xArithmCoreModel& CtxModel)
{
  const uint32 MPS     = CtxModel.getMPS();
  const uint32 ProbIdx = CtxModel.getProbIdx();
  const uint32 Qe      = c_Qe[ProbIdx];

  m_A -= Qe;

  uint32 D  = std::numeric_limits<uint32>::max();
  uint32 CX = m_C >> 16;

  if(CX < (uint32)m_A) //MPS path
  {
    if(!(m_A < 0x8000)) { return MPS; }
    D = (uint32)m_A < Qe ? 1 - MPS : MPS;
  }
  else //LPS patch
  {
    D = (uint32)m_A < Qe ? MPS : 1 - MPS;
    CX -= m_A;
    m_A = Qe;
    m_C = (m_C & 0x0000FFFF) | (CX << 16);
  }
  CtxModel.updateTab(D);
  xRenorm();

  return D;
}
uint32 xArithmCoreDec::decodeBinEP()
{
  m_A -= c_EqualProbQe;

  uint32 D  = std::numeric_limits<uint32>::max();
  uint32 CX = m_C >> 16;

  if(CX < (uint32)m_A) //MPS path
  {
    if(!((uint32)m_A < 0x8000)) { return c_EqualProbMPS; }
    D = (uint32)m_A < c_EqualProbQe ? 1 - c_EqualProbMPS : c_EqualProbMPS;
  }
  else //LPS patch
  {
    D = (uint32)m_A < c_EqualProbQe ? c_EqualProbMPS : 1 - c_EqualProbMPS;
    CX -= m_A;
    m_A = c_EqualProbQe;
    m_C = (m_C & 0x0000FFFF) | (CX << 16);
  }
  xRenorm();

  return D;
}
void xArithmCoreDec::xRenorm()
{
  do
  {
    if(m_CT == 0)
    {
      xByteIn();
      m_CT = 8;
    }
    m_A <<= 1;
    m_C <<= 1;
    m_CT--;
  }
  while(m_A < 0x8000);
}
void xArithmCoreDec::xByteIn()
{
  if(m_ByteBuffer->getDataSize() == 0) { return; } //out of buffer - behave like read zero
  uint32 B = m_ByteBuffer->extractU8();
#if PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
  if(B == 0xFF)
  {
    //unstuff 0
    uint32 B2 = m_ByteBuffer->extractU8();
    if(B2 == 0)
    {
      m_C = m_C | 0xFF00;
    }
    else
    {
      //some marker... no more data to read
      assert(0);
    }
  }
  else
#endif //PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
  {
    m_C += B << 8;
  }
}

//=====================================================================================================================================================================================

void xArithmCoreEnc::start()
{
  m_C  = 0;
  m_A  = 0x10000;
  m_CT = 11;
  m_SC = 0;
  m_ZC = 0;

  m_B  = -1;
}
#if PMBB_JPEG_ARITHM_AS_IN_STANDARD
void xArithmCoreEnc::encodeBinMP_as_in_standard(uint32 BinValue, xArithmCoreModel& CtxModel)
{
  if(BinValue == 0)
  {
    xCode0(CtxModel);
  }
  else
  {
    xCode1(CtxModel);
  }
}
void xArithmCoreEnc::xCode0(xArithmCoreModel& Ctx)
{
  uint32 MPS = Ctx.getMPS();
  if(MPS == 0)
  {
    xCodeMPS(Ctx);
  }
  else
  {
    xCodeLPS(Ctx);
  }
}
void xArithmCoreEnc::xCode1(xArithmCoreModel& Ctx)
{
  uint32 MPS = Ctx.getMPS();
  if(MPS == 1)
  {
    xCodeMPS(Ctx);
  }
  else
  {
    xCodeLPS(Ctx);
  }
}

void xArithmCoreEnc::xCodeLPS(xArithmCoreModel& Ctx)
{
  uint32 Index = Ctx.getProbIdx();
  uint32 Qe    = c_Qe[Index];
  m_A = m_A - Qe;
  if(!(m_A < Qe))
  {
    m_C = m_C + m_A;
    m_A = Qe;
  }
  Ctx.updateStdLPS(); //Estimate_Qe_after_LPS
  xRenorm();
}
void xArithmCoreEnc::xCodeMPS(xArithmCoreModel& Ctx)
{
  uint32 Index = Ctx.getProbIdx();
  uint32 Qe    = c_Qe[Index];

  m_A = m_A - Qe;

  if(m_A < 0x8000)
  {
    if(m_A < Qe)
    {
      m_C = m_C + m_A;
      m_A = Qe;
    }
    Ctx.updateStdMPS(); //Estimate_Qe_After_MPS
    xRenorm();
  }
}
#endif //PMBB_JPEG_ARITHM_AS_IN_STANDARD
void xArithmCoreEnc::encodeBinMP(uint32 BinValue, xArithmCoreModel& CtxModel)
{
  assert(BinValue == 1 || BinValue == 0);

  const uint32 MPS     = CtxModel.getMPS    ();
  const uint32 ProbIdx = CtxModel.getProbIdx();
  const uint32 Qe      = c_Qe[ProbIdx];

  m_A = m_A - Qe;

  if(BinValue == MPS)
  {    
    if(!((uint32)m_A < 0x8000)) { return; } //nothing more to do
    if((uint32)m_A < Qe)
    {
      m_C = m_C + m_A;
      m_A = Qe;
    }
  }
  else
  {
    if(!((uint32)m_A < Qe))
    {
      m_C = m_C + m_A;
      m_A = Qe;
    }
  }

  CtxModel.updateTab(BinValue);
  xRenorm();
}
void xArithmCoreEnc::encodeBinEP(uint32 BinValue)
{
  assert(BinValue == 1 || BinValue == 0);

  m_A = m_A - c_EqualProbQe;

  if(BinValue == c_EqualProbMPS)
  {
    if(!(m_A < 0x8000)) { return; } //nothing more to do
    if(m_A < c_EqualProbQe)
    {
      m_C = m_C + m_A;
      m_A = c_EqualProbQe;
    }
  }
  else
  {
    if(!(m_A < c_EqualProbQe))
    {
      m_C = m_C + m_A;
      m_A = c_EqualProbQe;
    }
  }

  xRenorm();
}
void xArithmCoreEnc::xRenorm()
{
  do
  {
    m_A <<= 1;
    m_C <<= 1;
    m_CT -= 1;
    if(m_CT == 0)
    {
      //byte_out - Figure D.7 - Encoder renormalization procedure
      int32 T = m_C >> 19;
      if(T > 0xFF)
      {
        //Handle overflow over all stacked 0xFF bytes
        if(m_B >= 0)
        {
          xOutput00s();
          m_ByteBuffer->appendU8((uint8)(m_B + 1));
#if PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
          if(m_B + 1 == 0xFF) { m_ByteBuffer->appendU8(0x00); }
#endif //PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
        }
        m_ZC += m_SC;    // carry-over converts stacked 0xFF bytes to 0x00
        m_SC = 0;        // Note: The 3 spacer bits in the C register guarantee that the new buffer byte can't be 0xFF here (see page 160 in the P&M JPEG book). 
        m_B  = T & 0xFF; // new output byte, might overflow later 
      }
      else if(T == 0xFF)
      {
        m_SC++;
      }
      else
      {
        //Output all stacked 0xFF bytes, they will not overflow any more
        if(m_B == 0)
        {
          m_ZC++;
        }
        else if(m_B >= 0)
        {
          xOutput00s();
          m_ByteBuffer->appendU8((uint8)m_B);
        }
        xOutputFFs();
        m_B = T & 0xFF; // new output byte (can still overflow)
      }
      m_C  &= 0x7FFFFL;
      m_CT += 8;
    }
  }
  while(m_A < 0x8000);
}
void xArithmCoreEnc::finish()
{
  // Section D.1.8: Termination of encoding 
  // Find the m_C in the coding interval with the largest number of trailing zero bits 

  uint32 T = (uint32)((m_A - 1 + m_C) & 0xFFFF0000UL);

  if(T < m_C)
  {
    m_C = T + 0x8000L;
  }
  else
  {
    m_C = T;
  }
  // Send remaining bytes to output 
  m_C <<= m_CT;
  if (m_C & 0xF8000000UL)
  {
    // One final overflow has to be handled 
    if (m_B >= 0) 
    {
      xOutput00s();
      m_ByteBuffer->appendU8((uint8)(m_B + 1));
#if PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
      if(m_B + 1 == 0xFF) { m_ByteBuffer->appendU8(0x00); }
#endif //PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
    }
    m_ZC += m_SC;  // carry-over converts stacked 0xFF bytes to 0x00 
    m_SC = 0;
  } 
  else
  {
    if(m_B == 0)
    {
      m_ZC++;
    }
    else if (m_B >= 0)
    {
      xOutput00s();
      m_ByteBuffer->appendU8((uint8)m_B);
    }
    xOutputFFs();
  }
  // Output final bytes only if they are not 0x00
  if (m_C & 0x7FFF800L)
  {
    //output final pending zero bytes
    xOutput00s();
    uint32 Data = (m_C >> 19) & 0xFF;
    m_ByteBuffer->appendU8((uint8)Data);
#if PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
    if(Data == 0xFF) { m_ByteBuffer->appendU8(0x00); }
#endif //PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION

    if (m_C & 0x7F800L) 
    {
      Data = (m_C >> 11) & 0xFF;
      m_ByteBuffer->appendU8((uint8)Data);
#if PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
      if(Data == 0xFF) { m_ByteBuffer->appendU8(0x00); }
#endif //PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
    }
  }
}
void xArithmCoreEnc::xOutput00s()
{
  if(m_ZC)
  {
    do { m_ByteBuffer->appendU8(0x00); }
    while(--m_ZC);
  }
}
void xArithmCoreEnc::xOutputFFs()
{
  if(m_SC)
  {
    xOutput00s();
    do
    {
      m_ByteBuffer->appendU8(0xFF);
#if PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
      m_ByteBuffer->appendU8(0x00);
#endif //PMBB_JPEG_ARITHM_AVOID_MARKER_EMULATION
    }
    while(--m_SC);
  }
}



//=====================================================================================================================================================================================

void xArithmCoreCnt::encodeBinMP(uint32 BinValue, xArithmCoreModel& CtxModel)
{
  assert(BinValue == 1 || BinValue == 0);

  const uint32 MPS     = CtxModel.getMPS    ();
  const uint32 ProbIdx = CtxModel.getProbIdx();
  const uint32 Qe      = xArithmCoreCmn::c_Qe[ProbIdx];

  m_A = m_A - Qe;
  if(BinValue == MPS)
  {
    m_FracBits += xArithmCoreCmn::c_EntropyBits[(ProbIdx<<1)+0];
    if(!((uint32)m_A < 0x8000)) { return; } //nothing more to do
    if((uint32)m_A < Qe) { m_A = Qe; }
  }
  else
  {
    m_FracBits += xArithmCoreCmn::c_EntropyBits[(ProbIdx<<1)+1];
    if(!((uint32)m_A < Qe)) { m_A = Qe; }
  }

  CtxModel.updateTab(BinValue);
  xPseudoRenorm();
}
void xArithmCoreCnt::encodeBinEP(uint32 BinValue)
{
  assert(BinValue == 1 || BinValue == 0);

  m_A = m_A - xArithmCoreCmn::c_EqualProbQe;

  if(BinValue == xArithmCoreCmn::c_EqualProbMPS)
  {
    if(!(m_A < 0x8000)) { return; } //nothing more to do
    if(m_A < xArithmCoreCmn::c_EqualProbQe) { m_A = xArithmCoreCmn::c_EqualProbQe; }
  }
  else
  {
    if(!(m_A < xArithmCoreCmn::c_EqualProbQe)) { m_A = xArithmCoreCmn::c_EqualProbQe; }
  }

  m_FracBits += xArithmCoreCmn::c_EqualProbEntropyBits;
  xPseudoRenorm();
}
void xArithmCoreCnt::xPseudoRenorm() //D.1.6 Renormalization in the encoder --> Figure D.7 - Encoder renormalization procedure
{
  do { m_A <<= 1; }
  while(m_A < 0x8000);
}

//=====================================================================================================================================================================================

} //end of namespace PMBB::JPEG