#include "worldfactory.h"
#include "file_finder.h"
#include "char_validity_check.h"

// FILE I/O
#include <sys/stat.h>
#ifdef _MSC_VER
#include "wdirent.h"
#include <direct.h>
#else
#include <dirent.h>
#endif // _MSC_VER

#define WORLD_OPTION_FILE "worldoptions.txt"
#define SAVE_MASTER "master.gsav"
#define SAVE_EXTENSION ".sav"
#define SAVE_DIR "save"
#define PATH_SEPARATOR "/"

typedef int (worldfactory::*worldgen_display)(WINDOW *, WORLDPTR);
// single instance of world generator
worldfactory *world_generator;

std::string world_options_header()
{
    return "\
# This file is autogenerated from the values picked during World Gen, and\n\
# should not be altered in any way once the world is generated. If you want\n\
# to edit it, you do so at your own risk.\n\
\n\
";
}


std::string get_next_valid_worldname(std::string test, worldfactory *factory)
{
    int test_value = 1;
    std::stringstream test_worldname;
    const std::string test_world_prefix = test + std::string(1, ' ');
    bool valid = factory->valid_worldname(test, true);
    std::string worldname = test;
    while (!valid){
        test_worldname.str("");
        test_worldname << test_world_prefix << test_value;
        if (factory->valid_worldname(test_worldname.str(), true)){
            worldname = test_worldname.str();
            valid = true;
        }else{
            ++test_value;
        }
    }

    return worldname;
}

worldfactory::worldfactory()
{
    active_world = NULL;
}

worldfactory::~worldfactory()
{
    for (std::map<std::string, WORLDPTR>::iterator it = all_worlds.begin(); it != all_worlds.end(); ++it) {
        delete it->second;
        it->second = NULL;
    }
    all_worlds.clear();
    all_worldnames.clear();
}

WORLDPTR worldfactory::make_new_world()
{
    // Window variables
    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;
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
    int lasttab; // give placement memory to menus, sorta.
    const int numtabs = tabs.size();
    while (curtab >= 0 && curtab < numtabs) {
        lasttab = curtab;
        draw_worldgen_tabs(wf_win, curtab, tab_strings);
        curtab += (world_generator->*tabs[curtab])(wf_win, retworld);

        if (curtab < 0) {
            if (!query_yn(_("Do you want to abort World Generation?"))) {
                curtab = lasttab;
            }
        }
    }
    if (curtab < 0) {
        delete retworld;
        return NULL;
    }

    // add world to world list
    all_worlds[retworld->world_name] = retworld;
    all_worldnames.push_back(retworld->world_name);

    std::stringstream path;
    path << SAVE_DIR << PATH_SEPARATOR << retworld->world_name;
    retworld->world_path = path.str();

    if (!save_world(retworld)) {
        std::string worldname = retworld->world_name;
        std::vector<std::string>::iterator it = std::find(all_worldnames.begin(), all_worldnames.end(), worldname);
        all_worldnames.erase(it);
        delete all_worlds[worldname];
        delete retworld;
        all_worlds.erase(worldname);
        return NULL;
    }
    return retworld;
}

WORLDPTR worldfactory::make_new_world(special_game_id special_type)
{
    std::string worldname;
    switch(special_type) {
        case SGAME_TUTORIAL:
            worldname = "TUTORIAL";
            break;
        case SGAME_DEFENSE:
            worldname = "DEFENSE";
            break;
        default:
            return NULL;
    }

    // look through worlds and see if worlname exists already. if so then just return
    // it instead of making a new world
    if (all_worlds.find(worldname) != all_worlds.end()) {
        return all_worlds[worldname];
    }

    WORLDPTR special_world = new WORLD();
    special_world->world_name = worldname;

    if (special_world->world_options.size() == 0) {
        for (std::map<std::string, cOpt>::iterator it = OPTIONS.begin(); it != OPTIONS.end(); ++it) {
            if (it->second.getPage() == "world_default") {
                special_world->world_options[it->first] = it->second;
            }
        }
    }
    special_world->world_options["DELETE_WORLD"].setValue("yes");

    // add world to world list!
    all_worlds[worldname] = special_world;
    all_worldnames.push_back(worldname);

    std::stringstream path;
    path << SAVE_DIR << PATH_SEPARATOR << worldname;
    special_world->world_path = path.str();

    if (!save_world(special_world)) {
        std::vector<std::string>::iterator it = std::find(all_worldnames.begin(), all_worldnames.end(), worldname);
        all_worldnames.erase(it);
        delete all_worlds[worldname];
        delete special_world;
        all_worlds.erase(worldname);
        return NULL;
    }

    return special_world;
}

