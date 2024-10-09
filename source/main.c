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
#define Offset16 uint16_t
//for Offset24 we'll just have 1 Offset16 and 1 Offset8
#define Offset32 uint32_t
//#define Version16Dot16 int32_t //just use 2 seperate uint16_t too

//from big endian to little endian
uint32_t to_leu32(uint32_t value) {
	return ((value & 0xFF000000) >> 24) |
    ((value & 0x00FF0000) >> 8) |
    ((value & 0x0000FF00) << 8) |
    ((value & 0x000000FF) << 24);
}

//from big endian to little endian
uint16_t to_leu16(uint16_t value) {
	return (value >> 8) | (value << 8);
}

int16_t to_le16(int16_t value) {
	return (value << 8) | ((value >> 8) & 0xFF);
}

int32_t to_le32(int32_t value) {
	uint32_t uvalue = (uint32_t)value;
	uvalue = ((uvalue >> 24) & 0x000000FF) |
    ((uvalue >> 8) & 0x0000FF00) |
    ((uvalue << 8) & 0x00FF0000) |
    ((uvalue << 24) & 0xFF000000);
	return (int32_t)uvalue;
}


typedef struct
{
	UFWORD advance_width;
	FWORD left_side_bearing;
} h_metric;

typedef struct
{
	int16_t number_of_contours;
	int16_t xmin;
	int16_t ymin;
	int16_t xmax;
	int16_t ymax;
} glyph_header;

typedef struct
{
	uint16_t platform_id;
	uint16_t encoding_id;
	Offset32 offset;
} encoding_record;

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
	void* offsets;
	int offsets_size;
	int offset_size; //either 2 bytes or 4 bytes. Offset16 or Offset32
} t_loca;


typedef struct
{
	glyph_header* glyphs;
	int glyphs_size;
} t_glyf;


typedef struct
{
	uint16_t version;
	uint16_t num_tables;
	encoding_record* e_records;
    
	//NOTT PART OF SPEC
	int e_records_size;
} t_cmap;


typedef struct
{
    uint16_t end_code;
    uint16_t start_code;
    
    int16_t id_delta;
    uint16_t id_range_offset;
} format4_segment;


typedef struct
{
    uint16_t format;
    uint16_t length;
    uint16_t language;
    
    uint16_t seg_count; //stored doubled. halved while reading
    
    uint16_t search_range;
    uint16_t entry_selector;
    uint16_t range_shift;
    
    format4_segment* segments;
    int segments_size;
    
    uint16_t* glyph_ids;
} format4;


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

