/*typedef struct SceModule2
{
   struct SceModule   *next; // 0
   u16               attribute; // 4
   u8               version[2]; // 6
   char            modname[27]; // 8
   char            terminal; // 0x23
   char            mod_state;   // 0x24
    char            unk1;    // 0x25
   char            unk2[2]; // 0x26
   u32               unk3;   // 0x28
   SceUID            modid; // 0x2C
   u32               unk4; // 0x30
   SceUID            mem_id; // 0x34
   u32               mpid_text;   // 0x38
   u32               mpid_data; // 0x3C
   void *            ent_top; // 0x40
   unsigned int      ent_size; // 0x44
   void *            stub_top; // 0x48
   u32               stub_size; // 0x4C
   u32               entry_addr_; // 0x50
   u32               unk5[4]; // 0x54
   u32               entry_addr; // 0x64
   u32               gp_value; // 0x68
   u32               text_addr; // 0x6C
   u32               text_size; // 0x70
   u32               data_size;   // 0x74
   u32               bss_size; // 0x78
   u32               nsegment; // 0x7C
   u32               segmentaddr[4]; // 0x80
   u32               segmentsize[4]; // 0x90
} SceModule2;*/

void initExceptionHandler();


