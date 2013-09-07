#include "world_factory.h"

#define dbg(x) dout((DebugLevel)(x),D_GAME) << __FILE__ << ":" << __LINE__ << ": "
// single static instance of the world generator
world_factory *world_generator;

typedef int (world_factory::*worldgen_display)(WINDOW *, WORLD *);

/**
 * Returns whether or not the given (ASCII) character is usable in a (player)
 * character's name. Only printable symbols not reserved by the filesystem are
 * permitted.
 * @param ch The char to check.
 * @return true if the char is allowed in a name, false if not.
 */

bool char_allowed(char ch)
{
    //Allow everything EXCEPT the following reserved characters:
    return (ch > 31 //0-31 are control characters
         && ch < 127 //DEL character
         && ch != '/' && ch != '\\' //Path separators
         && ch != '?' && ch != '*' && ch != '%' //Wildcards
         && ch != ':' //Mount point/drive marker
         && ch != '|' //Output pipe
         && ch != '"' //Filename (with spaces) marker
         && ch != '>' && ch != '<'); //Input/output redirection
}

std::string world_options_header()
{
    return "\
# This file is autogenerated from the values picked during World Gen, and\n\
# should not be altered in any way once the world is generated. If you want\n\
# to edit it, you will need to generate a new world.\n\
\n\
";
}

world_factory::world_factory()
{
    //ctor
    active_world = NULL;
}

world_factory::~world_factory()
{
    //dtor
    if (active_world)
    {
        delete active_world;
    }
}

WORLD *world_factory::make_new_world()
{
DebugLog() << "Starting worldgen procedures!\n";
    // Window variables
    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    // world to return after generating!
DebugLog() << "\tInitializing new WORLD object\n";
    WORLD *retworld = new WORLD();

    // set up windows!
    WINDOW *wf_win = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    // prepare tab display order
    std::vector<worldgen_display> tabs;
    std::vector<std::string> tab_strings;

DebugLog() << "\tCreating Worldgen TABS\n";
    tabs.push_back(&world_factory::show_worldgen_tab_options);
    tabs.push_back(&world_factory::show_worldgen_tab_confirm);

    tab_strings.push_back(_("World Gen Options"));
    tab_strings.push_back(_("CONFIRMATION"));

    int curtab = 0;
    const int numtabs = tabs.size();
DebugLog() << "\tStarting Loop\n";
    while (curtab >= 0 && curtab < numtabs)
    {
        draw_worldgen_tabs(wf_win, curtab, tab_strings);
DebugLog() << "\tEntering Tab:["<<curtab<<"]\n";
        curtab += (world_generator->*tabs[curtab])(wf_win, retworld);

        if (curtab < 0)
        {
            if (!query_yn(_("Do you want to abort World Generation?")))
            {
                curtab = 0;
            }
        }
    }
DebugLog() << "\tExiting Loop\n";
    if (curtab < 0)
    {
        delete retworld;
        return NULL;
    }

    // add world to world list!
    all_worlds[retworld->world_name] = retworld;
    all_worldnames.push_back(retworld->world_name);

    save_world(retworld);

    return retworld;
}

WORLD *world_factory::load_world(std::string world_name, bool setactive)
{
    return NULL;
}

void world_factory::set_active_world(WORLD* world)
{
    world_generator->active_world = world;
    if (world)
    {
        ACTIVE_WORLD_OPTIONS = world->world_options;
        DebugLog() << "ACTIVE_WORLD_OPTIONS set to world options\n";
        awo_populated = true;

        for (std::map<std::string, cOpt>::iterator it = ACTIVE_WORLD_OPTIONS.begin(); it != ACTIVE_WORLD_OPTIONS.end(); ++it)
        {
            DebugLog() << "\tKey: "<<it->first << "\tValue: "<<it->second.getValue()<<"\n";
        }
    }
    else
    {
        DebugLog() << "ACTIVE_WORLD_OPTIONS cleared\n";
        awo_populated = false;
    }
}

