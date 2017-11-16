// AVX_test2.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#define MYFUN 1

#ifdef MYFUN
//int _tmain(int argc, _TCHAR* argv[])
//{
//	return 0;
//}
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>

#if _MSC_VER >=1400	// VC2005才支持intrin.h
#include <intrin.h>	// 所有Intrinsics函数
#else
#include <emmintrin.h>	// MMX, SSE, SSE2
#endif


// CPUIDFIELD
typedef INT32 CPUIDFIELD;

#define  CPUIDFIELD_MASK_POS	0x0000001F	// 位偏移. 0~31.
#define  CPUIDFIELD_MASK_LEN	0x000003E0	// 位长. 1~32
#define  CPUIDFIELD_MASK_REG	0x00000C00	// 寄存器. 0=EAX, 1=EBX, 2=ECX, 3=EDX.
#define  CPUIDFIELD_MASK_FIDSUB	0x000FF000	// 子功能号(低8位).
#define  CPUIDFIELD_MASK_FID	0xFFF00000	// 功能号(最高4位 和 低8位).

#define CPUIDFIELD_SHIFT_POS	0
#define CPUIDFIELD_SHIFT_LEN	5
#define CPUIDFIELD_SHIFT_REG	10
#define CPUIDFIELD_SHIFT_FIDSUB	12
#define CPUIDFIELD_SHIFT_FID	20

#define CPUIDFIELD_MAKE(fid,fidsub,reg,pos,len)	(((fid)&0xF0000000) \
	| ((fid)<<CPUIDFIELD_SHIFT_FID & 0x0FF00000) \
	| ((fidsub)<<CPUIDFIELD_SHIFT_FIDSUB & CPUIDFIELD_MASK_FIDSUB) \
	| ((reg)<<CPUIDFIELD_SHIFT_REG & CPUIDFIELD_MASK_REG) \
	| ((pos)<<CPUIDFIELD_SHIFT_POS & CPUIDFIELD_MASK_POS) \
	| (((len)-1)<<CPUIDFIELD_SHIFT_LEN & CPUIDFIELD_MASK_LEN) \
	)
#define CPUIDFIELD_FID(cpuidfield)	( ((cpuidfield)&0xF0000000) | (((cpuidfield) & 0x0FF00000)>>CPUIDFIELD_SHIFT_FID) )
#define CPUIDFIELD_FIDSUB(cpuidfield)	( ((cpuidfield) & CPUIDFIELD_MASK_FIDSUB)>>CPUIDFIELD_SHIFT_FIDSUB )
#define CPUIDFIELD_REG(cpuidfield)	( ((cpuidfield) & CPUIDFIELD_MASK_REG)>>CPUIDFIELD_SHIFT_REG )
#define CPUIDFIELD_POS(cpuidfield)	( ((cpuidfield) & CPUIDFIELD_MASK_POS)>>CPUIDFIELD_SHIFT_POS )
#define CPUIDFIELD_LEN(cpuidfield)	( (((cpuidfield) & CPUIDFIELD_MASK_LEN)>>CPUIDFIELD_SHIFT_LEN) + 1 )

// 取得位域
#ifndef __GETBITS32
#define __GETBITS32(src,pos,len)	( ((src)>>(pos)) & (((UINT32)-1)>>(32-len)) )
#endif


#define CPUF_SSE4A	CPUIDFIELD_MAKE(0x80000001,0,2,6,1)
#define CPUF_AES	CPUIDFIELD_MAKE(1,0,2,25,1)
#define CPUF_PCLMULQDQ	CPUIDFIELD_MAKE(1,0,2,1,1)

#define CPUF_AVX	CPUIDFIELD_MAKE(1,0,2,28,1)
#define CPUF_AVX2	CPUIDFIELD_MAKE(7,0,1,5,1)
#define CPUF_OSXSAVE	CPUIDFIELD_MAKE(1,0,2,27,1)
#define CPUF_XFeatureSupportedMaskLo	CPUIDFIELD_MAKE(0xD,0,0,0,32)
#define CPUF_F16C	CPUIDFIELD_MAKE(1,0,2,29,1)
#define CPUF_FMA	CPUIDFIELD_MAKE(1,0,2,12,1)
#define CPUF_FMA4	CPUIDFIELD_MAKE(0x80000001,0,2,16,1)
#define CPUF_XOP	CPUIDFIELD_MAKE(0x80000001,0,2,11,1)


