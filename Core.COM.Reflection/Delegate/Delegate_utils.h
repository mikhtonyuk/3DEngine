namespace detail // hide the implementation details in a nested namespace.
{

	// implicit_cast< >
	// I believe this was originally going to be in the C++ standard but 
	// was left out by accident. It's even milder than static_cast.
	// I use it instead of static_cast<> to emphasize that I'm not doing
	// anything nasty. 
	// Usage is identical to static_cast<>
	template <class OutputClass, class InputClass>
	inline OutputClass implicit_cast(InputClass input)
	{
		return input;
	}

	// horrible_cast< >
	// This is truly evil. It completely subverts C++'s type system, allowing you 
	// to cast from any class to any other class. Technically, using a union 
	// to perform the cast is undefined behaviour (even in C). But we can see if
	// it is OK by checking that the union is the same size as each of its members.
	// horrible_cast<> should only be used for compiler-specific workarounds. 
	// Usage is identical to reinterpret_cast<>.

	// This union is declared outside the horrible_cast because BCC 5.5.1
	// can't inline a function with a nested class, and gives a warning.
	template <class OutputClass, class InputClass>
	union horrible_union
	{
		OutputClass out;
		InputClass in;
	};

	template <class OutputClass, class InputClass>
	inline OutputClass horrible_cast(const InputClass input)
	{
		horrible_union<OutputClass, InputClass> u;
		// Cause a compile-time error if in, out and u are not the same size.
		// If the compile fails here, it means the compiler has peculiar
		// unions which would prevent the cast from working.
		typedef int ERROR_CantUseHorrible_cast[sizeof(InputClass)==sizeof(u) && sizeof(InputClass)==sizeof(OutputClass) ? 1 : -1];
		u.in = input;
		return u.out;
	}

	////////////////////////////////////////////////////////////////////////////////
	//						Workarounds
	//
	////////////////////////////////////////////////////////////////////////////////

	// Backwards compatibility: This macro used to be necessary in the virtual inheritance
	// case for Intel and Microsoft. Now it just forward-declares the class.
#define FASTDELEGATEDECLARE(CLASSNAME)	class CLASSNAME;

	// Prevent use of the static function hack with the DOS medium model.
#ifdef __MEDIUM__
#undef FASTDELEGATE_USESTATICFUNCTIONHACK
#endif

	//	DefaultVoid - a workaround for 'void' templates in VC6.
	//
	//  (1) VC6 and earlier do not allow 'void' as a default template argument.
	//  (2) They also doesn't allow you to return 'void' from a function.
	//
	// Workaround for (1): Declare a dummy type 'DefaultVoid' which we use
	//   when we'd like to use 'void'. We convert it into 'void' and back
	//   using the templates DefaultVoidToVoid<> and VoidToDefaultVoid<>.
	// Workaround for (2): On VC6, the code for calling a void function is
	//   identical to the code for calling a non-void function in which the
	//   return value is never used, provided the return value is returned
	//   in the EAX register, rather than on the stack. 
	//   This is true for most fundamental types such as int, enum, void *.
	//   Const void * is the safest option since it doesn't participate 
	//   in any automatic conversions. But on a 16-bit compiler it might
	//   cause extra code to be generated, so we disable it for all compilers
	//   except for VC6 (and VC5).
#ifdef FASTDLGT_VC6
	// VC6 workaround
	typedef const void * DefaultVoid;
#else
	// On any other compiler, just use a normal void.
	typedef void DefaultVoid;
#endif

	// Translate from 'DefaultVoid' to 'void'.
	// Everything else is unchanged
	template <class T>
	struct DefaultVoidToVoid { typedef T type; };

	template <>
	struct DefaultVoidToVoid<DefaultVoid> {	typedef void type; };

	// Translate from 'void' into 'DefaultVoid'
	// Everything else is unchanged
	template <class T>
	struct VoidToDefaultVoid { typedef T type; };

	template <>
	struct VoidToDefaultVoid<void> { typedef DefaultVoid type; };



	////////////////////////////////////////////////////////////////////////////////
	//						Fast Delegates, part 1:
	//
	//		Conversion of member function pointer to a standard form
	//
	////////////////////////////////////////////////////////////////////////////////

	// GenericClass is a fake class, ONLY used to provide a type.
	// It is vitally important that it is never defined, so that the compiler doesn't
	// think it can optimize the invocation. For example, Borland generates simpler
	// code if it knows the class only uses single inheritance.

	// Compilers using Microsoft's structure need to be treated as a special case.
#ifdef  FASTDLGT_MICROSOFT_MFP

#ifdef FASTDLGT_HASINHERITANCE_KEYWORDS
	// For Microsoft and Intel, we want to ensure that it's the most efficient type of MFP 
	// (4 bytes), even when the /vmg option is used. Declaring an empty class 
	// would give 16 byte pointers in this case....
	class __single_inheritance GenericClass;
#endif
	// ...but for Codeplay, an empty class *always* gives 4 byte pointers.
	// If compiled with the /clr option ("managed C++"), the JIT compiler thinks
	// it needs to load GenericClass before it can call any of its functions,
	// (compiles OK but crashes at runtime!), so we need to declare an 
	// empty class to make it happy.
	// Codeplay and VC4 can't cope with the unknown_inheritance case either.
	class GenericClass {};
#else
	class GenericClass;
#endif

	// The size of a single inheritance member function pointer.
	const int SINGLE_MEMFUNCPTR_SIZE = sizeof(void (GenericClass::*)());

	//						SimplifyMemFunc< >::Convert()
	//
	//	A template function that converts an arbitrary member function pointer into the 
	//	simplest possible form of member function pointer, using a supplied 'this' pointer.
	//  According to the standard, this can be done legally with reinterpret_cast<>.
	//	For (non-standard) compilers which use member function pointers which vary in size 
	//  depending on the class, we need to use	knowledge of the internal structure of a 
	//  member function pointer, as used by the compiler. Template specialization is used
	//  to distinguish between the sizes. Because some compilers don't support partial 
	//	template specialisation, I use full specialisation of a wrapper struct.

	// general case -- don't know how to convert it. Force a compile failure
	template <int N>
	struct SimplifyMemFunc 
	{
		template <class X, class XFuncType, class GenericMemFuncType>
		inline static GenericClass* Convert(X *pthis, XFuncType function_to_bind, GenericMemFuncType &bound_func) 
		{ 
			// Unsupported member function type -- force a compile failure.
			// (it's illegal to have a array with negative size).
			typedef char ERROR_Unsupported_member_function_pointer_on_this_compiler[N-100];
			return 0; 
		}
	};

	// For compilers where all member func ptrs are the same size, everything goes here.
	// For non-standard compilers, only single_inheritance classes go here.
	template <>
	struct SimplifyMemFunc<SINGLE_MEMFUNCPTR_SIZE> 
	{	
		template <class X, class XFuncType, class GenericMemFuncType>
		inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, GenericMemFuncType &bound_func) 
		{
#if defined __DMC__  
			// Digital Mars doesn't allow you to cast between abitrary PMF's, 
			// even though the standard says you can. The 32-bit compiler lets you
			// static_cast through an int, but the DOS compiler doesn't.
			bound_func = horrible_cast<GenericMemFuncType>(function_to_bind);
#else 
			bound_func = reinterpret_cast<GenericMemFuncType>(function_to_bind);
#endif
			return reinterpret_cast<GenericClass *>(pthis);
		}
	};



	////////////////////////////////////////////////////////////////////////////////
	//						Fast Delegates, part 1b:
	//
	//					Workarounds for Microsoft and Intel
	//
	////////////////////////////////////////////////////////////////////////////////


	// Compilers with member function pointers which violate the standard (MSVC, Intel, Codeplay),
	// need to be treated as a special case.