WORLDPTR worldfactory::convert_to_world(std::string origin_path)
{
    // prompt for worldname? Nah, just make a worldname... the user can fix it later if they really don't want this as a name...
    std::string worldname = get_next_valid_worldname("ConvWorld", this);

    // check and loop on validity

    // create world informations
    WORLDPTR newworld = new WORLD();
    newworld->world_name = worldname;

    std::stringstream path;
    path << SAVE_DIR << PATH_SEPARATOR << worldname;
    newworld->world_path = path.str();

    // save world as conversion world
    if (save_world(newworld, true)){
        // move files from origin_path into new world path
        std::vector<std::string> origin_files = file_finder::get_files_from_path(".", origin_path, false);
        for (unsigned i = 0; i < origin_files.size(); ++i){
            std::string filename = origin_files[i].substr(origin_files[i].find_last_of("/\\"));

            rename(origin_files[i].c_str(), std::string(newworld->world_path + filename).c_str());
        }

        DebugLog() << "worldfactory::convert_to_world -- World Converted Successfully!\n";
        return newworld;
    }else{
        // something horribly wrong happened
        DebugLog() << "worldfactory::convert_to_world -- World Conversion Failed!\n";
        return NULL;
    }
}

void worldfactory::set_active_world(WORLDPTR world)
{
    world_generator->active_world = world;
    if (world) {
        ACTIVE_WORLD_OPTIONS = world->world_options;
    }else{
        ACTIVE_WORLD_OPTIONS.clear();
    }
}

bool worldfactory::save_world(WORLDPTR world, bool is_conversion)
{
    // if world is NULL then change it to the active_world
    if (!world) {
        world = active_world;
    }
    // if the active_world is NULL then return w/o saving
    if (!world) {
        return false;
    }

    std::ofstream fout;
    std::stringstream woption;

    woption << world->world_path << "/" << WORLD_OPTION_FILE;

    DIR *dir = opendir(world->world_path.c_str());

    if (!dir) {
        // if opendir doesn't work, the *dir pointer is empty.  If we try to close it, it creates a segfault.

#if(defined _WIN32 || defined __WIN32__)
        mkdir(world->world_path.c_str());
#else
        mkdir(world->world_path.c_str(), 0777);
#endif
        dir = opendir(world->world_path.c_str());
    }
    if (!dir) {
        // if opendir doesn't work, the *dir pointer is empty.  If we try to close it, it creates a segfault.

        DebugLog() << "Unable to create or open world[" << world->world_name << "] directory for saving\n";
        return false;
    }
    if (dir)
        closedir(dir); // don't need to keep the directory open

    if (!is_conversion){
        fout.open(woption.str().c_str());
        if (!fout.is_open()) {
            fout.close();
            return false;
        }
        fout << world_options_header() << std::endl;

        for (std::map<std::string, cOpt>::iterator it = world->world_options.begin(); it != world->world_options.end(); ++it) {
            fout << "#" << it->second.getTooltip() << std::endl;
            fout << "#Default: " << it->second.getDefaultText() << std::endl;
            fout << it->first << " " << it->second.getValue() << std::endl << std::endl;
        }
        fout.close();
    }
    return true;
}

