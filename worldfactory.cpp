#include "worldfactory.h"
#include "file_finder.h"

// FILE I/O
#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif // _MSC_VER

#define WORLD_OPTION_FILE "worldoptions.txt"
#define SAVE_EXTENSION = ".sav"
#define SAVE_DIR = "save"

typedef int (worldfactory::*worldgen_display)(WINDOW *, WORLDPTR);
// single instance of world generator
worldfactory *world_generator;

/**
 * Returns whether or not the given (ASCII) character is usable in a (player)
 * character's name. Only printable symbols not reserved by the filesystem are
 * permitted.
 * @param ch The char to check
 * @return true if the char is allowed in a name, false if not
 */
bool char_allowed(char ch)
{
    // Allow everything EXCEPT the following reserved characters
    return (ch > 31 // 0 - 31 are control characters
         && ch < 127 // DEL character
         && ch != '/' && ch != '\\' // Path separators
         && ch != '?' && ch != '*' && ch != '%' // Wildcards
         && ch != ':' // Mount point/drive marker
         && ch != '|' // Output pipe
         && ch != '"' // Filename (with spaces) marker
         && ch != '>' && ch != '<'); // Input/output redirection
}

std::string world_options_header()
{
    return "\
# This file is autogenerated from the values picked during World Gen, and\n\
# should not be altered in any way once the world is generated. If you want\n\
# to edit it, you do so at your own risk.\n\
\n\
";
}

worldfactory::worldfactory()
{
    active_world = NULL;
}

worldfactory::~worldfactory()
{
    if (active_world){
        delete active_world;
    }
    for (std::map<std::string, WORLDPTR>::iterator it = all_worlds.begin(); it != all_worlds.end(); ++it){
        delete it->second;
    }
    all_worlds.clear();
    all_worldnames.clear();
}

WORLDPTR worldfactory::make_new_world()
{
    // Window variables
    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT)? (TERMY - FULL_SCREEN_HEIGHT)/2: 0;
    // World to return after generating
    WORLDPTR retworld = new WORLD();
    // set up window
    WINDOW *wf_win = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    // prepare tab display order
    std::vector<worldgen_display> tabs;
    std::vector<std::string> tab_strings;

    tabs.push_back(&worldfactory::show_worldgen_tab_options);
    tabs.push_back(&worldfactory::show_worldgen_tab_confirm);

    tab_strings.push_back(_("World Gen Options"));
    tab_strings.push_back(_("CONFIRMATION"));

    int curtab = 0;
    const int numtabs = tabs.size();
    while (curtab >= 0 && curtab < numtabs){
        draw_worldgen_tabs(wf_win, curtab, tab_strings);
        curtab += (world_generator->*tabs[curtab])(wf_win, retworld);

        if (curtab < 0){
            if (!query_yn(_("Do you want to abort World Generation?"))){
                curtab = 0;
            }
        }
        if (curtab == numtabs){
            if (!query_yn(_("Finish construction of world and proceed?"))){
                curtab = numtabs - 1;
            }
        }
    }
    if (curtab < 0){
        delete retworld;
        return NULL;
    }

    // add world to world list
    all_worlds[retworld->world_name] = retworld;
    all_worldnames.push_back(retworld->world_name);

    save_world(retworld);
    return retworld;
}

WORLDPTR worldfactory::make_new_world(special_game_id special_type)
{
    std::string worldname;
    switch(special_type){
        case SGAME_TUTORIAL:
            worldname = "TUTORIAL";
            break;
        case SGAME_DEFENSE:
            worldname = "DEFENSE";
            break;
        default: return NULL;
    }

    // look through worlds and see if worlname exists already. if so then just return
    // it instead of making a new world
    if (all_worlds.find(worldname) != all_worlds.end()){
        return all_worlds[worldname];
    }

    WORLDPTR special_world = new WORLD();
    special_world->world_name = worldname;

    if (special_world->world_options.size() == 0){
        for (std::map<std::string, cOpt>::iterator it = OPTIONS.begin(); it != OPTIONS.end(); ++it){
            if (it->second.getPage() == "world_default"){
                special_world->world_options[it->first] = it->second;
            }
        }
    }
    special_world->world_options["DELETE_WORLD"].setValue("yes");

    // add world to world list!
    all_worlds[worldname] = special_world;
    all_worldnames.push_back(worldname);

    save_world(special_world);

    return special_world;
}

