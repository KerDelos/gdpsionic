#include "gdpsengine.hpp"

#include "Compiler.hpp"
#include "EnumHelpers.hpp"
#include "PSEngine.hpp"
#include "Parser.hpp"
#include "gdLogger.hpp"

#include <chrono>
#include <map>

#include "core/error/error_list.h"
#include "core/io/file_access.h"

void GDPSEngine::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_process"), &GDPSEngine::_process);
	ClassDB::bind_method(D_METHOD("load_game_from_file_path", "fpath"), &GDPSEngine::load_game_from_file_path);
	ClassDB::bind_method(D_METHOD("get_level_state"), &GDPSEngine::get_level_state);
	ClassDB::bind_method(D_METHOD("get_turn_deltas"), &GDPSEngine::get_turn_deltas);
	ClassDB::bind_method(D_METHOD("send_input", "input"), &GDPSEngine::send_input);
	ClassDB::bind_method(D_METHOD("tick", "delta"), &GDPSEngine::tick);
	ClassDB::bind_method(D_METHOD("get_level_count"), &GDPSEngine::get_level_count);
	ClassDB::bind_method(D_METHOD("get_messages_before_level", "p_level_idx"), &GDPSEngine::get_messages_before_level);
	ClassDB::bind_method(D_METHOD("get_messages_after_level", "p_level_idx"), &GDPSEngine::get_messages_after_level);
	ClassDB::bind_method(D_METHOD("load_level", "level_idx"), &GDPSEngine::load_level);
	ClassDB::bind_method(D_METHOD("is_level_complete"), &GDPSEngine::is_level_complete);
	ClassDB::bind_method(D_METHOD("get_texture_for_display"), &GDPSEngine::get_texture_for_display);
}

GDPSEngine::GDPSEngine() {
}

GDPSEngine::~GDPSEngine() {
	// add your cleanup here
}

void GDPSEngine::_init() {
	shared_ptr<PSLogger> gdlogger = make_shared<GDLogger>(GDLogger());
	gdlogger->log_verbosity = PSLogger::LogType::Warning;
	PSEngine::Config engine_config;
	engine_config.log_verbosity = PSLogger::LogType::Warning;
	m_psengine = PSEngine(engine_config, gdlogger);
}

void GDPSEngine::_process(float delta) {
}

void GDPSEngine::load_game_from_file_path(String p_fpath) {
	shared_ptr<PSLogger> gdlogger = make_shared<GDLogger>(GDLogger());

	Error err;
	Ref<FileAccess> gdfile = FileAccess::open(p_fpath, FileAccess::READ, &err);

	std::string file_content = "";
	if (gdfile->is_open()) {
		gdlogger->log(PSLogger::LogType::Log, "GDPsionic", "opened gd file !");
		file_content = gdfile->get_as_utf8_string().utf8();
	} else {
		gdlogger->log(PSLogger::LogType::Error, "GDPsionic", "did not open gd file");
	}

	optional<ParsedGame> parsed_game = Parser::parse_from_string(file_content, gdlogger);
	if (parsed_game.has_value()) {
		gdlogger->log(PSLogger::LogType::Log, "GDPsionic", "parsing complete,proceding to compilation");
		Compiler puzzle_compiler(gdlogger);
		std::optional<CompiledGame> compiled_puzzle_opt = puzzle_compiler.compile_game(parsed_game.value());
		if (!compiled_puzzle_opt.has_value()) {
			gdlogger->log(PSLogger::LogType::Error, "GDPsionic", "could not compile game properly");
		} else {
			gdlogger->log(PSLogger::LogType::Log, "GDPsionic", "loading game");
			m_psengine.load_game(compiled_puzzle_opt.value());
			m_compiled_game = compiled_puzzle_opt.value();
			cache_graphic_data();
			game_loaded = true; //todo this should probably be in the psengine somehow
		}
	} else {
		gdlogger->log(PSLogger::LogType::Error, "GDPsionic", "something went wrong during the game parsing");
	}

	gdfile->close();
}