std::map<std::string, WORLDPTR> worldfactory::get_all_worlds()
{
    std::map<std::string, WORLDPTR> retworlds;

    std::vector<std::string> qualifiers;
    qualifiers.push_back(WORLD_OPTION_FILE);
    qualifiers.push_back(SAVE_MASTER);

    if (all_worlds.size() > 0) {
        for (std::map<std::string, WORLDPTR>::iterator it = all_worlds.begin(); it != all_worlds.end(); ++it) {
            delete it->second;
        }
        all_worlds.clear();
        all_worldnames.clear();
    }
    // get the master files. These determine the validity of a world
    std::vector<std::string> world_dirs = file_finder::get_directories_with(qualifiers, SAVE_DIR, true);

    // check to see if there are >0 world directories found
    if (world_dirs.size() > 0) {
        // worlds exist by having an option file
        // create worlds
        for (unsigned i = 0; i < world_dirs.size(); ++i) {
            // get the option file again
            // we can assume that there is only one master.gsav, so just collect the first path
            bool no_options = true;
            std::vector<std::string> detected_world_op = file_finder::get_files_from_path(WORLD_OPTION_FILE, world_dirs[i], false);
            std::string world_op_file = WORLD_OPTION_FILE;
            if ( ! detected_world_op.empty() ) {
                no_options = false;
            }
            // get the save files
            std::vector<std::string> world_sav_files = file_finder::get_files_from_path(SAVE_EXTENSION, world_dirs[i], false);
            // split the save file names between the directory and the extension
            for (unsigned j = 0; j < world_sav_files.size(); ++j) {
                size_t save_index = world_sav_files[j].find(SAVE_EXTENSION);
                world_sav_files[j] = world_sav_files[j].substr(world_dirs[i].size() + 1, save_index - (world_dirs[i].size() + 1));
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
            for (unsigned j = 0; j < world_sav_files.size(); ++j) {
                retworlds[worldname]->world_saves.push_back(world_sav_files[j]);
            }
            // set world path
            retworlds[worldname]->world_path = world_dirs[i];

            // load options into the world
            if ( no_options ) {
                for (std::map<std::string, cOpt>::iterator it = OPTIONS.begin(); it != OPTIONS.end(); ++it) {
                    if (it->second.getPage() == "world_default") {
                        retworlds[worldname]->world_options[it->first] = it->second;
                    }
                }
                retworlds[worldname]->world_options["DELETE_WORLD"].setValue("yes");
                save_world(retworlds[worldname]);
            } else {
                retworlds[worldname]->world_options = get_world_options(detected_world_op[0]);
            }
        }
    }
    // check to see if there exists a worldname "save" which denotes that a world exists in the save
    // directory and not in a sub-world directory
    if (retworlds.find("save") != retworlds.end()){
        WORLDPTR converted_world = convert_to_world(retworlds["save"]->world_path);
        if (converted_world){
            converted_world->world_saves = retworlds["save"]->world_saves;
            converted_world->world_options = retworlds["save"]->world_options;

            std::vector<std::string>::iterator oldindex = std::find(all_worldnames.begin(), all_worldnames.end(), "save");

            delete retworlds["save"];
            retworlds.erase("save");
            all_worldnames.erase(oldindex);

            retworlds[converted_world->world_name] = converted_world;
            all_worldnames.push_back(converted_world->world_name);
        }
    }
    all_worlds = retworlds;
    return retworlds;
}

WORLDPTR worldfactory::pick_world()
{
    std::map<std::string, WORLDPTR> worlds = get_all_worlds();
    std::vector<std::string> world_names = all_worldnames;

    // filter out special worlds (TUTORIAL | DEFENSE) from world_names
    for (std::vector<std::string>::iterator it = world_names.begin(); it != world_names.end();) {
        if (*it == "TUTORIAL" || *it == "DEFENSE") {
            it = world_names.erase(it);
        } else {
            ++it;
        }
    }
    // if there is only one world to pick from, autoreturn it
    if (world_names.size() == 1) {
        return worlds[world_names[0]];
    }
    // if there are no worlds to pick from, immediately try to make one
    else if (world_names.empty()) {
        return make_new_world();
    }

    const int iTooltipHeight = 3;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 3 - iTooltipHeight;
    const int num_pages = world_names.size() / iContentHeight + 1; // at least 1 page
    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

    std::map<int, bool> mapLines;
    mapLines[3] = true;

    std::map<int, std::vector<std::string> > world_pages;
    int worldnum = 0;
    for (int i = 0; i < num_pages; ++i) {
        world_pages[i] = std::vector<std::string>();
        for (int j = 0; j < iContentHeight; ++j) {
            world_pages[i].push_back(world_names[worldnum++]);
            if (worldnum == world_names.size()) {
                break;
            }
        }
    }
    int sel = 0, selpage = 0;

    WINDOW *w_worlds_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);
    WINDOW *w_worlds_tooltip = newwin(iTooltipHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY, 1 + iOffsetX);
    WINDOW *w_worlds_header = newwin(1, FULL_SCREEN_WIDTH - 2, 1 + iTooltipHeight + iOffsetY, 1 + iOffsetX);
    WINDOW *w_worlds        = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    draw_border(w_worlds_border);
    mvwputch(w_worlds_border, 4, 0, c_dkgray, LINE_XXXO); // |-
    mvwputch(w_worlds_border, 4, 79, c_dkgray, LINE_XOXX); // -|

    for (std::map<int, bool>::iterator iter = mapLines.begin(); iter != mapLines.end(); ++iter) {
        mvwputch(w_worlds_border, FULL_SCREEN_HEIGHT - 1, iter->first + 1, c_dkgray, LINE_XXOX); // _|_
    }

    mvwprintz(w_worlds_border, 0, 31, c_ltred, _(" WORLD SELECTION "));
    wrefresh(w_worlds_border);

    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_worlds_header, 0, i, c_dkgray, LINE_OXXX);
        } else {
            mvwputch(w_worlds_header, 0, i, c_dkgray, LINE_OXOX); // Draw header line
        }
    }

    wrefresh(w_worlds_header);

    char ch = ' ';

    std::stringstream sTemp;

    do {
        //Clear the lines
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w_worlds, i, j, c_dkgray, LINE_XOXO);
                } else {
                    mvwputch(w_worlds, i, j, c_black, ' ');
                }

                if (i < iTooltipHeight) {
                    mvwputch(w_worlds_tooltip, i, j, c_black, ' ');
                }
            }
        }

        //Draw World Names
        for (unsigned i = 0; i < world_pages[selpage].size(); ++i) {
            sTemp.str("");
            sTemp << i + 1;
            mvwprintz(w_worlds, i, 0, c_white, sTemp.str().c_str());
            mvwprintz(w_worlds, i, 4, c_white, "");


            if (i == sel) {
                wprintz(w_worlds, c_yellow, ">> ");
            } else {
                wprintz(w_worlds, c_yellow, " ");
            }

            wprintz(w_worlds, c_white, "%s", (world_pages[selpage])[i].c_str());
        }

        //Draw Tabs
        mvwprintz(w_worlds_header, 0, 7, c_white, "");

        for (int i = 0; i < num_pages; ++i) {
            nc_color tabcolor = (selpage == i) ? hilite(c_white):c_white;
            if (world_pages[i].size() > 0) { //skip empty pages
                wprintz(w_worlds_header, c_white, "[");
                wprintz(w_worlds_header, tabcolor, _("Page %d"), i + 1);
                wprintz(w_worlds_header, c_white, "]");
                wputch(w_worlds_header, c_dkgray, LINE_OXOX);
            }
        }

        wrefresh(w_worlds_header);

        fold_and_print(w_worlds_tooltip, 0, 0, 78, c_white, _("Pick a world to enter game"));
        wrefresh(w_worlds_tooltip);

        wrefresh(w_worlds);

        ch = input();

        if (world_pages[selpage].size() > 0 || ch == '\t') {
            switch(ch) {
                case 'j': //move down
                    sel++;
                    if (sel >= world_pages[selpage].size()) {
                        sel = 0;
                    }
                    break;
                case 'k': //move up
                    sel--;
                    if (sel < 0) {
                        sel = world_pages[selpage].size() - 1;
                    }
                    break;
                case '>':
                case '\t': //Switch to next Page
                    sel = 0;
                    do { //skip empty pages
                        selpage++;
                        if (selpage >= world_pages.size()) {
                            selpage = 0;
                        }
                    } while(world_pages[selpage].size() == 0);

                    break;
                case '<':
                    sel = 0;
                    do { //skip empty pages
                        selpage--;
                        if (selpage < 0) {
                            selpage = world_pages.size() - 1;
                        }
                    } while(world_pages[selpage].size() == 0);
                    break;
                case '\n':
                    // we are wanting to get out of this by confirmation, so ask if we want to load the level [y/n prompt] and if yes exit
                    std::string querystring = string_format(_("Do you want to start the game in world [%s]?"), world_pages[selpage][sel].c_str());
                    if (query_yn(querystring.c_str())) {
                        werase(w_worlds);
                        werase(w_worlds_border);
                        werase(w_worlds_header);
                        werase(w_worlds_tooltip);
                        return all_worlds[world_pages[selpage][sel]];//sel + selpage * iContentHeight;
                    }
                    break;
            }
        }
    } while(ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE);

    werase(w_worlds);
    werase(w_worlds_border);
    werase(w_worlds_header);
    werase(w_worlds_tooltip);

    return NULL;
}