WORLDPTR worldfactory::load_world(std::string world_name, bool setactive)
{
    // TODO: implement this
    return NULL;
}

void worldfactory::set_active_world(WORLDPTR world)
{
    world_generator->active_world = world;
    if (world){
        ACTIVE_WORLD_OPTIONS = world->world_options;
        awo_populated = true;
    }else{
        awo_populated = false;
    }
}

void worldfactory::save_world(WORLDPTR world)
{

}

std::map<std::string, WORLDPTR> worldfactory::get_all_worlds()
{
    std::map<std::string, WORLDPTR> retworlds;

    if (all_worlds.size() > 0){
        for (std::map<std::string, WORLDPTR>::iterator it = all_worlds.begin(); it != all_worlds.end(); ++it){
            delete it->second;
        }
        all_worlds.clear();
        all_worldnames.clear();
    }
    // get the world option files. These determine the validity of a world
    std::vector<std::string> world_dirs;
    world_dirs = file_finder::get_folders_from_path(WORLD_OPTION_FILE, SAVE_DIR, true);

    // check to see if there are >0 world directories found
    if (world_dirs.size() > 0){
        // worlds exist by having an option file
        // create worlds
        for (int i = 0; i < world_dirs.size(); ++i){
            // get the option file again
            // we can assume that there is only one world_op_file, so just collect the first path
            std::string world_op_file = file_finder::get_files_from_path(WORLD_OPTION_FILE, world_dirs[i], false)[0];
            // get the save files
            std::vector<std::string> world_sav_files=file_finder::get_files_from_path(SAVE_EXTENSION, world_dirs[i], false);
            // split the save file names between the directory and the extension
            for (int j = 0; j < world_sav_files.size(); ++j){
                unsigned save_index = world_sav_files[j].find(SAVE_EXTENSION);
                world_sav_files[j] = world_sav_files[j].substr(world_dirs[i].size() + 1, save_index);
            }
            // the directory name is the name of the world
            std::string worldname;
            unsigned name_index = world_dirs[i].find_last_of("/\\");
            worldname = world_dirs[i].substr(name_index + 1);

            // create and store the world
            retworlds[worldname] = new WORLD();
            // give the world a name
            retworlds[worldname]->world_name = worldname;
            all_worldnames.push_back(worldname);
            // add sav files
            for (int j = 0; j < world_sav_files.size(); ++j){
                retworlds[worldname]->world_saves.push_back(world_sav_files[j]);
            }
            // load options into the world
            retworlds[worldname]->world_options = get_world_options(world_op_file);
        }
    }
    all_worlds = retworlds;
    return retworlds;
}

WORLDPTR worldfactory::pick_world()
{
    return NULL;
}

WORLDPTR worldfactory::pick_world(special_game_id special_type)
{
    return NULL;
}

void worldfactory::remove_world(std::string worldname)
{

}

std::string worldfactory::pick_random_name()
{
    return "WOOT";
}

int worldfactory::show_worldgen_tab_options(WINDOW* win, WORLDPTR world)
{
    return 0;
}

int worldfactory::show_worldgen_tab_confirm(WINDOW* win, WORLDPTR world)
{
    return 0;
}

void worldfactory::draw_worldgen_tabs(WINDOW* win, int current, std::vector<std::string> tabs)
{

}

bool worldfactory::valid_worldname(std::string name)
{
    return true;
}

std::map<std::string, cOpt> worldfactory::get_world_options(std::string path)
{
    std::map<std::string, cOpt> retoptions;
    std::ifstream fin;

    fin.open(path.c_str());

    if (!fin.is_open()){
        fin.close();
        save_options();
        fin.open(path.c_str());
        if (!fin.is_open()){
            fin.close();
            DebugLog() << "Could neither read nor create world options file\n";
            return retoptions;
        }
    }
    std::string sLine;

    while (!fin.eof()){
        getline(fin, sLine);

        if (sLine != "" && sLine[0] != '#' && std::count(sLine.begin(), sLine.end(), ' ') == 1){
            int ipos = sLine.find(' ');
            retoptions[sLine.substr(0, ipos)] = OPTIONS[sLine.substr(0, ipos)]; // init to OPTIONS current
            retoptions[sLine.substr(0, ipos)].setValue(sLine.substr(ipos + 1, sLine.length()));
        }
    }
    fin.close();

    return retoptions;
}