std::map<std::string, WORLD*> world_factory::get_all_worlds()
{
    std::map<std::string, WORLD*> retworlds;

    if (all_worlds.size() > 0)
    {
        for (std::map<std::string, WORLD*>::iterator it = all_worlds.begin(); it != all_worlds.end(); ++it)
        {
            delete it->second;
        }
        all_worlds.clear();
        all_worldnames.clear();
        //return all_worlds;
    }

    const std::string save_dir = "save";
    const std::string save_ext = ".sav";

    struct dirent *dp, *sdp;
    struct stat _buff;
    DIR *save = opendir(save_dir.c_str()), *world;

    if (!save)
    {
        #if (defined _WIN32 || defined __WIN32__)
            mkdir(save_dir.c_str());
        #else
            mkdir(save_dir.c_str(), 0777);
        #endif
        save = opendir(save_dir.c_str());
    }

    if (!save) {
        dbg(D_ERROR) << "game:opening_screen: Unable to make save directory.";
        debugmsg("Could not make './save' directory");
        endwin();
        exit(1);
    }

    // read through save directory looking for folders. These folders are the World folders containing the save data
    while ((dp = readdir(save)))
    {
        if (stat(dp->d_name, &_buff) != 0x4)
        {
            //DebugLog() << "Potential Folder Found: "<< dp->d_name << "\n";
            // ignore "." and ".."
            if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
            {
                std::string subdirname = save_dir + "/" + dp->d_name;
                //subdirname.append(dp->d_name);
                // now that we have the world directory found and saved, we need to populate it with appropriate save file information
                world = opendir(subdirname.c_str());
                if (world == NULL)
                {
                    //DebugLog() << "Could not open directory: "<< dp->d_name << "\n";
                    continue;
                }
                else
                {
                    //worldlist.push_back(dp->d_name);
                    retworlds[dp->d_name] = new WORLD();
                    retworlds[dp->d_name]->world_name = dp->d_name;
                    // add the world name to the all_worldnames vector
                    all_worldnames.push_back(dp->d_name);
                }
                while ((sdp = readdir(world)))
                {
                    std::string tmp = sdp->d_name;
                    if (tmp.find(".sav") != std::string::npos)
                    {
                        //world_savegames[dp->d_name].push_back(tmp.substr(0, tmp.find(".sav")));
                        retworlds[dp->d_name]->world_saves.push_back(tmp.substr(0,tmp.find(save_ext.c_str())));
                    }
                }
                closedir(world);
                retworlds[dp->d_name]->world_options = get_world_options(subdirname);
                // close the world directory
            }
        }
    }

    all_worlds = retworlds;
    return retworlds;
}

WORLD *world_factory::pick_world()
{
    // get world list
    std::map<std::string, WORLD*> all_worlds = get_all_worlds();
    std::vector<std::string> world_names = all_worldnames;


    // if there is only one world to pick from, autoreturn that world
    if (world_names.size() == 1)
    {
        return all_worlds[world_names[0]];
    }

    // go through world picker UI
    const int iTooltipHeight = 3;
    const int iContentHeight = FULL_SCREEN_HEIGHT-3-iTooltipHeight;

    const int num_pages = world_names.size() / iContentHeight + 1; // at least 1 page

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    std::map<int, bool> mapLines;
    mapLines[3] = true;

    std::map<int, std::vector<std::string> > world_pages;
    int worldnum = 0;
    for (int i = 0; i < num_pages; ++i)
    {
        world_pages[i] = std::vector<std::string>();
        for (int j = 0; j < iContentHeight; ++j)
        {
            world_pages[i].push_back(world_names[worldnum++]);

            if (worldnum == world_names.size())
            {
                break;
            }
        }
    }

    int sel = 0, selpage = 0;

    WINDOW* w_worlds_border = newwin(FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH, iOffsetY, iOffsetX);

    WINDOW* w_worlds_tooltip = newwin(iTooltipHeight, FULL_SCREEN_WIDTH - 2, 1 + iOffsetY, 1 + iOffsetX);
    WINDOW* w_worlds_header = newwin(1, FULL_SCREEN_WIDTH - 2, 1 + iTooltipHeight + iOffsetY, 1 + iOffsetX);
    WINDOW* w_worlds = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    wborder(w_worlds_border, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX, LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX);
    mvwputch(w_worlds_border, 4,  0, c_ltgray, LINE_XXXO); // |-
    mvwputch(w_worlds_border, 4, 79, c_ltgray, LINE_XOXX); // -|

    for (std::map<int, bool>::iterator iter = mapLines.begin(); iter != mapLines.end(); ++iter) {
        mvwputch(w_worlds_border, FULL_SCREEN_HEIGHT-1, iter->first + 1, c_ltgray, LINE_XXOX); // _|_
    }

    mvwprintz(w_worlds_border, 0, 36, c_ltred, _(" WORLD SELECTION "));
    wrefresh(w_worlds_border);

    for (int i = 0; i < 78; i++) {
        if (mapLines[i]) {
            mvwputch(w_worlds_header, 0, i, c_ltgray, LINE_OXXX);
        } else {
            mvwputch(w_worlds_header, 0, i, c_ltgray, LINE_OXOX); // Draw header line
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
                    mvwputch(w_worlds, i, j, c_ltgray, LINE_XOXO);
                } else {
                    mvwputch(w_worlds, i, j, c_black, ' ');
                }

                if (i < iTooltipHeight) {
                    mvwputch(w_worlds_tooltip, i, j, c_black, ' ');
                }
            }
        }

        //Draw World Names
        for (int i = 0; i < world_pages[selpage].size(); ++i)
        {
            nc_color cLineColor = c_ltgreen;

            sTemp.str("");
            sTemp << i + 1;
            mvwprintz(w_worlds, i, 0, c_white, sTemp.str().c_str());
            mvwprintz(w_worlds, i, 4, c_white, "");


            if (i == sel)
            {
                wprintz(w_worlds, c_yellow, ">> ");
            }
            else
            {
                wprintz(w_worlds, c_yellow, "   ");
            }

            wprintz(w_worlds, c_white, "%s", (world_pages[selpage])[i].c_str());
        }

        //Draw Tabs
        mvwprintz(w_worlds_header, 0, 7, c_white, "");
        for (int i = 0; i < num_pages; ++i) {
            if (world_pages[i].size() > 0) { //skip empty pages
                wprintz(w_worlds_header, c_white, "[");
                wprintz(w_worlds_header, (selpage == i) ? hilite(c_white) : c_white, "Page %d", i+1);
                wprintz(w_worlds_header, c_white, "]");
                wputch(w_worlds_header, c_white, LINE_OXOX);
            }
        }

        wrefresh(w_worlds_header);

        fold_and_print(w_worlds_tooltip, 0, 0, 78, c_white, "Pick a world to enter game");
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
                        sel = world_pages[selpage].size()-1;
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
                            selpage = world_pages.size()-1;
                        }
                    } while(world_pages[selpage].size() == 0);
                    break;
                case '\n':
                    // we are wanting to get out of this by confirmation, so ask if we want to load the level [y/n prompt] and if yes exit
                    std::stringstream querystring;
                    querystring << "Do you want to start the game in world [" << world_pages[selpage][sel] << "]?";
                    if (query_yn(querystring.str().c_str()))
                    {
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
    // we are assumed to have quit, so return -1
    return NULL;
}