// SSE系列指令集的支持级别. simd_sse_level 函数的返回值。
#define SIMD_SSE_NONE	0	// 不支持
#define SIMD_SSE_1	1	// SSE
#define SIMD_SSE_2	2	// SSE2
#define SIMD_SSE_3	3	// SSE3
#define SIMD_SSE_3S	4	// SSSE3
#define SIMD_SSE_41	5	// SSE4.1
#define SIMD_SSE_42	6	// SSE4.2

const char*	simd_sse_names[] = {
	"None",
	"SSE",
	"SSE2",
	"SSE3",
	"SSSE3",
	"SSE4.1",
	"SSE4.2",
};


// AVX系列指令集的支持级别. simd_avx_level 函数的返回值。
#define SIMD_AVX_NONE	0	// 不支持
#define SIMD_AVX_1	1	// AVX
#define SIMD_AVX_2	2	// AVX2

const char*	simd_avx_names[] = {
	"None",
	"AVX",
	"AVX2"
};



char szBuf[64];
INT32 dwBuf[4];

#if defined(_WIN64)
// 64位下不支持内联汇编. 应使用__cpuid、__cpuidex等Intrinsics函数。
#else
#if _MSC_VER < 1600	// VS2010. 据说VC2008 SP1之后才支持__cpuidex
void __cpuidex(INT32 CPUInfo[4], INT32 InfoType, INT32 ECXValue)
{
	if (NULL==CPUInfo)	return;
	_asm{
		// load. 读取参数到寄存器
		mov edi, CPUInfo;	// 准备用edi寻址CPUInfo
		mov eax, InfoType;
		mov ecx, ECXValue;
		// CPUID
		cpuid;
		// save. 将寄存器保存到CPUInfo
		mov	[edi], eax;
		mov	[edi+4], ebx;
		mov	[edi+8], ecx;
		mov	[edi+12], edx;
	}
}
#endif	// #if _MSC_VER < 1600	// VS2010. 据说VC2008 SP1之后才支持__cpuidex

#if _MSC_VER < 1400	// VC2005才支持__cpuid
void __cpuid(INT32 CPUInfo[4], INT32 InfoType)
{
	__cpuidex(CPUInfo, InfoType, 0);
}
#endif	// #if _MSC_VER < 1400	// VC2005才支持__cpuid

#endif	// #if defined(_WIN64)

// 根据CPUIDFIELD从缓冲区中获取字段.
UINT32	getcpuidfield_buf(const INT32 dwBuf[4], CPUIDFIELD cpuf)
{
	return __GETBITS32(dwBuf[CPUIDFIELD_REG(cpuf)], CPUIDFIELD_POS(cpuf), CPUIDFIELD_LEN(cpuf));
}

// 根据CPUIDFIELD获取CPUID字段.
UINT32	getcpuidfield(CPUIDFIELD cpuf)
{
	INT32 dwBuf[4];
	__cpuidex(dwBuf, CPUIDFIELD_FID(cpuf), CPUIDFIELD_FIDSUB(cpuf));
	return getcpuidfield_buf(dwBuf, cpuf);
}

// 取得CPU厂商（Vendor）
//
// result: 成功时返回字符串的长度（一般为12）。失败时返回0。
// pvendor: 接收厂商信息的字符串缓冲区。至少为13字节。
int cpu_getvendor(char* pvendor)
{
	INT32 dwBuf[4];
	if (NULL==pvendor)	return 0;
	// Function 0: Vendor-ID and Largest Standard Function
	__cpuid(dwBuf, 0);
	// save. 保存到pvendor
	*(INT32*)&pvendor[0] = dwBuf[1];	// ebx: 前四个字符
	*(INT32*)&pvendor[4] = dwBuf[3];	// edx: 中间四个字符
	*(INT32*)&pvendor[8] = dwBuf[2];	// ecx: 最后四个字符
	pvendor[12] = '\0';
	return 12;
}

