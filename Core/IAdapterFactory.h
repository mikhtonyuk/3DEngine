/*========================================================
* AdapterFactory.h
* @author Sergey Mikhtonyuk
* @date 23 November 2008
=========================================================*/
#ifndef _IADAPTERFACTORY_H__
#define _IADAPTERFACTORY_H__

#include "../Core.COM/Interfaces.h"

namespace Core
{
	/// IAdapterFactory interface
	/** When you want to add some behavior to someone else's object 
	 *  or to add this behavior in more layer-preserving way than IAdaptable you can use
	 *  AdapterFactories. The principle is similar to IAdaplable except that
	 *  responsibility of adapter creation is now take the factory. Multiple adapter
	 *  factories form an adapter registry where you can register your factory and
	 *  query for adapters afterwards. One factory constructs adapters only for one source interface 
	 *  
	 *  @ingroup Core */
	SCOM_INTERFACE(IAdapterFactory, "babe39a9-529d-4bd5-853e-4a2974f3aa21", SCOM::IUnknown)
	{
	public:
		/// List of interfaces that can be adapted
		/** @return null-terminated list of GUIDs */
		virtual SCOM::GUID** SourceInterfaceList() = 0;

		/// List of adapter interfaces
		/** @return null-terminated list of GUIDs */
		virtual SCOM::GUID** DestInterfaceList() = 0;

		/// Constructs the adapter for a specified object
		/** @param object Object to be adapted
		 *  @param iid ID of adapter interface
		 *  @param ppAdapter pointer to the resulting object */
		virtual SCOM::HResult GetAdapter(SCOM::IUnknown* object, SCOM_RIID iid, void **ppAdapter) = 0;
	};


} // namespace


#endif // _ADAPTERFACTORY_H__