std::map<std::string, cOpt> world_factory::get_world_options(std::string path)
{
    std::map<std::string, cOpt> retoptions;
    const std::string worldop_fname = "/worldoptions.txt";

    std::ifstream fin;
    std::stringstream worldopfile;

    worldopfile << path << worldop_fname;
    fin.open(worldopfile.str().c_str());

    if(!fin.is_open()) {
        fin.close();
        save_options();
        fin.open(worldopfile.str().c_str());
        if(!fin.is_open()) {
            DebugLog() << "Could neither read nor create world options file\n";
            return retoptions;
        }
    }
    std::string sLine;

    while (!fin.eof())
    {
        getline(fin, sLine);

        if (sLine != "" && sLine[0] != '#' && std::count(sLine.begin(), sLine.end(), ' ') == 1)
        {
            int ipos = sLine.find(' ');
            retoptions[sLine.substr(0, ipos)] = OPTIONS[sLine.substr(0, ipos)]; // init to OPTIONS current, works since it's a value type not a reference type :D
            retoptions[sLine.substr(0, ipos)].setValue(sLine.substr(ipos+1, sLine.length()));
        }
    }

    fin.close();

    return retoptions;
}

void world_factory::save_world(WORLD *world)
{
    // if world arg == NULL then change it to the active_world
    if (!world)
    {
        world = active_world;
    }
    // if the active world == NULL then return w/o saving
    if (!world)
    {
        return;
    }

    std::ofstream fout;
    std::stringstream woption;
    woption << "save/" << world->world_name << "/worldoptions.txt";

    std::string savedir = "save/";
    savedir += world->world_name;

    DIR *dir = opendir(savedir.c_str());
    if (!dir)
    {
        #if (defined _WIN32 || defined __WIN32__)
            mkdir(savedir.c_str());
        #else
            mkdir(savedir.c_str(), 0777);
        #endif
        dir = opendir(savedir.c_str());
    }
    if (!dir)
    {
        return;
    }

    fout.open(woption.str().c_str());

    if (!fout.is_open())
    {
        return;
    }
    fout << world_options_header() << std::endl;

    for (std::map<std::string, cOpt>::iterator it = world->world_options.begin(); it != world->world_options.end(); ++it)
    {
        fout << "#" << it->second.getTooltip() << std::endl;
        fout << "#Default: " << it->second.getDefaultText() << std::endl;
        fout << it->first << " " << it->second.getValue() << std::endl << std::endl;
    }

    fout.close();
}