// 取得CPU商标（Brand）
//
// result: 成功时返回字符串的长度（一般为48）。失败时返回0。
// pbrand: 接收商标信息的字符串缓冲区。至少为49字节。
int cpu_getbrand(char* pbrand)
{
	INT32 dwBuf[4];
	if (NULL==pbrand)	return 0;
	// Function 0x80000000: Largest Extended Function Number
	__cpuid(dwBuf, 0x80000000);
	if (dwBuf[0] < 0x80000004)	return 0;
	// Function 80000002h,80000003h,80000004h: Processor Brand String
	__cpuid((INT32*)&pbrand[0], 0x80000002);	// 前16个字符
	__cpuid((INT32*)&pbrand[16], 0x80000003);	// 中间16个字符
	__cpuid((INT32*)&pbrand[32], 0x80000004);	// 最后16个字符
	pbrand[48] = '\0';
	return 48;
}


// 是否支持MMX指令集
BOOL	simd_mmx(BOOL* phwmmx)
{
	const INT32	BIT_D_MMX = 0x00800000;	// bit 23
	BOOL	rt = FALSE;	// result
	INT32 dwBuf[4];

	// check processor support
	__cpuid(dwBuf, 1);	// Function 1: Feature Information
	if ( dwBuf[3] & BIT_D_MMX )	rt=TRUE;
	if (NULL!=phwmmx)	*phwmmx=rt;

	// check OS support
	if ( rt )
	{
#if defined(_WIN64)
		// VC编译器不支持64位下的MMX。
		rt=FALSE;
#else
		__try 
		{
			_mm_empty();	// MMX instruction: emms
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			rt=FALSE;
		}
#endif	// #if defined(_WIN64)
	}

	return rt;
}

// 检测SSE系列指令集的支持级别
int	simd_sse_level(int* phwsse)
{
	const INT32	BIT_D_SSE = 0x02000000;	// bit 25
	const INT32	BIT_D_SSE2 = 0x04000000;	// bit 26
	const INT32	BIT_C_SSE3 = 0x00000001;	// bit 0
	const INT32	BIT_C_SSSE3 = 0x00000100;	// bit 9
	const INT32	BIT_C_SSE41 = 0x00080000;	// bit 19
	const INT32	BIT_C_SSE42 = 0x00100000;	// bit 20
	int	rt = SIMD_SSE_NONE;	// result
	INT32 dwBuf[4];

	// check processor support
	__cpuid(dwBuf, 1);	// Function 1: Feature Information
	if ( dwBuf[3] & BIT_D_SSE )
	{
		rt = SIMD_SSE_1;
		if ( dwBuf[3] & BIT_D_SSE2 )
		{
			rt = SIMD_SSE_2;
			if ( dwBuf[2] & BIT_C_SSE3 )
			{
				rt = SIMD_SSE_3;
				if ( dwBuf[2] & BIT_C_SSSE3 )
				{
					rt = SIMD_SSE_3S;
					if ( dwBuf[2] & BIT_C_SSE41 )
					{
						rt = SIMD_SSE_41;
						if ( dwBuf[2] & BIT_C_SSE42 )
						{
							rt = SIMD_SSE_42;
						}
					}
				}
			}
		}
	}
	if (NULL!=phwsse)	*phwsse=rt;

	// check OS support
	__try 
	{
		__m128 xmm1 = _mm_setzero_ps();	// SSE instruction: xorps
		if (0!=*(int*)&xmm1)	rt = SIMD_SSE_NONE;	// 避免Release模式编译优化时剔除上一条语句
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		rt = SIMD_SSE_NONE;
	}

	return rt;
}

