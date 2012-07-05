//
// Mani Admin Plugin
//
// Copyright © 2009-2012 Giles Millward (Mani). All rights reserved.
//
// This file is part of ManiAdminPlugin.
//
// Mani Admin Plugin is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Mani Admin Plugin is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Mani Admin Plugin.  If not, see <http://www.gnu.org/licenses/>.
//

//

#ifndef DETOUR_MACROS_H
#define DETOUR_MACROS_H

// ********************************************************************************
// ***                          for member functions							***
// ********************************************************************************
#define MEMBER_CALL(name) ( this->* name##_Original )

#define DECL_MEMBER_DETOUR10(name, returntype, param1, param2, param3, param4, param5, param6, param7, param8, param9, paramA ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9, paramA pA ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9, paramA pA ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9, paramA pA ) = 0;  \
	returntype name##Class::name( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9, paramA pA )

#define DECL_MEMBER_DETOUR10_void(name, param1, param2, param3, param4, param5, param6, param7, param8, param9, paramA ) \
	DECL_MEMBER_DETOUR10( name, void *, param1, param2, param3, param4, param5, param6, param7, param8, param9, paramA )

#define DECL_MEMBER_DETOUR9(name, returntype, param1, param2, param3, param4, param5, param6, param7, param8, param9 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9 ) = 0; \
	returntype name##Class::name( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9 )

#define DECL_MEMBER_DETOUR9_void(name, param1, param2, param3, param4, param5, param6, param7, param8, param9 ) \
	DECL_MEMBER_DETOUR9( name, void *, param1, param2, param3, param4, param5, param6, param7, param8, param9 )

#define DECL_MEMBER_DETOUR8(name, returntype, param1, param2, param3, param4, param5, param6, param7, param8 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8 ) = 0; \
	returntype name##Class::name( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8 )

#define DECL_MEMBER_DETOUR8_void(name, param1, param2, param3, param4, param5, param6, param7, param8 ) \
	DECL_MEMBER_DETOUR8( name, void *, param1, param2, param3, param4, param5, param6, param7, param8 )

#define DECL_MEMBER_DETOUR7(name, returntype, param1, param2, param3, param4, param5, param6, param7 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7 ) = 0; \
	returntype name##Class::name( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7 )

#define DECL_MEMBER_DETOUR7_void(name, param1, param2, param3, param4, param5, param6, param7 ) \
	DECL_MEMBER_DETOUR7( name, void *, param1, param2, param3, param4, param5, param6, param7 )

#define DECL_MEMBER_DETOUR6(name, returntype, param1, param2, param3, param4, param5, param6 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6 ) = 0; \
	returntype name##Class::name( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6 )

#define DECL_MEMBER_DETOUR6_void(name, param1, param2, param3, param4, param5, param6 ) \
	DECL_MEMBER_DETOUR6( name, void *, param1, param2, param3, param4, param5, param6 )

#define DECL_MEMBER_DETOUR5(name, returntype, param1, param2, param3, param4, param5 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5 ) = 0; \
	returntype name##Class::name( param1 p1, param2 p2, param3 p3, param4 p4, param5 p5 )

#define DECL_MEMBER_DETOUR5_void(name, param1, param2, param3, param4, param5 ) \
	DECL_MEMBER_DETOUR5( name, void *, param1, param2, param3, param4, param5 )

#define DECL_MEMBER_DETOUR4(name, returntype, param1, param2, param3, param4 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2, param3 p3, param4 p4 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2, param3 p3, param4 p4 ) = 0; \
	returntype name##Class::name( param1 p1, param2 p2, param3 p3, param4 p4 )

#define DECL_MEMBER_DETOUR4_void(name, param1, param2, param3, param4 ) \
	DECL_MEMBER_DETOUR4( name, void *, param1, param2, param3, param4 )

#define DECL_MEMBER_DETOUR3(name, returntype, param1, param2, param3 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2, param3 p3 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2, param3 p3 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2, param3 p3 ) = 0; \
	returntype name##Class::name( param1 p1, param2 p2, param3 p3 )

#define DECL_MEMBER_DETOUR3_void(name, param1, param2, param3 ) \
	DECL_MEMBER_DETOUR2( name, void *, param1, param2, param3 )

#define DECL_MEMBER_DETOUR2(name, returntype, param1, param2 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1, param2 p2 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1, param2 p2 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1, param2 p2 ) = 0; \
	returntype name##Class::name( param1 p1, param2 p2 )

#define DECL_MEMBER_DETOUR2_void(name, param1, param2 ) \
	DECL_MEMBER_DETOUR2( name, void *, param1, param2 )

