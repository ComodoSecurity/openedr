//
// edrav2.libedr project
//
// Author: Yury Ermakov (16.04.2019)
// Reviewer:
//
#include "pch.h"
#include "policycompiler.h"
#include "eventenricher.h"
#include "signcontext.h"
#include "eventfilter.h"
#include "lane.h"
#include "outputfilter.h"

//
// Classes registration
//
CMD_BEGIN_LIBRARY_DEFINITION(libedr)
CMD_DEFINE_LIBRARY_CLASS(edr::policy::PolicyCompiler)
CMD_DEFINE_LIBRARY_CLASS(EventEnricher)
CMD_DEFINE_LIBRARY_CLASS(ContextService)
CMD_DEFINE_LIBRARY_CLASS(PerProcessEventFilter)
CMD_DEFINE_LIBRARY_CLASS(edr::LaneOperation)
CMD_DEFINE_LIBRARY_CLASS(OutputFilter)
CMD_END_LIBRARY_DEFINITION(libedr)