Dictionary GDPSEngine::get_level_state() {
	PSEngine::Level level_state = m_psengine.get_level_state();
	Dictionary lvl;
	lvl["level_index"] = level_state.level_idx;
	lvl["width"] = level_state.size.x;
	lvl["height"] = level_state.size.y;

	Array cells;
	for (const PSEngine::Cell &cell : level_state.cells) {
		Dictionary cl;
		cl["x"] = cell.position.x;
		cl["y"] = cell.position.y;

		Array objects;
		for (const auto &pair : cell.objects) {
			objects.append(pair.first->identifier.c_str());
		}
		cl["objects"] = objects;

		cells.append(cl);
	}
	lvl["cells"] = cells;

	return lvl;
}

Dictionary GDPSEngine::send_input(String p_input) {
	Dictionary result;

	if (p_input == "undo") {
		m_psengine.undo();
	} else if (p_input == "restart") {
		m_psengine.restart_level();
	} else {
		std::map<String, PSEngine::InputType> input_mapping = {
			{ "up", PSEngine::InputType::Up },
			{ "left", PSEngine::InputType::Left },
			{ "right", PSEngine::InputType::Right },
			{ "down", PSEngine::InputType::Down },
			{ "action", PSEngine::InputType::Action },
		};

		if (input_mapping.find(p_input) != input_mapping.end()) {
			auto start_time = std::chrono::high_resolution_clock::now();

			auto history = m_psengine.receive_input(input_mapping[p_input]).value_or(PSEngine::TurnHistory());

			auto mid_time = std::chrono::high_resolution_clock::now();

			result = convert_turn_deltas(history);

			auto end_time = std::chrono::high_resolution_clock::now();

			auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
			auto input_duration = std::chrono::duration_cast<std::chrono::microseconds>(mid_time - start_time);
			auto convert_duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - mid_time);
			std::cout << "send_input ran in " << total_duration.count() << " microseconds.\n";
			std::cout << "input phase ran in " << input_duration.count() << " microseconds.\n";
			std::cout << "convert phase ran in " << convert_duration.count() << " microseconds.\n";

		} else {
			print_error("Incorrect input sent to the psengine");
		}
	}

	return result;
}

Dictionary GDPSEngine::tick(float p_delta) {
	return convert_turn_deltas(m_psengine.tick(p_delta).value_or(PSEngine::TurnHistory()));
}

Dictionary GDPSEngine::convert_turn_deltas(PSEngine::TurnHistory p_turn_delta) {
	Dictionary gd_turn;
	gd_turn["was_turn_cancelled"] = p_turn_delta.was_turn_cancelled;

	Array gd_turn_deltas;

	for (const auto &ps_subturn_delta : p_turn_delta.subturns) {
		Dictionary gd_subturn;

		Array gd_messages;
		for (const auto &ps_command : ps_subturn_delta.gather_all_subturn_commands()) {
			if (ps_command.type == CompiledGame::CommandType::Message) {
				gd_messages.append(ps_command.message.c_str());
			}
		}

		gd_subturn["messages"] = gd_messages;

		Array gd_subturn_deltas;
		for (const auto &ps_rule_delta : ps_subturn_delta.steps) {
			Dictionary gd_rule_delta;
			if (ps_rule_delta.is_movement_resolution) {
				gd_rule_delta["is_movement"] = true;

				Array gd_movements;
				for (const auto &ps_movement_delta : ps_rule_delta.movement_deltas) {
					Dictionary gd_movement;
					gd_movement["origin_x"] = ps_movement_delta.origin.x;
					gd_movement["origin_y"] = ps_movement_delta.origin.y;
					gd_movement["destination_x"] = ps_movement_delta.destination.x;
					gd_movement["destination_y"] = ps_movement_delta.destination.y;
					gd_movement["moved_successfully"] = ps_movement_delta.moved_successfully;
					gd_movement["move_direction"] = enum_to_str(ps_movement_delta.move_direction, PSEngine::to_absolute_direction).value_or("ERROR").c_str();
					gd_movement["object"] = ps_movement_delta.object.get() != nullptr ? ps_movement_delta.object->identifier.c_str() : "ERROR";

					gd_movements.append(gd_movement);
				}
				gd_rule_delta["movements"] = gd_movements;
			} else {
				gd_rule_delta["is_movement"] = false;

				gd_rule_delta["rule"] = ps_rule_delta.rule_applied.rule_line;

				Array gd_rule_apllications;

				for (const auto &ps_application_delta : ps_rule_delta.rule_application_deltas) {
					Dictionary gd_application_delta;
					gd_application_delta["direction"] = enum_to_str(ps_application_delta.rule_direction, PSEngine::to_absolute_direction).value_or("ERROR").c_str();

					//todo maybe this is not necessary information ?
					Array gd_pattern_match_infos;
					for (const auto &ps_pattern_match_info : ps_application_delta.match_infos) {
						Dictionary gd_pattern_match;
						gd_pattern_match["x"] = ps_pattern_match_info.origin.x;
						gd_pattern_match["y"] = ps_pattern_match_info.origin.y;

						Array gd_wildcard_match_distances;
						for (const auto &ps_wildcard_match_distance : ps_pattern_match_info.wildcard_match_distances) {
							gd_wildcard_match_distances.append(ps_wildcard_match_distance);
						}
						gd_pattern_match["wildcard_match_distances"] = gd_wildcard_match_distances;

						gd_pattern_match_infos.append(gd_pattern_match);
					}
					gd_application_delta["pattern_match_info"] = gd_pattern_match_infos;

					Array gd_deltas;

					for (const auto &ps_delta : ps_application_delta.object_deltas) {
						Dictionary gd_delta;
						gd_delta["x"] = ps_delta.cell_position.x;
						gd_delta["y"] = ps_delta.cell_position.y;
						//todo the pointer check shouldn't be necessary and probably means i need to fix something in psionic
						gd_delta["object"] = ps_delta.object.get() != nullptr ? ps_delta.object->identifier.c_str() : "ERROR";
						gd_delta["type"] = enum_to_str(ps_delta.type, CompiledGame::to_object_delta_type).value_or("ERROR").c_str();

						gd_deltas.append(gd_delta);
					}

					gd_application_delta["objects_deltas"] = gd_deltas;
					gd_rule_apllications.append(gd_application_delta);
				}

				gd_rule_delta["rule_applications"] = gd_rule_apllications;
			}
			gd_subturn_deltas.append(gd_rule_delta);
		}

		gd_subturn["steps"] = gd_subturn_deltas;
		gd_turn_deltas.append(gd_subturn);
	}

	gd_turn["subturns"] = gd_turn_deltas;

	return gd_turn;
}