#define DECL_MEMBER_DETOUR1(name, returntype, param1 ) \
	class name##Class { \
	public: \
		returntype name ( param1 p1 ); \
		static returntype ( name##Class::* name##_Original )( param1 p1 ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( param1 p1 ) = 0; \
	returntype name##Class::name( param1 p1 )

#define DECL_MEMBER_DETOUR1_void(name, param1 ) \
	DECL_MEMBER_DETOUR1( name, void *, param1 )

#define DECL_MEMBER_DETOUR0(name, returntype) \
	class name##Class { \
	public: \
		returntype name ( void ); \
		static returntype ( name##Class::* name##_Original )( void ); \
	}; \
	returntype ( name##Class::* name##Class::name##_Original )( void ) = 0; \
	returntype name##Class::name( void )

#define DECL_MEMBER_DETOUR0_void(name) \
	DECL_MEMBER_DETOUR0(name, void *)

#define GET_MEMBER_CALLBACK(name) (void *)GetFunctionAddress(&name##Class::name)
#define GET_MEMBER_TRAMPOLINE(name) (void **)(&name##Class::name##_Original)
// ********************************************************************************
// ***                        for non member functions							***
// ********************************************************************************
#define ASSIGN_ORIGINAL(name, addr) \
	if ( addr ) \
		name##OriginalFunc = &addr

#define NON_MEMBER_CALL(name)			((name##_)(*name##OriginalFunc))
#define GET_NON_MEMBER_CALLBACK(name)	_name##Func
#define GET_NON_MEMBER_TRAMPOLINE(name)	name##OriginalFunc

#define DECL_DETOUR10(name, returntype, param1, param2, param3, param4, param5, param6, param7, param8, param9, paramA) \
	typedef returntype (*name##_)(param1, param2, param3, param4, param5, param6, param7, param8, param9, paramA); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9, paramA pA)

#define DECL_DETOUR10_void(name, param1, param2, param3, param4, param5, param6, param7, param8, param9, paramA) \
	DECL_DETOUR10(name, void *, param1, param2, param3, param4, param5, param6, param7, param8, param9, paramA)

#define DECL_DETOUR9(name, returntype, param1, param2, param3, param4, param5, param6, param7, param8, param9) \
	typedef returntype (*name##_)(param1, param2, param3, param4, param5, param6, param7, param8, param9); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8, param9 p9)

#define DECL_DETOUR9_void(name, param1, param2, param3, param4, param5, param6, param7, param8, param9) \
	DECL_DETOUR9(name, void *, param1, param2, param3, param4, param5, param6, param7, param8, param9)

#define DECL_DETOUR8(name, returntype, param1, param2, param3, param4, param5, param6, param7, param8) \
	typedef returntype (*name##_)(param1, param2, param3, param4, param5, param6, param7, param8); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7, param8 p8)

#define DECL_DETOUR8_void(name, param1, param2, param3, param4, param5, param6, param7, param8) \
	DECL_DETOUR8(name, void *, param1, param2, param3, param4, param5, param6, param7, param8)

#define DECL_DETOUR7(name, returntype, param1, param2, param3, param4, param5, param6, param7) \
	typedef returntype (*name##_)(param1, param2, param3, param4, param5, param6, param7); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6, param7 p7)

#define DECL_DETOUR7_void(name, param1, param2, param3, param4, param5, param6, param7) \
	DECL_DETOUR7(name, void *, param1, param2, param3, param4, param5, param6, param7)

#define DECL_DETOUR6(name, returntype, param1, param2, param3, param4, param5, param6) \
	typedef returntype (*name##_)(param1, param2, param3, param4, param5, param6); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2, param3 p3, param4 p4, param5 p5, param6 p6)

#define DECL_DETOUR6_void(name, param1, param2, param3, param4, param5, param6) \
	DECL_DETOUR6(name, void *, param1, param2, param3, param4, param5, param6)

#define DECL_DETOUR5(name, returntype, param1, param2, param3, param4, param5) \
	typedef returntype (*name##_)(param1, param2, param3, param4, param5); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2, param3 p3, param4 p4, param5 p5)

#define DECL_DETOUR5_void(name, param1, param2, param3, param4, param5) \
	DECL_DETOUR5(name, void *, param1, param2, param3, param4, param5)

#define DECL_DETOUR4(name, returntype, param1, param2, param3, param4) \
	typedef returntype (*name##_)(param1, param2, param3, param4); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2, param3 p3, param4 p4)

#define DECL_DETOUR4_void(name, param1, param2, param3, param4) \
	DECL_DETOUR4(name, void *, param1, param2, param3, param4)

#define DECL_DETOUR3(name, returntype, param1, param2, param3) \
	typedef returntype (*name##_)(param1, param2, param3); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2, param3 p3)

#define DECL_DETOUR3_void(name, param1, param2, param3) \
	DECL_DETOUR3(name, void *, param1, param2, param3)

#define DECL_DETOUR2(name, returntype, param1, param2) \
	typedef returntype (*name##_)(param1, param2); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1, param2 p2)

#define DECL_DETOUR2_void(name, param1, param2) \
	DECL_DETOUR2(name, void *, param1, param2)

#define DECL_DETOUR1(name, returntype, param1) \
	typedef returntype (*name##_)(param1); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (param1 p1)

#define DECL_DETOUR1_void(name, param1) \
	DECL_DETOUR1(name, void *, param1)

#define DECL_DETOUR0(name, returntype) \
	typedef returntype (*name##_)( void); \
	name##_ name##Func; \
	void ** name##OriginalFunc; \
	returntype _name##Func (void)

#define DECL_DETOUR0_void(name) \
	DECL_DETOUR0(name, void *)
#endif // DETOUR_MACROS_H