// UI Functions
int world_factory::show_worldgen_tab_options(WINDOW *win, WORLD* world)
{
DebugLog() << "\tEntering TAB: OPTIONS\n";
    //werase(win);
    const int iTooltipHeight = 1;
    const int iContentHeight = FULL_SCREEN_HEIGHT-3-iTooltipHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    WINDOW* w_options = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

    std::stringstream sTemp;

    std::map<int, bool> mapLines;
    mapLines[3] = true;
    mapLines[60] = true;
    // only populate once
    if (world->world_options.size() == 0)
    {
DebugLog() << "\tLoading Options\n";
        for (std::map<std::string, cOpt>::iterator it = OPTIONS.begin(); it != OPTIONS.end(); ++it)
        {
            if (it->second.getPage() == "world_default")
            {
                world->world_options[it->first] = it->second;
            }
        }
    }

    std::vector<std::string> keys;
    for (std::map<std::string, cOpt>::iterator it = world->world_options.begin(); it != world->world_options.end(); ++it)
    {
        keys.push_back(it->first);
    }

    //draw_tabs(win, 1, _("World Gen Options"), _("Confirmation"));

    wrefresh(win);
    refresh();

    char ch=' ';
    int sel=0;

    int numoptions = world->world_options.size();
    int curoption=0;
    do
    {
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
        for (std::map<std::string, cOpt>::iterator it = world->world_options.begin(); it != world->world_options.end(); ++it)
        {
            nc_color cLineColor = c_ltgreen;

            sTemp.str("");
            sTemp << curoption + 1;
            mvwprintz(w_options, curoption , 0, c_white, sTemp.str().c_str());
            mvwprintz(w_options, curoption , 4, c_white, "");

            if (sel == curoption) {
                wprintz(w_options, c_yellow, ">> ");
            } else {
                wprintz(w_options, c_yellow, "   ");
            }

            wprintz(w_options, c_white, "%s", (it->second.getMenuText()).c_str());

            if (it->second.getValue() == "False") {
                cLineColor = c_ltred;
            }

            mvwprintz(w_options, curoption, 62, (sel == curoption) ? hilite(cLineColor) : cLineColor, "%s", (it->second.getValue()).c_str());
            //mvwprintz(win, 4, 2, c_white, )
            ++curoption;
        }

        wrefresh(w_options);
        refresh();

        ch = input();
        if (world->world_options.size() > 0 || ch == '\t')
        {
            switch(ch) {
                case 'j': //move down
                    sel++;
                    if (sel >= world->world_options.size()) {
                        sel = 0;
                    }
                    break;
                case 'k': //move up
                    sel--;
                    if (sel < 0) {
                        sel = world->world_options.size()-1;
                    }
                    break;
                case 'l': //set to prev value
                    world->world_options[keys[sel]].setNext();
                    //bStuffChanged = true;
                    break;
                case 'h': //set to next value
                    world->world_options[keys[sel]].setPrev();
                    //bStuffChanged = true;
                    break;

                case '<':
                    werase(w_options);
                    delwin(w_options);
                    return -1;
                    break;
                case '>':
                    werase(w_options);
                    delwin(w_options);
                    return 1;
                    break;
            }
        }
    }while (true);

    return 0;
}

