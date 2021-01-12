#include "GameState.h"
#include "SaveState.h"
#include "Misc.h"

#include <cassert>
#include <SDL.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <limits>
#include <math.h>

GameState::GameState(const char* filename)
{
    sdl_window = SDL_CreateWindow( "Pressure", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920, 1080, 0);
    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	sdl_texture = loadTexture();
    SDL_SetRenderDrawColor(sdl_renderer, 0x0, 0x0, 0x0, 0xFF);
	assert(sdl_texture);

    std::ifstream loadfile(filename);
    if (loadfile.fail() || loadfile.eof())
    {
        for (int i = 0; i < LEVEL_COUNT; i++)
        {
            levels[i] = new Level(i);
        }
        current_level = 0;
    }
    else
    {
        SaveObjectMap* omap = SaveObject::load(loadfile)->get_map();
        SaveObjectList* slist = omap->get_item("levels")->get_list();
        
        for (int i = 0; i < LEVEL_COUNT; i++)
        {
            if (i < slist->get_count())
                levels[i] = new Level(i, slist->get_item(i));
            else
                levels[i] = new Level(i);
        }
        current_level = omap->get_num("current_level");
        delete omap;
    }
    set_level(current_level);
}

void GameState::save(const char*  filename)
{
    std::ofstream outfile (filename);
    outfile << std::setprecision(std::numeric_limits<double>::digits);
    SaveObjectMap omap;
    
    SaveObjectList* slist = new SaveObjectList;
    
    for (int i = 0; i < LEVEL_COUNT; i++)
    {
        slist->add_item(levels[i]->save());
    }
    omap.add_item("levels", slist);
    omap.add_num("current_level", current_level);
    omap.save(outfile);
}


GameState::~GameState()
{
	SDL_DestroyTexture(sdl_texture);
	SDL_DestroyRenderer(sdl_renderer);
	SDL_DestroyWindow(sdl_window);
}

extern const char embedded_data_binary_texture_png_start;
extern const char embedded_data_binary_texture_png_end;

SDL_Texture* GameState::loadTexture()
{
    SDL_Surface* loadedSurface = IMG_Load_RW(SDL_RWFromMem((void*)&embedded_data_binary_texture_png_start,
                                    &embedded_data_binary_texture_png_end - &embedded_data_binary_texture_png_start),1);
	assert(loadedSurface);
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(sdl_renderer, loadedSurface);
	assert(newTexture);
	SDL_FreeSurface(loadedSurface);
	return newTexture;
}


void GameState::advance()
{
    levels[current_level]->advance(10);
}

void GameState::draw_char(XYPos& pos, char c)
{
    SDL_Rect src_rect = {(c % 16) * 16, 192 + (c / 16) * 16, 16, 16};
    SDL_Rect dst_rect = {pos.x, pos.y, 16, 16};
    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
    pos.x += 16;

}