Dictionary GDPSEngine::get_turn_deltas() {
	PSEngine::TurnHistory ps_turn_deltas = m_psengine.get_turn_deltas();

	return convert_turn_deltas(ps_turn_deltas);
}

Ref<Image> GDPSEngine::get_texture_for_display() {
	//gd script code, temporarily here so it doesn't get lost
	/*
	var image = Globals.psengine.get_texture_for_display()
	var im_data = image.data;
	var texture = ImageTexture.new()
	texture.create_from_image(image)
	#ResourceSaver.save("res://saved_texture.tres", texture)
	#for some reason it didn't work before saving it as a texture and using the saved texture in the sprite
	$Sprite.set_texture(texture);
	*/

	Ref<Image> image;
	// image.instance();

	// if (!game_loaded) {
	// 	//todo log error
	// 	return image;
	// }

	// static map<CompiledGame::Color::ColorName, Color> color_map = {
	// 	{ CompiledGame::Color::ColorName::None, Color(1.0, 0.5, 0.5, 1.0) },
	// 	{ CompiledGame::Color::ColorName::Black, Color::named("Black") },
	// 	{ CompiledGame::Color::ColorName::White, Color::named("White") },
	// 	{ CompiledGame::Color::ColorName::LightGray, Color::named("LightGray") },
	// 	{ CompiledGame::Color::ColorName::Gray, Color::named("Gray") },
	// 	{ CompiledGame::Color::ColorName::DarkGray, Color::named("DarkGray") },
	// 	{ CompiledGame::Color::ColorName::Grey, Color::named("Gray") },
	// 	{ CompiledGame::Color::ColorName::Red, Color::named("Red") },
	// 	{ CompiledGame::Color::ColorName::DarkRed, Color::named("DarkRed") },
	// 	{ CompiledGame::Color::ColorName::LightRed, Color::named("crimson") },
	// 	{ CompiledGame::Color::ColorName::Brown, Color::named("Brown") },
	// 	{ CompiledGame::Color::ColorName::DarkBrown, Color::named("saddlebrown") },
	// 	{ CompiledGame::Color::ColorName::LightBrown, Color::named("sandybrown") },
	// 	{ CompiledGame::Color::ColorName::Orange, Color::named("Orange") },
	// 	{ CompiledGame::Color::ColorName::Yellow, Color::named("Yellow") },
	// 	{ CompiledGame::Color::ColorName::Green, Color::named("Green") },
	// 	{ CompiledGame::Color::ColorName::DarkGreen, Color::named("DarkGreen") },
	// 	{ CompiledGame::Color::ColorName::LightGreen, Color::named("LightGreen") },
	// 	{ CompiledGame::Color::ColorName::Blue, Color::named("Blue") },
	// 	{ CompiledGame::Color::ColorName::LightBlue, Color::named("LightBlue") },
	// 	{ CompiledGame::Color::ColorName::DarkBlue, Color::named("DarkBlue") },
	// 	{ CompiledGame::Color::ColorName::Purple, Color::named("Purple") },
	// 	{ CompiledGame::Color::ColorName::Pink, Color::named("Pink") },
	// 	{ CompiledGame::Color::ColorName::Transparent, Color(0.0, 0.0, 0.0, 0.0) },
	// };

	// PSEngine::Level level = m_psengine.get_level_state();
	// const int PIXEL_NUMBER = 5; //todo this should be read for the engine or the compiled game
	// vector<vector<string>> level_content = get_ordered_level_objects_by_collision_layers();

	// image->create(level.size.x * PIXEL_NUMBER, level.size.y * PIXEL_NUMBER, false, Image::Format::FORMAT_RGBAF);

	// image->lock();
	// for (int x = 0; x < level.size.x * PIXEL_NUMBER; ++x) {
	// 	for (int y = 0; y < level.size.y * PIXEL_NUMBER; ++y) {
	// 		int cell_x = x / PIXEL_NUMBER;
	// 		int cell_y = y / PIXEL_NUMBER;
	// 		vector<string> current_cell = level_content[cell_y * level.size.x + cell_x];

	// 		int sprite_x = x % PIXEL_NUMBER;
	// 		int sprite_y = y % PIXEL_NUMBER;

	// 		bool first_layer = true;

	// 		for (const string &obj : current_cell) {
	// 			CompiledGame::ObjectGraphicData graphic_data = m_cached_graphic_data[obj];

	// 			if (graphic_data.pixels.size() == 0) {
	// 				image->set_pixel(x, y, color_map[graphic_data.colors[0].name]);
	// 			} else {
	// 				int pixel_color = graphic_data.pixels[sprite_y * PIXEL_NUMBER + sprite_x];
	// 				if (pixel_color == -1) {
	// 					if (first_layer) //so we don't override colors of layers below
	// 					{
	// 						image->set_pixel(x, y, color_map[CompiledGame::Color::ColorName::Transparent]);
	// 					}
	// 				} else {
	// 					image->set_pixel(x, y, color_map[graphic_data.colors[pixel_color].name]);
	// 				}
	// 			}
	// 			first_layer = false;
	// 		}
	// 	}
	// }

	// image->unlock();

	return image;
}

