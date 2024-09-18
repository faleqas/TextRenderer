#include <stdio.h>
#include <stdbool.h>
#include <memory.h>

#include <SDL2/SDL.h>

#include "dynamic_array.h"

static bool running = true;


void load_ttf(const char* path);
void main_loop();


int main(int argc, char** argv)
{
	int* arr = NULL;
	int arr_size = 0;

	printf("\n");

 	SDL_Init(0);

	load_ttf("Consolas.ttf");
	main_loop();

	SDL_Quit();
	return 0;
}


#define Fixed int32_t
#define FWORD int16_t
#define UFWORD uint16_t
#define F2DOT14 int16_t
#define LONGDATETIME int64_t
#define Offset8 uint8_t
#define OFfset16 uint16_t
//for Offset24 we'll just have 1 Offset16 and 1 Offset8
#define Offset32 uint32_t
//#define Version16Dot16 int32_t //just use 2 seperate uint16_t too

//from big endian to little endian
uint32_t to_le32(uint32_t value) {
	return ((value & 0xFF000000) >> 24) |
		((value & 0x00FF0000) >> 8) |
		((value & 0x0000FF00) << 8) |
		((value & 0x000000FF) << 24);
}

//from big endian to little endian
uint16_t to_le16(uint16_t value) {
	return (value >> 8) | (value << 8);
}


typedef struct
{
	UFWORD advance_width;
	FWORD left_side_bearing;
} h_metric;


//t_ stands for table
typedef struct
{
	uint16_t major_version;
	uint16_t minor_version;

	Fixed font_revision;
	uint32_t checksum_adjustment;
	uint32_t magic_number;

	uint16_t flags;
	uint16_t units_per_em;

	LONGDATETIME created;
	LONGDATETIME modified;
	
	int16_t xmin;
	int16_t ymin;
	int16_t xmax;
	int16_t ymax;

	uint16_t mac_style;
	uint16_t lowest_rec_ppem;

	int16_t font_direction_hint;
	int16_t index_to_loc_format;
	int16_t glyph_data_format;
} t_head;


typedef struct
{
	Fixed version;
	uint16_t num_glyphs;
	uint16_t max_points;
	uint16_t max_contours;
	uint16_t max_composite_points;
	uint16_t max_zones;
	uint16_t max_twilight_points;
	uint16_t max_storage;
	uint16_t max_function_defs;
	uint16_t max_instruction_defs;
	uint16_t max_stack_elements;
	uint16_t max_sizeof_instructions;
	uint16_t max_component_elements;
	uint16_t max_component_depth;
} t_maxp;


typedef struct
{
	Fixed version;

	FWORD ascent;
	FWORD descent;

	FWORD line_gap;

	UFWORD advance_width_max;

	FWORD min_left_side_bearing;
	FWORD min_right_side_bearing
		;
	FWORD x_max_extent;

	int16_t caret_slope_rise;
	int16_t caret_slope_run;
	FWORD caret_offset;

	int _RESERVED1;
	int _RESERVED2;

	int16_t metric_data_format;
	uint16_t num_of_long_hor_metrics;
} t_hhea;


typedef struct
{
	h_metric* metrics;
	FWORD* left_side_bearing;

	//NOT PART OF SPEC
	int metrics_size; //in bytes to use dynamic_array.h functions
	int left_side_bearing_size;
} t_hmtx;


typedef struct
{
	char tag[4];
	uint32_t checksum;
	Offset32 offset;
	uint32_t length;
} table_record;

typedef struct
{
	uint32_t sfnt_version;
	uint16_t num_tables;
	uint16_t search_range;
	uint16_t entry_selector;
	uint16_t range_shift;
} table_directory;

//The values are not converted from big endian to little endian automatically. before using an offset or something use to_le32 or whatever

