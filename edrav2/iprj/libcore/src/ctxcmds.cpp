//
// edrav2.libcore project
//
// Basic conext aware commands
//
// Author: Yury Podpruzhnikov (03.02.2019)
// Reviewer: Denis Bogdanov (14.03.2019)
//
#include "pch.h"
#include "command.h"
#include "ctxcmds.h"

// Set component for logging
#undef CMD_COMPONENT
#define CMD_COMPONENT "ctxcmd"

namespace openEdr {

//////////////////////////////////////////////////////////////////////////
//
// CallCtxCmd
//
//////////////////////////////////////////////////////////////////////////

//
//
//
void CallCtxCmd::VariantWithVars::init(const ContextAwareValue& cavSrc, 
	const Variant& vPatches, bool fClone)
{
	m_cavSrc = cavSrc;
	m_fClone = fClone;

	if (vPatches.isNull())
		return;

	m_patches.reserve(vPatches.getSize());
	for (auto [sKey, vValue] : Dictionary(vPatches))
		m_patches.emplace_back(Patch{ std::string(sKey), ContextAwareValue(vValue) });
}

//
//
//
std::optional<openEdr::Variant> CallCtxCmd::VariantWithVars::get(const Variant& vContext)
{
	if (!m_cavSrc.isValid() && m_patches.empty())
		return std::nullopt;

	Variant vSrc;
	if (m_cavSrc.isValid())
	{
		vSrc = m_cavSrc.get(vContext);
		if (m_fClone)
			vSrc = vSrc.clone();
	}
	else
	{
		// Create empty dictionary because m_patches is exist
		vSrc = Dictionary();
	}

	for (auto& patch : m_patches)
		putByPath(vSrc, patch.sDstPath, patch.cavValue.get(vContext));

	return vSrc;
}

//
//
//
void CallCtxCmd::finalConstruct(Variant vConfig)
{
	// get wrapped cmd
	do
	{
		Variant vCmd = vConfig.get("command");
		if (m_pWrappedCtxCmd = queryInterfaceSafe<IContextCommand>(vCmd))
			break;
		if (m_pWrappedCmd = queryInterfaceSafe<ICommand>(vCmd))
			break;
		m_cavWrappedDynCmd = vCmd;
	} while (false);

	// get param options
	m_params.init(vConfig.getSafe("params"), vConfig.get("ctxParams", nullptr), 
		vConfig.get("cloneParam", true));

	m_ctx.init(vConfig.getSafe("ctx"), vConfig.get("ctxVars", nullptr),
		vConfig.get("cloneCtx", true));
}

//
//
//
Variant CallCtxCmd::execute(Variant vContext, Variant /*vParam*/)
{
	auto vParams = m_params.get(vContext).value_or(nullptr);

	// call ICommand	
	if (m_pWrappedCmd)
		return m_pWrappedCmd->execute(vParams);

	// call IContextCommand
	if (m_pWrappedCtxCmd)
	{
		auto vCmdCtx = m_ctx.get(vContext).value_or(vContext);
		return m_pWrappedCtxCmd->execute(vCmdCtx, vParams);
	}

	// call dynamic cmd
	Variant vDynCmd = m_cavWrappedDynCmd.get(vContext);
	auto pDynCtxCmd = queryInterfaceSafe<IContextCommand>(vDynCmd);
	if (pDynCtxCmd)
	{
		auto vCmdCtx = m_ctx.get(vContext).value_or(vContext);
		return pDynCtxCmd->execute(vCmdCtx, vParams);
	}

	auto pDynCmd = queryInterfaceSafe<ICommand>(vDynCmd);
	if (pDynCmd)
		return pDynCmd->execute(vParams);

	// Invalid dynamic cmd
	error::InvalidArgument(SL, FMT("Invalid <command> parameter <" << vDynCmd << ">")).throwException();
}

//
//
//
void ForEachCtxCmd::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	// get wrapped cmd
	do
	{
		Variant vCmd = vConfig.get("command");
		if (m_pWrappedCtxCmd = queryInterfaceSafe<IContextCommand>(vCmd))
			break;
		if (m_pWrappedCmd = queryInterfaceSafe<ICommand>(vCmd))
			break;
		m_cavWrappedDynCmd = vCmd;
	} while (false);

	m_sPath = vConfig.get("path", "");
	m_cavData = vConfig.get("data", {});
	TRACE_END("Error during object creation");
}

