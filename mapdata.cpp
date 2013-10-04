#include "mapdata.h"
#include "color.h"

#include <ostream>

std::vector<ter_t> terlist;
std::map<std::string, ter_t> termap;

std::vector<furn_t> furnlist;
std::map<std::string, furn_t> furnmap;

std::ostream & operator<<(std::ostream & out, const submap * sm)
{
 out << "submap(";
 if( !sm )
 {
  out << "NULL)";
  return out;
 }

 out << "\n\tter:";
 for(int x = 0; x < SEEX; ++x)
 {
  out << "\n\t" << x << ": ";
  for(int y = 0; y < SEEY; ++y)
   out << sm->ter[x][y] << ", ";
 }

 out << "\n\titm:";
 for(int x = 0; x < SEEX; ++x)
 {
  for(int y = 0; y < SEEY; ++y)
  {
   if( !sm->itm[x][y].empty() )
   {
    for( std::vector<item>::const_iterator it = sm->itm[x][y].begin(),
      end = sm->itm[x][y].end(); it != end; ++it )
    {
     out << "\n\t("<<x<<","<<y<<") ";
     out << *it << ", ";
    }
   }
  }
 }

   out << "\n\t)";
 return out;
}

std::ostream & operator<<(std::ostream & out, const submap & sm)
{
 out << (&sm);
 return out;
}

void load_furniture(JsonObject &jsobj)
{
  furn_t new_furniture;
  new_furniture.id = jsobj.get_string("id");
  new_furniture.name = _(jsobj.get_string("name").c_str());
  new_furniture.sym = jsobj.get_string("symbol").c_str()[0];

  bool has_color = jsobj.has_member("color");
  bool has_bgcolor = jsobj.has_member("bgcolor");
  if(has_color && has_bgcolor) {
    debugmsg("Found both color and bgcolor for %s, use only one of these.", new_furniture.name.c_str());
    new_furniture.color = c_white;
  } else if(has_color) {
    new_furniture.color = color_from_string(jsobj.get_string("color"));
  } else if(has_bgcolor) {
    new_furniture.color = bgcolor_from_string(jsobj.get_string("bgcolor"));
  } else {
    debugmsg("Furniture %s needs at least one of: color, bgcolor.", new_furniture.name.c_str());
  }

  new_furniture.movecost = jsobj.get_int("move_cost_mod");
  new_furniture.move_str_req = jsobj.get_int("required_str");

  new_furniture.transparent = false;
  JsonArray flags = jsobj.get_array("flags");
  while(flags.has_more()) {
    new_furniture.set_flag(flags.next_string());
  }

  if(jsobj.has_member("examine_action")) {
    std::string function_name = jsobj.get_string("examine_action");
    new_furniture.examine = iexamine_function_from_string(function_name);
  } else {
    //If not specified, default to no action
    new_furniture.examine = iexamine_function_from_string("none");
  }

  new_furniture.open = "";
  if ( jsobj.has_member("open") ) {
      new_furniture.open = jsobj.get_string("open");
  }
  new_furniture.close = "";
  if ( jsobj.has_member("close") ) {
      new_furniture.close = jsobj.get_string("close");
  }

  new_furniture.loadid = furnlist.size();
  furnmap[new_furniture.id] = new_furniture;
  furnlist.push_back(new_furniture);
}

void load_terrain(JsonObject &jsobj)
{
  ter_t new_terrain;
  new_terrain.id = jsobj.get_string("id");
  new_terrain.name = _(jsobj.get_string("name").c_str());

  //Special case for the LINE_ symbols
  std::string symbol = jsobj.get_string("symbol");
  if("LINE_XOXO" == symbol) {
    new_terrain.sym = LINE_XOXO;
  } else if("LINE_OXOX" == symbol) {
    new_terrain.sym = LINE_OXOX;
  } else {
    new_terrain.sym = symbol.c_str()[0];
  }

  new_terrain.color = color_from_string(jsobj.get_string("color"));
  new_terrain.movecost = jsobj.get_int("move_cost");

  if(jsobj.has_member("trap")) {
    new_terrain.trap = trap_id_from_string(jsobj.get_string("trap"));
  } else {
    new_terrain.trap = tr_null;
  }


  new_terrain.transparent = false;
  JsonArray flags = jsobj.get_array("flags");
  while(flags.has_more()) {
    new_terrain.set_flag(flags.next_string());
  }

  if(jsobj.has_member("examine_action")) {
    std::string function_name = jsobj.get_string("examine_action");
    new_terrain.examine = iexamine_function_from_string(function_name);
  } else {
    //If not specified, default to no action
    new_terrain.examine = iexamine_function_from_string("none");
  }

  new_terrain.open = "";
  if ( jsobj.has_member("open") ) {
      new_terrain.open = jsobj.get_string("open");
  }
  new_terrain.close = "";
  if ( jsobj.has_member("close") ) {
      new_terrain.close = jsobj.get_string("close");
  }

  new_terrain.loadid=terlist.size();
  termap[new_terrain.id]=new_terrain;
  terlist.push_back(new_terrain);
}