// 检测AVX系列指令集的支持级别.
int	simd_avx_level(int* phwavx)
{
	int	rt = SIMD_AVX_NONE;	// result

	// check processor support
	if (0!=getcpuidfield(CPUF_AVX))
	{
		rt = SIMD_AVX_1;

		printf("cpu support:avx 1\n");
		if (0!=getcpuidfield(CPUF_AVX2))
		{
			rt = SIMD_AVX_2;
			printf("cpu support:avx 2\n");
		}
	}
	if (NULL!=phwavx)	*phwavx=rt;

	// check OS support
	if (0!=getcpuidfield(CPUF_OSXSAVE))	// XGETBV enabled for application use.
	{
		UINT32 n = getcpuidfield(CPUF_XFeatureSupportedMaskLo);	// XCR0: XFeatureSupportedMask register.
		if (6==(n&6))	// XCR0[2:1] = ‘11b’ (XMM state and YMM state are enabled by OS).
		{
			printf("os support:avx\n");
			return rt;
		} else
		{
				printf("os dose not support:avx\n");

		}
	}else
	{

		printf("can not query os support:avx\n");
	
	}
	return SIMD_AVX_NONE;
}



int _tmain2(int argc, _TCHAR* argv[])
{
	int i;

	//__cpuidex(dwBuf, 0,0);
	//__cpuid(dwBuf, 0);
	//printf("%.8X\t%.8X\t%.8X\t%.8X\n", dwBuf[0],dwBuf[1],dwBuf[2],dwBuf[3]);

	cpu_getvendor(szBuf);
	printf("CPU Vendor:\t%s\n", szBuf);

	cpu_getbrand(szBuf);
	printf("CPU Name:\t%s\n", szBuf);

	BOOL bhwmmx;	// 硬件支持MMX.
	BOOL bmmx;	// 操作系统支持MMX.
	bmmx = simd_mmx(&bhwmmx);
	printf("MMX: %d\t// hw: %d\n", bmmx, bhwmmx);

	int	nhwsse;	// 硬件支持SSE.
	int	nsse;	// 操作系统支持SSE.
	nsse = simd_sse_level(&nhwsse);
	printf("SSE: %d\t// hw: %d\n", nsse, nhwsse);
	for(i=1; i<sizeof(simd_sse_names)/sizeof(simd_sse_names[0]); ++i)
	{
		if (nhwsse>=i)	printf("\t%s\n", simd_sse_names[i]);
	}

	// test SSE4A/AES/PCLMULQDQ
	printf("SSE4A: %d\n", getcpuidfield(CPUF_SSE4A));
	printf("AES: %d\n", getcpuidfield(CPUF_AES));
	printf("PCLMULQDQ: %d\n", getcpuidfield(CPUF_PCLMULQDQ));

	// test AVX
	int	nhwavx;	// 硬件支持AVX.
	int	navx;	// 操作系统支持AVX.
	navx = simd_avx_level(&nhwavx);
	printf("AVX: %d\t// hw: %d\n", navx, nhwavx);
	for(i=1; i<sizeof(simd_avx_names)/sizeof(simd_avx_names[0]); ++i)
	{
		if (nhwavx>=i)	printf("\t%s\n", simd_avx_names[i]);
	}

	// test F16C/FMA/FMA4/XOP
	printf("F16C: %d\n", getcpuidfield(CPUF_F16C));
	printf("FMA: %d\n", getcpuidfield(CPUF_FMA));
	printf("FMA4: %d\n", getcpuidfield(CPUF_FMA4));
	printf("XOP: %d\n", getcpuidfield(CPUF_XOP));

	return 0;
}


char szformat[]="%s %s\n";
char szHello[]="Hello";
char szWorld[]="world";

int _tmain(int argc, _TCHAR* argv[])
{
_asm
	{
		
     	mov eax, offset szWorld
		push eax
		mov eax, offset szHello
		push eax
	    mov eax, offset szformat
		push eax
		call printf
		pop ebx
		pop ebx
		pop ebx

	}

	return 0;
}
#endif

// InstructionSet.cpp   
// Compile by using: cl /EHsc /W4 InstructionSet.cpp  
// processor: x86, x64  
// Uses the __cpuid intrinsic to get information about   
// CPU extended instruction set support.  
// InstructionSet.cpp   
// Compile by using: cl /EHsc /W4 InstructionSet.cpp  
// processor: x86, x64  
// Uses the __cpuid intrinsic to get information about   
// CPU extended instruction set support.  


#ifndef MYFUN
  
#include <iostream>  
#include <vector>  
#include <bitset>  
#include <array>  
#include <string>  
#include <intrin.h>  
  
