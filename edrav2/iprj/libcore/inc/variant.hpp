//
// EDRAv2.libcore project
// Variant & basic data types declaration
// 
// Autor: Yury Podpruzhnikov (30.11.2018)
// Reviewer: Denis Bogdanov (10.12.2018)
//
#pragma once

#include "basic.hpp"

// Forward declarations
#include "variant/forward.hpp"

// Simple types
#include "variant/detail/value.hpp"

// The Variant class declaration
#include "variant/variant_forward.hpp"

// Definition of classes which is used by the Variant class 
#include "variant/iterator.hpp"
#include "variant/detail/basic_dictionary.hpp"
#include "variant/detail/basic_sequence.hpp"
#include "variant/detail/value_dataproxy.hpp"

// The Variant class implementation
#include "variant/variant_impl.hpp"

// Definition of classes which is NOT used by the Variant class
#include "variant/sequence.hpp"
#include "variant/dictionary.hpp"

// Serialization 
#include "variant/serialize.hpp"

// VariantProxy 
#include "variant/variantproxy.hpp"

//
// Put basic data types to global cmd namespace
//
namespace cmd {

using variant::Variant;
using variant::Dictionary;
using variant::Sequence;

} // namespace cmd
