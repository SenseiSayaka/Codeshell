#include "fat12.h"
#include "util.h"
#include "stdlib/stdio.h"
#include "vga.h"
static uint8_t* img = 0;
static Fat12BPB* bpb = 0;
static uint8_t* fat_table = 0;
static Fat12DirEntry* root_dir = 0;
static uint8_t* data_start = 0;
static uint32_t root_start = 0;
static uint8_t fat12_ready = 0;


void fat12_dump_bpb() {
    if (!fat12_ready) { printf("\nfs not ready\n"); return; }
    printf("\n=== BPB ===\n");
    printf("bytes_per_sector:    %d\n", bpb->bytes_per_sector);
    printf("sectors_per_cluster: %d\n", bpb->sectors_per_cluster);
    printf("reserved_sectors:    %d\n", bpb->reserved_sectors);
    printf("fat_count:           %d\n", bpb->fat_count);
    printf("root_entry_count:    %d\n", bpb->root_entry_count);
    printf("sector_per_fat:      %d\n", bpb->sector_per_fat);
    printf("root_start:          %d\n", root_start);
    printf("img addr:            0x%x\n", (uint32_t)img);
    printf("data_start addr:     0x%x\n", (uint32_t)data_start);
}

void fat12_init(void* image)
{
	if (!image) return;
	img = (uint8_t*) image;
	bpb = (Fat12BPB*) img;

	// FAT table immediatly after reserved sectors
	fat_table = img + bpb->reserved_sectors * bpb->bytes_per_sector;

	// root directory - after all FAT tables
	root_start = bpb->reserved_sectors + bpb->fat_count * bpb->sector_per_fat;
	root_dir = (Fat12DirEntry*)(img + root_start * bpb->bytes_per_sector);

	// array of data - after root directory
	uint32_t root_sectors = (bpb->root_entry_count * 32 + bpb->bytes_per_sector - 1)
			/ bpb->bytes_per_sector;
	data_start = img + (root_start + root_sectors) * bpb->bytes_per_sector;
	fat12_ready = 1;

}
//next cluster from FAT12
static uint16_t fat12_next_cluster(uint16_t cluster)
{
	uint32_t offset = cluster + cluster / 2;
	uint16_t val = *(uint16_t*)(fat_table + offset);
	if(cluster & 1)
		return val >> 4;
	else
		return val & 0x0FFF;
}
// convert name 8.3 > FILENAME.EXT
static void format_name(const uint8_t* raw_name, const uint8_t* raw_ext, char* out) {
    int i = 0, j = 0;
    while (i < 8 && raw_name[i] != ' ') out[j++] = raw_name[i++];
    if (raw_ext[0] != ' ') {
        out[j++] = '.';
        int k = 0;
        while (k < 3 && raw_ext[k] != ' ') out[j++] = raw_ext[k++];  // k++ !
    }
    out[j] = '\0';
}
// get pointer to data cluster start
static uint8_t* cluster_to_ptr(uint16_t cluster)
{
	return data_start + (cluster - 2) * bpb->sectors_per_cluster * bpb->bytes_per_sector;
}
int fat12_ls(Fat12File* out, uint32_t max_count)
{
    if (!fat12_ready) return -1;
    
    uint32_t found = 0;
    for(uint32_t i = 0; i < bpb->root_entry_count && found < max_count; i++)
    {
        Fat12DirEntry* e = &root_dir[i];

        if(e->name[0] == 0x00) { break; }
        if(e->name[0] == 0xE5) continue;
        if(e->attributes & ATTR_VOLUME_ID) continue;

        
        format_name(e->name, e->ext, out[found].name);

		print("\n");        
        print(out[found].name);
        print("\n");

        out[found].size          = e->file_size;
        out[found].first_cluster = e->first_cluster;
        out[found].is_dir        = (e->attributes & ATTR_DIRECTORY) ? 1 : 0;
        found++;
    }
    
    return found;
}
int fat12_read(const char* filename, uint8_t* buf, uint32_t buf_size)
{
    if (!fat12_ready) return -1;
   

    for(uint32_t i = 0; i < bpb->root_entry_count; i++)
    {
        Fat12DirEntry* e = &root_dir[i];
        if(e->name[0] == 0x00) { break; }
        if(e->name[0] == 0xE5) continue;
        if(e->attributes & ATTR_VOLUME_ID) continue;

        char name[13];
        format_name(e->name, e->ext, name);
        print("\n");
        print("\n");

        if(k_strcmp(name, filename) != 0) continue;

        

        uint32_t written = 0;
        uint16_t cluster = e->first_cluster;
        uint32_t cluster_size = bpb->sectors_per_cluster * bpb->bytes_per_sector;
        uint32_t iter = 0;

        while (cluster >= 2 && cluster < 0xFF8 && written < buf_size) {
            if (iter++ > 4096) {
                printf("[fat12_read] loop detected at iter %d cluster=%d\n", iter, cluster);
                break;
            }
            

            uint8_t* src = cluster_to_ptr(cluster);
            uint32_t copy = cluster_size;
            if(written + copy > buf_size) copy = buf_size - written;

            uint8_t* dst = buf + written;
            for(uint32_t b = 0; b < copy; b++) dst[b] = src[b];
            written += copy;

            cluster = fat12_next_cluster(cluster);
        }
        
        return (int)written;
    }
    return -1;
}

