class InstructionSet  
{  
    // forward declarations  
    class InstructionSet_Internal;  
  
public:  
    // getters  
    static std::string Vendor(void) { return CPU_Rep.vendor_; }  
    static std::string Brand(void) { return CPU_Rep.brand_; }  
  
    static bool SSE3(void) { return CPU_Rep.f_1_ECX_[0]; }  
    static bool PCLMULQDQ(void) { return CPU_Rep.f_1_ECX_[1]; }  
    static bool MONITOR(void) { return CPU_Rep.f_1_ECX_[3]; }  
    static bool SSSE3(void) { return CPU_Rep.f_1_ECX_[9]; }  
    static bool FMA(void) { return CPU_Rep.f_1_ECX_[12]; }  
    static bool CMPXCHG16B(void) { return CPU_Rep.f_1_ECX_[13]; }  
    static bool SSE41(void) { return CPU_Rep.f_1_ECX_[19]; }  
    static bool SSE42(void) { return CPU_Rep.f_1_ECX_[20]; }  
    static bool MOVBE(void) { return CPU_Rep.f_1_ECX_[22]; }  
    static bool POPCNT(void) { return CPU_Rep.f_1_ECX_[23]; }  
    static bool AES(void) { return CPU_Rep.f_1_ECX_[25]; }  
    static bool XSAVE(void) { return CPU_Rep.f_1_ECX_[26]; }  
    static bool OSXSAVE(void) { return CPU_Rep.f_1_ECX_[27]; }  
    static bool AVX(void) { return CPU_Rep.f_1_ECX_[28]; }  
    static bool F16C(void) { return CPU_Rep.f_1_ECX_[29]; }  
    static bool RDRAND(void) { return CPU_Rep.f_1_ECX_[30]; }  
  
    static bool MSR(void) { return CPU_Rep.f_1_EDX_[5]; }  
    static bool CX8(void) { return CPU_Rep.f_1_EDX_[8]; }  
    static bool SEP(void) { return CPU_Rep.f_1_EDX_[11]; }  
    static bool CMOV(void) { return CPU_Rep.f_1_EDX_[15]; }  
    static bool CLFSH(void) { return CPU_Rep.f_1_EDX_[19]; }  
    static bool MMX(void) { return CPU_Rep.f_1_EDX_[23]; }  
    static bool FXSR(void) { return CPU_Rep.f_1_EDX_[24]; }  
    static bool SSE(void) { return CPU_Rep.f_1_EDX_[25]; }  
    static bool SSE2(void) { return CPU_Rep.f_1_EDX_[26]; }  
  
    static bool FSGSBASE(void) { return CPU_Rep.f_7_EBX_[0]; }  
    static bool BMI1(void) { return CPU_Rep.f_7_EBX_[3]; }  
    static bool HLE(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_7_EBX_[4]; }  
    static bool AVX2(void) { return CPU_Rep.f_7_EBX_[5]; }  
    static bool BMI2(void) { return CPU_Rep.f_7_EBX_[8]; }  
    static bool ERMS(void) { return CPU_Rep.f_7_EBX_[9]; }  
    static bool INVPCID(void) { return CPU_Rep.f_7_EBX_[10]; }  
    static bool RTM(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_7_EBX_[11]; }  
    static bool AVX512F(void) { return CPU_Rep.f_7_EBX_[16]; }  
    static bool RDSEED(void) { return CPU_Rep.f_7_EBX_[18]; }  
    static bool ADX(void) { return CPU_Rep.f_7_EBX_[19]; }  
    static bool AVX512PF(void) { return CPU_Rep.f_7_EBX_[26]; }  
    static bool AVX512ER(void) { return CPU_Rep.f_7_EBX_[27]; }  
    static bool AVX512CD(void) { return CPU_Rep.f_7_EBX_[28]; }  
    static bool SHA(void) { return CPU_Rep.f_7_EBX_[29]; }  
  
    static bool PREFETCHWT1(void) { return CPU_Rep.f_7_ECX_[0]; }  
  
