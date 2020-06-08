#ifndef GDPSENGINE_H
#define GDPSENGINE_H

#include "core/reference.h"
#include "core/dictionary.h"

#include "PSEngine.hpp"
#include "PSLogger.hpp"

class GDPSEngine: public Reference {
    GDCLASS(GDPSEngine, Reference)

private:
    PSEngine m_psengine;

public:
    static void _bind_methods();

    GDPSEngine();
    ~GDPSEngine();

    void _init(); // our initializer called by Godot

    void _process(float delta);

    void load_game_from_file_path(String p_fpath);

    Dictionary get_level_state();
    void send_input(String p_input);
};

#endif