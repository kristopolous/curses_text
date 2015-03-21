#include "ctext.h"
#include <unistd.h>
#include <string.h>
#include <algorithm>		// std::max

using namespace std;

#ifndef _WIN32

#define CTEXT_UNDER_X		0x01
#define CTEXT_OVER_X		0x02
#define CTEXT_UNDER_Y		0x04
#define CTEXT_OVER_Y		0x08

#define CTEXT_OVER			(CTEXT_OVER_Y | CTEXT_OVER_X)
#define CTEXT_UNDER			(CTEXT_UNDER_Y | CTEXT_UNDER_X)

const ctext_config config_default = {
	.m_buffer_size = CTEXT_DEFAULT_BUFFER_SIZE,
	.m_bounding_box = CTEXT_DEFAULT_BOUNDING_BOX,
	.m_do_wrap = CTEXT_DEFAULT_DO_WRAP,
	.m_append_top = CTEXT_DEFAULT_APPEND_TOP,
	.m_scroll_on_append = CTEXT_DEFAULT_SCROLL_ON_APPEND,
	.m_auto_newline = CTEXT_DEFAULT_AUTO_NEWLINE,
};

ctext::ctext(WINDOW *win, ctext_config *config)
{
	this->m_win = win;

	this->m_debug = new ofstream();
	this->m_debug->open("debug1.txt");

	this->m_do_draw = true;
	
	if(config) 
	{
		memcpy(&this->m_config, config, sizeof(ctext_config));
	} 
	else 
	{
		memcpy(&this->m_config, &config_default, sizeof(ctext_config));
	}

	this->m_pos.x = 0;
	this->m_pos.y = 0;

	this->m_attr_mask = 0;

	this->m_max_y = 0;

	// initialized the buffer with the empty row
	this->add_row();
}

int8_t ctext::set_config(ctext_config *config)
{
	memcpy(&this->m_config, config, sizeof(ctext_config));
	return this->redraw();
}

int8_t ctext::get_config(ctext_config *config)
{
	memcpy(config, &this->m_config, sizeof(ctext_config));
	return 0;
}

int8_t ctext::attach_curses_window(WINDOW *win)
{
	this->m_win = win;
	return this->redraw();
}

int32_t ctext::putchar(int32_t c)
{
	return this->printf("%c", c);
}

int32_t ctext::clear(int32_t row_count)
{
	int32_t ret = 0;

	if(row_count == -1) 
	{
		ret = this->m_buffer.size();
		this->m_buffer.clear();
		this->add_row();
	}
	else if(this->m_buffer.size()) 
	{
		ret = this->m_buffer.size();
		this->m_buffer.erase(this->m_buffer.begin(), this->m_buffer.begin() + row_count);
		ret -= this->m_buffer.size();
	}

	// We do the same logic when removing content
	// .. perhaps forcing things down or upward
	if(this->m_config.m_scroll_on_append)
	{
		this->get_win_size();
		// now we force it.
		this->direct_scroll(0, this->m_buffer.size() - this->m_win_height);
	}

	this->redraw();
	return ret;
}

int8_t ctext::ob_start()
{
	int8_t ret = this->m_do_draw;
	this->m_do_draw = false;
	return ret;
}

int8_t ctext::ob_end()
{
	int8_t ret = !this->m_do_draw;
	this->m_do_draw = true;
	this->redraw();
	return ret;
}

int8_t ctext::direct_scroll(int32_t x, int32_t y)
{
	if(this->m_config.m_bounding_box) 
	{
		y = min(y, this->m_max_y - this->m_win_height);
		x = max(0, x);
		y = max(0, y);
	}

	this->m_pos.x = x;
	this->m_pos.y = y;

	return 0;
}

int8_t ctext::scroll_to(ctext_pos *pos)
{
	return this->scroll_to(pos->x, pos->y);
}

int8_t ctext::scroll_to(int32_t x, int32_t y)
{
	this->direct_scroll(x, y);
	return this->redraw();
}

int8_t ctext::get_offset(ctext_pos *pos)
{
	return this->get_offset(&pos->x, &pos->y);
}

int8_t ctext::get_offset(int32_t*x, int32_t*y)
{
	*x = this->m_pos.x;
	*y = this->m_pos.y;

	return 0;
}

int8_t ctext::get_offset_percent(float*percent)
{
	this->get_win_size();
	*percent = (float)(this->m_pos.y) / (this->m_max_y - this->m_win_height);

	return 0;
}