int world_factory::show_worldgen_tab_confirm(WINDOW *win, WORLD* world)
{
DebugLog() << "\tEntering TAB: CONFIRMATION\n";
    //werase(win);
    const int iTooltipHeight = 1;
    const int iContentHeight = FULL_SCREEN_HEIGHT-3-iTooltipHeight;

    const int iOffsetX = (TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0;
    const int iOffsetY = (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0;

    WINDOW* w_confirmation = newwin(iContentHeight, FULL_SCREEN_WIDTH - 2, iTooltipHeight + 2 + iOffsetY, 1 + iOffsetX);

DebugLog() << "\t\tDrawing Tabs\n";
    //draw_tabs(win, 1, _("World Gen Options"), _("Confirmation"));
DebugLog() << "\t\tDrawing screen statics\n";


    unsigned namebar_pos = 3 + utf8_width(_("World Name:"));

DebugLog() << "\t\tDone drawing screen statics\n";
    int line = 1;
    bool noname = false, loop = true;
    long ch;

    std::string worldname = world->world_name;
DebugLog() << "\tEntering CONFIRMATION loop\n";
    do
    {
        mvwprintz(w_confirmation, 2, 2, c_ltgray, _("World Name:"));
        mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "______________________________");

        fold_and_print(w_confirmation, 10, 2, 76, c_ltgray,
                    _("When you are satisfied with the world as it is and are ready to continue, press >"));
        fold_and_print(w_confirmation, 12, 2, 76, c_ltgray, _("To go back and review your world, press <"));
        fold_and_print(w_confirmation, 14, 2, 76, c_green, _("To pick a random name for your world, press ?."));

        if (!noname)
        {
            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "%s", worldname.c_str());
            if (line == 1)
            {
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


        if (ch == '>')
        {
            if (worldname.size() == 0)
            {
                mvwprintz(w_confirmation, 2, namebar_pos, h_ltgray, _("______NO NAME ENTERED!!!!_____"));
                noname = true;
                wrefresh(w_confirmation);
                if (!query_yn(_("Are you SURE you're finished? Your name will be randomly generated.")))
                {
                    continue;
                }
                else
                {
                    world->world_name = pick_random_name();
                    if (!valid_worldname(world->world_name))
                    {
                        continue;
                    }
                    return 1;
                }
            }
            else if (query_yn(_("Are you SURE you're finished?")) && valid_worldname(worldname))
            {
                world->world_name = worldname;
                werase(w_confirmation);
                delwin(w_confirmation);

                return 1;
            }
            else
            {
                continue;
            }
        }
        else if (ch == '<')
        {
            world->world_name = worldname;
            werase(w_confirmation);
            delwin(w_confirmation);
            return -1;
        }
        else if (ch == '?')
        {
            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "______________________________");
            world->world_name = worldname = pick_random_name();
        }
        else
        {
            switch (line) {
                case 1:
                    if (ch == KEY_BACKSPACE || ch == 127)
                    {
                        if (worldname.size() > 0)
                        {
                            //erease utf8 character TODO: make a function
                            while(worldname.size()>0 && ((unsigned char)worldname[worldname.size()-1])>=128 &&
                                                        ((unsigned char)worldname[(int)worldname.size()-1])<=191)
                            {
                                worldname.erase(worldname.size()-1);
                            }
                            worldname.erase(worldname.size()-1);
                            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "______________________________");
                            mvwprintz(w_confirmation, 2, namebar_pos, c_ltgray, "%s", worldname.c_str());
                            wprintz(w_confirmation, h_ltgray, "_");
                        }
                    }
                    else if (char_allowed(ch) &&  utf8_width(worldname.c_str()) < 30)
                    {
                        worldname.push_back(ch);
                    }
                    else if(ch==KEY_F(2))
                    {
                        std::string tmp = get_input_string_from_file();
                        int tmplen = utf8_width(tmp.c_str());
                        if(tmplen>0 && tmplen+utf8_width(worldname.c_str())<30)
                        {
                            worldname.append(tmp);
                        }
                    }
                    //experimental unicode input
                    else if(ch>127)
                    {
                        std::string tmp = utf32_to_utf8(ch);
                        int tmplen = utf8_width(tmp.c_str());
                        if(tmplen>0 && tmplen+utf8_width(worldname.c_str())<30)
                        {
                            worldname.append(tmp);
                        }
                    }
                    break;
            }
        }
    }while (true);

    return 0;
}

void world_factory::draw_worldgen_tabs(WINDOW* w, int current, std::vector<std::string> tabs)
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
    for (int i = 0; i < tabs.size(); ++i)
    {
        draw_tab(w, x, tabs[i], (i == current)? true : false);
        x += utf8_width(tabs[i].c_str()) + 5;
    }

    mvwputch(w, 2,  0, c_ltgray, LINE_OXXO); // |^
    mvwputch(w, 2, 79, c_ltgray, LINE_OOXX); // ^|

    mvwputch(w, 4, 0, c_ltgray, LINE_XOXO);  // |
    mvwputch(w, 4, 79, c_ltgray, LINE_XOXO); // |

    mvwputch(w, 24, 0, c_ltgray, LINE_XXOO); // |_
    mvwputch(w, 24, 79, c_ltgray, LINE_XOOX);// _|
}


std::string world_factory::pick_random_name()
{
    return "WOOT";
}
void world_factory::remove_world(std::string worldname)
{
    for (std::vector<std::string>::iterator it = all_worldnames.begin(); it != all_worldnames.end(); ++it)
    {
        if (*it == worldname)
        {
            all_worldnames.erase(it);
            break;
        }
    }
    delete all_worlds[worldname];
    all_worlds.erase(worldname);
}
bool world_factory::valid_worldname(std::string name)
{
    if (std::find(all_worldnames.begin(), all_worldnames.end(), name) == all_worldnames.end())
    {
        return true;
    }
    std::stringstream msg;
    msg << name << " is not a valid world name, already exists!";
    popup_getkey(msg.str().c_str());
    return false;
}
