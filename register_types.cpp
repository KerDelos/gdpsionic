/* register_types.cpp */

#include "register_types.h"

#include "core/object/class_db.h"
#include "gdpsengine.hpp"

void initialize_gdpsionic_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	ClassDB::register_class<GDPSEngine>();
}

void uninitialize_gdpsionic_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}
	// Nothing to do here in this example.
}