int8_t ctext::get_buf_size(int32_t*buf_size)
{
	*buf_size = this->m_max_y;

	return 0;
}

int32_t ctext::available_rows()
{
	// since our buffer clearing scheme permits us to overflow,
	// we have to bind this to make sure that we return >= 0 values
	return max(this->m_config.m_buffer_size - this->m_max_y - 1, 0);
}

int32_t ctext::up(int32_t amount) 
{
	return this->down(-amount);
}

int32_t ctext::page_down(int32_t page_count) 
{
	this->get_win_size();
	return this->down(page_count * this->m_win_height);
}

int32_t ctext::page_up(int32_t page_count) 
{
	this->get_win_size();
	return this->down(-page_count * this->m_win_height);
}

// let's do this real fast.
int8_t ctext::map_to_win(int32_t buffer_x, int32_t buffer_y, ctext_pos*win)
{
	int8_t ret = 0;

	// This is the trivial case.
	if(!this->m_config.m_do_wrap)
	{
		// these are trivial.
		win->y = buffer_y - this->m_pos.y;
		win->x = buffer_x - this->m_pos.x;

		return 
				((buffer_x < this->m_pos.x))
			| ((buffer_x > this->m_pos.x + this->m_win_width) << 1)
			|	((buffer_y < this->m_pos.y) << 2)
			| ((buffer_y > this->m_pos.y + this->m_win_height) << 3);
	}
	// Otherwise it's much more challenging.
	else
	{
		// If we are below the fold or we are at the first line and before
		// the start of where we ought to be drawing
		if(buffer_y < this->m_pos.y || (buffer_y == this->m_pos.y && buffer_x < this->m_pos.x))
		{
			// we omit win calculations here since they
			// would be more expensive then we'd like
			win->x = win->y = -1;

			ret |= CTEXT_UNDER_Y;
		}
		else 
		{
			// to see if it's an overflow y is a bit harder
			int32_t new_y = this->m_pos.y;

			int32_t new_offset = this->m_pos.x;

			string *data = &this->m_buffer[new_y].data;

			for(win->y = 0; win->y < this->m_win_height; win->y++)
			{
				new_offset += this->m_win_width;

				// there's an edge case that requires this
				// twice due to a short circuit exit that 
				// would be triggered at the end of a buffer, 
				// see below.
				if(buffer_y == new_y && buffer_x < new_offset)
				{
					win->x = buffer_x - (new_offset - this->m_win_width);
					return ret;
				}

				if(new_offset > (int32_t)data->size()) 
				{
					new_offset = 0;
					new_y ++;
					if(new_y > this->m_max_y)
					{
						break;
					}
					data = &this->m_buffer[new_y].data;
				}

				if(
					// We've passed it and we know it's not
					// an underflow, so we're done.
					(buffer_y < new_y) ||

					// We are at the end line and our test point
					// is below the offset that we just flew past
					(buffer_y == new_y && buffer_x < new_offset)
				)
				{
					win->x = buffer_x - (new_offset - this->m_win_width);
					return ret;
				}
			}

			// keep the y at the end
			// win->y = -1;

			// If we get here that means that we went all the way through
			// a "generation" and didn't reach our hit point ... that means
			// it's an overflow.
			ret |= CTEXT_OVER_Y;	
		}
	}

	return ret;
}

int8_t ctext::y_scroll_calculate(int32_t amount, int32_t *x, int32_t *y)
{
	if(this->m_config.m_do_wrap)
	{
		int32_t new_y = this->m_pos.y;
		int32_t new_offset = this->m_pos.x;
		ctext_row *p_row = &this->m_buffer[this->m_pos.y];	

		this->get_win_size();

		while(amount > 0)
		{
			new_offset += this->m_win_width;
			amount --;
			if(new_offset > (int32_t)p_row->data.size())
			{
				if(new_y + 1 >= (int32_t)this->m_buffer.size())
				{
					break;
				}
				new_offset = 0;
				new_y++;
				p_row = &this->m_buffer[new_y];
			}
		} 

		while(amount < 0)
		{
			new_offset -= this->m_win_width;
			amount ++;
			if(new_offset < 0)
			{
				if(new_y - 1 < 0)
				{
					break;
				}
				new_y--;
				p_row = &this->m_buffer[new_y];
				new_offset = p_row->data.size() - p_row->data.size() % this->m_win_width;
			}
		}
		*x = new_offset;
		*y = new_y;
	}
	else
	{
		*x = this->m_pos.x;
		*y = this->m_pos.y + amount;
	}

	return 0;
}