    static bool LAHF(void) { return CPU_Rep.f_81_ECX_[0]; }  
    static bool LZCNT(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_ECX_[5]; }  
    static bool ABM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[5]; }  
    static bool SSE4a(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[6]; }  
    static bool XOP(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[11]; }  
    static bool TBM(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_ECX_[21]; }  
  
    static bool SYSCALL(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[11]; }  
    static bool MMXEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[22]; }  
    static bool RDTSCP(void) { return CPU_Rep.isIntel_ && CPU_Rep.f_81_EDX_[27]; }  
    static bool _3DNOWEXT(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[30]; }  
    static bool _3DNOW(void) { return CPU_Rep.isAMD_ && CPU_Rep.f_81_EDX_[31]; }  
  
private:  
    static const InstructionSet_Internal CPU_Rep;  
  
    class InstructionSet_Internal  
    {  
    public:  
        InstructionSet_Internal()  
            /*: nIds_{ 0 },  
            nExIds_{ 0 },  
            isIntel_{ false },  
            isAMD_{ false },  
            f_1_ECX_{ 0 },  
            f_1_EDX_{ 0 },  
            f_7_EBX_{ 0 },  
            f_7_ECX_{ 0 },  
            f_81_ECX_{ 0 },  
            f_81_EDX_{ 0 },  
            data_{},  
            extdata_{} */ 
        {  
            //int cpuInfo[4] = {-1};  
            std::array<int, 4> cpui;  
  
            // Calling __cpuid with 0x0 as the function_id argument  
            // gets the number of the highest valid function ID.  
            __cpuid(cpui.data(), 0);  
            nIds_ = cpui[0];  
  
            for (int i = 0; i <= nIds_; ++i)  
            {  
                __cpuidex(cpui.data(), i, 0);  
                data_.push_back(cpui);  
            }  
  
            // Capture vendor string  
            char vendor[0x20];  
            memset(vendor, 0, sizeof(vendor));  
            *reinterpret_cast<int*>(vendor) = data_[0][1];  
            *reinterpret_cast<int*>(vendor + 4) = data_[0][3];  
            *reinterpret_cast<int*>(vendor + 8) = data_[0][2];  
            vendor_ = vendor;  
            if (vendor_ == "GenuineIntel")  
            {  
                isIntel_ = true;  
            }  
            else if (vendor_ == "AuthenticAMD")  
            {  
                isAMD_ = true;  
            }  
  
            // load bitset with flags for function 0x00000001  
            if (nIds_ >= 1)  
            {  
                f_1_ECX_ = data_[1][2];  
                f_1_EDX_ = data_[1][3];  
            }  
  
            // load bitset with flags for function 0x00000007  
            if (nIds_ >= 7)  
            {  
                f_7_EBX_ = data_[7][1];  
                f_7_ECX_ = data_[7][2];  
            }  
  
            // Calling __cpuid with 0x80000000 as the function_id argument  
            // gets the number of the highest valid extended ID.  
            __cpuid(cpui.data(), 0x80000000);  
            nExIds_ = cpui[0];  
  
            char brand[0x40];  
            memset(brand, 0, sizeof(brand));  
  
            for (int i = 0x80000000; i <= nExIds_; ++i)  
            {  
                __cpuidex(cpui.data(), i, 0);  
                extdata_.push_back(cpui);  
            }  
  
            // load bitset with flags for function 0x80000001  
            if (nExIds_ >= 0x80000001)  
            {  
                f_81_ECX_ = extdata_[1][2];  
                f_81_EDX_ = extdata_[1][3];  
            }  
  
            // Interpret CPU brand string if reported  
            if (nExIds_ >= 0x80000004)  
            {  
                memcpy(brand, extdata_[2].data(), sizeof(cpui));  
                memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));  
                memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));  
                brand_ = brand;  
            }  
        };  
  
        int nIds_;  
        int nExIds_;  
        std::string vendor_;  
        std::string brand_;  
        bool isIntel_;  
        bool isAMD_;  
        std::bitset<32> f_1_ECX_;  
        std::bitset<32> f_1_EDX_;  
        std::bitset<32> f_7_EBX_;  
        std::bitset<32> f_7_ECX_;  
        std::bitset<32> f_81_ECX_;  
        std::bitset<32> f_81_EDX_;  
        std::vector<std::array<int, 4>> data_;  
        std::vector<std::array<int, 4>> extdata_;  
    };  
};  
  
