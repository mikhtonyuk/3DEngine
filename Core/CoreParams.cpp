/*========================================================
* CoreParams.cpp
* @author Sergey Mikhtonyuk
* @date 21 May 2009
=========================================================*/
#include "CoreParams.h"
#include "GlobalEnvironment.h"
#include "../Core.FileSystem/FileSystem.h"
#include "../Core.COM/Intellectual.h"

namespace Core
{
	//////////////////////////////////////////////////////////////////////////
	using namespace SCOM;
	using namespace FileSystem;
	//////////////////////////////////////////////////////////////////////////

	bool CoreParams::ShouldLoad(const std::string& plugin) const
	{
		return mLoadOnlyPlugins.find(plugin) != mLoadOnlyPlugins.end();
	}

	//////////////////////////////////////////////////////////////////////////

	bool CoreParams::ParseFile(const char* file)
	{
		// Init xml adapter
		ComPtr<IFile> f = gEnv->FileSystem->GetResource(file);
		if(!f) return false;

		ComPtr<IXMLFileAdapter> adapter;
		gEnv->FileSystem->CreateFileAdapter(f, UUID_PPV(IXMLFileAdapter, adapter.wrapped()));
		if(!adapter) return false;

		try { adapter->Parse(); }
		catch (...) { return false; }

		TiXmlElement* xml = adapter->GetDoc().FirstChildElement();

		// Parse file
		TiXmlElement* lo = xml->FirstChildElement("loadonly");
		if(!lo) return false;

		TiXmlElement* pl = lo->FirstChildElement("plugin");
		while(pl)
		{
			const char* name = pl->Attribute("name");
			if(name)
				mLoadOnlyPlugins.insert(name);

			pl = pl->NextSiblingElement("plugin");
		}

		return mLoadOnlyPlugins.size() != 0;
	}

	//////////////////////////////////////////////////////////////////////////

} // namespace