#ifdef FASTDLGT_MICROSOFT_MFP

	// We use unions to perform horrible_casts. I would like to use #pragma pack(push, 1)
	// at the start of each function for extra safety, but VC6 seems to ICE
	// intermittently if you do this inside a template.

	// __multiple_inheritance classes go here
	// Nasty hack for Microsoft and Intel (IA32 and Itanium)
	template<>
	struct SimplifyMemFunc< SINGLE_MEMFUNCPTR_SIZE + sizeof(int) >  
	{
		template <class X, class XFuncType, class GenericMemFuncType>
		inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, GenericMemFuncType &bound_func) 
		{ 
			// We need to use a horrible_cast to do this conversion.
			// In MSVC, a multiple inheritance member pointer is internally defined as:
			union 
			{
				XFuncType func;
				struct 
				{	 
					GenericMemFuncType funcaddress; // points to the actual member function
					int delta;						// #BYTES to be added to the 'this' pointer
				} s;
			} u;
			// Check that the horrible_cast will work
			typedef int ERROR_CantUsehorrible_cast[sizeof(function_to_bind)==sizeof(u.s)? 1 : -1];
			u.func = function_to_bind;
			bound_func = u.s.funcaddress;
			return reinterpret_cast<GenericClass *>(reinterpret_cast<char *>(pthis) + u.s.delta); 
		}
	};

	// virtual inheritance is a real nuisance. It's inefficient and complicated.
	// On MSVC and Intel, there isn't enough information in the pointer itself to
	// enable conversion to a closure pointer. Earlier versions of this code didn't
	// work for all cases, and generated a compile-time error instead.
	// But a very clever hack invented by John M. Dlugosz solves this problem.
	// My code is somewhat different to his: I have no asm code, and I make no 
	// assumptions about the calling convention that is used.

	// In VC++ and ICL, a virtual_inheritance member pointer 
	// is internally defined as:
	struct MicrosoftVirtualMFP 
	{
		void (GenericClass::*codeptr)(); // points to the actual member function
		int delta;		// #bytes to be added to the 'this' pointer
		int vtable_index; // or 0 if no virtual inheritance
	};
	// The CRUCIAL feature of Microsoft/Intel MFPs which we exploit is that the
	// m_codeptr member is *always* called, regardless of the values of the other
	// members. (This is *not* true for other compilers, eg GCC, which obtain the
	// function address from the vtable if a virtual function is being called).
	// Dlugosz's trick is to make the codeptr point to a probe function which
	// returns the 'this' pointer that was used.

	// Define a generic class that uses virtual inheritance.
	// It has a trival member function that returns the value of the 'this' pointer.
	struct GenericVirtualClass : virtual public GenericClass
	{
		typedef GenericVirtualClass * (GenericVirtualClass::*ProbePtrType)();
		GenericVirtualClass * GetThis() { return this; }
	};

	// __virtual_inheritance classes go here
	template <>
	struct SimplifyMemFunc<SINGLE_MEMFUNCPTR_SIZE + 2*sizeof(int) >
	{

		template <class X, class XFuncType, class GenericMemFuncType>
		inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, GenericMemFuncType &bound_func) 
		{
			union 
			{
				XFuncType func;
				GenericClass* (X::*ProbeFunc)();
				MicrosoftVirtualMFP s;
			} u;
			u.func = function_to_bind;
			bound_func = reinterpret_cast<GenericMemFuncType>(u.s.codeptr);
			union 
			{
				GenericVirtualClass::ProbePtrType virtfunc;
				MicrosoftVirtualMFP s;
			} u2;
			// Check that the horrible_cast<>s will work
			typedef int ERROR_CantUsehorrible_cast[sizeof(function_to_bind)==sizeof(u.s)
				&& sizeof(function_to_bind)==sizeof(u.ProbeFunc)
				&& sizeof(u2.virtfunc)==sizeof(u2.s) ? 1 : -1];
			// Unfortunately, taking the address of a MF prevents it from being inlined, so 
			// this next line can't be completely optimised away by the compiler.
			u2.virtfunc = &GenericVirtualClass::GetThis;
			u.s.codeptr = u2.s.codeptr;
			return (pthis->*u.ProbeFunc)();
		}
	};