//for reading tables with a constant size
static void read_const_ttf_table(FILE* fp, table_record* record, void* table, size_t size)
{
	char str_tag[5];
	memcpy(str_tag, record->tag, 4);
	str_tag[4] = 0;
	printf("Reading '%s' table\n", str_tag);

	long current_offset = ftell(fp);
	fseek(fp, to_le32(record->offset), SEEK_SET);
	fread(table, size, 1, fp);

	fseek(fp, current_offset, SEEK_SET);
}


void load_ttf(const char* path)
{
	FILE* fp;
	fopen_s(&fp, path, "rb");
	if (!fp) return;
	fseek(fp, 0, SEEK_SET);

	table_directory td;
	fread(&td, sizeof(table_directory), 1, fp);
	td.num_tables = to_le16(td.num_tables);
	printf("NUM TABLES: %hu\n", td.num_tables);

	const const char* table_order[] =
	{
		"head",
		"maxp",
		"hhea",
		"hmtx"
	};
	const int table_count = sizeof(table_order) / sizeof(const char*);

	t_head head;
	t_maxp maxp;
	t_hhea hhea;
	t_hmtx hmtx = {0};

	long table_records_offset = ftell(fp);

	for (int i = 0; i < table_count; i++)
	{
		for (int j = 0; j < td.num_tables; j++)
		{
			table_record record;
			fread(&record, sizeof(table_record), 1, fp);

			char str_tag[5];
			memcpy(str_tag, record.tag, 4);
			str_tag[4] = 0;

			if (SDL_strcmp(str_tag, table_order[i]) == 0)
			{
				if (SDL_strcmp(str_tag, "head") == 0)
				{
					read_const_ttf_table(fp, &record, &head, sizeof(t_head));
				}
				else if (SDL_strcmp(str_tag, "maxp") == 0)
				{
					read_const_ttf_table(fp, &record, &maxp, sizeof(t_maxp));
				}
				else if (SDL_strcmp(str_tag, "hhea") == 0)
				{
					read_const_ttf_table(fp, &record, &hhea, sizeof(t_hhea));
				}
				else if (SDL_strcmp(str_tag, "hmtx") == 0)
				{
					printf("Reading '%s' table\n", str_tag);
					long current_offset = ftell(fp);
					fseek(fp, to_le32(record.offset), SEEK_SET);

					for (int i = 0; i < to_le16(hhea.num_of_long_hor_metrics); i++)
					{
						h_metric metric = {
							to_le16(fgetwc(fp)),
							to_le16(fgetwc(fp))
						};
						array_push(&(hmtx.metrics), &(hmtx.metrics_size), sizeof(h_metric), &metric);
					}

					int loop_count = to_le16(maxp.num_glyphs) - to_le16(hhea.num_of_long_hor_metrics);
					for (int i = 0; i < loop_count; i++)
					{
						FWORD left_side_bearing;
						fread_s(&left_side_bearing, sizeof(FWORD), sizeof(FWORD), 1, fp);
						array_push(&(hmtx.left_side_bearing), &(hmtx.left_side_bearing_size), sizeof(FWORD), &left_side_bearing);
					}

					fseek(fp, current_offset, SEEK_SET);
				}

				//start looking for the next table in table_order
				break;
			}
		}

		fseek(fp, table_records_offset, SEEK_SET);
	}

	fclose(fp);
}


void main_loop()
{
	SDL_Window* window = SDL_CreateWindow("TextRenderer", 100, 100, 800, 600, 0);
	SDL_Surface* bitmap = SDL_GetWindowSurface(window);
	unsigned char* pixels = bitmap->pixels;
	const size_t bitmap_size = (bitmap->w * bitmap->h) * bitmap->format->BytesPerPixel;

	SDL_Event e;
	while (running)
	{
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_QUIT)
			{
				running = false;
			}
		}

		for (int i = 0; i < bitmap_size; i += bitmap->format->BytesPerPixel)
		{
			//BGR
			pixels[i] = 80;
			pixels[i + 1] = 80;
			pixels[i + 2] = 80;
		}

		SDL_UpdateWindowSurface(window);
	}
}