void GameState::render()
{
    SDL_RenderClear(sdl_renderer);
    XYPos window_size;
    SDL_GetWindowSize(sdl_window, &window_size.x, &window_size.y);
    XYPos pos;
    
    {
        SDL_Rect src_rect = {280, 240, 360, 240};       // Background
        SDL_Rect dst_rect = {0, 0, 1920, 1080};
        SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
    }
    {                                               // Input pipes
        SDL_Rect src_rect = {160, 0, 32, 32};       // W
        SDL_Rect dst_rect = {-32 * scale + grid_offset.x, (4 * 32) * scale + grid_offset.y, 32 * scale, 32 * scale};
        SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);

        src_rect = {224, 0, 32, 32};                // E
        dst_rect = {(9 * 32) * scale + grid_offset.x, (4 * 32) * scale + grid_offset.y, 32 * scale, 32 * scale};
        SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);

        src_rect = {192, 0, 32, 32};                // N
        dst_rect = {(4 * 32) * scale + grid_offset.x, -32 * scale + grid_offset.y, 32 * scale, 32 * scale};
        SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);

        src_rect = {128, 0, 32, 32};                // S
        dst_rect = {(4 * 32) * scale + grid_offset.x, (9 * 32) * scale + grid_offset.y, 32 * scale, 32 * scale};
        SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
    }

    for (pos.y = 0; pos.y < 9; pos.y++)
    for (pos.x = 0; pos.x < 9; pos.x++)
    {
        SDL_Rect src_rect = {256+32, 32, 16, 16};
        if (pos.y > 0)
            src_rect.y += 16;
        if (pos.y == 8)
            src_rect.y += 16;
        if (pos.x > 0)
            src_rect.x += 16;
        if (pos.x == 8)
            src_rect.x += 16;
            
        SDL_Rect dst_rect = {pos.x * 32 * scale + grid_offset.x, pos.y * 32 * scale + grid_offset.y, 32 * scale, 32 * scale};
        SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
    }
    
    for (pos.y = 0; pos.y < 9; pos.y++)
    for (pos.x = 0; pos.x < 9; pos.x++)
    {
        SDL_Rect src_rect = current_circuit->elements[pos.y][pos.x]->getimage();
        SDL_Rect dst_rect = {pos.x * 32 * scale + grid_offset.x, pos.y * 32 * scale + grid_offset.y, 32 * scale, 32 * scale};
        SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
    }

    if (mouse_state == MOUSE_STATE_PIPE)
    {
        if (pipe_start_ns)
        {
            SDL_Rect src_rect = {0, 0, 1, 1};
            SDL_Rect dst_rect = {(pipe_start_grid_pos.x * 32 + 16 - 4)  * scale + grid_offset.x, (pipe_start_grid_pos.y * 32 - 4) * scale + grid_offset.y, 8 * scale, 8 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            XYPos mouse_grid = ((mouse - grid_offset) / scale) / 32;
            if (mouse_grid.y >= pipe_start_grid_pos.y)   //north - southwards
            {
                    Connections con;
                    XYPos mouse_rel = ((mouse - grid_offset) / scale) - (pipe_start_grid_pos * 32);
                    mouse_rel.x -= 16;
                    if (mouse_rel.y > abs(mouse_rel.x))     // south
                        con = CONNECTIONS_NS;
                    else if (mouse_rel.x >= 0)               // east
                        con = CONNECTIONS_NE;
                    else                                    // west
                        con = CONNECTIONS_NW;
                    SDL_Rect src_rect = {(con % 4) * 32, (con / 4) * 32, 32, 32};
                    SDL_Rect dst_rect = {pipe_start_grid_pos.x * 32 * scale + grid_offset.x, pipe_start_grid_pos.y * 32 * scale + grid_offset.y, 32 * scale, 32 * scale};
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            }
            else                                        //south - northwards
            {
                    Connections con;
                    XYPos mouse_rel = ((mouse - grid_offset) / scale) - (pipe_start_grid_pos * 32);
                    mouse_rel.x -= 16;
                    if (-mouse_rel.y > abs(mouse_rel.x))    // north
                        con = CONNECTIONS_NS;
                    else if (mouse_rel.x >= 0)               // east
                        con = CONNECTIONS_ES;
                    else                                    // west
                        con = CONNECTIONS_WS;
                    SDL_Rect src_rect = {(con % 4) * 32, (con / 4) * 32, 32, 32};
                    SDL_Rect dst_rect = {pipe_start_grid_pos.x * 32 * scale + grid_offset.x, (pipe_start_grid_pos.y - 1) * 32 * scale + grid_offset.y, 32 * scale, 32 * scale};
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            }
        }
        else
        {
            SDL_Rect src_rect = {0, 0, 1, 1};
            SDL_Rect dst_rect = {(pipe_start_grid_pos.x * 32 - 4)  * scale + grid_offset.x, (pipe_start_grid_pos.y * 32 + 16 - 4) * scale + grid_offset.y, 8 * scale, 8 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            XYPos mouse_grid = ((mouse - grid_offset) / scale) / 32;
            if (mouse_grid.x >= pipe_start_grid_pos.x)   //west - eastwards
            {
                    Connections con;
                    XYPos mouse_rel = ((mouse - grid_offset) / scale) - (pipe_start_grid_pos * 32);
                    mouse_rel.y -= 16;
                    if (mouse_rel.x > abs(mouse_rel.y))     // west
                        con = CONNECTIONS_EW;
                    else if (mouse_rel.y >= 0)               // south
                        con = CONNECTIONS_WS;
                    else                                    // north
                        con = CONNECTIONS_NW;
                    SDL_Rect src_rect = {(con % 4) * 32, (con / 4) * 32, 32, 32};
                    SDL_Rect dst_rect = {pipe_start_grid_pos.x * 32 * scale + grid_offset.x, pipe_start_grid_pos.y * 32 * scale + grid_offset.y, 32 * scale, 32 * scale};
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            }
            else                                        //east - westwards
            {
                    Connections con;
                    XYPos mouse_rel = ((mouse - grid_offset) / scale) - (pipe_start_grid_pos * 32);
                    mouse_rel.y -= 16;
                    if (-mouse_rel.x > abs(mouse_rel.y))    // east
                        con = CONNECTIONS_EW;
                    else if (mouse_rel.y >= 0)               // south
                        con = CONNECTIONS_ES;
                    else                                    // north
                        con = CONNECTIONS_NE;
                    SDL_Rect src_rect = {(con % 4) * 32, (con / 4) * 32, 32, 32};
                    SDL_Rect dst_rect = {(pipe_start_grid_pos.x - 1) * 32 * scale + grid_offset.x, pipe_start_grid_pos.y * 32 * scale + grid_offset.y, 32 * scale, 32 * scale};
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            }
        }
    }
    else if (mouse_state == MOUSE_STATE_PLACING_VALVE)
    {
        XYPos mouse_grid = ((mouse - grid_offset)/ scale) / 32;
        if (mouse_grid.inside(XYPos(9,9)))
        {
            SDL_Rect src_rect = {direction * 32, 4 * 32, 32, 32};
            SDL_Rect dst_rect = {mouse_grid.x * 32 * scale + grid_offset.x, mouse_grid.y * 32 * scale + grid_offset.y, 32 * scale, 32 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
    }
    else if (mouse_state == MOUSE_STATE_PLACING_SOURCE)
    {
        XYPos mouse_grid = ((mouse - grid_offset) / scale) / 32;
        if (mouse_grid.inside(XYPos(9,9)))
        {
            SDL_Rect src_rect = {(4 + direction) * 32, 0, 32, 32};
            SDL_Rect dst_rect = {mouse_grid.x * 32 * scale + grid_offset.x, mouse_grid.y * 32 * scale + grid_offset.y, 32 * scale, 32 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
    }

    for (pos.y = 0; pos.y < 10; pos.y++)
    for (pos.x = 0; pos.x < 9; pos.x++)
    {
        if (!current_circuit->connections_ns[pos.y][pos.x].touched)
            continue;
        Pressure vented = (current_circuit->connections_ns[pos.y][pos.x].vented);
        unsigned value = pressure_as_percent(current_circuit->connections_ns[pos.y][pos.x].value);
        
        if (vented > 2)
        {
            SDL_Rect src_rect = {16*int(rand & 3), 166, 16, 16};
            SDL_Rect dst_rect = {(pos.x * 32  + 7 + int(rand % 3)) * scale + grid_offset.x, (pos.y * 32 - 9  + int(rand % 3)) * scale + grid_offset.y, 16 * scale, 16 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
        if (value == 100)
        {
            SDL_Rect src_rect = {40, 161, 9, 5};
            SDL_Rect dst_rect = {(pos.x * 32  + 11) * scale + grid_offset.x, (pos.y * 32 - 3) * scale + grid_offset.y, 9 * scale, 5 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
        else
        {
            SDL_Rect src_rect = {(int(value) / 10) * 4, 161, 4, 5};
            SDL_Rect dst_rect = {(pos.x * 32  + 11) * scale + grid_offset.x, (pos.y * 32 - 3) * scale + grid_offset.y, 4 * scale, 5 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            src_rect.w = 5;
            src_rect.x = (value % 10) * 4;
            dst_rect.w = 5 * scale;
            dst_rect.x += 4 * scale;
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
    }
    for (pos.y = 0; pos.y < 9; pos.y++)
    for (pos.x = 0; pos.x < 10; pos.x++)
    {
        if (!current_circuit->connections_ew[pos.y][pos.x].touched)
            continue;
        Pressure vented = (current_circuit->connections_ew[pos.y][pos.x].vented);
        unsigned value = pressure_as_percent(current_circuit->connections_ew[pos.y][pos.x].value);
        if (vented > 2)
        {
            SDL_Rect src_rect = {16*int(rand & 3), 166, 16, 16};
            SDL_Rect dst_rect = {(pos.x * 32  - 9) * scale + int(rand % 3 * scale) + grid_offset.x, (pos.y * 32 + 7 ) * scale + int(rand % 3 * scale) + grid_offset.y, 16 * scale, 16 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
        if (value == 100)
        {
            SDL_Rect src_rect = {40, 161, 9, 5};
            SDL_Rect dst_rect = {(pos.x * 32  - 5) * scale + grid_offset.x, (pos.y * 32 + 13) * scale + grid_offset.y, 9 * scale, 5 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
        else
        {
            SDL_Rect src_rect = {(int(value) / 10) * 4, 161, 4, 5};
            SDL_Rect dst_rect = {(pos.x * 32  - 5) * scale + grid_offset.x, (pos.y * 32 + 13) * scale + grid_offset.y, 4 * scale, 5 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            src_rect.w = 5;
            src_rect.x = (value % 10) * 4;
            dst_rect.w = 5 * scale;
            dst_rect.x += 4 * scale;
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
    }
    
    
    {                                               // Panel background
        for (pos.y = 0; pos.y < 11; pos.y++)
        for (pos.x = 0; pos.x < 8; pos.x++)
        {
            int panel_colour = 0;
            switch (panel_state)
            {
                case PANEL_STATE_LEVEL_SELECT:
                    panel_colour = 4;
                    break;
                case PANEL_STATE_MONITOR:
                    panel_colour = 2;
                    break;
            }
            
            
            SDL_Rect src_rect = {256 + (panel_colour * 48), 112, 16, 16};
            if (pos.y > 0)
                src_rect.y += 16;
            if (pos.y == 10)
                src_rect.y += 16;
            if (pos.x > 0)
                src_rect.x += 16;
            if (pos.x == 7)
                src_rect.x += 16;

            SDL_Rect dst_rect = {(pos.x * 32 + 32 * 11) * scale, (pos.y * 32) * scale, 32 * scale, 32 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }
        {
            SDL_Rect src_rect = {256, 160, 32 * 7, 32};
            SDL_Rect dst_rect = {(16 + 32 * 11) * scale, (16) * scale, 32 * 7 * scale, 32 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
        }

    }

    
    
    if (panel_state == PANEL_STATE_LEVEL_SELECT)
    {
        unsigned level_index = 0;
        
        for (pos.y = 0; pos.y < 8; pos.y++)
        for (pos.x = 0; pos.x < 7; pos.x++)
        {
            if (level_index >= LEVEL_COUNT)
                break;
            SDL_Rect src_rect = {256, 80, 32, 32};
            if (level_index == current_level)
                src_rect = {288, 80, 32, 32};
            SDL_Rect dst_rect = {pos.x * 32 * scale + panel_offset.x, pos.y * 32 * scale + panel_offset.y, 32 * scale, 32 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);

            src_rect = {0, 230 + (int(level_index) * 24), 24, 24};
            dst_rect = {(pos.x * 32 + 4) * scale + panel_offset.x, (pos.y * 32 + 4) * scale + panel_offset.y, 24 * scale, 24 * scale};
            SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
            level_index++;
        }

    } else if (panel_state == PANEL_STATE_MONITOR)
    {
        int mon_offset = 0;
        for (unsigned mon_index = 0; mon_index < 4; mon_index++)
        {
            unsigned graph_size;
            if (mon_index == selected_monitor)
            {
                for (int x = 0; x < 7; x++)
                {
                    SDL_Rect src_rect = {448, 112, 32, 32};
                    SDL_Rect dst_rect = {x * 32 * scale + panel_offset.x, mon_offset * scale + panel_offset.y, 32 * scale, 32 * scale};
                    if (x > 0)
                        src_rect.x += 8;
                    if (x == 6)
                        src_rect.x += 8;
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);

                    src_rect.y += 16;
                    src_rect.h = 1;
                    dst_rect.y += 32 * scale;
                    dst_rect.h = 48 * scale;
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
                    
                    src_rect.h = 32;
                    dst_rect.h = 32 * scale;
                    dst_rect.y += 48 * scale;
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
                }
                {
                    SDL_Rect src_rect = {84, 166, 1, 1};
                    SDL_Rect dst_rect = {5 * scale + panel_offset.x, (mon_offset + 5) * scale + panel_offset.y, 200 * scale, 101 * scale};
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);

                }
                for (int x = 0; x < 10; x++)
                {
                    SDL_Rect src_rect = {105, 160, 11, 101};
                    SDL_Rect dst_rect = {(x * 20 + 5) * scale + panel_offset.x, (mon_offset + 5) * scale + panel_offset.y, 11 * scale, 101 * scale};
                    if (x != 0)
                    {
                        src_rect.w = 4;
                        dst_rect.w = 4 * scale;
                    }
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
                }
            }
            else
            {
                for (int x = 0; x < 7; x++)
                {
                    SDL_Rect src_rect = {448, 112, 32, 48};
                    SDL_Rect dst_rect = {x * 32 * scale + panel_offset.x, mon_offset * scale + panel_offset.y, 32 * scale, 48 * scale};
                    if (x > 0)
                        src_rect.x += 8;
                    if (x == 6)
                        src_rect.x += 8;

                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
                }
                {
                    SDL_Rect src_rect = {84, 166, 1, 1};
                    SDL_Rect dst_rect = {5 * scale + panel_offset.x, (mon_offset + 5) * scale + panel_offset.y, 200 * scale, 35 * scale};
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);

                }
                for (int x = 0; x < 10; x++)
                {
                    SDL_Rect src_rect = {85, 160, 11, 35};
                    SDL_Rect dst_rect = {(x * 20 + 5) * scale + panel_offset.x, (mon_offset + 5) * scale + panel_offset.y, 11 * scale, 35 * scale};
                    if (x != 0)
                    {
                        src_rect.w = 4;
                        dst_rect.w = 4 * scale;
                    }
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
                }
            }

            unsigned width = 200 / levels[current_level]->sim_point_count;
            assert (width);
            for (int x = 0; x < levels[current_level]->sim_point_count; x++)
            {

                IOValue& io_val = levels[current_level]->sim_points[x].get(mon_index);

                if (!io_val.observed)
                {
                    SDL_Rect src_rect = {84, 161, 1, 1};
                    int offset = 100 - io_val.in_value;
                    if (mon_index != selected_monitor)
                        offset = (offset + 2) / 3;

                    SDL_Rect dst_rect = {(x * int(width) + 5) * scale + panel_offset.x, (mon_offset + 5 + offset) * scale + panel_offset.y, int(width) * scale, 1 * scale};
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
                }
                else
                {
                    int offset = 100 - io_val.recorded;
                    if (mon_index != selected_monitor)
                        offset = (offset + 2) / 3;

                    {
                        SDL_Rect src_rect = {84, 161, 1, 1};
                        int t_offset = 100 - io_val.in_value;
                        if (mon_index != selected_monitor)
                            t_offset = (t_offset + 2) / 3;
                        
                        int size = abs(offset - t_offset) + 1;
                        int top = offset < t_offset ? offset : t_offset;

                        SDL_Rect dst_rect = {(x * int(width) + 5) * scale + panel_offset.x, (mon_offset + 5 + top) * scale + panel_offset.y, int(width) * scale, size * scale};
                        SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
                    }

                    SDL_Rect src_rect = {84, 160, 1, 1};

                    SDL_Rect dst_rect = {(x * int(width) + 5) * scale + panel_offset.x, (mon_offset + 5 + offset) * scale + panel_offset.y, int(width) * scale, 1 * scale};
                    SDL_RenderCopy(sdl_renderer, sdl_texture, &src_rect, &dst_rect);
                }
            }
            
            if (mon_index == selected_monitor)
                mon_offset += 112 + 4;
            else
                mon_offset += 48 + 4;


        }
    }



    SDL_RenderPresent(sdl_renderer);
}

void GameState::set_level(unsigned level_index)
{
    current_level = level_index;
    current_circuit = levels[current_level]->circuit;
    levels[current_level]->reset();
}

void GameState::mouse_click_in_grid()
{
    XYPos pos = (mouse - grid_offset) / scale;
    XYPos grid = pos / 32;
    if (grid.x >= 9 || grid.y >= 9)
        return;
    if (mouse_state == MOUSE_STATE_NONE)
    {
        mouse_state = MOUSE_STATE_PIPE;
        XYPos pos = (mouse - grid_offset) / scale;
        XYPos grid = pos / 32;
        pos -= grid * 32;
        pipe_start_grid_pos = grid;
        if (pos.x > pos.y)      // upper right
        {
            if ((31 - pos.x) > pos.y)
            {
                pipe_start_ns = true; //up
            }
            else
            {
                pipe_start_grid_pos.x++; //right
                pipe_start_ns = false;
            }
        }
        else                    // lower left
        {
            if ((31 - pos.x) > pos.y)
            {
                pipe_start_ns = false; //left
            }
            else
            {
                pipe_start_grid_pos.y++; //down
                pipe_start_ns = true;
            }
        }
    }
    else if (mouse_state == MOUSE_STATE_PIPE)
    {
        XYPos mouse_grid = ((mouse - grid_offset) / scale) / 32;
        XYPos mouse_rel = ((mouse - grid_offset) / scale) - (pipe_start_grid_pos * 32);
        if (pipe_start_ns)
        {
            mouse_rel.x -= 16;
            if (mouse_rel.y < 0)    //south - northwards
            {
                pipe_start_grid_pos.y--;
                Connections con;
                if (-mouse_rel.y > abs(mouse_rel.x))    // north
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_NS);
                }
                else if (mouse_rel.x < 0)               // west
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_WS);
                    pipe_start_ns = false;
                }
                else                                    // east
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_ES);
                    pipe_start_ns = false;
                    pipe_start_grid_pos.x++;
                }

            }
            else
            {
                Connections con;
                if (mouse_rel.y > abs(mouse_rel.x))    // north
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_NS);
                    pipe_start_grid_pos.y++;

                }
                else if (mouse_rel.x < 0)               // west
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_NW);
                    pipe_start_ns = false;
                }
                else                                    // east
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_NE);
                    pipe_start_ns = false;
                    pipe_start_grid_pos.x++;
                }

            }

        }
        else
        {
            mouse_rel.y -= 16;
            if (mouse_rel.x < 0)    //east - westwards
            {
                pipe_start_grid_pos.x--;
                Connections con;
                if (-mouse_rel.x > abs(mouse_rel.y))    // west
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_EW);
                }
                else if (mouse_rel.y < 0)               // north
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_NE);
                    pipe_start_ns = true;
                }
                else                                    // south
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_ES);
                    pipe_start_ns = true;
                    pipe_start_grid_pos.y++;
                }

            }
            else                    //west - eastwards
            {
                Connections con;
                if (mouse_rel.x > abs(mouse_rel.y))    // west
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_EW);
                    pipe_start_grid_pos.x++;

                }
                else if (mouse_rel.y < 0)               // north
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_NW);
                    pipe_start_ns = true;
                }
                else                                    // south
                {
                    current_circuit->set_element_pipe(pipe_start_grid_pos, CONNECTIONS_WS);
                    pipe_start_ns = true;
                    pipe_start_grid_pos.y++;
                }

            }
        }
    }
    else if (mouse_state == MOUSE_STATE_PLACING_VALVE)
    {
        XYPos mouse_grid = ((mouse - grid_offset) / scale) / 32;
        current_circuit->set_element_valve(mouse_grid, direction);
    }
    else if (mouse_state == MOUSE_STATE_PLACING_SOURCE)
    {
        XYPos mouse_grid = ((mouse - grid_offset) / scale) / 32;
        current_circuit->set_element_source(mouse_grid, direction);
    }
}

