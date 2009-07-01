/********************************************************************
Virtual function table hook mechanism
written by LanceVorgin aka stosw aka like 90 other names
ty to qizmo for helping and uni for owning me
oh and go play in traffic trimbo - real mature eh - I blame the drugs.
********************************************************************/

#ifndef _VFNHOOK_H
#define _VFNHOOK_H

#define ADDRTYPE unsigned long

#define VTBL( classptr ) (*(ADDRTYPE*)classptr)
#define PVFN_( classptr , offset ) (VTBL( classptr ) + offset)
#define VFN_( classptr , offset ) *(ADDRTYPE*)PVFN_( classptr , offset )
#define PVFN( classptr , offset ) PVFN_( classptr , ( offset * sizeof(void*) ) )
#define VFN( classptr , offset ) VFN_( classptr , ( offset * sizeof(void*) ) )

#define HDEFVFUNC( funcname , returntype , proto ) \
	typedef returntype ( VFUNC * funcname##Func ) proto ; \
	extern funcname##Func funcname ;

#if defined _WIN32 || defined WIN32 

#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
#include <windows.h>

class CVirtualCallGate
{
public:

	void Build(void* pOrigFunc, void* pNewFunc, void* pOrgFuncCaller)
	{
		BYTE szGate[] =
		{
			//pop a   push c   push a   mov a, <dword>   jmp a
			0x58,   0x51,   0x50,   0xB8, 0,0,0,0,   0xFF, 0xE0,
				//pop a   pop c   push a   mov a, <dword>   jmp a
				0x58,   0x59,   0x50,   0xB8, 0,0,0,0,   0xFF, 0xE0
		};

		DWORD dwIDontCare;

		VirtualProtect( m_szGate, 20, (PAGE_EXECUTE_READWRITE ), &dwIDontCare );

		memcpy(m_szGate, &szGate, sizeof(szGate));

		*(ADDRTYPE*)&m_szGate[4]   = (ADDRTYPE)pNewFunc;
		*(ADDRTYPE*)&m_szGate[14]  = (ADDRTYPE)pOrigFunc;

		*(ADDRTYPE*)pOrgFuncCaller = (ADDRTYPE)&m_szGate[10];
	}

	ADDRTYPE Gate()
	{
		return (ADDRTYPE)&m_szGate[0];
	}

private:
	char m_szGate[20];
};

inline bool DeProtect( void* pMemory, unsigned int uiLen, bool bLock = false )
{
	DWORD dwIDontCare;

	if ( !VirtualProtect( pMemory, uiLen, ( (bLock) ? PAGE_READONLY : PAGE_EXECUTE_READWRITE ), &dwIDontCare ) )
	{
		if ( bLock )
			perror( "Virtual Class memory protect failed (using VirtualProtect)" );
		else
			perror( "Virtual Class memory unprotect failed (using VirtualProtect)" );
		return false;
	}
	return true;
}

#define VFUNC __stdcall

#define DEFVFUNC( funcname , returntype , proto ) \
	funcname##Func funcname = NULL; \
	void* funcname##Raw_Org = NULL; \
	CVirtualCallGate funcname##Gate;

#define HOOKVFUNC( classptr , index , funcname , newfunc ) \
	DeProtect((void*)VTBL( classptr ), ( index * sizeof(void*)) + 4, false ); \
	funcname##Raw_Org = (void*)VFN( classptr , index ); \
	funcname##Gate.Build( funcname##Raw_Org , newfunc , & funcname ); \
	*(ADDRTYPE*)PVFN( classptr , index ) = funcname##Gate.Gate(); \
	DeProtect((void*)VTBL( classptr ), ( index * sizeof(void*)) + 4, true );

#define UNHOOKVFUNC( classptr , index , funcname ) \
	DeProtect((void*)VTBL( classptr ), ( index * sizeof(void*)) + 4, false ); \
	*(ADDRTYPE*)PVFN( classptr , index ) = (ADDRTYPE) funcname##Raw_Org ; \
	DeProtect((void*)VTBL( classptr ), ( index * sizeof(void*)) + 4, true );

#elif defined __linux__ 

#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>

#ifndef PAGESIZE
#define PAGESIZE 4096
#endif

#define SH_LALIGN(x) (void*)((intptr_t)(x) & ~(PAGESIZE-1))
#define SH_LALDIF(x) ((intptr_t)(x) & (PAGESIZE-1))

inline bool DeProtect(void* pMemory, unsigned int uiLen, bool bLock = false)
{
	if ( mprotect(SH_LALIGN(pMemory) , uiLen + SH_LALDIF(pMemory), PROT_READ | PROT_EXEC | ( (bLock) ? 0 : PROT_WRITE ) ) )
	{
		if ( bLock )
			perror( "Virtual Class memory protect failed (using mprotect)" );
		else
			perror( "Virtual Class memory unprotect failed (using mprotect)" );
		return true;
	}
	return false;
}

#define VFUNC

#define DEFVFUNC( funcname , returntype , proto ) \
	funcname##Func funcname = NULL;

#define HOOKVFUNC( classptr , index , funcname , newfunc ) \
	DeProtect((void*)VTBL( classptr ), ( ( index + 1 ) * sizeof(void*)), false ); \
	funcname = ( funcname##Func )VFN( classptr , index ); \
	*(ADDRTYPE*)PVFN( classptr , index ) = (ADDRTYPE)newfunc ; \
	DeProtect((void*)VTBL( classptr ), ( ( index + 1 ) * sizeof(void*)), true );

#define UNHOOKVFUNC( classptr , index , funcname , newfunc ) \
	DeProtect((void*)VTBL( classptr ), ( ( index + 1 ) * sizeof(void*)), false ); \
	*(ADDRTYPE*)PVFN( classptr , index ) = (ADDRTYPE)funcname ; \
	DeProtect((void*)VTBL( classptr ), ( ( index + 1 ) * sizeof(void*)), true );
#else

   #error "Unsupported Platform."

#endif 

#define DEFVFUNC_( funcname , returntype , proto ) \
	HDEFVFUNC(funcname, returntype, proto); \
	DEFVFUNC(funcname, returntype, proto)

#endif // _VFNHOOK_H