int32_t ctext::down(int32_t amount) 
{
	int32_t new_x, new_y;
	this->y_scroll_calculate(amount, &new_x, &new_y);
	return this->scroll_to(new_x, new_y);
}

int32_t ctext::jump_to_first_line()
{
	int32_t current_line = this->m_pos.y;

	// now we try to scroll above the first
	// line.	the bounding box rule will
	// take care of the differences for us.
	this->scroll_to(this->m_pos.x, 0 - this->m_win_height + 1);

	return current_line - this->m_pos.y;
}

int32_t ctext::jump_to_last_line()
{
	int32_t current_line = this->m_pos.y;

	this->get_win_size();
	this->scroll_to(this->m_pos.x, this->m_max_y - 1);
	return current_line - this->m_pos.y;
}

int32_t ctext::left(int32_t amount) 
{
	return this->right(-amount);
}

int32_t ctext::right(int32_t amount) 
{
	return this->scroll_to(this->m_pos.x + amount, this->m_pos.y);
}

void ctext::get_win_size() 
{
	int32_t width = 0, height = 0;

	if(this->m_win)
	{
		getmaxyx(this->m_win, height, width);
	}
	this->m_win_width = width;
	this->m_win_height = height;
}

int8_t ctext::rebuf()
{
	// memory management is expensive, so we only
	if((int32_t)this->m_buffer.size() > (this->m_config.m_buffer_size * 11 / 10))
	{
		this->m_buffer.erase(this->m_buffer.begin(), this->m_buffer.end() - this->m_config.m_buffer_size);
	}
	
	this->m_max_y = this->m_buffer.size() - 1;
	
	//
	// Since we've changed the bounding box of the content we have to
	// issue a rescroll on exactly our previous parameters. This may
	// force us inward or may retain our position.
	// 
	return this->direct_scroll(this->m_pos.x, this->m_pos.y);
}

void ctext::add_format_if_needed()
{
	attr_t attrs; 
	int16_t color_pair;

	if(!this->m_win) 
	{
		return;
	}

	if(this->m_buffer.empty())
	{
		return;
	}

	// get the most current row.
	ctext_row *p_row = &this->m_buffer.back();

	ctext_format p_format = {0,0,0};
	if(!p_row->format.empty()) 
	{
		// and the most current format
		p_format = p_row->format.back();
	} 

	wattr_get(this->m_win, &attrs, &color_pair, 0);

	if(attrs != p_format.attrs || color_pair != p_format.color_pair)
	{
		// our properties have changed so we need to record this.
		ctext_format new_format = 
		{
			// this is our offset
			.offset = (int32_t)p_row->data.size(),

			.attrs = attrs,
			.color_pair = color_pair
		};

		// if the new thing we are adding has the same
		// offset as the previous, then we dump the
		// previous.
		if(p_format.offset == new_format.offset && !p_row->format.empty())
		{
			p_row->format.pop_back();
		}
		p_row->format.push_back(new_format);
	}
}

ctext_row* ctext::add_row()
{
	ctext_row row;

	// if there is an exsting line, then
	// we carry over the format from the
	// last line..
	if(!this->m_buffer.empty())
	{
		ctext_row p_row = this->m_buffer.back();

		if(!p_row.format.empty()) 
		{
			ctext_format p_format( p_row.format.back() );

			// set the offset to the initial.
			p_format.offset = 0;
			row.format.push_back(p_format);
		}
	}

	this->m_buffer.push_back(row);

	return &this->m_buffer.back();
}

char* next_type(char* search, const char delim) 
{
	while(*search && *search != delim)
	{
 		search ++;
	}
	return search;
}

int8_t ctext::vprintf(const char*format, va_list ap)
{
	char *p_line, *n_line;
	char large_buffer[CTEXT_BUFFER_SIZE];

	this->add_format_if_needed();
	ctext_row *p_row = &this->m_buffer.back();

	vsnprintf(large_buffer, CTEXT_BUFFER_SIZE, format, ap);

	p_line = large_buffer;
	do 
  {
		n_line = next_type(p_line, '\n');

		string wstr(p_line, n_line - p_line);
		p_row->data += wstr;

		if(*n_line)
		{
			p_row = this->add_row();
		}
		p_line = n_line + 1;
	} 
  while (*n_line);

	if(this->m_config.m_auto_newline)
	{
		this->add_row();
	}
	
	// Since we are adding content we need to see if we are
	// to force on scroll.
	if(this->m_config.m_scroll_on_append)
	{
		this->get_win_size();
		// now we force it.
		this->direct_scroll(0, this->m_buffer.size() - this->m_win_height);
	}

	return this->redraw();
}

