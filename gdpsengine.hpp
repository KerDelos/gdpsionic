#ifndef GDPSENGINE_H
#define GDPSENGINE_H

#include "core/io/image.h"
#include "core/object/ref_counted.h"
#include "core/variant/dictionary.h"

#include <map>
#include <string>
#include <vector>

#include "CompiledGame.hpp"
#include "PSEngine.hpp"
#include "PSLogger.hpp"

class GDPSEngine : public RefCounted {
	GDCLASS(GDPSEngine, RefCounted)

private:
	PSEngine m_psengine;
	CompiledGame m_compiled_game;

	map<string, CompiledGame::ObjectGraphicData> m_cached_graphic_data;

	bool game_loaded = false;

public:
	static void _bind_methods();

	GDPSEngine();
	~GDPSEngine();

	void _init(); // our initializer called by Godot

	void _process(float delta);

	void load_game_from_file_path(String p_fpath);

	Dictionary get_level_state();

	Dictionary tick(float p_delta);
	Dictionary send_input(String p_input);
	Dictionary get_turn_deltas();

	Array get_messages_before_level(int p_level_idx);
	Array get_messages_after_level(int p_level_idx);

	int get_level_count();
	void load_level(int p_level_idx);

	bool is_level_complete();

	Ref<Image> get_texture_for_display();

protected:
	Dictionary convert_turn_deltas(PSEngine::TurnHistory p_turn_delta);

	vector<vector<string>> get_ordered_level_objects_by_collision_layers() const;
	void cache_graphic_data();
};

#endif