void worldfactory::remove_world(std::string worldname)
{
    std::vector<std::string>::iterator it = std::find(all_worldnames.begin(), all_worldnames.end(), worldname);
    if (it != all_worldnames.end()) {
        all_worldnames.erase(it);

        delete all_worlds[worldname];
        all_worlds.erase(worldname);
    }
}

std::string worldfactory::pick_random_name()
{
    // TODO: add some random worldname parameters to name generator
    return get_next_valid_worldname("WOOT", this);
}

int worldfactory::show_worldgen_tab_options(WINDOW *win, WORLDPTR world)
{
    const int iTooltipHeight = 1;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 3 - iTooltipHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

    WINDOW *w_options = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    std::stringstream sTemp;

    std::map<int, bool> mapLines;
    mapLines[3] = true;
    mapLines[60] = true;
    // only populate once
    if (world->world_options.size() == 0) {
        for (std::map<std::string, cOpt>::iterator it = OPTIONS.begin(); it != OPTIONS.end(); ++it) {
            if (it->second.getPage() == "world_default") {
                world->world_options[it->first] = it->second;
            }
        }
    }

    std::vector<std::string> keys;
    for (std::map<std::string, cOpt>::iterator it = world->world_options.begin(); it != world->world_options.end(); ++it) {
        keys.push_back(it->first);
    }

    for (std::map<int, bool>::iterator mLine = mapLines.begin(); mLine != mapLines.end(); ++mLine){
        if (mLine->second){
            mvwputch(win, FULL_SCREEN_HEIGHT-1, mLine->first+1, c_ltgray, LINE_XXOX); // _|_
        }
    }
    mvwputch(win, 2, 61, c_ltgray, LINE_OXXX); //TODO: replace 61 by dynamic mapLines stuff, like it done above
    wrefresh(win);
    refresh();

    InputEvent ch;

    int sel = 0;

    int curoption = 0;
    do {
        for (int i = 0; i < iContentHeight; i++) {
            for (int j = 0; j < 79; j++) {
                if (mapLines[j]) {
                    mvwputch(w_options, i, j, c_ltgray, LINE_XOXO);
                } else {
                    mvwputch(w_options, i, j, c_black, ' ');
                }
            }
        }
        curoption = 0;
        for (std::map<std::string, cOpt>::iterator it = world->world_options.begin(); it != world->world_options.end(); ++it) {
            nc_color cLineColor = c_ltgreen;

            sTemp.str("");
            sTemp << curoption + 1;
            mvwprintz(w_options, curoption , 0, c_white, sTemp.str().c_str());
            mvwprintz(w_options, curoption , 4, c_white, "");

            if (sel == curoption) {
                wprintz(w_options, c_yellow, ">> ");
            } else {
                wprintz(w_options, c_yellow, " ");
            }

            wprintz(w_options, c_white, "%s", (it->second.getMenuText()).c_str());

            if (it->second.getValue() == "False") {
                cLineColor = c_ltred;
            }

            mvwprintz(w_options, curoption, 62, (sel == curoption) ? hilite(cLineColor) : cLineColor, "%s", (it->second.getValueName()).c_str());
            ++curoption;
        }

        wrefresh(w_options);
        refresh();

        ch = get_input();
        if (world->world_options.size() > 0 || ch == Tab) {
            switch(ch) {
                case DirectionS: //move down
                    sel++;
                    if (sel >= world->world_options.size()) {
                        sel = 0;
                    }
                    break;
                case DirectionN: //move up
                    sel--;
                    if (sel < 0) {
                        sel = world->world_options.size() - 1;
                    }
                    break;
                case DirectionW: //set to prev value
                    world->world_options[keys[sel]].setPrev();
                    break;
                case DirectionE: //set to next value
                    world->world_options[keys[sel]].setNext();
                    break;

                case DirectionUp: // '<'
                    werase(w_options);
                    delwin(w_options);
                    return -1;
                    break;
                case DirectionDown: // '>'
                    werase(w_options);
                    delwin(w_options);
                    return 1;
                    break;
                case Cancel:
                    return -999;
                    break;
            }
        }
    } while (true);

    return 0;
}