// Initialize static member data  
const InstructionSet::InstructionSet_Internal InstructionSet::CPU_Rep;  
  
// Print out supported instruction set extensions  
int main()  
{  
    auto& outstream = std::cout;  
  
    auto support_message = [&outstream](std::string isa_feature, bool is_supported) {  
        outstream << isa_feature << (is_supported ? " supported" : " not supported") << std::endl;  
    };  
  
    std::cout << InstructionSet::Vendor() << std::endl;  
    std::cout << InstructionSet::Brand() << std::endl;  
  
    support_message("3DNOW",       InstructionSet::_3DNOW());  
    support_message("3DNOWEXT",    InstructionSet::_3DNOWEXT());  
    support_message("ABM",         InstructionSet::ABM());  
    support_message("ADX",         InstructionSet::ADX());  
    support_message("AES",         InstructionSet::AES());  
    support_message("AVX",         InstructionSet::AVX());  
    support_message("AVX2",        InstructionSet::AVX2());  
    support_message("AVX512CD",    InstructionSet::AVX512CD());  
    support_message("AVX512ER",    InstructionSet::AVX512ER());  
    support_message("AVX512F",     InstructionSet::AVX512F());  
    support_message("AVX512PF",    InstructionSet::AVX512PF());  
    support_message("BMI1",        InstructionSet::BMI1());  
    support_message("BMI2",        InstructionSet::BMI2());  
    support_message("CLFSH",       InstructionSet::CLFSH());  
    support_message("CMPXCHG16B",  InstructionSet::CMPXCHG16B());  
    support_message("CX8",         InstructionSet::CX8());  
    support_message("ERMS",        InstructionSet::ERMS());  
    support_message("F16C",        InstructionSet::F16C());  
    support_message("FMA",         InstructionSet::FMA());  
    support_message("FSGSBASE",    InstructionSet::FSGSBASE());  
    support_message("FXSR",        InstructionSet::FXSR());  
    support_message("HLE",         InstructionSet::HLE());  
    support_message("INVPCID",     InstructionSet::INVPCID());  
    support_message("LAHF",        InstructionSet::LAHF());  
    support_message("LZCNT",       InstructionSet::LZCNT());  
    support_message("MMX",         InstructionSet::MMX());  
    support_message("MMXEXT",      InstructionSet::MMXEXT());  
    support_message("MONITOR",     InstructionSet::MONITOR());  
    support_message("MOVBE",       InstructionSet::MOVBE());  
    support_message("MSR",         InstructionSet::MSR());  
    support_message("OSXSAVE",     InstructionSet::OSXSAVE());  
    support_message("PCLMULQDQ",   InstructionSet::PCLMULQDQ());  
    support_message("POPCNT",      InstructionSet::POPCNT());  
    support_message("PREFETCHWT1", InstructionSet::PREFETCHWT1());  
    support_message("RDRAND",      InstructionSet::RDRAND());  
    support_message("RDSEED",      InstructionSet::RDSEED());  
    support_message("RDTSCP",      InstructionSet::RDTSCP());  
    support_message("RTM",         InstructionSet::RTM());  
    support_message("SEP",         InstructionSet::SEP());  
    support_message("SHA",         InstructionSet::SHA());  
    support_message("SSE",         InstructionSet::SSE());  
    support_message("SSE2",        InstructionSet::SSE2());  
    support_message("SSE3",        InstructionSet::SSE3());  
    support_message("SSE4.1",      InstructionSet::SSE41());  
    support_message("SSE4.2",      InstructionSet::SSE42());  
    support_message("SSE4a",       InstructionSet::SSE4a());  
    support_message("SSSE3",       InstructionSet::SSSE3());  
    support_message("SYSCALL",     InstructionSet::SYSCALL());  
    support_message("TBM",         InstructionSet::TBM());  
    support_message("XOP",         InstructionSet::XOP());  
    support_message("XSAVE",       InstructionSet::XSAVE());  
}  
#endif