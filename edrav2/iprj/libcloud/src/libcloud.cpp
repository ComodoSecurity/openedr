//
// edrav2.libcloud project
//
// Author: Yury Ermakov (11.03.2019)
// Reviewer: Denis Bogdanov (15.03.2019)
//
#include "pch.h"
#include "aws.h"
#include "fls.h"
#include "cloudsvc.h"
#include "cloudsvc_legacy.h"
#include "valkyrie.h"
#include "gcp.h"

//
// Classes registration
//
CMD_BEGIN_LIBRARY_DEFINITION(libcloud)
CMD_DEFINE_LIBRARY_CLASS(cloud::aws::Core)
CMD_DEFINE_LIBRARY_CLASS(cloud::aws::FirehoseClient)
CMD_DEFINE_LIBRARY_CLASS(cloud::fls::FlsService)
CMD_DEFINE_LIBRARY_CLASS(cloud::legacy::CloudService)
CMD_DEFINE_LIBRARY_CLASS(cloud::CloudService)
CMD_DEFINE_LIBRARY_CLASS(cloud::valkyrie::ValkyrieService)
CMD_DEFINE_LIBRARY_CLASS(cloud::gcp::PubSubClient)
CMD_DEFINE_LIBRARY_CLASS(cloud::gcp::Controller)
CMD_END_LIBRARY_DEFINITION(libcloud)