int worldfactory::show_worldgen_tab_confirm(WINDOW *win, WORLDPTR world)
{
    const int iTooltipHeight = 1;
    const int iContentHeight = FULL_SCREEN_HEIGHT - 3 - iTooltipHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0;

    WINDOW *w_confirmation = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    unsigned namebar_pos = 3 + utf8_width(_("World Name:"));

    int line = 1;
    bool noname = false;
    long ch;

    std::string worldname = world->world_name;
    do {
        mvwprintz(w_confirmation, 2, 2, c_ltgray, _("World Name:"));
        mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "______________________________");

        fold_and_print(w_confirmation, 10, 2, 76, c_ltgray,
                       _("When you are satisfied with the world as it is and are ready to continue, press >"));
        fold_and_print(w_confirmation, 12, 2, 76, c_ltgray, _("To go back and review your world, press <"));
        fold_and_print(w_confirmation, 14, 2, 76, c_green, _("To pick a random name for your world, press ?."));

        if (!noname) {
            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "%s", worldname.c_str());
            if (line == 1) {
                wprintz(w_confirmation, h_ltgray, "_");
            }
        }

        wrefresh(win);
        wrefresh(w_confirmation);
        refresh();
        ch = input();
        if (noname) {
            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "______________________________");
            noname = false;
        }


        if (ch == '>') {
            if (worldname.size() == 0) {
                mvwprintz(w_confirmation, 2, namebar_pos, h_ltgray, _("______NO NAME ENTERED!!!!_____"));
                noname = true;
                wrefresh(w_confirmation);
                if (!query_yn(_("Are you SURE you're finished? World name will be randomly generated."))) {
                    continue;
                    continue;
                } else {
                    world->world_name = pick_random_name();
                    if (!valid_worldname(world->world_name)) {
                        continue;
                    }
                    return 1;
                }
            } else if (query_yn(_("Are you SURE you're finished?")) && valid_worldname(worldname)) {
                world->world_name = worldname;
                werase(w_confirmation);
                delwin(w_confirmation);

                return 1;
            } else {
                continue;
            }
        } else if (ch == '<') {
            world->world_name = worldname;
            werase(w_confirmation);
            delwin(w_confirmation);
            return -1;
        } else if (ch == '?') {
            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "______________________________");
            world->world_name = worldname = pick_random_name();
        } else if (ch == KEY_ESCAPE){
            world->world_name = worldname; // cache the current worldname just in case they say No to the exit query
            return -999;
        } else {
            switch (line) {
                case 1:
                    if (ch == KEY_BACKSPACE || ch == 127) {
                        if (worldname.size() > 0) {
                            //erase utf8 character TODO: make a function
                            while(worldname.size() > 0 && ((unsigned char)worldname[worldname.size() - 1]) >= 128 &&
                                    ((unsigned char)worldname[(int)worldname.size() - 1]) <= 191) {
                                worldname.erase(worldname.size() - 1);
                            }
                            worldname.erase(worldname.size() - 1);
                            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "______________________________ ");
                            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "%s", worldname.c_str());
                            wprintz(w_confirmation, h_ltgray, "_");
                        }
                    } else if (is_char_allowed(ch) && utf8_width(worldname.c_str()) < 30) {
                        worldname.push_back(ch);
                    } else if(ch == KEY_F(2)) {
                        std::string tmp = get_input_string_from_file();
                        int tmplen = utf8_width(tmp.c_str());
                        if(tmplen > 0 && tmplen + utf8_width(worldname.c_str()) < 30) {
                            worldname.append(tmp);
                        }
                    }
                    //experimental unicode input
                    else if(ch > 127) {
                        std::string tmp = utf32_to_utf8(ch);
                        int tmplen = utf8_width(tmp.c_str());
                        if(tmplen > 0 && tmplen + utf8_width(worldname.c_str()) < 30) {
                            worldname.append(tmp);
                        }
                    }
                    break;
            }
        }
    } while (true);

    return 0;
}