void GameState::mouse_click_in_panel()
{
    {
        XYPos menu_icons_pos = (mouse / scale - XYPos((16 + 32 * 11), (16)));
        if (menu_icons_pos.y < 0)
            return;
        XYPos menu_icons_grid_pos = menu_icons_pos / 32;
        if (menu_icons_grid_pos.y == 0)
        {
            unsigned sel = menu_icons_grid_pos.x;
            switch (sel)
            {
                case 0:
                    panel_state = PANEL_STATE_LEVEL_SELECT;
                    break;
                case 4:
                    panel_state = PANEL_STATE_MONITOR;
                    break;
            }
            return;
        }
    }
    
    XYPos panel_pos = ((mouse - panel_offset) / scale);
    XYPos panel_grid_pos = panel_pos / 32;
    if (panel_pos.y < 0 || panel_pos.x < 0)
        return;

//    printf("%d %d\n", panel_grid_pos.x, panel_grid_pos.y);
    
    
    if (panel_state == PANEL_STATE_LEVEL_SELECT)
    {
        unsigned level_index = panel_grid_pos.x + panel_grid_pos.y * 7;
        set_level(level_index);
        return;
    }
}

bool GameState::events()
{
    SDL_Event e;
    while(SDL_PollEvent(&e))
    {
	    switch (e.type)
        {
            case SDL_QUIT:
		        return true;
            case SDL_KEYDOWN:
            {
                bool down = (e.type == SDL_KEYDOWN);
                if (down)
                {
                    if (e.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
                        return true;
                    else if (e.key.keysym.scancode == SDL_SCANCODE_TAB)
                    {
                        if (mouse_state == MOUSE_STATE_PLACING_VALVE)
                            mouse_state = MOUSE_STATE_PLACING_SOURCE;
                        else
                            mouse_state = MOUSE_STATE_PLACING_VALVE;
                    }
                    else if (e.key.keysym.scancode == SDL_SCANCODE_Q)
                        direction = Direction((int(direction) + 4 - 1) % 4);
                    else if (e.key.keysym.scancode == SDL_SCANCODE_E)
                        direction = Direction((int(direction) + 1) % 4);
                    else printf("Uncaught key: %d\n", e.key.keysym);
                }
                break;
            }
            case SDL_TEXTINPUT:
            {
                break;
            }
            case SDL_KEYUP:
                break;
            case SDL_MOUSEMOTION:
            {
                mouse.x = e.motion.x;
                mouse.y = e.motion.y;
                if (mouse_state == MOUSE_STATE_DELETING)
                {
                    XYPos grid = ((mouse - grid_offset) / scale) / 32;
                    if (grid.x >= 9 || grid.y >= 9)
                        break;
                    current_circuit->set_element_pipe(grid, CONNECTIONS_NONE);
                }
                break;
            }
            case SDL_MOUSEBUTTONUP:
            {
                mouse.x = e.button.x;
                mouse.y = e.button.y;
                if (e.button.button == SDL_BUTTON_RIGHT)
                {
                    if (mouse_state == MOUSE_STATE_DELETING)
                        mouse_state = MOUSE_STATE_NONE;
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN:
            {
                mouse.x = e.button.x;
                mouse.y = e.button.y;
                if (e.button.button == SDL_BUTTON_LEFT)
                {
                    if (mouse.x < panel_offset.x)
                        mouse_click_in_grid();
                    else
                        mouse_click_in_panel();
                }
                else if (e.button.button == SDL_BUTTON_RIGHT)
                {
                    if (mouse_state == MOUSE_STATE_NONE)
                    {
                        XYPos grid = ((mouse - grid_offset) / scale) / 32;
                        current_circuit->set_element_pipe(grid, CONNECTIONS_NONE);
                        mouse_state = MOUSE_STATE_DELETING;
                    }
                    else
                        mouse_state = MOUSE_STATE_NONE;
                }
                break;
            }
            case SDL_MOUSEWHEEL:
            {
            break;
            }
        }
    }

   return false;
}