ter_id terfind(const std::string id) {
    if( termap.find(id) == termap.end() ) {
         popup("Can't find %s",id.c_str());
         return 0;
    }
    return termap[id].loadid;
};

ter_id t_null,
    t_hole, // Real nothingness; makes you fall a z-level
    // Ground
    t_dirt, t_sand, t_dirtmound, t_pit_shallow, t_pit,
    t_pit_corpsed, t_pit_covered, t_pit_spiked, t_pit_spiked_covered,
    t_rock_floor, t_rubble, t_ash, t_metal, t_wreckage,
    t_grass,
    t_metal_floor,
    t_pavement, t_pavement_y, t_sidewalk, t_concrete,
    t_floor,
    t_dirtfloor,//Dirt floor(Has roof)
    t_grate,
    t_slime,
    t_bridge,
    // Lighting related
    t_skylight, t_emergency_light_flicker, t_emergency_light,
    // Walls
    t_wall_log_half, t_wall_log, t_wall_log_chipped, t_wall_log_broken, t_palisade, t_palisade_gate, t_palisade_gate_o,
    t_wall_half, t_wall_wood, t_wall_wood_chipped, t_wall_wood_broken,
    t_wall_v, t_wall_h, t_concrete_v, t_concrete_h,
    t_wall_metal_v, t_wall_metal_h,
    t_wall_glass_v, t_wall_glass_h,
    t_wall_glass_v_alarm, t_wall_glass_h_alarm,
    t_reinforced_glass_v, t_reinforced_glass_h,
    t_bars,
    t_door_c, t_door_b, t_door_o, t_door_locked_interior, t_door_locked, t_door_locked_alarm, t_door_frame,
    t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o, t_door_boarded,
    t_door_metal_c, t_door_metal_o, t_door_metal_locked,
    t_door_bar_c, t_door_bar_o, t_door_bar_locked,
    t_door_glass_c, t_door_glass_o,
    t_portcullis,
    t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open, t_curtains,
    t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded,
    t_window_stained_green, t_window_stained_red, t_window_stained_blue,
    t_rock, t_fault,
    t_paper,
    // Tree
    t_tree, t_tree_young, t_tree_apple, t_underbrush, t_shrub, t_shrub_blueberry, t_shrub_strawberry, t_trunk,
    t_root_wall,
    t_wax, t_floor_wax,
    t_fence_v, t_fence_h, t_chainfence_v, t_chainfence_h, t_chainfence_posts,
    t_fence_post, t_fence_wire, t_fence_barbed, t_fence_rope,
    t_railing_v, t_railing_h,
    // Nether
    t_marloss, t_fungus, t_tree_fungal,
    // Water, lava, etc.
    t_water_sh, t_water_dp, t_water_pool, t_sewage,
    t_lava,
    // More embellishments than you can shake a stick at.
    t_sandbox, t_slide, t_monkey_bars, t_backboard,
    t_gas_pump, t_gas_pump_smashed,
    t_generator_broken,
    t_missile, t_missile_exploded,
    t_radio_tower, t_radio_controls,
    t_console_broken, t_console, t_gates_mech_control, t_gates_control_concrete, t_barndoor, t_palisade_pulley,
    t_sewage_pipe, t_sewage_pump,
    t_centrifuge,
    t_column,
    t_vat,
    // Staircases etc.
    t_stairs_down, t_stairs_up, t_manhole, t_ladder_up, t_ladder_down, t_slope_down,
     t_slope_up, t_rope_up,
    t_manhole_cover,
    // Special
    t_card_science, t_card_military, t_card_reader_broken, t_slot_machine,
     t_elevator_control, t_elevator_control_off, t_elevator, t_pedestal_wyrm,
     t_pedestal_temple,
    // Temple tiles
    t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue,
     t_switch_rg, t_switch_gb, t_switch_rb, t_switch_even,
    num_terrain_types;

