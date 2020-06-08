#include "gdpsengine.hpp"


#include "Parser.hpp"
#include "Compiler.hpp"
#include "PSEngine.hpp"
#include "gdLogger.hpp"

#include <map>


#include "core/print_string.h"
#include "core/os/file_access.h"


void GDPSEngine::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_process"), &GDPSEngine::_process);
    ClassDB::bind_method(D_METHOD("load_game_from_file_path","fpath"), &GDPSEngine::load_game_from_file_path);
    ClassDB::bind_method(D_METHOD("get_level_state"), &GDPSEngine::get_level_state);
    ClassDB::bind_method(D_METHOD("send_input","input"), &GDPSEngine::send_input);
}

GDPSEngine::GDPSEngine() {
}

GDPSEngine::~GDPSEngine() {
    // add your cleanup here
}

void GDPSEngine::_init() {
    shared_ptr<PSLogger> gdlogger = make_shared<GDLogger>(GDLogger());
    m_psengine = PSEngine(gdlogger);
}


void GDPSEngine::_process(float delta) {

  
}

void GDPSEngine::load_game_from_file_path(String p_fpath)
{
    shared_ptr<PSLogger> gdlogger = make_shared<GDLogger>(GDLogger());

    Error err;
	FileAccess *gdfile = FileAccess::open(p_fpath, FileAccess::READ, &err);

    std::string file_content = "";
    if(gdfile->is_open())
    {
        print_line("gdpsengine : opened gd file");
        file_content = gdfile->get_as_utf8_string().utf8();
        print_line(file_content.c_str());
        
    }
    else
    {
        print_line("gdpsengine : did not open gd file");
    }

    optional<ParsedGame> parsed_game = Parser::parse_from_string(file_content,gdlogger);
    if(parsed_game.has_value())
    {
        print_line("gdpsengine : parsing complete,proceding to compilation") ;
        Compiler puzzle_compiler(gdlogger);
        std::optional<CompiledGame> compiled_puzzle_opt = puzzle_compiler.compile_game(parsed_game.value());
        if(!compiled_puzzle_opt.has_value())
        {
            print_line("gdpsengine : could not compile game properly"); 
        }
        else
        {
            m_psengine.load_game(compiled_puzzle_opt.value());
            m_psengine.Load_first_level();
            m_psengine.print_game_state();
        }
    }
    else
    {
        print_line("gdpsengine : something went wrong during the game parsing"); 
    }

    gdfile->close();
	memdelete(gdfile);
}

Dictionary GDPSEngine::get_level_state()
{
    PSEngine::Level level_state = m_psengine.get_level_state();
    Dictionary lvl;
    lvl["level_index"]= level_state.level_idx;
    lvl["width"]= level_state.width;
    lvl["height"]= level_state.height;

    Array cells;
    for(const PSEngine::Cell& cell: level_state.cells)
    {
        Dictionary cl;
        cl["x"] = cell.x;
        cl["y"] = cell.y;

        Array objects;
        for(const auto& pair: cell.objects)
        {
            objects.append(pair.first->identifier.c_str());
        }
        cl["objects"] = objects;

        cells.append(cl);
    }
    lvl["cells"] = cells;

    return lvl;
}

void GDPSEngine::send_input(String p_input)
{
    if(p_input == "undo")
    {
        m_psengine.undo();
    }
    else if (p_input == "restart")
    {
        m_psengine.restart_level();
    }
    else
    {
        std::map<String,PSEngine::InputType> input_mapping = {
            {"up", PSEngine::InputType::Up},
            {"left", PSEngine::InputType::Left},
            {"right", PSEngine::InputType::Right},
            {"down", PSEngine::InputType::Down},
            {"action", PSEngine::InputType::Action},
        };

        if( input_mapping.find(p_input) != input_mapping.end() )
        {
            m_psengine.receive_input(input_mapping[p_input]);
        }
        else
        {
            print_error("Incorrect input sent to the psengine");
        }
    }
    m_psengine.print_game_state();
}