vector<vector<string>> GDPSEngine::get_ordered_level_objects_by_collision_layers() const {
	vector<vector<string>> result;

	for (const auto &cell : m_psengine.get_level_state().cells) {
		vector<string> cell_content;

		for (const auto &col_layer : m_compiled_game.collision_layers) {
			for (const auto &obj : cell.objects) {
				if (col_layer->is_object_on_layer(obj.first)) {
					cell_content.push_back(obj.first->identifier);
				}
			}
		}

		result.push_back(cell_content);
	}
	return result;
}

void GDPSEngine::cache_graphic_data() {
	m_cached_graphic_data.clear();

	for (const auto &pair : m_compiled_game.graphics_data) {
		m_cached_graphic_data[pair.first->identifier] = pair.second;
	}
}

int GDPSEngine::get_level_count() {
	return m_psengine.get_number_of_levels();
}

void GDPSEngine::load_level(int p_level_idx) {
	m_psengine.load_level(p_level_idx);
}

bool GDPSEngine::is_level_complete() {
	return m_psengine.is_level_won();
}

Array GDPSEngine::get_messages_before_level(int p_level_idx) {
	Array messages;
	for (const string &msg : m_psengine.get_messages_before_level(p_level_idx)) {
		messages.append(msg.c_str());
	}
	return messages;
}

Array GDPSEngine::get_messages_after_level(int p_level_idx) {
	Array messages;
	for (const string &msg : m_psengine.get_messages_after_level(p_level_idx)) {
		messages.append(msg.c_str());
	}
	return messages;
}