void set_ter_ids() {
    t_null=terfind("t_null");
    t_hole=terfind("t_hole");
    t_dirt=terfind("t_dirt");
    t_sand=terfind("t_sand");
    t_dirtmound=terfind("t_dirtmound");
    t_pit_shallow=terfind("t_pit_shallow");
    t_pit=terfind("t_pit");
    t_pit_corpsed=terfind("t_pit_corpsed");
    t_pit_covered=terfind("t_pit_covered");
    t_pit_spiked=terfind("t_pit_spiked");
    t_pit_spiked_covered=terfind("t_pit_spiked_covered");
    t_rock_floor=terfind("t_rock_floor");
    t_rubble=terfind("t_rubble");
    t_ash=terfind("t_ash");
    t_metal=terfind("t_metal");
    t_wreckage=terfind("t_wreckage");
    t_grass=terfind("t_grass");
    t_metal_floor=terfind("t_metal_floor");
    t_pavement=terfind("t_pavement");
    t_pavement_y=terfind("t_pavement_y");
    t_sidewalk=terfind("t_sidewalk");
    t_concrete=terfind("t_concrete");
    t_floor=terfind("t_floor");
    t_dirtfloor=terfind("t_dirtfloor");
    t_grate=terfind("t_grate");
    t_slime=terfind("t_slime");
    t_bridge=terfind("t_bridge");
    t_skylight=terfind("t_skylight");
    t_emergency_light_flicker=terfind("t_emergency_light_flicker");
    t_emergency_light=terfind("t_emergency_light");
    t_wall_log_half=terfind("t_wall_log_half");
    t_wall_log=terfind("t_wall_log");
    t_wall_log_chipped=terfind("t_wall_log_chipped");
    t_wall_log_broken=terfind("t_wall_log_broken");
    t_palisade=terfind("t_palisade");
    t_palisade_gate=terfind("t_palisade_gate");
    t_palisade_gate_o=terfind("t_palisade_gate_o");
    t_wall_half=terfind("t_wall_half");
    t_wall_wood=terfind("t_wall_wood");
    t_wall_wood_chipped=terfind("t_wall_wood_chipped");
    t_wall_wood_broken=terfind("t_wall_wood_broken");
    t_wall_v=terfind("t_wall_v");
    t_wall_h=terfind("t_wall_h");
    t_concrete_v=terfind("t_concrete_v");
    t_concrete_h=terfind("t_concrete_h");
    t_wall_metal_v=terfind("t_wall_metal_v");
    t_wall_metal_h=terfind("t_wall_metal_h");
    t_wall_glass_v=terfind("t_wall_glass_v");
    t_wall_glass_h=terfind("t_wall_glass_h");
    t_wall_glass_v_alarm=terfind("t_wall_glass_v_alarm");
    t_wall_glass_h_alarm=terfind("t_wall_glass_h_alarm");
    t_reinforced_glass_v=terfind("t_reinforced_glass_v");
    t_reinforced_glass_h=terfind("t_reinforced_glass_h");
    t_bars=terfind("t_bars");
    t_door_c=terfind("t_door_c");
    t_door_b=terfind("t_door_b");
    t_door_o=terfind("t_door_o");
    t_door_locked_interior=terfind("t_door_locked_interior");
    t_door_locked=terfind("t_door_locked");
    t_door_locked_alarm=terfind("t_door_locked_alarm");
    t_door_frame=terfind("t_door_frame");
    t_chaingate_l=terfind("t_chaingate_l");
    t_fencegate_c=terfind("t_fencegate_c");
    t_fencegate_o=terfind("t_fencegate_o");
    t_chaingate_c=terfind("t_chaingate_c");
    t_chaingate_o=terfind("t_chaingate_o");
    t_door_boarded=terfind("t_door_boarded");
    t_door_metal_c=terfind("t_door_metal_c");
    t_door_metal_o=terfind("t_door_metal_o");
    t_door_metal_locked=terfind("t_door_metal_locked");
    t_door_bar_c=terfind("t_door_bar_c");
    t_door_bar_o=terfind("t_door_bar_o");
    t_door_bar_locked=terfind("t_door_bar_locked");
    t_door_glass_c=terfind("t_door_glass_c");
    t_door_glass_o=terfind("t_door_glass_o");
    t_portcullis=terfind("t_portcullis");
    t_recycler=terfind("t_recycler");
    t_window=terfind("t_window");
    t_window_taped=terfind("t_window_taped");
    t_window_domestic=terfind("t_window_domestic");
    t_window_domestic_taped=terfind("t_window_domestic_taped");
    t_window_open=terfind("t_window_open");
    t_curtains=terfind("t_curtains");
    t_window_alarm=terfind("t_window_alarm");
    t_window_alarm_taped=terfind("t_window_alarm_taped");
    t_window_empty=terfind("t_window_empty");
    t_window_frame=terfind("t_window_frame");
    t_window_boarded=terfind("t_window_boarded");
    t_window_stained_green=terfind("t_window_stained_green");
    t_window_stained_red=terfind("t_window_stained_red");
    t_window_stained_blue=terfind("t_window_stained_blue");
    t_rock=terfind("t_rock");
    t_fault=terfind("t_fault");
    t_paper=terfind("t_paper");
    t_tree=terfind("t_tree");
    t_tree_young=terfind("t_tree_young");
    t_tree_apple=terfind("t_tree_apple");
    t_underbrush=terfind("t_underbrush");
    t_shrub=terfind("t_shrub");
    t_shrub_blueberry=terfind("t_shrub_blueberry");
    t_shrub_strawberry=terfind("t_shrub_strawberry");
    t_trunk=terfind("t_trunk");
    t_root_wall=terfind("t_root_wall");
    t_wax=terfind("t_wax");
    t_floor_wax=terfind("t_floor_wax");
    t_fence_v=terfind("t_fence_v");
    t_fence_h=terfind("t_fence_h");
    t_chainfence_v=terfind("t_chainfence_v");
    t_chainfence_h=terfind("t_chainfence_h");
    t_chainfence_posts=terfind("t_chainfence_posts");
    t_fence_post=terfind("t_fence_post");
    t_fence_wire=terfind("t_fence_wire");
    t_fence_barbed=terfind("t_fence_barbed");
    t_fence_rope=terfind("t_fence_rope");
    t_railing_v=terfind("t_railing_v");
    t_railing_h=terfind("t_railing_h");
    t_marloss=terfind("t_marloss");
    t_fungus=terfind("t_fungus");
    t_tree_fungal=terfind("t_tree_fungal");
    t_water_sh=terfind("t_water_sh");
    t_water_dp=terfind("t_water_dp");
    t_water_pool=terfind("t_water_pool");
    t_sewage=terfind("t_sewage");
    t_lava=terfind("t_lava");
    t_sandbox=terfind("t_sandbox");
    t_slide=terfind("t_slide");
    t_monkey_bars=terfind("t_monkey_bars");
    t_backboard=terfind("t_backboard");
    t_gas_pump=terfind("t_gas_pump");
    t_gas_pump_smashed=terfind("t_gas_pump_smashed");
    t_generator_broken=terfind("t_generator_broken");
    t_missile=terfind("t_missile");
    t_missile_exploded=terfind("t_missile_exploded");
    t_radio_tower=terfind("t_radio_tower");
    t_radio_controls=terfind("t_radio_controls");
    t_console_broken=terfind("t_console_broken");
    t_console=terfind("t_console");
    t_gates_mech_control=terfind("t_gates_mech_control");
    t_gates_control_concrete=terfind("t_gates_control_concrete");
    t_barndoor=terfind("t_barndoor");
    t_palisade_pulley=terfind("t_palisade_pulley");
    t_sewage_pipe=terfind("t_sewage_pipe");
    t_sewage_pump=terfind("t_sewage_pump");
    t_centrifuge=terfind("t_centrifuge");
    t_column=terfind("t_column");
    t_vat=terfind("t_vat");
    t_stairs_down=terfind("t_stairs_down");
    t_stairs_up=terfind("t_stairs_up");
    t_manhole=terfind("t_manhole");
    t_ladder_up=terfind("t_ladder_up");
    t_ladder_down=terfind("t_ladder_down");
    t_slope_down=terfind("t_slope_down");
    t_slope_up=terfind("t_slope_up");
    t_rope_up=terfind("t_rope_up");
    t_manhole_cover=terfind("t_manhole_cover");
    t_card_science=terfind("t_card_science");
    t_card_military=terfind("t_card_military");
    t_card_reader_broken=terfind("t_card_reader_broken");
    t_slot_machine=terfind("t_slot_machine");
    t_elevator_control=terfind("t_elevator_control");
    t_elevator_control_off=terfind("t_elevator_control_off");
    t_elevator=terfind("t_elevator");
    t_pedestal_wyrm=terfind("t_pedestal_wyrm");
    t_pedestal_temple=terfind("t_pedestal_temple");
    t_rock_red=terfind("t_rock_red");
    t_rock_green=terfind("t_rock_green");
    t_rock_blue=terfind("t_rock_blue");
    t_floor_red=terfind("t_floor_red");
    t_floor_green=terfind("t_floor_green");
    t_floor_blue=terfind("t_floor_blue");
    t_switch_rg=terfind("t_switch_rg");
    t_switch_gb=terfind("t_switch_gb");
    t_switch_rb=terfind("t_switch_rb");
    t_switch_even=terfind("t_switch_even");
    num_terrain_types = terlist.size(); 
};