#if (_MSC_VER <1300)

	// Nasty hack for Microsoft Visual C++ 6.0
	// unknown_inheritance classes go here
	// There is a compiler bug in MSVC6 which generates incorrect code in this case!!
	template <>
	struct SimplifyMemFunc<SINGLE_MEMFUNCPTR_SIZE + 3*sizeof(int) >
	{
		template <class X, class XFuncType, class GenericMemFuncType>
		inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, 
			GenericMemFuncType &bound_func) {
				// There is an apalling but obscure compiler bug in MSVC6 and earlier:
				// vtable_index and 'vtordisp' are always set to 0 in the 
				// unknown_inheritance case!
				// This means that an incorrect function could be called!!!
				// Compiling with the /vmg option leads to potentially incorrect code.
				// This is probably the reason that the IDE has a user interface for specifying
				// the /vmg option, but it is disabled -  you can only specify /vmg on 
				// the command line. In VC1.5 and earlier, the compiler would ICE if it ever
				// encountered this situation.
				// It is OK to use the /vmg option if /vmm or /vms is specified.

				// Fortunately, the wrong function is only called in very obscure cases.
				// It only occurs when a derived class overrides a virtual function declared 
				// in a virtual base class, and the member function 
				// points to the *Derived* version of that function. The problem can be
				// completely averted in 100% of cases by using the *Base class* for the 
				// member fpointer. Ie, if you use the base class as an interface, you'll
				// stay out of trouble.
				// Occasionally, you might want to point directly to a derived class function
				// that isn't an override of a base class. In this case, both vtable_index 
				// and 'vtordisp' are zero, but a virtual_inheritance pointer will be generated.
				// We can generate correct code in this case. To prevent an incorrect call from
				// ever being made, on MSVC6 we generate a warning, and call a function to 
				// make the program crash instantly. 
				typedef char ERROR_VC6CompilerBug[-100];
				return 0; 
		}
	};