//TODO(omar): add a function pointer to call a specific function that loads a table if needed
static void read_const_ttf_table(FILE* fp, table_record* record, void* table, size_t size)
{
	char str_tag[5];
	memcpy(str_tag, record->tag, 4);
	str_tag[4] = 0;
	printf("Reading '%s' table\n", str_tag);
    
	long current_offset = ftell(fp);
	fseek(fp, to_leu32(record->offset), SEEK_SET);
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
	td.num_tables = to_leu16(td.num_tables);
	printf("NUM TABLES: %hu\n\n", td.num_tables);
    
	const const char* table_order[] =
	{
		"head",
		"maxp",
		"hhea",
		"hmtx",
		"loca",
		"glyf",
		"cmap"
	};
	const int table_count = sizeof(table_order) / sizeof(const char*);
    
	t_head head;
	t_maxp maxp;
	t_hhea hhea;
	t_hmtx hmtx = {0};
	t_loca loca = {0};
	t_glyf glyf = { 0 };
	t_cmap cmap = { 0 };
    
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
			//printf("%s\n", str_tag);
            
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
					fseek(fp, to_leu32(record.offset), SEEK_SET);
                    
					for (int i = 0; i < to_leu16(hhea.num_of_long_hor_metrics); i++)
					{
						FWORD left_side_bearing;
						fread_s(&left_side_bearing, sizeof(FWORD), sizeof(FWORD), 1, fp);
						left_side_bearing = to_le16(left_side_bearing);
						h_metric metric = {
							to_leu16(fgetwc(fp)),
							left_side_bearing
						};
						array_push(&(hmtx.metrics), &(hmtx.metrics_size), sizeof(h_metric), &metric);
					}
                    
					int loop_count = to_leu16(maxp.num_glyphs) - to_leu16(hhea.num_of_long_hor_metrics);
					for (int i = 0; i < loop_count; i++)
					{
						FWORD left_side_bearing;
						fread_s(&left_side_bearing, sizeof(FWORD), sizeof(FWORD), 1, fp);
						left_side_bearing = to_le16(left_side_bearing);
                        
						array_push(&(hmtx.left_side_bearing), &(hmtx.left_side_bearing_size), sizeof(FWORD), &left_side_bearing);
					}
                    
					fseek(fp, current_offset, SEEK_SET);
				}
				else if (SDL_strcmp(str_tag, "loca") == 0)
				{
					printf("Reading '%s' table\n", str_tag);
					long current_offset = ftell(fp);
					fseek(fp, to_leu32(record.offset), SEEK_SET);
                    
					if (to_le16(head.index_to_loc_format) == 0)
					{
						loca.offset_size = 2;
					}
					else
					{
						loca.offset_size = 4;
					}
                    
					for (int i = 0; i < to_leu16(maxp.num_glyphs) + 1; i++)
					{
						if (loca.offset_size == 2)
						{
							Offset16 offset = fgetwc(fp);
							offset = to_leu16(offset);
							array_push(&(loca.offsets), &(loca.offsets_size), loca.offset_size, &offset);
						}
						else
						{
							Offset32 offset;
							fread_s(&offset, sizeof(Offset32), sizeof(Offset32), 1, fp);
							offset = to_leu32(offset);
							array_push(&(loca.offsets), &(loca.offsets_size), loca.offset_size, &offset);
						}
					}
                    
					fseek(fp, current_offset, SEEK_SET);
				}
				else if (SDL_strcmp(str_tag, "glyf") == 0)
				{
					printf("Reading '%s' table\n", str_tag);
					long current_offset = ftell(fp);
					fseek(fp, to_leu32(record.offset), SEEK_SET);
                    
					int loca_count = loca.offsets_size / loca.offset_size;
					for (int i = 0; i < loca_count - 1; i++)
					{
						Offset32 offset;
						if (loca.offset_size == 2)
						{
							offset = ((Offset16*)loca.offsets)[i];
							offset = to_leu16(offset);
                            
							if (head.index_to_loc_format == 0)
							{
								offset *= 2;
							}
						}
						else
						{
							offset = ((Offset32*)loca.offsets)[i];
							offset = to_leu32(offset);
						}
						fseek(fp, record.offset + offset, SEEK_SET);
                        
						glyph_header glyph;
						fread_s(&glyph, sizeof(glyph_header), sizeof(glyph_header), 1, fp);
                        
						array_push(&(glyf.glyphs), &(glyf.glyphs_size), sizeof(glyph_header), &glyph);
					}
					fseek(fp, current_offset, SEEK_SET);
				}
				else if (SDL_strcmp(str_tag, "cmap") == 0)
				{
					printf("Reading '%s' table\n", str_tag);
					long current_offset = ftell(fp);
					fseek(fp, to_leu32(record.offset), SEEK_SET);
                    
					fread_s(&(cmap.version), sizeof(uint16_t), sizeof(uint16_t), 1, fp);
					fread_s(&(cmap.num_tables), sizeof(uint16_t), sizeof(uint16_t), 1, fp);
					//printf("	cmap.version = %u", to_leu16(cmap.version));
                    
					if (cmap.version != 0)
					{
						printf("ERROR: The cmap table version is %u. only version 0 is supported. Loading failed.", cmap.version);
						goto destroy_everything;
					}
					
					for (int i = 0; i < to_leu16(cmap.num_tables); i++)
					{
						encoding_record e_record;
						fread_s(&e_record, sizeof(encoding_record), sizeof(encoding_record), 1, fp);
						array_push(&(cmap.e_records), &(cmap.e_records_size), sizeof(encoding_record), &e_record);
                        
                        printf("    Platform ID: %u\n",
                               to_leu16(e_record.platform_id));
                        
                        printf("    Encoding ID: %u\n",
                               to_leu16(e_record.encoding_id));
                        
                        printf("    Offset: %u\n", to_leu32(e_record.offset));
					}
                    printf("\n");
                    
                    Offset32 selected_offset = -1;
                    for (int i = 0; i < to_leu16(cmap.num_tables); i++)
                    {
                        const encoding_record* e_record = cmap.e_records + i;
                        uint16_t platform_id = to_leu16(e_record->platform_id);
                        uint16_t encoding_id = to_leu16(e_record->encoding_id);
                        
                        //TODO(omar): use an enum if we add even 1 more platform
                        bool is_windows_platform = false;
                        bool is_unicode_platform = false;
                        
                        switch (platform_id)
                        {
                            case 3:
                            {
                                switch (encoding_id)
                                {
                                    case 0:
                                    case 1:
                                    case 10:
                                    {
                                        is_windows_platform = true;
                                    } break;
                                    
                                    default: break;
                                };
                            } break;
                            
                            case 0:
                            {
                                if ( (encoding_id >= 0) &&
                                    (encoding_id <= 4) )
                                {
                                    is_unicode_platform = true;
                                }
                            } break;
                            
                            default: break;
                        };
                        
                        if (is_windows_platform || is_unicode_platform)
                        {
                            selected_offset = to_leu32(e_record->offset);
                            break;
                        }
                    }
                    
                    if (selected_offset == -1)
                    {
                        
                        printf("ERROR: The font doesn't contain a recognized platform and encoding. Loading failed.\n");
                        goto destroy_everything;
                    }
                    else
                    {
                        printf("    Selected offset is: %u\n", selected_offset);
                    }
                    
                    uint16_t format_id;
                    fread_s(&format_id, sizeof(uint16_t), sizeof(uint16_t), 1, fp);
                    format_id = to_leu16(format_id);
                    if (format_id != 4)
                    {
                        printf("ERROR: Unsupported format '%u'. Required '4'\n",
                               format_id);
                        goto destroy_everything;
                    }
                    printf("    Format is: %u\n", format_id);
                    
                    format4 format =
                    {
                        format_id,
                        to_leu16(fgetwc(fp)),
                        to_leu16(fgetwc(fp)),
                        to_leu16(fgetwc(fp)),
                        
                        to_leu16(fgetwc(fp)),
                        to_leu16(fgetwc(fp)),
                        to_leu16(fgetwc(fp)),
                        
                        0,
                        0,
                        0
                    };
                    format.seg_count /= 2;
                    
                    format.segments_size = sizeof(format4_segment) * format.seg_count;
                    format.segments = malloc(format.segments_size);
                    
                    for (int i = 0; i < format.seg_count; i++)
                    {
                        format.segments[i].end_code = to_leu16(fgetwc(fp));
                    }
                    
                    fgetwc(fp); //reserved pad
                    
                    for (int i = 0; i < format.seg_count; i++)
                    {
                        format.segments[i].start_code = to_leu16(fgetwc(fp));
                    }
                    
                    for (int i = 0; i < format.seg_count; i++)
                    {
                        format.segments[i].id_delta = to_leu16(fgetwc(fp));
                    }
                    
                    const long id_range_offset_start = ftell(fp);
                    for (int i = 0; i < format.seg_count; i++)
                    {
                        format.segments[i].id_range_offset = to_leu16(fgetwc(fp));
                    }
                    
                    
                    fseek(fp, current_offset, SEEK_SET);
                    goto pass_table;
				}
                
				//start looking for the next table in table_order
				pass_table:
				break;
			}
		}
        
		fseek(fp, table_records_offset, SEEK_SET);
	}
    
    destroy_everything:
	if (cmap.e_records) free(cmap.e_records);
	if (loca.offsets) free(loca.offsets);
	if (glyf.glyphs) free(glyf.glyphs);
	if (hmtx.left_side_bearing) free(hmtx.left_side_bearing);
	if (hmtx.metrics) free(hmtx.metrics);
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