int cprintf(ctext*win, const char *format, ...)
{
	int ret;
	va_list args;
	va_start(args, format);
	ret = win->vprintf(format, args);
	va_end(args);
	return ret;
}

int8_t ctext::printf(const char*format, ...)
{
	int8_t ret;

	va_list args;
	va_start(args, format);
	ret = this->vprintf(format, args);

	va_end(args);
	return ret;
}

int8_t ctext::nprintf(const char*format, ...)
{
	int8_t ret;
	va_list args;

	// first turn off the rerdaw flag
	this->ob_start();

	// then call the variadic version
	va_start(args, format);
	ret = this->vprintf(format, args);

	va_end(args);

	//
	// then manually untoggle the flag
	// (this is necessary because ob_end
	// does TWO things, breaking loads of
	// anti-patterns I'm sure.)
	//
	this->m_do_draw = true;

	return ret;
}

int8_t ctext::redraw_partial_test()
{
	attr_t res_attrs; 
	int16_t res;
	int16_t res_color_pair;
	int32_t x, y, end_x;
	string *data;

	this->get_win_size();
	wattr_get(this->m_win, &res_attrs, &res_color_pair, 0);
	wattr_off(this->m_win, COLOR_PAIR(res_color_pair), 0);
	
	this->rebuf();
	werase(this->m_win);

	x = this->m_pos.x;
	y = this->m_pos.y;
	data = &this->m_buffer[y].data;

	while(!(res & CTEXT_OVER_Y)) 
	{
		this->m_attr_mask ^= A_REVERSE;
		end_x = min(x + 7, (int32_t)data->size());	
		res = this->redraw_partial(x, y, end_x, y);
		wrefresh(this->m_win);
		usleep(1000 * 500);

		x = end_x;
		
		if(end_x == (int32_t)data->size() || (res & CTEXT_OVER_X)) 
		{
			y++;
			x = 0;
			if(y > this->m_max_y)
			{
				break;
			}
			data = &this->m_buffer[y].data;
		}
	}

	wrefresh(this->m_win);
	wattr_set(this->m_win, res_attrs, res_color_pair, 0);

	return 0;
}