#else 

	// Nasty hack for Microsoft and Intel (IA32 and Itanium)
	// unknown_inheritance classes go here 
	// This is probably the ugliest bit of code I've ever written. Look at the casts!
	// There is a compiler bug in MSVC6 which prevents it from using this code.
	template <>
	struct SimplifyMemFunc<SINGLE_MEMFUNCPTR_SIZE + 3*sizeof(int) >
	{
		template <class X, class XFuncType, class GenericMemFuncType>
		inline static GenericClass *Convert(X *pthis, XFuncType function_to_bind, 
			GenericMemFuncType &bound_func) 
		{
			// The member function pointer is 16 bytes long. We can't use a normal cast, but
			// we can use a union to do the conversion.
			union 
			{
				XFuncType func;
				// In VC++ and ICL, an unknown_inheritance member pointer 
				// is internally defined as:
				struct 
				{
					GenericMemFuncType m_funcaddress; // points to the actual member function
					int delta;		// #bytes to be added to the 'this' pointer
					int vtordisp;		// #bytes to add to 'this' to find the vtable
					int vtable_index; // or 0 if no virtual inheritance
				} s;
			} u;
			// Check that the horrible_cast will work
			typedef int ERROR_CantUsehorrible_cast[sizeof(XFuncType)==sizeof(u.s)? 1 : -1];
			u.func = function_to_bind;
			bound_func = u.s.funcaddress;
			int virtual_delta = 0;
			if (u.s.vtable_index) 
			{ // Virtual inheritance is used
				// First, get to the vtable. 
				// It is 'vtordisp' bytes from the start of the class.
				const int * vtable = *reinterpret_cast<const int *const*>(
					reinterpret_cast<const char *>(pthis) + u.s.vtordisp );

				// 'vtable_index' tells us where in the table we should be looking.
				virtual_delta = u.s.vtordisp + *reinterpret_cast<const int *>( 
					reinterpret_cast<const char *>(vtable) + u.s.vtable_index);
			}
			// The int at 'virtual_delta' gives us the amount to add to 'this'.
			// Finally we can add the three components together. Phew!
			return reinterpret_cast<GenericClass *>(
				reinterpret_cast<char *>(pthis) + u.s.delta + virtual_delta);
		};
	};
#endif // MSVC 7 and greater

#endif // MS/Intel hacks

}  // namespace detail