void worldfactory::draw_worldgen_tabs(WINDOW *w, int current, std::vector<std::string> tabs)
{
    wclear(w);

    for (int i = 1; i < 79; i++) {
        mvwputch(w, 2, i, c_ltgray, LINE_OXOX);
        mvwputch(w, 24, i, c_ltgray, LINE_OXOX);

        if (i > 2 && i < 24) {
            mvwputch(w, i, 0, c_ltgray, LINE_XOXO);
            mvwputch(w, i, 79, c_ltgray, LINE_XOXO);
        }
    }

    int x = 2;
    for (int i = 0; i < tabs.size(); ++i) {
        draw_tab(w, x, tabs[i], (i == current) ? true : false);
        x += utf8_width(tabs[i].c_str()) + 5;
    }

    mvwputch(w, 2, 0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w, 2, 79, c_ltgray, LINE_OOXX); // ^|

    mvwputch(w, 4, 0, c_ltgray, LINE_XOXO); // |
    mvwputch(w, 4, 79, c_ltgray, LINE_XOXO); // |

    mvwputch(w, 24, 0, c_ltgray, LINE_XXOO); // |_
    mvwputch(w, 24, 79, c_ltgray, LINE_XOOX);// _|
}

bool worldfactory::valid_worldname(std::string name, bool automated)
{
    std::string msg;

    if (name == "save"){
        msg = string_format(_("%s is not a valid world name, it is a reserved name"), name.c_str());
    }else if (std::find(all_worldnames.begin(), all_worldnames.end(), name) == all_worldnames.end()) {
        return true;
    }else{
        msg = string_format(_("%s is not a valid world name, already exists!"), name.c_str());
    }
    if (!automated){
        popup_getkey(msg.c_str());
    }
    return false;
}

std::map<std::string, cOpt> worldfactory::get_world_options(std::string path)
{
    std::map<std::string, cOpt> retoptions;
    std::ifstream fin;

    fin.open(path.c_str());

    if (!fin.is_open()) {
        fin.close();
        save_options();

        fin.open(path.c_str());
        if (!fin.is_open()) {
            fin.close();
            DebugLog() << "Could neither read nor create world options file\n";
            return retoptions;
        }
    }
    std::string sLine;

    while (!fin.eof()) {
        getline(fin, sLine);

        if (sLine != "" && sLine[0] != '#' && std::count(sLine.begin(), sLine.end(), ' ') == 1) {
            int ipos = sLine.find(' ');
            // make sure that the option being loaded is part of the world_default page in OPTIONS
            if(OPTIONS[sLine.substr(0, ipos)].getPage() == "world_default"){
                retoptions[sLine.substr(0, ipos)] = OPTIONS[sLine.substr(0, ipos)]; // init to OPTIONS current
                retoptions[sLine.substr(0, ipos)].setValue(sLine.substr(ipos + 1, sLine.length()));
            }
        }
    }
    fin.close();

    return retoptions;
}