//
//
//
Variant ForEachCtxCmd::execute(Variant vContext, [[maybe_unused]] Variant vParam)
{
	TRACE_BEGIN;
	Variant vData = m_cavData.get(vContext);

	// call ICommand
	// "path" is unsupported
	if (m_pWrappedCmd)
	{
		if (!vData.isSequenceLike())
			return Sequence({ m_pWrappedCmd->execute(vData) });

		Variant vResult = Sequence();
		for (auto vItem : vData)
			vResult.push_back(m_pWrappedCmd->execute(vItem));
		return vResult;
	}

	// call IContextCommand
	if (m_pWrappedCtxCmd)
	{
		if (!vData.isSequenceLike())
		{
			if (m_sPath.empty())
				return Sequence({ m_pWrappedCtxCmd->execute(vContext, vData) });
			variant::putByPath(vContext, m_sPath, vData, true /* fCreatePaths */);
			return Sequence({ m_pWrappedCtxCmd->execute(vContext) });
		}

		Variant vResult = Sequence();
		if (m_sPath.empty())
		{
			for (auto vItem : vData)
				vResult.push_back(m_pWrappedCtxCmd->execute(vContext, vItem));
			return vResult;
		}

		for (auto vItem : vData)
		{
			variant::putByPath(vContext, m_sPath, vItem, true /* fCreatePaths */);
			vResult.push_back(m_pWrappedCtxCmd->execute(vContext));
		}
		return vResult;
	}

	// call dynamic ctx cmd
	Variant vDynCmd = m_cavWrappedDynCmd.get(vContext);
	auto pDynCtxCmd = queryInterfaceSafe<IContextCommand>(vDynCmd);
	if (pDynCtxCmd)
	{
		if (!vData.isSequenceLike())
		{
			if (m_sPath.empty())
				return Sequence({ pDynCtxCmd->execute(vContext, vData) });
			variant::putByPath(vContext, m_sPath, vData, true /* fCreatePaths */);
			return Sequence({ pDynCtxCmd->execute(vContext) });
		}

		Variant vResult = Sequence();
		if (m_sPath.empty())
		{
			for (auto vItem : vData)
				vResult.push_back(pDynCtxCmd->execute(vContext, vItem));
			return vResult;
		}

		for (auto vItem : vData)
		{
			variant::putByPath(vContext, m_sPath, vItem, true /* fCreatePaths */);
			vResult.push_back(pDynCtxCmd->execute(vContext));
		}
		return vResult;
	}

	// call cmd
	// "path" is unsupported
	auto pDynCmd = queryInterfaceSafe<ICommand>(vDynCmd);
	if (pDynCmd)
	{
		if (!vData.isSequenceLike())
			return Sequence({ pDynCmd->execute(vData) });

		Variant vResult = Sequence();
		for (auto vItem : vData)
			vResult.push_back(pDynCmd->execute(vItem));
		return vResult;
	}

	// Invalid dynamic cmd
	error::InvalidArgument(SL, FMT("Invalid <command> parameter <" << vDynCmd << ">")).throwException();
	TRACE_END("Error during command execution");
}

//
//
//
void MakeDictionaryCtxCmd::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	if (!vConfig.has("data"))
		error::InvalidArgument(SL, "Missing field <data>").throwException();

	auto vData = vConfig["data"];
	
	if (vData.isDictionaryLike())
	{
		m_data.reserve(vData.getSize());
		for (const auto& [sPath, vValue] : Dictionary(vData))
			m_data.emplace_back(sPath, vValue);
		return;
	}
	
	if (vData.isSequenceLike())
	{
		m_data.reserve(vData.getSize());
		for (const auto& vItem : vData)
			m_data.emplace_back(vItem["name"], vItem["value"]);
		return;
	}

	error::InvalidArgument(SL, "Data must be dictionary or sequence").throwException();
	TRACE_END("Error during object creation");
}

//
//
//
Variant MakeDictionaryCtxCmd::execute(Variant vContext, [[maybe_unused]] Variant vParam)
{
	TRACE_BEGIN;
	Variant vResult = Dictionary();
	for (const auto& value : m_data)
		variant::putByPath(vResult, value.sPath, value.cavValue.get(vContext), true /* fCreatePaths */);
	return vResult;
	TRACE_END("Error during command execution");
}

//
//
//
void MakeSequenceCtxCmd::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	if (!vConfig.has("data"))
		error::InvalidArgument(SL, "Missing field <data>").throwException();

	auto vData = vConfig["data"];
	if (vData.isSequenceLike())
	{
		m_data.reserve(vData.getSize());
		for (const auto& vItem : vData)
			m_data.emplace_back(vItem);
		return;
	}
	m_data.emplace_back(vData);

	TRACE_END("Error during object creation");
}

//
//
//
Variant MakeSequenceCtxCmd::execute(Variant vContext, [[maybe_unused]] Variant vParam)
{
	TRACE_BEGIN;
	Variant vResult = Sequence();
	for (const auto& cavValue : m_data)
		vResult.push_back(cavValue.get(vContext));
	return vResult;
	TRACE_END("Error during command execution");
}

//
//
//
void CachedValueCtxCmd::finalConstruct(Variant vConfig)
{
	TRACE_BEGIN;
	if (!vConfig.has("path"))
		error::InvalidArgument(SL, "Missing field <path>").throwException();
	if (!vConfig.has("value"))
		error::InvalidArgument(SL, "Missing field <value>").throwException();

	m_sPath = vConfig["path"];
	m_cavValue = vConfig["value"];
	TRACE_END("Error during object creation");
}

//
//
//
Variant CachedValueCtxCmd::execute(Variant vContext, [[maybe_unused]] Variant vParam)
{
	TRACE_BEGIN;
	auto optValue = variant::getByPathSafe(vContext, m_sPath);
	if (optValue.has_value())
		return optValue.value();

	auto vValue = m_cavValue.get(vContext);
	variant::putByPath(vContext, m_sPath, vValue, true /* fCreatePaths */);
	return vValue;
	TRACE_END("Error during command execution");
}

} // namespace openEdr 
