#ifndef GSI_H
#define GSI_H

#include <vector>
#include <string>
#include <stack>
#include <condition_variable>
#include <string>
#include <list>
#include <mutex>
#include <queue>

#include "json.h"
#include "color.h"
#include "pldata.h"
#include "bodypart.h"
#include "character.h"
#include "game.h"
#include "input.h"

#ifdef _WINDOWS
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif


class gsi : public JsonSerializer
{
public:

    std::condition_variable gsi_update;
    std::condition_variable gsi_writer;

    static gsi& get()
    {
        static gsi instance;
        return instance;
    }

    gsi(gsi const&) = delete;
    void operator=(gsi const&) = delete;



    bool update_player(player & u);
    bool update_safemode(safe_mode_type safe_mode, int turnssincelastmon);
    bool update_input(std::vector<std::string> registered_actions, std::string category);

    // Everything that goes in the output goes here
    void serialize(JsonOut &jsout) const;

private:
    // Blank constructor
    gsi() 
    {
        ctxt.push("default");
        mctxt.push("default");
    }

    std::stack<std::string> ctxt;  // current input context
    std::stack<std::string> mctxt; // current input context, for menus

    std::vector<std::string> invlets;        // inventory letters in use
    std::vector<std::string> invlets_c;  // inventory letters' corresponding colors
    std::vector<std::string> invlets_s;  // Special status of inventory item:
                                    // Yellow for on/activated
                                    // Green for radioactive
                                    // Red for armed explosive
                                    // Gray for no use in current context
                                    // Books share several of these colors but should be made different

    bool is_self_aware;

    int hunger_level, thirst_level, fatigue_level;
    std::array<int, num_hp_parts> hp_cur_level, hp_max_level;
    std::array<int, num_hp_parts> limb_state;
    std::array<float, num_hp_parts> splint_state;
    int temp_level, temp_change;
    void update_hunger(int hunger, int starvation);
    void update_thirst(int thirst);
    void update_fatigue(int fatigue);
    void update_body(std::array<int, num_hp_parts> hp_cur, std::array<int, num_hp_parts> hp_max,
        std::array<nc_color, num_hp_parts> bp_status, std::array<float, num_hp_parts> splints);
    void update_temp(std::array<int, num_bp> temp_cur, std::array<int, num_bp> temp_conv);
    void update_invlets(Character &character);

    void update_light(player & player);

    int stamina, stamina_max;
    int power_level, max_power_level;
    int pain;
    int morale;
    int safe_mode;
    std::vector<std::string> bound_actions;
    std::vector<std::vector<std::string>> bound_keys;
    

    std::string light_level;
};

class gsi_socket
{
public:
    static gsi_socket& get()
    {
        static gsi_socket instance;
        return instance;
    }

    gsi_socket(gsi_socket const&) = delete;
    void operator=(gsi_socket const&) = delete;

    std::queue<std::string> commandqueue;
    void sockListen();
    void processCommands();
    void sockout();

private:
    gsi_socket() {}

    SOCKET ListenSocket;
    std::set<int> ports;

    void tryReceive();
    void sockInit();
    void sockQuit();
};

#endif
