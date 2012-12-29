/*========================================================
* FunctionType.h
* @author Sergey Mikhtonyuk
* @date 21 June 2009
=========================================================*/
#ifndef _FUNCTIONTYPE_H__
#define _FUNCTIONTYPE_H__

#include "Delegate/DelegateBase.h"

namespace Reflection
{
	//////////////////////////////////////////////////////////////////////////

	/// Specification for function types
	/** @ingroup Reflection */
	class FunctionType : public Type
	{
	public:

		FunctionType(GenericInvoker inv, bool isMethod, Type* rt, Type** arguments, int argc);

		~FunctionType();

		size_t Name(char* buf, size_t size) const;

		size_t FullName(char* buf, size_t size) const;

		TypeTag Tag() const { return mIsMethod ? RL_T_METHOD : RL_T_FUNCTION; }

		size_t Size() const { return 0; }

		void Assign(void* to, void* val) const { NOT_IMPLEMENTED(); }

		bool ToString(void * value, std::string& out) const;

		bool TryParse(void * value, const char* str) const { return false; }

		void* CreateInstance() const { NOT_IMPLEMENTED(); return 0; }

		void DestroyInstance(void* v) const { }

		/// Make a function call (in case of methods first parameter is class instance)
		void Invoke(DelegateBase* deleg, void** args, void* result) const;

		ArchType getArchType() const { return RL_ARCH_FUNCTION; }

		bool Equal(const Type* other) const;

		/// Returns the result type
		Type* getReturnType() const { return mReturnType; }

		/// Returns the number of arguments
		int getArgumentNumber() const;

		/// Returns the types of arguments
		Type** getArgumentTypes() const { return (Type**)mArguments; }

	private:
		bool			mIsMethod;
		GenericInvoker	mInvoker;
		Type*			mReturnType;
		Type*			mArguments[DELEG_MAX_INVOKE_PARAMS + 1];
	};

	//////////////////////////////////////////////////////////////////////////

} // namespace

#endif // _FUNCTIONTYPE_H__