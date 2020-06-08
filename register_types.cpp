/* register_types.cpp */

#include "register_types.h"

#include "core/class_db.h"
#include "gdpsengine.hpp"

void register_gdpsionic_types() {
    ClassDB::register_class<GDPSEngine>();
}

void unregister_gdpsionic_types() {
   // Nothing to do here in this example.
}