// 
// redraw_partial takes a buffer offset and sees if it is
// to be drawn within the current view port, which is specified
// by m_pos.x and m_pos.y. 
//
int16_t ctext::redraw_partial(
		int32_t buf_start_x, int32_t buf_start_y, 
		int32_t buf_end_x, int32_t buf_end_y)
{
	bool b_format = false;
	string to_add;
	ctext_row *p_source;
	vector<ctext_format>::iterator p_format;
	bool is_first_line = true;
	int16_t ret = 0;
	int32_t num_added_x = 0;
	int32_t num_to_add = 0;
	int32_t win_current_x;
	int32_t win_current_end_x;
	int32_t buf_offset_x;

	// we need to get relative start and end positions.
	ctext_pos win_start, win_end;

	ret = this->map_to_win(buf_start_x, buf_start_y, &win_start);

	// This means that none of this will map to screen, 
	// return the overflow and bail.
	if(ret & CTEXT_OVER_Y)
	{
		return ret;
	}

	ret = this->map_to_win(buf_end_x, buf_end_y, &win_end);

	*this->m_debug << "(" << win_start.x << " " << win_start.y << ") " <<
			 "(" << win_end.x << " " << win_end.y << ") " << endl;

	// This also means that none of this will map to screen, 
	// return the underflow and bail.
	if(ret & CTEXT_UNDER_Y)
	{
		return ret;
	}

	//
	// We start as m_pos.y in our list and move up to
	// m_pos.y + m_win_height except in the case of 
	// wrap around.  Because of this special case,
	// we compute when to exit slightly differently.
	//
	// This is the current line of output, which stays
	// below m_win_height
	//
	int32_t win_current_y = win_start.y;

	// this is for horizontal scroll.
	int32_t start_char = max(0, this->m_pos.x);
	int32_t buf_current_y = buf_start_y;
	
	while(win_current_y <= win_end.y)
	{
		win_current_end_x = this->m_win_width;

		// if we are at the last line to generate
		if(win_current_y == win_end.y)
		{
			// then we make sure that we end
			// where we are supposed to.
			win_current_end_x = win_end.x;
		}

		wredrawln(this->m_win, win_current_y, 1);
		
		if((buf_current_y < this->m_max_y) && (buf_current_y >= 0))
		{
			// We only buf_current_y into the object if we have the
			// data to do so.
			p_source = &this->m_buffer[buf_current_y];
			p_format = p_source->format.begin();

			// Reset the offset.
			win_current_x = -min(0, (int32_t)this->m_pos.x);

			if(is_first_line)
			{
				buf_offset_x = buf_start_x;
				win_current_x += win_start.x;
			}
			else 
			{
				buf_offset_x = start_char;
			}

			for(;;) 
			{
				// our initial num_to_add is the remainder of window space
				// - our start (end of the screen - starting position)
				num_to_add = win_current_end_x - win_current_x;
				b_format = false;

				wstandend(this->m_win);

				// if we have a format to account for and we haven't yet,
				if(!p_source->format.empty() && p_format->offset <= buf_offset_x)
				{
					// then we add it 
					wattr_set(this->m_win, p_format->attrs | this->m_attr_mask, p_format->color_pair, 0);

					// and tell ourselves below that we've done this.
					b_format = true;

					// see if there's another num_to_add point
					if((p_format + 1) != p_source->format.end())
					{
						// If it's before our newline then we'll have to do something
						// with with that.
						//
						// The first one is the characters we are to print this time,
						// the second is how many characters we would have asked for
						// if there was no format specified.
						num_to_add = min((p_format + 1)->offset - buf_offset_x, num_to_add); 
					}
				}

				// if we can get that many characters than we grab them
				// otherwise we do the empty string
				if(buf_offset_x < (int32_t)p_source->data.size())
				{
					to_add = p_source->data.substr(buf_offset_x, num_to_add);

					mvwaddstr(this->m_win, win_current_y, win_current_x, to_add.c_str());
					is_first_line = false;
				}
				else
				{
					to_add = "";
				}

				// This is the number of characters we've placed into
				// the window.
				num_added_x = to_add.size();
				buf_offset_x += num_added_x;

				// See if we need to reset our format
				if(b_format) 
				{
					// If the amount of data we tried to grab is less than
					// the width of the window - win_offset then we know to
					// turn off our attributes

					// and push our format forward if necessary
					if( (p_format + 1) != p_source->format.end() &&
							(p_format + 1)->offset >= buf_offset_x 
						)
					{
						p_format ++;
					}
				}

				// if we are at the end of the string, we break out
				if((int32_t)p_source->data.size() <= buf_offset_x || (num_added_x == 0 && p_source->data.size() > 0))
				{
					break;
				}

				// otherwise, move win_current_x forward
				win_current_x += num_added_x;
				
				// otherwise, if we are wrapping, then we do that here.
				if(win_current_x == win_current_end_x)
				{
					// if we've hit the vertical bottom
					// of our window then we break out
					// of this
					//
					// otherwise if we are not wrapping then
					// we also break out of this
					if(win_current_y == win_end.y)
					{
						break;
					}

					// otherwise move our line forward
					win_current_y++;

					// if we are at the last line to generate
					if(win_current_y == win_end.y)
					{
						// then we make sure that we end
						// where we are supposed to.
						win_current_end_x = win_end.x;
					}

					// we reset the win_current_x back to its
					// initial state
					win_current_x = 0;

					// and we loop again.
				}
			}
		}
		buf_current_y++;
		win_current_y++;
	}

	return ret;
}

