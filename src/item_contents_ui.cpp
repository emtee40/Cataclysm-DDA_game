#include "item_contents_ui.h"

#include "item.h"
#include "item_category.h"
#include "output.h"
#include "string_input_popup.h"

item_contents_ui::item_contents_ui( const item *containing_item )
    : pContaining_item( containing_item )
{
    std::list<const item *> tempList = pContaining_item->all_items_top();
    for( const item *content : tempList ) {
        vItemList.push_back( content );
    }

    sort_mode = contents_ui_sort_mode::CATEGORY;
    vFilteredItemList = vItemList;
    sort_item_list();

    ctxt = input_context( "INVENTORY", keyboard_mode::keychar );

    ctxt.register_action( "DOWN", to_translation( "Next item" ) );
    ctxt.register_action( "UP", to_translation( "Previous item" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Page down" ) );
    ctxt.register_action( "PAGE_UP", to_translation( "Page up" ) );
    ctxt.register_action( "QUIT", to_translation( "Cancel" ) );
    ctxt.register_action( "TOGGLE_FAVORITE", to_translation( "Toggle favorite" ) );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    ctxt.register_action( "ANY_INPUT" ); // For invlets
    ctxt.register_action( "INVENTORY_FILTER" );
    ctxt.register_action( "EXAMINE_CONTENTS" );
    ctxt.register_action( "SCROLL_ITEM_INFO_UP" );
    ctxt.register_action( "SCROLL_ITEM_INFO_DOWN" );
    ctxt.register_action( "SORT" );
}

void item_contents_ui::draw_details()
{
    //Get description of currently selected item:
    if( selected_item_line >= 0 && selected_item_line < static_cast<int>( vFilteredItemList.size() ) ) {
        std::vector<iteminfo> vSelectedItemDetails;
        std::vector<iteminfo> vDummy;
        vFilteredItemList[selected_item_line]->info( true, vSelectedItemDetails );

        if( vSelectedItemDetails.empty() ) {
            vSelectedItemDetails.emplace_back( "DESCRIPTION", _( "ERROR: Item details empty" ) );
        }
        item_info_data data = item_info_data( vFilteredItemList[selected_item_line]->display_name(), "",
                                              vSelectedItemDetails, vDummy, item_info_scroll );
        data.without_getch = true;
        data.without_border = true;
        data.scrollbar_left = false;
        data.use_full_win = true;
        data.handle_scrolling = true;
        draw_item_info( w_item_info, data );
    }
}

void item_contents_ui::draw_footer()
{
    //Draw keybindings in footer
    for( size_t i = 0; i < keybinding_tips.size(); ++i ) {
        nc_color dummy = c_white;
        print_colored_text( w_data, point( keybinding_x, data_lines + 2 + i ),
                            dummy, c_white, keybinding_tips[i] );
    }

    //Draw filter popup
    int yPos = catacurses::getmaxy( w_data ) - 1;
    if( spopup ) {
        mvwprintz( w_data, point( 2, yPos ), c_cyan, "< " );
        mvwprintz( w_data, point( ( catacurses::getmaxx( w_data ) / 2 ) - 4, yPos ), c_cyan, " >" );

        spopup->query_string( /*loop=*/false, /*draw_only=*/true );
    } else {
        if( !vFilteredItemList.empty() || !filter_string.empty() ) {
            std::string text = string_format( filter_string.empty() ? _( "[%s] Filter" ) : _( "[%s] Filter: " ),
                                              ctxt.get_desc( "INVENTORY_FILTER" ) );

            mvwprintz( w_data, point( 2, yPos ), c_light_gray, "< " );
            wprintz( w_data, c_light_gray, text );
            wprintz( w_data, c_white, filter_string );
            wprintz( w_data, c_light_gray, " >" );
        }
    }

    //Draw sort mode information
    wprintz( w_data, c_light_gray, "     " );
    wprintz( w_data, c_light_gray, string_format( _( "[%s] Sort mode: " ), ctxt.get_desc( "SORT" ) ) );
    if( sort_mode == contents_ui_sort_mode::CATEGORY ) {
        wprintz( w_data, c_light_gray, _( "Category" ) );
    } else if( sort_mode == contents_ui_sort_mode::NAME ) {
        wprintz( w_data, c_light_gray, _( "Name" ) );
    } else {
        wprintz( w_data, c_light_gray, _( "ERROR" ) );
    }
}

void item_contents_ui::draw_header()
{
    // NOLINTNEXTLINE(cata-use-named-point-constants)
    trim_and_print( w_head, point( 1, 1 ), ui_width, c_white, pContaining_item->display_name() );
}

void item_contents_ui::draw_item_list()
{
    max_item_name_width = data_width - 2;
    int item_min = 0;
    max_items = vFilteredItemList.size();
    int istart = 0;
    int iend = 0;
    if( max_items > data_lines ) {
        if( selected_item_line <= item_min + data_lines / 2 ) {
            istart = item_min;
            iend = item_min + data_lines;
        } else if( selected_item_line >= max_items - data_lines / 2 ) {
            istart = max_items - data_lines;
            iend = max_items;
        } else {
            istart = selected_item_line - data_lines / 2;
            iend = selected_item_line + data_lines / 2;
        }
    } else {
        istart = 0;
        iend = std::min<int>( vFilteredItemList.size(), data_height + 1 );
    }
    for( int i = istart; i < iend; ++i ) {
        std::string tmp_name = vFilteredItemList[i]->display_name();
        const bool highlight = i == selected_item_line;
        const nc_color col = highlight ? h_white : c_white;
        const point print_from( 2, i - istart );
        if( highlight ) {
            cursor_pos = print_from;
            trim_and_print( w_data, print_from, max_item_name_width, col, remove_color_tags( tmp_name ) );
        } else {
            trim_and_print( w_data, print_from, max_item_name_width, col, tmp_name );
        }
    }
}

void item_contents_ui::execute()
{
    item_info_scroll = 0;

    head_height = 3;
    ui_width = 0;
    data_lines = 0;
    data_height = 0;
    item_info_width = 0;

    keybinding_x = 1;

    border_helper borders;
    border_helper::border_info &border_header = borders.add_border();
    border_helper::border_info &border_data = borders.add_border();

    ui.on_screen_resize( [&]( ui_adaptor & ui ) {

        ui_width = TERMX * 4 / 5;
        const int wStart = ( TERMX - ui_width ) / 2;

        // Keybinding tips
        static const translation inline_fmt = to_translation(
                //~ %1$s: action description text before key,
                //~ %2$s: key description,
                //~ %3$s: action description text after key.
                "keybinding", "%1$s[<color_yellow>%2$s</color>]%3$s" );
        static const translation separate_fmt = to_translation(
                //~ %1$s: key description,
                //~ %2$s: action description.
                "keybinding", "[<color_yellow>%1$s</color>]%2$s" );
        std::vector<std::string> act_descs;
        const auto add_action_desc = [&]( const std::string & act, const std::string & txt ) {
            act_descs.emplace_back( ctxt.get_desc( act, txt, input_context::allow_all_keys,
                                                   inline_fmt, separate_fmt ) );
        };
        add_action_desc( "CONFIRM", _( "Confirm your selection" ) );
        add_action_desc( "TOGGLE_FAVORITE", pgettext( "crafting gui", "Favorite" ) );
        add_action_desc( "HELP_KEYBINDINGS", pgettext( "crafting gui", "Keybindings" ) );
        keybinding_tips = foldstring( enumerate_as_string( act_descs, enumeration_conjunction::none ),
                                      ui_width - keybinding_x * 2 );

        foot_height = keybinding_tips.size() + 3;
        data_lines = TERMY - head_height - foot_height;
        data_height = TERMY - head_height;
        data_width = ui_width * 1 / 3;

        w_head = catacurses::newwin( head_height, ui_width, point( wStart, 0 ) );
        w_data = catacurses::newwin( data_height, ui_width, point( wStart,
                                     head_height ) );

        // NOLINTNEXTLINE(cata-use-named-point-constants)
        border_header.set( point( wStart, 0 ), point( ui_width, head_height ) );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        border_data.set( point( wStart, head_height - 1 ), point( ui_width, data_height + 1 ) );

        item_info_width = ui_width - data_width;
        const int item_info_height = data_height - foot_height;
        const point item_info( wStart + ui_width - item_info_width, head_height );

        w_item_info = catacurses::newwin( item_info_height, item_info_width,
                                          item_info );

        if( spopup ) {
            spopup->window( w_data, point( 4, catacurses::getmaxy( w_data ) - 1 ),
                            ( catacurses::getmaxx( w_data ) / 2 ) - 4 );
        }

        ui.position( point( wStart, 0 ), point( ui_width, TERMY ) );
    } );
    ui.mark_resize();

    selected_item_line = 0;
    bool done = false;

    ui.on_redraw( [&]( const ui_adaptor & ) {
        // Clear the screen of item data, and draw it anew
        werase( w_data );
        werase( w_item_info );

        borders.draw_border( w_data );
        borders.draw_border( w_head );
        draw_header();
        draw_item_list();

        draw_scrollbar( w_data, selected_item_line, data_lines, max_items, point_zero );
        wnoutrefresh( w_data );
        wnoutrefresh( w_head );

        draw_details();
        draw_footer();

        if( cursor_pos ) {
            // place the cursor at the selected item name as expected by screen readers
            wmove( w_data, cursor_pos.value() );
            wnoutrefresh( w_data );
        }
    } );

    do {
        ui_manager::redraw();
        const int scroll_item_info_lines = catacurses::getmaxy( w_item_info ) - 4;
        const std::string action = ctxt.handle_input();
        const int max_items = static_cast<int>( vFilteredItemList.size() );
        const int scroll_rate = max_items > 20 ? 10 : 3;
        if( action == "SCROLL_ITEM_INFO_UP" ) {
            item_info_scroll -= scroll_item_info_lines;
        } else if( action == "SCROLL_ITEM_INFO_DOWN" ) {
            item_info_scroll += scroll_item_info_lines;
        } else if( action == "DOWN" ) {
            selected_item_line++;
        } else if( action == "UP" ) {
            selected_item_line--;
        } else if( action == "PAGE_DOWN" ) {
            if( selected_item_line == max_items - 1 ) {
                selected_item_line = 0;
            } else if( selected_item_line + scroll_rate >= max_items ) {
                selected_item_line = max_items - 1;
            } else {
                selected_item_line += +scroll_rate;
            }
        } else if( action == "PAGE_UP" ) {
            if( selected_item_line == 0 ) {
                selected_item_line = max_items - 1;
            } else if( selected_item_line <= scroll_rate ) {
                selected_item_line = 0;
            } else {
                selected_item_line += -scroll_rate;
            }
        } else if( action == "EXAMINE_CONTENTS" ) {
            if( selected_item_line >= 0 && selected_item_line < max_items ) {
                if( !vFilteredItemList[selected_item_line]->is_container_empty() ) {
                    item_contents_ui contents_window( vFilteredItemList[selected_item_line] );
                    contents_window.execute();
                }
            }
        } else if( action == "INVENTORY_FILTER" ) {
            set_filter();
        } else if( action == "QUIT" ) {
            done = true;
        } else if( action == "RESET_FILTER" ) {
            filter_string.clear();
        } else if( action == "SORT" ) {
            switch( sort_mode ) {
                case contents_ui_sort_mode::NAME:
                    sort_mode = contents_ui_sort_mode::CATEGORY;
                    break;
                case contents_ui_sort_mode::CATEGORY:
                    sort_mode = contents_ui_sort_mode::NAME;
                    break;
            }
            sort_item_list();
        } else if( action == "TOGGLE_FAVORITE" ) {
            //TODO
        } else if( action == "HELP_KEYBINDINGS" ) {
            // Regenerate keybinding tips
            ui.mark_resize();
        }
        if( selected_item_line < 0 ) {
            selected_item_line = vItemList.size() - 1;
        } else if( selected_item_line >= static_cast<int>( vItemList.size() ) ) {
            selected_item_line = 0;
        }
    } while( !done );
}

void item_contents_ui::set_filter()
{
    spopup = std::make_unique<string_input_popup>();
    spopup->max_length( 256 )
    .text( filter_string );

    ui.mark_resize();

    do {
        ui_manager::redraw();
        spopup->query_string( /*loop=*/false );
    } while( !spopup->confirmed() && !spopup->canceled() );

    if( spopup->confirmed() ) {
        filter_string = spopup->text();
        selected_item_line = 0;
        vFilteredItemList.clear();
        for( const item *containedItem : vItemList ) {
            if( containedItem->display_name().find( filter_string ) != std::string::npos ) {
                vFilteredItemList.emplace_back( containedItem );
            }
        }
        sort_item_list();
        ui.mark_resize();
    }
    spopup.reset();
}

void item_contents_ui::sort_item_list()
{
    switch( sort_mode ) {
        case contents_ui_sort_mode::NAME:
            //Sort item list alphabetically.  Uses tname() rather than display_name() to avoid quirks caused by item condition
            std::sort( vFilteredItemList.begin(), vFilteredItemList.end(), []( const item * a,
            const item * b ) {
                return localized_compare( a->tname( 1, false, 0, true ), b->tname( 1, false, 0, true ) );
            } );
            break;
        case contents_ui_sort_mode::CATEGORY:
            //Category is sorted by getting the item -> then the item category object -> then the translated name of the item category
            std::sort( vFilteredItemList.begin(), vFilteredItemList.end(), []( const item * a,
            const item * b ) {
                return localized_compare( a->get_category_shallow().name(), b->get_category_shallow().name() );
            } );
            break;
    }
}