furn_id furnfind(const std::string id) {
    if( furnmap.find(id) == furnmap.end() ) {
         popup("Can't find %s",id.c_str());
         return 0;
    }
    return furnmap[id].loadid;
};

furn_id f_null,
    f_hay,
    f_bulletin,
    f_indoor_plant,
    f_bed, f_toilet, f_makeshift_bed,
    f_sink, f_oven, f_woodstove, f_fireplace, f_bathtub,
    f_chair, f_armchair, f_sofa, f_cupboard, f_trashcan, f_desk, f_exercise,
    f_bench, f_table, f_pool_table,
    f_counter,
    f_fridge, f_glass_fridge, f_dresser, f_locker,
    f_rack, f_bookcase,
    f_washer, f_dryer,
    f_dumpster, f_dive_block,
    f_crate_c, f_crate_o,
    f_canvas_wall, f_canvas_door, f_canvas_door_o, f_groundsheet, f_fema_groundsheet,
    f_skin_wall, f_skin_door, f_skin_door_o,  f_skin_groundsheet,
    f_mutpoppy,
    f_safe_c, f_safe_l, f_safe_o,
    f_plant_seed, f_plant_seedling, f_plant_mature, f_plant_harvest,
    num_furniture_types;

void set_furn_ids() {
    f_null=furnfind("f_null");
    f_hay=furnfind("f_hay");
    f_bulletin=furnfind("f_bulletin");
    f_indoor_plant=furnfind("f_indoor_plant");
    f_bed=furnfind("f_bed");
    f_toilet=furnfind("f_toilet");
    f_makeshift_bed=furnfind("f_makeshift_bed");
    f_sink=furnfind("f_sink");
    f_oven=furnfind("f_oven");
    f_woodstove=furnfind("f_woodstove");
    f_fireplace=furnfind("f_fireplace");
    f_bathtub=furnfind("f_bathtub");
    f_chair=furnfind("f_chair");
    f_armchair=furnfind("f_armchair");
    f_sofa=furnfind("f_sofa");
    f_cupboard=furnfind("f_cupboard");
    f_trashcan=furnfind("f_trashcan");
    f_desk=furnfind("f_desk");
    f_exercise=furnfind("f_exercise");
    f_bench=furnfind("f_bench");
    f_table=furnfind("f_table");
    f_pool_table=furnfind("f_pool_table");
    f_counter=furnfind("f_counter");
    f_fridge=furnfind("f_fridge");
    f_glass_fridge=furnfind("f_glass_fridge");
    f_dresser=furnfind("f_dresser");
    f_locker=furnfind("f_locker");
    f_rack=furnfind("f_rack");
    f_bookcase=furnfind("f_bookcase");
    f_washer=furnfind("f_washer");
    f_dryer=furnfind("f_dryer");
    f_dumpster=furnfind("f_dumpster");
    f_dive_block=furnfind("f_dive_block");
    f_crate_c=furnfind("f_crate_c");
    f_crate_o=furnfind("f_crate_o");
    f_canvas_wall=furnfind("f_canvas_wall");
    f_canvas_door=furnfind("f_canvas_door");
    f_canvas_door_o=furnfind("f_canvas_door_o");
    f_groundsheet=furnfind("f_groundsheet");
    f_fema_groundsheet=furnfind("f_fema_groundsheet");
    f_skin_wall=furnfind("f_skin_wall");
    f_skin_door=furnfind("f_skin_door");
    f_skin_door_o=furnfind("f_skin_door_o");
    f_skin_groundsheet=furnfind("f_skin_groundsheet");
    f_mutpoppy=furnfind("f_mutpoppy");
    f_safe_c=furnfind("f_safe_c");
    f_safe_l=furnfind("f_safe_l");
    f_safe_o=furnfind("f_safe_o");
    f_plant_seed=furnfind("f_plant_seed");
    f_plant_seedling=furnfind("f_plant_seedling");
    f_plant_mature=furnfind("f_plant_mature");
    f_plant_harvest=furnfind("f_plant_harvest");
    num_furniture_types = furnlist.size(); 
}