int8_t ctext::redraw() 
{
	// Bail out if we aren't supposed to draw
	// this time.
	// Calculate the bounds of everything first.
	this->rebuf();
	if(!this->m_do_draw)
	{
		return 0;
	}

	if(!this->m_win)
	{
		// Not doing anything without a window.
		return -1;
	}

	attr_t res_attrs; 
	int16_t res_color_pair;
	bool is_first_line = true;
	wattr_get(this->m_win, &res_attrs, &res_color_pair, 0);
	wattr_off(this->m_win, COLOR_PAIR(res_color_pair), 0);
	
	this->get_win_size();

	//
	// By this time, if we are bounded by a box,
	// it has been accounted for.
	//
	// Really our only point of interest is
	// whether we need to append to bottom
	// or append to top.
	//
	// We will assume that we can
	// populate the window quick enough
	// to avoid linear updating or paging.
	//	... it's 2015 after all.
	//
	werase(this->m_win);

	// Regardless of whether this is append to top
	// or bottom we generate top to bottom.

	int32_t start_char = max(0, this->m_pos.x);
	int32_t buf_offset = start_char;
	// the endchar will be in the substr
	
	//
	// We start as m_pos.y in our list and move up to
	// m_pos.y + m_win_height except in the case of 
	// wrap around.  Because of this special case,
	// we compute when to exit slightly differently.
	//
	// This is the current line of output, which stays
	// below m_win_height
	//
	int32_t line = 0;

	// start at the beginning of the buffer.
	int32_t index = this->m_pos.y;
	int32_t directionality = +1;
	int32_t cutoff;
	int32_t num_added = 0;
	int32_t win_offset = 0;
	bool b_format = false;
	string to_add;
	ctext_row *p_source;
	vector<ctext_format>::iterator p_format;

	// if we are appending to the top then we start
	// at the end and change our directionality.
	if(this->m_config.m_append_top)
	{
		directionality = -1;
		index = this->m_pos.y + this->m_win_height - 1;
	}

	while(line <= this->m_win_height)
	{
		wredrawln(this->m_win, line, 1);
		
		if((index < this->m_max_y) && (index >= 0))
		{
			// We only index into the object if we have the
			// data to do so.
			p_source = &this->m_buffer[index];
			p_format = p_source->format.begin();

			// Reset the offset.
			win_offset = -min(0, (int32_t)this->m_pos.x);
			buf_offset = start_char;

			if(this->m_config.m_do_wrap)
			{
				buf_offset = is_first_line ? this->m_pos.x : 0;
			}

			for(;;) 
			{
				// our initial cutoff is the remainder of window space
				// - our start
				cutoff = this->m_win_width - win_offset;
				b_format = false;

				wstandend(this->m_win);
				// if we have a format to account for and we haven't yet,
				if(!p_source->format.empty() && p_format->offset <= buf_offset)
				{
					// then we add it 
					wattr_set(this->m_win, p_format->attrs, p_format->color_pair, 0);

					// and tell ourselves below that we've done this.
					b_format = true;

					// see if there's another cutoff point
					if((p_format + 1) != p_source->format.end())
					{
						// If it's before our newline then we'll have to do something
						// with with that.
						//
						// The first one is the characters we are to print this time,
						// the second is how many characters we would have asked for
						// if there was no format specified.
						cutoff = min((p_format + 1)->offset - buf_offset, cutoff); 
					}
				}

				// if we can get that many characters than we grab them
				// otherwise we do the empty string
				if(buf_offset < (int32_t)p_source->data.size())
				{
					to_add = p_source->data.substr(buf_offset, cutoff);

					mvwaddstr(this->m_win, line, win_offset, to_add.c_str());
					is_first_line = false;
				}
				else
				{
					to_add = "";
				}

				// This is the number of characters we've placed into
				// the window.
				num_added = to_add.size();
				buf_offset += num_added;

				// See if we need to reset our format
				if(b_format) 
				{
					// If the amount of data we tried to grab is less than
					// the width of the window - win_offset then we know to
					// turn off our attributes

					// and push our format forward if necessary
					if( (p_format + 1) != p_source->format.end() &&
							(p_format + 1)->offset >= buf_offset 
						)
					{
						p_format ++;
					}
				}

				// if we are at the end of the string, we break out
				if((int32_t)p_source->data.size() <= buf_offset || (num_added == 0 && p_source->data.size() > 0))
				{
					break;
				}

				// otherwise, move win_offset forward
				win_offset += num_added;
				
				// otherwise, if we are wrapping, then we do that here.
				if(win_offset == this->m_win_width)
				{
					// if we've hit the vertical bottom
					// of our window then we break out
					// of this
					//
					// otherwise if we are not wrapping then
					// we also break out of this
					if(line == this->m_win_height || !this->m_config.m_do_wrap)
					{
						break;
					}

					// otherwise move our line forward
					line++;

					// we reset the win_offset back to its
					// initial state
					win_offset = 0;

					// and we loop again.
				}
			}
		}
		index += directionality;
		line++;
	}

	wrefresh(this->m_win);
	wattr_set(this->m_win, res_attrs, res_color_pair, 0);

	return 0;
}

#endif // _WIN32
