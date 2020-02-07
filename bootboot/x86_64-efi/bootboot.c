/*
 * x86_64-efi/bootboot.c
 *
 * Copyright (C) 2017 - 2020 bzt (bztsrc@gitlab)
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * This file is part of the BOOTBOOT Protocol package.
 * @brief Booting code for EFI
 *
 */

// DEBUG already defined in efidebug.h

#define BBDEBUG 1
//#define GOP_DEBUG BBDEBUG

#if BBDEBUG
#define DBG(fmt, ...) do{Print(fmt,__VA_ARGS__); }while(0);
#else
#define DBG(fmt, ...)
#endif

// get UEFI functions and environment
#include <efi.h>
#include <efilib.h>
#include <eficon.h>
#include <efiprot.h>
#include <efigpt.h>
// get BOOTBOOT specific stuff
#include "../bootboot.h"
#include "tinf.h"
// comment out this include if you don't want FS/Z support
//#include "../../osZ/include/osZ/fsZ.h"

/*** ELF64 defines and structs ***/
#define ELFMAG      "\177ELF"
#define SELFMAG     4
#define EI_CLASS    4       /* File class byte index */
#define ELFCLASS64  2       /* 64-bit objects */
#define EI_DATA     5       /* Data encoding byte index */
#define ELFDATA2LSB 1       /* 2's complement, little endian */
#define PT_LOAD     1       /* Loadable program segment */
#define EM_X86_64   62      /* AMD x86-64 architecture */

typedef struct
{
  unsigned char e_ident[16];/* Magic number and other info */
  UINT16    e_type;         /* Object file type */
  UINT16    e_machine;      /* Architecture */
  UINT32    e_version;      /* Object file version */
  UINT64    e_entry;        /* Entry point virtual address */
  UINT64    e_phoff;        /* Program header table file offset */
  UINT64    e_shoff;        /* Section header table file offset */
  UINT32    e_flags;        /* Processor-specific flags */
  UINT16    e_ehsize;       /* ELF header size in bytes */
  UINT16    e_phentsize;    /* Program header table entry size */
  UINT16    e_phnum;        /* Program header table entry count */
  UINT16    e_shentsize;    /* Section header table entry size */
  UINT16    e_shnum;        /* Section header table entry count */
  UINT16    e_shstrndx;     /* Section header string table index */
} Elf64_Ehdr;

typedef struct
{
  UINT32    p_type;         /* Segment type */
  UINT32    p_flags;        /* Segment flags */
  UINT64    p_offset;       /* Segment file offset */
  UINT64    p_vaddr;        /* Segment virtual address */
  UINT64    p_paddr;        /* Segment physical address */
  UINT64    p_filesz;       /* Segment size in file */
  UINT64    p_memsz;        /* Segment size in memory */
  UINT64    p_align;        /* Segment alignment */
} Elf64_Phdr;

/*** PE32+ defines and structs ***/
#define MZ_MAGIC                    0x5a4d      /* "MZ" */
#define PE_MAGIC                    0x00004550  /* "PE\0\0" */
#define IMAGE_FILE_MACHINE_AMD64    0x8664      /* AMD x86_64 architecture */
#define PE_OPT_MAGIC_PE32PLUS       0x020b      /* PE32+ format */
typedef struct
{
  UINT16 magic;         /* MZ magic */
  UINT16 reserved[29];  /* reserved */
  UINT32 peaddr;        /* address of pe header */
} mz_hdr;

typedef struct {
  UINT32 magic;         /* PE magic */
  UINT16 machine;       /* machine type */
  UINT16 sections;      /* number of sections */
  UINT32 timestamp;     /* time_t */
  UINT32 sym_table;     /* symbol table offset */
  UINT32 symbols;       /* number of symbols */
  UINT16 opt_hdr_size;  /* size of optional header */
  UINT16 flags;         /* flags */
  UINT16 file_type;     /* file type, PE32PLUS magic */
  UINT8  ld_major;      /* linker major version */
  UINT8  ld_minor;      /* linker minor version */
  UINT32 text_size;     /* size of text section(s) */
  UINT32 data_size;     /* size of data section(s) */
  UINT32 bss_size;      /* size of bss section(s) */
  INT32 entry_point;    /* file offset of entry point */
  INT32 code_base;      /* relative code addr in ram */
} pe_hdr;

/*** EFI defines and structs ***/
struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
struct EFI_FILE_PROTOCOL;

#ifndef EFI_MP_SERVICES_PROTOCOL_GUID
#define EFI_MP_SERVICES_PROTOCOL_GUID \
  { 0x3fdda605, 0xa76e, 0x4f46, {0xad, 0x29, 0x12, 0xf4, 0x53, 0x1b, 0x3d, 0x08} }
typedef struct _EFI_MP_SERVICES_PROTOCOL EFI_MP_SERVICES_PROTOCOL;

#define PROCESSOR_AS_BSP_BIT         0x00000001

typedef struct {
  UINT64                     ProcessorId;
  UINT32                     StatusFlag;
} EFI_PROCESSOR_INFORMATION;

typedef
EFI_STATUS
(EFIAPI *EFI_MP_SERVICES_DUMMY)(
  IN  EFI_MP_SERVICES_PROTOCOL  *This
  );

typedef
VOID
(EFIAPI *EFI_AP_PROCEDURE)(
  IN OUT VOID *Buffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MP_SERVICES_GET_NUMBER_OF_PROCESSORS)(
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  OUT UINTN                     *NumberOfProcessors,
  OUT UINTN                     *NumberOfEnabledProcessors
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MP_SERVICES_GET_PROCESSOR_INFO)(
  IN  EFI_MP_SERVICES_PROTOCOL   *This,
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer
  );

typedef
EFI_STATUS
(EFIAPI *EFI_MP_SERVICES_STARTUP_THIS_AP)(
  IN  EFI_MP_SERVICES_PROTOCOL  *This,
  IN  EFI_AP_PROCEDURE          Procedure,
  IN  UINTN                     ProcessorNumber,
  IN  EFI_EVENT                 WaitEvent               OPTIONAL,
  IN  UINTN                     TimeoutInMicroseconds,
  IN  VOID                      *ProcedureArgument      OPTIONAL,
  OUT BOOLEAN                   *Finished               OPTIONAL
  );

struct _EFI_MP_SERVICES_PROTOCOL {
  EFI_MP_SERVICES_GET_NUMBER_OF_PROCESSORS  GetNumberOfProcessors;
  EFI_MP_SERVICES_GET_PROCESSOR_INFO        GetProcessorInfo;
  EFI_MP_SERVICES_DUMMY                     StartupAllAPs;
  EFI_MP_SERVICES_STARTUP_THIS_AP           StartupThisAP;
  EFI_MP_SERVICES_DUMMY                     SwitchBSP;
  EFI_MP_SERVICES_DUMMY                     EnableDisableAP;
  EFI_MP_SERVICES_DUMMY                     WhoAmI;
};
#endif

typedef
EFI_STATUS
(EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
  IN struct EFI_SIMPLE_FILE_SYSTEM_PROTOCOL    *This,
  OUT struct EFI_FILE_PROTOCOL                 **Root
  );

/* Intel EFI headers has simple file protocol, but not GNU EFI */
#ifndef EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION
typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  UINT64                                      Revision;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef struct _EFI_FILE_PROTOCOL {
  UINT64                Revision;
  EFI_FILE_OPEN         Open;
  EFI_FILE_CLOSE        Close;
  EFI_FILE_DELETE       Delete;
  EFI_FILE_READ         Read;
  EFI_FILE_WRITE        Write;
  EFI_FILE_GET_POSITION GetPosition;
  EFI_FILE_SET_POSITION SetPosition;
  EFI_FILE_GET_INFO     GetInfo;
  EFI_FILE_SET_INFO     SetInfo;
  EFI_FILE_FLUSH        Flush;
} EFI_FILE_PROTOCOL;
#endif

#ifndef EFI_PCI_OPTION_ROM_TABLE_GUID
#define EFI_PCI_OPTION_ROM_TABLE_GUID \
  { 0x7462660f, 0x1cbd, 0x48da, {0xad, 0x11, 0x91, 0x71, 0x79, 0x13, 0x83, 0x1c} }
typedef struct {
  EFI_PHYSICAL_ADDRESS   RomAddress;
  EFI_MEMORY_TYPE        MemoryType;
  UINT32                 RomLength;
  UINT32                 Seg;
  UINT8                  Bus;
  UINT8                  Dev;
  UINT8                  Func;
  BOOLEAN                ExecutedLegacyBiosImage;
  BOOLEAN                DontLoadEfiRom;
} EFI_PCI_OPTION_ROM_DESCRIPTOR;

typedef struct {
  UINT64                         PciOptionRomCount;
  EFI_PCI_OPTION_ROM_DESCRIPTOR   *PciOptionRomDescriptors;
} EFI_PCI_OPTION_ROM_TABLE;
#endif

/*** other defines and structs ***/
typedef struct {
    UINT8 magic[8];
    UINT8 chksum;
    CHAR8 oemid[6];
    UINT8 revision;
    UINT32 rsdt;
    UINT32 length;
    UINT64 xsdt;
    UINT32 echksum;
} __attribute__((packed)) ACPI_RSDPTR;

#define PAGESIZE 4096

/**
 * return type for fs drivers
 */
typedef struct {
    UINT8 *ptr;
    UINTN size;
} file_t;

/*** common variables ***/
file_t env;         // environment file descriptor
file_t initrd;      // initrd file descriptor
file_t core;        // kernel file descriptor
BOOTBOOT *bootboot; // the BOOTBOOT structure
UINT64 *paging;     // paging table for MMU
UINT64 entrypoint;  // kernel entry point
EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *Volume;
EFI_FILE_HANDLE                 RootDir;
EFI_FILE_PROTOCOL               *Root;
SIMPLE_INPUT_INTERFACE          *CI;
unsigned char *kne, bsp_done=0;

// default environment variables. M$ states that 1024x768 must be supported
int reqwidth = 1024, reqheight = 768;
char *kernelname="sys/core";

// alternative environment name
char *cfgname="sys/config";

#ifdef _FS_Z_H_
/**
 * SHA-256
 */
typedef struct {
   UINT8 d[64];
   UINT32 l;
   UINT32 b[2];
   UINT32 s[8];
} SHA256_CTX;
#define SHA_ADD(a,b,c) if(a>0xffffffff-(c))b++;a+=c;
#define SHA_ROTL(a,b) (((a)<<(b))|((a)>>(32-(b))))
#define SHA_ROTR(a,b) (((a)>>(b))|((a)<<(32-(b))))
#define SHA_CH(x,y,z) (((x)&(y))^(~(x)&(z)))
#define SHA_MAJ(x,y,z) (((x)&(y))^((x)&(z))^((y)&(z)))
#define SHA_EP0(x) (SHA_ROTR(x,2)^SHA_ROTR(x,13)^SHA_ROTR(x,22))
#define SHA_EP1(x) (SHA_ROTR(x,6)^SHA_ROTR(x,11)^SHA_ROTR(x,25))
#define SHA_SIG0(x) (SHA_ROTR(x,7)^SHA_ROTR(x,18)^((x)>>3))
#define SHA_SIG1(x) (SHA_ROTR(x,17)^SHA_ROTR(x,19)^((x)>>10))
static UINT32 sha256_k[64]={
   0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
   0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
   0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
   0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
   0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
   0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
   0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
   0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
void sha256_t(SHA256_CTX *ctx)
{
   UINT32 a,b,c,d,e,f,g,h,i,j,t1,t2,m[64];
   for(i=0,j=0;i<16;i++,j+=4) m[i]=(ctx->d[j]<<24)|(ctx->d[j+1]<<16)|(ctx->d[j+2]<<8)|(ctx->d[j+3]);
   for(;i<64;i++) m[i]=SHA_SIG1(m[i-2])+m[i-7]+SHA_SIG0(m[i-15])+m[i-16];
   a=ctx->s[0];b=ctx->s[1];c=ctx->s[2];d=ctx->s[3];
   e=ctx->s[4];f=ctx->s[5];g=ctx->s[6];h=ctx->s[7];
   for(i=0;i<64;i++) {
       t1=h+SHA_EP1(e)+SHA_CH(e,f,g)+sha256_k[i]+m[i];
       t2=SHA_EP0(a)+SHA_MAJ(a,b,c);h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
   ctx->s[0]+=a;ctx->s[1]+=b;ctx->s[2]+=c;ctx->s[3]+=d;
   ctx->s[4]+=e;ctx->s[5]+=f;ctx->s[6]+=g;ctx->s[7]+=h;
}
void SHA256_Init(SHA256_CTX *ctx)
{
    ctx->l=0;ctx->b[0]=ctx->b[1]=0;
    ctx->s[0]=0x6a09e667;ctx->s[1]=0xbb67ae85;ctx->s[2]=0x3c6ef372;ctx->s[3]=0xa54ff53a;
    ctx->s[4]=0x510e527f;ctx->s[5]=0x9b05688c;ctx->s[6]=0x1f83d9ab;ctx->s[7]=0x5be0cd19;
}
void SHA256_Update(SHA256_CTX *ctx, const void *data, int len)
{
    UINT8 *d=(UINT8 *)data;
    for(;len--;d++) {
        ctx->d[ctx->l++]=*d;
        if(ctx->l==64) {sha256_t(ctx);SHA_ADD(ctx->b[0],ctx->b[1],512);ctx->l=0;}
    }
}
void SHA256_Final(unsigned char *h, SHA256_CTX *ctx)
{
    UINT32 i=ctx->l;
    ctx->d[i++]=0x80;
    if(ctx->l<56) {while(i<56) ctx->d[i++]=0x00;}
    else {while(i<64) ctx->d[i++]=0x00;sha256_t(ctx);ZeroMem(ctx->d,56);}
    SHA_ADD(ctx->b[0],ctx->b[1],ctx->l*8);
    ctx->d[63]=ctx->b[0];ctx->d[62]=ctx->b[0]>>8;ctx->d[61]=ctx->b[0]>>16;ctx->d[60]=ctx->b[0]>>24;
    ctx->d[59]=ctx->b[1];ctx->d[58]=ctx->b[1]>>8;ctx->d[57]=ctx->b[1]>>16;ctx->d[56]=ctx->b[1]>>24;
    sha256_t(ctx);
    for(i=0;i<4;i++) {
        h[i]   =(ctx->s[0]>>(24-i*8)); h[i+4] =(ctx->s[1]>>(24-i*8));
        h[i+8] =(ctx->s[2]>>(24-i*8)); h[i+12]=(ctx->s[3]>>(24-i*8));
        h[i+16]=(ctx->s[4]>>(24-i*8)); h[i+20]=(ctx->s[5]>>(24-i*8));
        h[i+24]=(ctx->s[6]>>(24-i*8)); h[i+28]=(ctx->s[7]>>(24-i*8));
    }
}

/**
 * precalculated CRC32c lookup table for polynomial 0x1EDC6F41 (castagnoli-crc)
 */
UINT32 crc32c_lookup[256]={
    0x00000000L, 0xF26B8303L, 0xE13B70F7L, 0x1350F3F4L, 0xC79A971FL, 0x35F1141CL, 0x26A1E7E8L, 0xD4CA64EBL,
    0x8AD958CFL, 0x78B2DBCCL, 0x6BE22838L, 0x9989AB3BL, 0x4D43CFD0L, 0xBF284CD3L, 0xAC78BF27L, 0x5E133C24L,
    0x105EC76FL, 0xE235446CL, 0xF165B798L, 0x030E349BL, 0xD7C45070L, 0x25AFD373L, 0x36FF2087L, 0xC494A384L,
    0x9A879FA0L, 0x68EC1CA3L, 0x7BBCEF57L, 0x89D76C54L, 0x5D1D08BFL, 0xAF768BBCL, 0xBC267848L, 0x4E4DFB4BL,
    0x20BD8EDEL, 0xD2D60DDDL, 0xC186FE29L, 0x33ED7D2AL, 0xE72719C1L, 0x154C9AC2L, 0x061C6936L, 0xF477EA35L,
    0xAA64D611L, 0x580F5512L, 0x4B5FA6E6L, 0xB93425E5L, 0x6DFE410EL, 0x9F95C20DL, 0x8CC531F9L, 0x7EAEB2FAL,
    0x30E349B1L, 0xC288CAB2L, 0xD1D83946L, 0x23B3BA45L, 0xF779DEAEL, 0x05125DADL, 0x1642AE59L, 0xE4292D5AL,
    0xBA3A117EL, 0x4851927DL, 0x5B016189L, 0xA96AE28AL, 0x7DA08661L, 0x8FCB0562L, 0x9C9BF696L, 0x6EF07595L,
    0x417B1DBCL, 0xB3109EBFL, 0xA0406D4BL, 0x522BEE48L, 0x86E18AA3L, 0x748A09A0L, 0x67DAFA54L, 0x95B17957L,
    0xCBA24573L, 0x39C9C670L, 0x2A993584L, 0xD8F2B687L, 0x0C38D26CL, 0xFE53516FL, 0xED03A29BL, 0x1F682198L,
    0x5125DAD3L, 0xA34E59D0L, 0xB01EAA24L, 0x42752927L, 0x96BF4DCCL, 0x64D4CECFL, 0x77843D3BL, 0x85EFBE38L,
    0xDBFC821CL, 0x2997011FL, 0x3AC7F2EBL, 0xC8AC71E8L, 0x1C661503L, 0xEE0D9600L, 0xFD5D65F4L, 0x0F36E6F7L,
    0x61C69362L, 0x93AD1061L, 0x80FDE395L, 0x72966096L, 0xA65C047DL, 0x5437877EL, 0x4767748AL, 0xB50CF789L,
    0xEB1FCBADL, 0x197448AEL, 0x0A24BB5AL, 0xF84F3859L, 0x2C855CB2L, 0xDEEEDFB1L, 0xCDBE2C45L, 0x3FD5AF46L,
    0x7198540DL, 0x83F3D70EL, 0x90A324FAL, 0x62C8A7F9L, 0xB602C312L, 0x44694011L, 0x5739B3E5L, 0xA55230E6L,
    0xFB410CC2L, 0x092A8FC1L, 0x1A7A7C35L, 0xE811FF36L, 0x3CDB9BDDL, 0xCEB018DEL, 0xDDE0EB2AL, 0x2F8B6829L,
    0x82F63B78L, 0x709DB87BL, 0x63CD4B8FL, 0x91A6C88CL, 0x456CAC67L, 0xB7072F64L, 0xA457DC90L, 0x563C5F93L,
    0x082F63B7L, 0xFA44E0B4L, 0xE9141340L, 0x1B7F9043L, 0xCFB5F4A8L, 0x3DDE77ABL, 0x2E8E845FL, 0xDCE5075CL,
    0x92A8FC17L, 0x60C37F14L, 0x73938CE0L, 0x81F80FE3L, 0x55326B08L, 0xA759E80BL, 0xB4091BFFL, 0x466298FCL,
    0x1871A4D8L, 0xEA1A27DBL, 0xF94AD42FL, 0x0B21572CL, 0xDFEB33C7L, 0x2D80B0C4L, 0x3ED04330L, 0xCCBBC033L,
    0xA24BB5A6L, 0x502036A5L, 0x4370C551L, 0xB11B4652L, 0x65D122B9L, 0x97BAA1BAL, 0x84EA524EL, 0x7681D14DL,
    0x2892ED69L, 0xDAF96E6AL, 0xC9A99D9EL, 0x3BC21E9DL, 0xEF087A76L, 0x1D63F975L, 0x0E330A81L, 0xFC588982L,
    0xB21572C9L, 0x407EF1CAL, 0x532E023EL, 0xA145813DL, 0x758FE5D6L, 0x87E466D5L, 0x94B49521L, 0x66DF1622L,
    0x38CC2A06L, 0xCAA7A905L, 0xD9F75AF1L, 0x2B9CD9F2L, 0xFF56BD19L, 0x0D3D3E1AL, 0x1E6DCDEEL, 0xEC064EEDL,
    0xC38D26C4L, 0x31E6A5C7L, 0x22B65633L, 0xD0DDD530L, 0x0417B1DBL, 0xF67C32D8L, 0xE52CC12CL, 0x1747422FL,
    0x49547E0BL, 0xBB3FFD08L, 0xA86F0EFCL, 0x5A048DFFL, 0x8ECEE914L, 0x7CA56A17L, 0x6FF599E3L, 0x9D9E1AE0L,
    0xD3D3E1ABL, 0x21B862A8L, 0x32E8915CL, 0xC083125FL, 0x144976B4L, 0xE622F5B7L, 0xF5720643L, 0x07198540L,
    0x590AB964L, 0xAB613A67L, 0xB831C993L, 0x4A5A4A90L, 0x9E902E7BL, 0x6CFBAD78L, 0x7FAB5E8CL, 0x8DC0DD8FL,
    0xE330A81AL, 0x115B2B19L, 0x020BD8EDL, 0xF0605BEEL, 0x24AA3F05L, 0xD6C1BC06L, 0xC5914FF2L, 0x37FACCF1L,
    0x69E9F0D5L, 0x9B8273D6L, 0x88D28022L, 0x7AB90321L, 0xAE7367CAL, 0x5C18E4C9L, 0x4F48173DL, 0xBD23943EL,
    0xF36E6F75L, 0x0105EC76L, 0x12551F82L, 0xE03E9C81L, 0x34F4F86AL, 0xC69F7B69L, 0xD5CF889DL, 0x27A40B9EL,
    0x79B737BAL, 0x8BDCB4B9L, 0x988C474DL, 0x6AE7C44EL, 0xBE2DA0A5L, 0x4C4623A6L, 0x5F16D052L, 0xAD7D5351L
};
UINT32 crc32_calc(char *start,int length)
{
    UINT32 crc32_val=0;
    while(length--) crc32_val=(crc32_val>>8)^crc32c_lookup[(crc32_val&0xff)^(unsigned char)*start++];
    return crc32_val;
}

/**
 * AES-256-CBC
 */
#define GETU32(pt) (((UINT32)(pt)[0]<<24)^((UINT32)(pt)[1]<<16)^((UINT32)(pt)[2]<<8)^((UINT32)(pt)[3]))
#define PUTU32(ct,st) {(ct)[0]=(UINT8)((st)>>24);(ct)[1]=(UINT8)((st)>>16);(ct)[2]=(UINT8)((st)>>8);(ct)[3]=(UINT8)(st);}
static const UINT32 Te0[256]={
    0xc66363a5U,0xf87c7c84U,0xee777799U,0xf67b7b8dU,0xfff2f20dU,0xd66b6bbdU,0xde6f6fb1U,0x91c5c554U,
    0x60303050U,0x02010103U,0xce6767a9U,0x562b2b7dU,0xe7fefe19U,0xb5d7d762U,0x4dababe6U,0xec76769aU,
    0x8fcaca45U,0x1f82829dU,0x89c9c940U,0xfa7d7d87U,0xeffafa15U,0xb25959ebU,0x8e4747c9U,0xfbf0f00bU,
    0x41adadecU,0xb3d4d467U,0x5fa2a2fdU,0x45afafeaU,0x239c9cbfU,0x53a4a4f7U,0xe4727296U,0x9bc0c05bU,
    0x75b7b7c2U,0xe1fdfd1cU,0x3d9393aeU,0x4c26266aU,0x6c36365aU,0x7e3f3f41U,0xf5f7f702U,0x83cccc4fU,
    0x6834345cU,0x51a5a5f4U,0xd1e5e534U,0xf9f1f108U,0xe2717193U,0xabd8d873U,0x62313153U,0x2a15153fU,
    0x0804040cU,0x95c7c752U,0x46232365U,0x9dc3c35eU,0x30181828U,0x379696a1U,0x0a05050fU,0x2f9a9ab5U,
    0x0e070709U,0x24121236U,0x1b80809bU,0xdfe2e23dU,0xcdebeb26U,0x4e272769U,0x7fb2b2cdU,0xea75759fU,
    0x1209091bU,0x1d83839eU,0x582c2c74U,0x341a1a2eU,0x361b1b2dU,0xdc6e6eb2U,0xb45a5aeeU,0x5ba0a0fbU,
    0xa45252f6U,0x763b3b4dU,0xb7d6d661U,0x7db3b3ceU,0x5229297bU,0xdde3e33eU,0x5e2f2f71U,0x13848497U,
    0xa65353f5U,0xb9d1d168U,0x00000000U,0xc1eded2cU,0x40202060U,0xe3fcfc1fU,0x79b1b1c8U,0xb65b5bedU,
    0xd46a6abeU,0x8dcbcb46U,0x67bebed9U,0x7239394bU,0x944a4adeU,0x984c4cd4U,0xb05858e8U,0x85cfcf4aU,
    0xbbd0d06bU,0xc5efef2aU,0x4faaaae5U,0xedfbfb16U,0x864343c5U,0x9a4d4dd7U,0x66333355U,0x11858594U,
    0x8a4545cfU,0xe9f9f910U,0x04020206U,0xfe7f7f81U,0xa05050f0U,0x783c3c44U,0x259f9fbaU,0x4ba8a8e3U,
    0xa25151f3U,0x5da3a3feU,0x804040c0U,0x058f8f8aU,0x3f9292adU,0x219d9dbcU,0x70383848U,0xf1f5f504U,
    0x63bcbcdfU,0x77b6b6c1U,0xafdada75U,0x42212163U,0x20101030U,0xe5ffff1aU,0xfdf3f30eU,0xbfd2d26dU,
    0x81cdcd4cU,0x180c0c14U,0x26131335U,0xc3ecec2fU,0xbe5f5fe1U,0x359797a2U,0x884444ccU,0x2e171739U,
    0x93c4c457U,0x55a7a7f2U,0xfc7e7e82U,0x7a3d3d47U,0xc86464acU,0xba5d5de7U,0x3219192bU,0xe6737395U,
    0xc06060a0U,0x19818198U,0x9e4f4fd1U,0xa3dcdc7fU,0x44222266U,0x542a2a7eU,0x3b9090abU,0x0b888883U,
    0x8c4646caU,0xc7eeee29U,0x6bb8b8d3U,0x2814143cU,0xa7dede79U,0xbc5e5ee2U,0x160b0b1dU,0xaddbdb76U,
    0xdbe0e03bU,0x64323256U,0x743a3a4eU,0x140a0a1eU,0x924949dbU,0x0c06060aU,0x4824246cU,0xb85c5ce4U,
    0x9fc2c25dU,0xbdd3d36eU,0x43acacefU,0xc46262a6U,0x399191a8U,0x319595a4U,0xd3e4e437U,0xf279798bU,
    0xd5e7e732U,0x8bc8c843U,0x6e373759U,0xda6d6db7U,0x018d8d8cU,0xb1d5d564U,0x9c4e4ed2U,0x49a9a9e0U,
    0xd86c6cb4U,0xac5656faU,0xf3f4f407U,0xcfeaea25U,0xca6565afU,0xf47a7a8eU,0x47aeaee9U,0x10080818U,
    0x6fbabad5U,0xf0787888U,0x4a25256fU,0x5c2e2e72U,0x381c1c24U,0x57a6a6f1U,0x73b4b4c7U,0x97c6c651U,
    0xcbe8e823U,0xa1dddd7cU,0xe874749cU,0x3e1f1f21U,0x964b4bddU,0x61bdbddcU,0x0d8b8b86U,0x0f8a8a85U,
    0xe0707090U,0x7c3e3e42U,0x71b5b5c4U,0xcc6666aaU,0x904848d8U,0x06030305U,0xf7f6f601U,0x1c0e0e12U,
    0xc26161a3U,0x6a35355fU,0xae5757f9U,0x69b9b9d0U,0x17868691U,0x99c1c158U,0x3a1d1d27U,0x279e9eb9U,
    0xd9e1e138U,0xebf8f813U,0x2b9898b3U,0x22111133U,0xd26969bbU,0xa9d9d970U,0x078e8e89U,0x339494a7U,
    0x2d9b9bb6U,0x3c1e1e22U,0x15878792U,0xc9e9e920U,0x87cece49U,0xaa5555ffU,0x50282878U,0xa5dfdf7aU,
    0x038c8c8fU,0x59a1a1f8U,0x09898980U,0x1a0d0d17U,0x65bfbfdaU,0xd7e6e631U,0x844242c6U,0xd06868b8U,
    0x824141c3U,0x299999b0U,0x5a2d2d77U,0x1e0f0f11U,0x7bb0b0cbU,0xa85454fcU,0x6dbbbbd6U,0x2c16163aU,
};
static const UINT32 Te1[256]={
    0xa5c66363U,0x84f87c7cU,0x99ee7777U,0x8df67b7bU,0x0dfff2f2U,0xbdd66b6bU,0xb1de6f6fU,0x5491c5c5U,
    0x50603030U,0x03020101U,0xa9ce6767U,0x7d562b2bU,0x19e7fefeU,0x62b5d7d7U,0xe64dababU,0x9aec7676U,
    0x458fcacaU,0x9d1f8282U,0x4089c9c9U,0x87fa7d7dU,0x15effafaU,0xebb25959U,0xc98e4747U,0x0bfbf0f0U,
    0xec41adadU,0x67b3d4d4U,0xfd5fa2a2U,0xea45afafU,0xbf239c9cU,0xf753a4a4U,0x96e47272U,0x5b9bc0c0U,
    0xc275b7b7U,0x1ce1fdfdU,0xae3d9393U,0x6a4c2626U,0x5a6c3636U,0x417e3f3fU,0x02f5f7f7U,0x4f83ccccU,
    0x5c683434U,0xf451a5a5U,0x34d1e5e5U,0x08f9f1f1U,0x93e27171U,0x73abd8d8U,0x53623131U,0x3f2a1515U,
    0x0c080404U,0x5295c7c7U,0x65462323U,0x5e9dc3c3U,0x28301818U,0xa1379696U,0x0f0a0505U,0xb52f9a9aU,
    0x090e0707U,0x36241212U,0x9b1b8080U,0x3ddfe2e2U,0x26cdebebU,0x694e2727U,0xcd7fb2b2U,0x9fea7575U,
    0x1b120909U,0x9e1d8383U,0x74582c2cU,0x2e341a1aU,0x2d361b1bU,0xb2dc6e6eU,0xeeb45a5aU,0xfb5ba0a0U,
    0xf6a45252U,0x4d763b3bU,0x61b7d6d6U,0xce7db3b3U,0x7b522929U,0x3edde3e3U,0x715e2f2fU,0x97138484U,
    0xf5a65353U,0x68b9d1d1U,0x00000000U,0x2cc1ededU,0x60402020U,0x1fe3fcfcU,0xc879b1b1U,0xedb65b5bU,
    0xbed46a6aU,0x468dcbcbU,0xd967bebeU,0x4b723939U,0xde944a4aU,0xd4984c4cU,0xe8b05858U,0x4a85cfcfU,
    0x6bbbd0d0U,0x2ac5efefU,0xe54faaaaU,0x16edfbfbU,0xc5864343U,0xd79a4d4dU,0x55663333U,0x94118585U,
    0xcf8a4545U,0x10e9f9f9U,0x06040202U,0x81fe7f7fU,0xf0a05050U,0x44783c3cU,0xba259f9fU,0xe34ba8a8U,
    0xf3a25151U,0xfe5da3a3U,0xc0804040U,0x8a058f8fU,0xad3f9292U,0xbc219d9dU,0x48703838U,0x04f1f5f5U,
    0xdf63bcbcU,0xc177b6b6U,0x75afdadaU,0x63422121U,0x30201010U,0x1ae5ffffU,0x0efdf3f3U,0x6dbfd2d2U,
    0x4c81cdcdU,0x14180c0cU,0x35261313U,0x2fc3ececU,0xe1be5f5fU,0xa2359797U,0xcc884444U,0x392e1717U,
    0x5793c4c4U,0xf255a7a7U,0x82fc7e7eU,0x477a3d3dU,0xacc86464U,0xe7ba5d5dU,0x2b321919U,0x95e67373U,
    0xa0c06060U,0x98198181U,0xd19e4f4fU,0x7fa3dcdcU,0x66442222U,0x7e542a2aU,0xab3b9090U,0x830b8888U,
    0xca8c4646U,0x29c7eeeeU,0xd36bb8b8U,0x3c281414U,0x79a7dedeU,0xe2bc5e5eU,0x1d160b0bU,0x76addbdbU,
    0x3bdbe0e0U,0x56643232U,0x4e743a3aU,0x1e140a0aU,0xdb924949U,0x0a0c0606U,0x6c482424U,0xe4b85c5cU,
    0x5d9fc2c2U,0x6ebdd3d3U,0xef43acacU,0xa6c46262U,0xa8399191U,0xa4319595U,0x37d3e4e4U,0x8bf27979U,
    0x32d5e7e7U,0x438bc8c8U,0x596e3737U,0xb7da6d6dU,0x8c018d8dU,0x64b1d5d5U,0xd29c4e4eU,0xe049a9a9U,
    0xb4d86c6cU,0xfaac5656U,0x07f3f4f4U,0x25cfeaeaU,0xafca6565U,0x8ef47a7aU,0xe947aeaeU,0x18100808U,
    0xd56fbabaU,0x88f07878U,0x6f4a2525U,0x725c2e2eU,0x24381c1cU,0xf157a6a6U,0xc773b4b4U,0x5197c6c6U,
    0x23cbe8e8U,0x7ca1ddddU,0x9ce87474U,0x213e1f1fU,0xdd964b4bU,0xdc61bdbdU,0x860d8b8bU,0x850f8a8aU,
    0x90e07070U,0x427c3e3eU,0xc471b5b5U,0xaacc6666U,0xd8904848U,0x05060303U,0x01f7f6f6U,0x121c0e0eU,
    0xa3c26161U,0x5f6a3535U,0xf9ae5757U,0xd069b9b9U,0x91178686U,0x5899c1c1U,0x273a1d1dU,0xb9279e9eU,
    0x38d9e1e1U,0x13ebf8f8U,0xb32b9898U,0x33221111U,0xbbd26969U,0x70a9d9d9U,0x89078e8eU,0xa7339494U,
    0xb62d9b9bU,0x223c1e1eU,0x92158787U,0x20c9e9e9U,0x4987ceceU,0xffaa5555U,0x78502828U,0x7aa5dfdfU,
    0x8f038c8cU,0xf859a1a1U,0x80098989U,0x171a0d0dU,0xda65bfbfU,0x31d7e6e6U,0xc6844242U,0xb8d06868U,
    0xc3824141U,0xb0299999U,0x775a2d2dU,0x111e0f0fU,0xcb7bb0b0U,0xfca85454U,0xd66dbbbbU,0x3a2c1616U,
};
static const UINT32 Te2[256]={
    0x63a5c663U,0x7c84f87cU,0x7799ee77U,0x7b8df67bU,0xf20dfff2U,0x6bbdd66bU,0x6fb1de6fU,0xc55491c5U,
    0x30506030U,0x01030201U,0x67a9ce67U,0x2b7d562bU,0xfe19e7feU,0xd762b5d7U,0xabe64dabU,0x769aec76U,
    0xca458fcaU,0x829d1f82U,0xc94089c9U,0x7d87fa7dU,0xfa15effaU,0x59ebb259U,0x47c98e47U,0xf00bfbf0U,
    0xadec41adU,0xd467b3d4U,0xa2fd5fa2U,0xafea45afU,0x9cbf239cU,0xa4f753a4U,0x7296e472U,0xc05b9bc0U,
    0xb7c275b7U,0xfd1ce1fdU,0x93ae3d93U,0x266a4c26U,0x365a6c36U,0x3f417e3fU,0xf702f5f7U,0xcc4f83ccU,
    0x345c6834U,0xa5f451a5U,0xe534d1e5U,0xf108f9f1U,0x7193e271U,0xd873abd8U,0x31536231U,0x153f2a15U,
    0x040c0804U,0xc75295c7U,0x23654623U,0xc35e9dc3U,0x18283018U,0x96a13796U,0x050f0a05U,0x9ab52f9aU,
    0x07090e07U,0x12362412U,0x809b1b80U,0xe23ddfe2U,0xeb26cdebU,0x27694e27U,0xb2cd7fb2U,0x759fea75U,
    0x091b1209U,0x839e1d83U,0x2c74582cU,0x1a2e341aU,0x1b2d361bU,0x6eb2dc6eU,0x5aeeb45aU,0xa0fb5ba0U,
    0x52f6a452U,0x3b4d763bU,0xd661b7d6U,0xb3ce7db3U,0x297b5229U,0xe33edde3U,0x2f715e2fU,0x84971384U,
    0x53f5a653U,0xd168b9d1U,0x00000000U,0xed2cc1edU,0x20604020U,0xfc1fe3fcU,0xb1c879b1U,0x5bedb65bU,
    0x6abed46aU,0xcb468dcbU,0xbed967beU,0x394b7239U,0x4ade944aU,0x4cd4984cU,0x58e8b058U,0xcf4a85cfU,
    0xd06bbbd0U,0xef2ac5efU,0xaae54faaU,0xfb16edfbU,0x43c58643U,0x4dd79a4dU,0x33556633U,0x85941185U,
    0x45cf8a45U,0xf910e9f9U,0x02060402U,0x7f81fe7fU,0x50f0a050U,0x3c44783cU,0x9fba259fU,0xa8e34ba8U,
    0x51f3a251U,0xa3fe5da3U,0x40c08040U,0x8f8a058fU,0x92ad3f92U,0x9dbc219dU,0x38487038U,0xf504f1f5U,
    0xbcdf63bcU,0xb6c177b6U,0xda75afdaU,0x21634221U,0x10302010U,0xff1ae5ffU,0xf30efdf3U,0xd26dbfd2U,
    0xcd4c81cdU,0x0c14180cU,0x13352613U,0xec2fc3ecU,0x5fe1be5fU,0x97a23597U,0x44cc8844U,0x17392e17U,
    0xc45793c4U,0xa7f255a7U,0x7e82fc7eU,0x3d477a3dU,0x64acc864U,0x5de7ba5dU,0x192b3219U,0x7395e673U,
    0x60a0c060U,0x81981981U,0x4fd19e4fU,0xdc7fa3dcU,0x22664422U,0x2a7e542aU,0x90ab3b90U,0x88830b88U,
    0x46ca8c46U,0xee29c7eeU,0xb8d36bb8U,0x143c2814U,0xde79a7deU,0x5ee2bc5eU,0x0b1d160bU,0xdb76addbU,
    0xe03bdbe0U,0x32566432U,0x3a4e743aU,0x0a1e140aU,0x49db9249U,0x060a0c06U,0x246c4824U,0x5ce4b85cU,
    0xc25d9fc2U,0xd36ebdd3U,0xacef43acU,0x62a6c462U,0x91a83991U,0x95a43195U,0xe437d3e4U,0x798bf279U,
    0xe732d5e7U,0xc8438bc8U,0x37596e37U,0x6db7da6dU,0x8d8c018dU,0xd564b1d5U,0x4ed29c4eU,0xa9e049a9U,
    0x6cb4d86cU,0x56faac56U,0xf407f3f4U,0xea25cfeaU,0x65afca65U,0x7a8ef47aU,0xaee947aeU,0x08181008U,
    0xbad56fbaU,0x7888f078U,0x256f4a25U,0x2e725c2eU,0x1c24381cU,0xa6f157a6U,0xb4c773b4U,0xc65197c6U,
    0xe823cbe8U,0xdd7ca1ddU,0x749ce874U,0x1f213e1fU,0x4bdd964bU,0xbddc61bdU,0x8b860d8bU,0x8a850f8aU,
    0x7090e070U,0x3e427c3eU,0xb5c471b5U,0x66aacc66U,0x48d89048U,0x03050603U,0xf601f7f6U,0x0e121c0eU,
    0x61a3c261U,0x355f6a35U,0x57f9ae57U,0xb9d069b9U,0x86911786U,0xc15899c1U,0x1d273a1dU,0x9eb9279eU,
    0xe138d9e1U,0xf813ebf8U,0x98b32b98U,0x11332211U,0x69bbd269U,0xd970a9d9U,0x8e89078eU,0x94a73394U,
    0x9bb62d9bU,0x1e223c1eU,0x87921587U,0xe920c9e9U,0xce4987ceU,0x55ffaa55U,0x28785028U,0xdf7aa5dfU,
    0x8c8f038cU,0xa1f859a1U,0x89800989U,0x0d171a0dU,0xbfda65bfU,0xe631d7e6U,0x42c68442U,0x68b8d068U,
    0x41c38241U,0x99b02999U,0x2d775a2dU,0x0f111e0fU,0xb0cb7bb0U,0x54fca854U,0xbbd66dbbU,0x163a2c16U,
};
static const UINT32 Te3[256]={
    0x6363a5c6U,0x7c7c84f8U,0x777799eeU,0x7b7b8df6U,0xf2f20dffU,0x6b6bbdd6U,0x6f6fb1deU,0xc5c55491U,
    0x30305060U,0x01010302U,0x6767a9ceU,0x2b2b7d56U,0xfefe19e7U,0xd7d762b5U,0xababe64dU,0x76769aecU,
    0xcaca458fU,0x82829d1fU,0xc9c94089U,0x7d7d87faU,0xfafa15efU,0x5959ebb2U,0x4747c98eU,0xf0f00bfbU,
    0xadadec41U,0xd4d467b3U,0xa2a2fd5fU,0xafafea45U,0x9c9cbf23U,0xa4a4f753U,0x727296e4U,0xc0c05b9bU,
    0xb7b7c275U,0xfdfd1ce1U,0x9393ae3dU,0x26266a4cU,0x36365a6cU,0x3f3f417eU,0xf7f702f5U,0xcccc4f83U,
    0x34345c68U,0xa5a5f451U,0xe5e534d1U,0xf1f108f9U,0x717193e2U,0xd8d873abU,0x31315362U,0x15153f2aU,
    0x04040c08U,0xc7c75295U,0x23236546U,0xc3c35e9dU,0x18182830U,0x9696a137U,0x05050f0aU,0x9a9ab52fU,
    0x0707090eU,0x12123624U,0x80809b1bU,0xe2e23ddfU,0xebeb26cdU,0x2727694eU,0xb2b2cd7fU,0x75759feaU,
    0x09091b12U,0x83839e1dU,0x2c2c7458U,0x1a1a2e34U,0x1b1b2d36U,0x6e6eb2dcU,0x5a5aeeb4U,0xa0a0fb5bU,
    0x5252f6a4U,0x3b3b4d76U,0xd6d661b7U,0xb3b3ce7dU,0x29297b52U,0xe3e33eddU,0x2f2f715eU,0x84849713U,
    0x5353f5a6U,0xd1d168b9U,0x00000000U,0xeded2cc1U,0x20206040U,0xfcfc1fe3U,0xb1b1c879U,0x5b5bedb6U,
    0x6a6abed4U,0xcbcb468dU,0xbebed967U,0x39394b72U,0x4a4ade94U,0x4c4cd498U,0x5858e8b0U,0xcfcf4a85U,
    0xd0d06bbbU,0xefef2ac5U,0xaaaae54fU,0xfbfb16edU,0x4343c586U,0x4d4dd79aU,0x33335566U,0x85859411U,
    0x4545cf8aU,0xf9f910e9U,0x02020604U,0x7f7f81feU,0x5050f0a0U,0x3c3c4478U,0x9f9fba25U,0xa8a8e34bU,
    0x5151f3a2U,0xa3a3fe5dU,0x4040c080U,0x8f8f8a05U,0x9292ad3fU,0x9d9dbc21U,0x38384870U,0xf5f504f1U,
    0xbcbcdf63U,0xb6b6c177U,0xdada75afU,0x21216342U,0x10103020U,0xffff1ae5U,0xf3f30efdU,0xd2d26dbfU,
    0xcdcd4c81U,0x0c0c1418U,0x13133526U,0xecec2fc3U,0x5f5fe1beU,0x9797a235U,0x4444cc88U,0x1717392eU,
    0xc4c45793U,0xa7a7f255U,0x7e7e82fcU,0x3d3d477aU,0x6464acc8U,0x5d5de7baU,0x19192b32U,0x737395e6U,
    0x6060a0c0U,0x81819819U,0x4f4fd19eU,0xdcdc7fa3U,0x22226644U,0x2a2a7e54U,0x9090ab3bU,0x8888830bU,
    0x4646ca8cU,0xeeee29c7U,0xb8b8d36bU,0x14143c28U,0xdede79a7U,0x5e5ee2bcU,0x0b0b1d16U,0xdbdb76adU,
    0xe0e03bdbU,0x32325664U,0x3a3a4e74U,0x0a0a1e14U,0x4949db92U,0x06060a0cU,0x24246c48U,0x5c5ce4b8U,
    0xc2c25d9fU,0xd3d36ebdU,0xacacef43U,0x6262a6c4U,0x9191a839U,0x9595a431U,0xe4e437d3U,0x79798bf2U,
    0xe7e732d5U,0xc8c8438bU,0x3737596eU,0x6d6db7daU,0x8d8d8c01U,0xd5d564b1U,0x4e4ed29cU,0xa9a9e049U,
    0x6c6cb4d8U,0x5656faacU,0xf4f407f3U,0xeaea25cfU,0x6565afcaU,0x7a7a8ef4U,0xaeaee947U,0x08081810U,
    0xbabad56fU,0x787888f0U,0x25256f4aU,0x2e2e725cU,0x1c1c2438U,0xa6a6f157U,0xb4b4c773U,0xc6c65197U,
    0xe8e823cbU,0xdddd7ca1U,0x74749ce8U,0x1f1f213eU,0x4b4bdd96U,0xbdbddc61U,0x8b8b860dU,0x8a8a850fU,
    0x707090e0U,0x3e3e427cU,0xb5b5c471U,0x6666aaccU,0x4848d890U,0x03030506U,0xf6f601f7U,0x0e0e121cU,
    0x6161a3c2U,0x35355f6aU,0x5757f9aeU,0xb9b9d069U,0x86869117U,0xc1c15899U,0x1d1d273aU,0x9e9eb927U,
    0xe1e138d9U,0xf8f813ebU,0x9898b32bU,0x11113322U,0x6969bbd2U,0xd9d970a9U,0x8e8e8907U,0x9494a733U,
    0x9b9bb62dU,0x1e1e223cU,0x87879215U,0xe9e920c9U,0xcece4987U,0x5555ffaaU,0x28287850U,0xdfdf7aa5U,
    0x8c8c8f03U,0xa1a1f859U,0x89898009U,0x0d0d171aU,0xbfbfda65U,0xe6e631d7U,0x4242c684U,0x6868b8d0U,
    0x4141c382U,0x9999b029U,0x2d2d775aU,0x0f0f111eU,0xb0b0cb7bU,0x5454fca8U,0xbbbbd66dU,0x16163a2cU,
};

static const UINT32 Td0[256]={
    0x51f4a750U,0x7e416553U,0x1a17a4c3U,0x3a275e96U,0x3bab6bcbU,0x1f9d45f1U,0xacfa58abU,0x4be30393U,
    0x2030fa55U,0xad766df6U,0x88cc7691U,0xf5024c25U,0x4fe5d7fcU,0xc52acbd7U,0x26354480U,0xb562a38fU,
    0xdeb15a49U,0x25ba1b67U,0x45ea0e98U,0x5dfec0e1U,0xc32f7502U,0x814cf012U,0x8d4697a3U,0x6bd3f9c6U,
    0x038f5fe7U,0x15929c95U,0xbf6d7aebU,0x955259daU,0xd4be832dU,0x587421d3U,0x49e06929U,0x8ec9c844U,
    0x75c2896aU,0xf48e7978U,0x99583e6bU,0x27b971ddU,0xbee14fb6U,0xf088ad17U,0xc920ac66U,0x7dce3ab4U,
    0x63df4a18U,0xe51a3182U,0x97513360U,0x62537f45U,0xb16477e0U,0xbb6bae84U,0xfe81a01cU,0xf9082b94U,
    0x70486858U,0x8f45fd19U,0x94de6c87U,0x527bf8b7U,0xab73d323U,0x724b02e2U,0xe31f8f57U,0x6655ab2aU,
    0xb2eb2807U,0x2fb5c203U,0x86c57b9aU,0xd33708a5U,0x302887f2U,0x23bfa5b2U,0x02036abaU,0xed16825cU,
    0x8acf1c2bU,0xa779b492U,0xf307f2f0U,0x4e69e2a1U,0x65daf4cdU,0x0605bed5U,0xd134621fU,0xc4a6fe8aU,
    0x342e539dU,0xa2f355a0U,0x058ae132U,0xa4f6eb75U,0x0b83ec39U,0x4060efaaU,0x5e719f06U,0xbd6e1051U,
    0x3e218af9U,0x96dd063dU,0xdd3e05aeU,0x4de6bd46U,0x91548db5U,0x71c45d05U,0x0406d46fU,0x605015ffU,
    0x1998fb24U,0xd6bde997U,0x894043ccU,0x67d99e77U,0xb0e842bdU,0x07898b88U,0xe7195b38U,0x79c8eedbU,
    0xa17c0a47U,0x7c420fe9U,0xf8841ec9U,0x00000000U,0x09808683U,0x322bed48U,0x1e1170acU,0x6c5a724eU,
    0xfd0efffbU,0x0f853856U,0x3daed51eU,0x362d3927U,0x0a0fd964U,0x685ca621U,0x9b5b54d1U,0x24362e3aU,
    0x0c0a67b1U,0x9357e70fU,0xb4ee96d2U,0x1b9b919eU,0x80c0c54fU,0x61dc20a2U,0x5a774b69U,0x1c121a16U,
    0xe293ba0aU,0xc0a02ae5U,0x3c22e043U,0x121b171dU,0x0e090d0bU,0xf28bc7adU,0x2db6a8b9U,0x141ea9c8U,
    0x57f11985U,0xaf75074cU,0xee99ddbbU,0xa37f60fdU,0xf701269fU,0x5c72f5bcU,0x44663bc5U,0x5bfb7e34U,
    0x8b432976U,0xcb23c6dcU,0xb6edfc68U,0xb8e4f163U,0xd731dccaU,0x42638510U,0x13972240U,0x84c61120U,
    0x854a247dU,0xd2bb3df8U,0xaef93211U,0xc729a16dU,0x1d9e2f4bU,0xdcb230f3U,0x0d8652ecU,0x77c1e3d0U,
    0x2bb3166cU,0xa970b999U,0x119448faU,0x47e96422U,0xa8fc8cc4U,0xa0f03f1aU,0x567d2cd8U,0x223390efU,
    0x87494ec7U,0xd938d1c1U,0x8ccaa2feU,0x98d40b36U,0xa6f581cfU,0xa57ade28U,0xdab78e26U,0x3fadbfa4U,
    0x2c3a9de4U,0x5078920dU,0x6a5fcc9bU,0x547e4662U,0xf68d13c2U,0x90d8b8e8U,0x2e39f75eU,0x82c3aff5U,
    0x9f5d80beU,0x69d0937cU,0x6fd52da9U,0xcf2512b3U,0xc8ac993bU,0x10187da7U,0xe89c636eU,0xdb3bbb7bU,
    0xcd267809U,0x6e5918f4U,0xec9ab701U,0x834f9aa8U,0xe6956e65U,0xaaffe67eU,0x21bccf08U,0xef15e8e6U,
    0xbae79bd9U,0x4a6f36ceU,0xea9f09d4U,0x29b07cd6U,0x31a4b2afU,0x2a3f2331U,0xc6a59430U,0x35a266c0U,
    0x744ebc37U,0xfc82caa6U,0xe090d0b0U,0x33a7d815U,0xf104984aU,0x41ecdaf7U,0x7fcd500eU,0x1791f62fU,
    0x764dd68dU,0x43efb04dU,0xccaa4d54U,0xe49604dfU,0x9ed1b5e3U,0x4c6a881bU,0xc12c1fb8U,0x4665517fU,
    0x9d5eea04U,0x018c355dU,0xfa877473U,0xfb0b412eU,0xb3671d5aU,0x92dbd252U,0xe9105633U,0x6dd64713U,
    0x9ad7618cU,0x37a10c7aU,0x59f8148eU,0xeb133c89U,0xcea927eeU,0xb761c935U,0xe11ce5edU,0x7a47b13cU,
    0x9cd2df59U,0x55f2733fU,0x1814ce79U,0x73c737bfU,0x53f7cdeaU,0x5ffdaa5bU,0xdf3d6f14U,0x7844db86U,
    0xcaaff381U,0xb968c43eU,0x3824342cU,0xc2a3405fU,0x161dc372U,0xbce2250cU,0x283c498bU,0xff0d9541U,
    0x39a80171U,0x080cb3deU,0xd8b4e49cU,0x6456c190U,0x7bcb8461U,0xd532b670U,0x486c5c74U,0xd0b85742U,
};
static const UINT32 Td1[256]={
    0x5051f4a7U,0x537e4165U,0xc31a17a4U,0x963a275eU,0xcb3bab6bU,0xf11f9d45U,0xabacfa58U,0x934be303U,
    0x552030faU,0xf6ad766dU,0x9188cc76U,0x25f5024cU,0xfc4fe5d7U,0xd7c52acbU,0x80263544U,0x8fb562a3U,
    0x49deb15aU,0x6725ba1bU,0x9845ea0eU,0xe15dfec0U,0x02c32f75U,0x12814cf0U,0xa38d4697U,0xc66bd3f9U,
    0xe7038f5fU,0x9515929cU,0xebbf6d7aU,0xda955259U,0x2dd4be83U,0xd3587421U,0x2949e069U,0x448ec9c8U,
    0x6a75c289U,0x78f48e79U,0x6b99583eU,0xdd27b971U,0xb6bee14fU,0x17f088adU,0x66c920acU,0xb47dce3aU,
    0x1863df4aU,0x82e51a31U,0x60975133U,0x4562537fU,0xe0b16477U,0x84bb6baeU,0x1cfe81a0U,0x94f9082bU,
    0x58704868U,0x198f45fdU,0x8794de6cU,0xb7527bf8U,0x23ab73d3U,0xe2724b02U,0x57e31f8fU,0x2a6655abU,
    0x07b2eb28U,0x032fb5c2U,0x9a86c57bU,0xa5d33708U,0xf2302887U,0xb223bfa5U,0xba02036aU,0x5ced1682U,
    0x2b8acf1cU,0x92a779b4U,0xf0f307f2U,0xa14e69e2U,0xcd65daf4U,0xd50605beU,0x1fd13462U,0x8ac4a6feU,
    0x9d342e53U,0xa0a2f355U,0x32058ae1U,0x75a4f6ebU,0x390b83ecU,0xaa4060efU,0x065e719fU,0x51bd6e10U,
    0xf93e218aU,0x3d96dd06U,0xaedd3e05U,0x464de6bdU,0xb591548dU,0x0571c45dU,0x6f0406d4U,0xff605015U,
    0x241998fbU,0x97d6bde9U,0xcc894043U,0x7767d99eU,0xbdb0e842U,0x8807898bU,0x38e7195bU,0xdb79c8eeU,
    0x47a17c0aU,0xe97c420fU,0xc9f8841eU,0x00000000U,0x83098086U,0x48322bedU,0xac1e1170U,0x4e6c5a72U,
    0xfbfd0effU,0x560f8538U,0x1e3daed5U,0x27362d39U,0x640a0fd9U,0x21685ca6U,0xd19b5b54U,0x3a24362eU,
    0xb10c0a67U,0x0f9357e7U,0xd2b4ee96U,0x9e1b9b91U,0x4f80c0c5U,0xa261dc20U,0x695a774bU,0x161c121aU,
    0x0ae293baU,0xe5c0a02aU,0x433c22e0U,0x1d121b17U,0x0b0e090dU,0xadf28bc7U,0xb92db6a8U,0xc8141ea9U,
    0x8557f119U,0x4caf7507U,0xbbee99ddU,0xfda37f60U,0x9ff70126U,0xbc5c72f5U,0xc544663bU,0x345bfb7eU,
    0x768b4329U,0xdccb23c6U,0x68b6edfcU,0x63b8e4f1U,0xcad731dcU,0x10426385U,0x40139722U,0x2084c611U,
    0x7d854a24U,0xf8d2bb3dU,0x11aef932U,0x6dc729a1U,0x4b1d9e2fU,0xf3dcb230U,0xec0d8652U,0xd077c1e3U,
    0x6c2bb316U,0x99a970b9U,0xfa119448U,0x2247e964U,0xc4a8fc8cU,0x1aa0f03fU,0xd8567d2cU,0xef223390U,
    0xc787494eU,0xc1d938d1U,0xfe8ccaa2U,0x3698d40bU,0xcfa6f581U,0x28a57adeU,0x26dab78eU,0xa43fadbfU,
    0xe42c3a9dU,0x0d507892U,0x9b6a5fccU,0x62547e46U,0xc2f68d13U,0xe890d8b8U,0x5e2e39f7U,0xf582c3afU,
    0xbe9f5d80U,0x7c69d093U,0xa96fd52dU,0xb3cf2512U,0x3bc8ac99U,0xa710187dU,0x6ee89c63U,0x7bdb3bbbU,
    0x09cd2678U,0xf46e5918U,0x01ec9ab7U,0xa8834f9aU,0x65e6956eU,0x7eaaffe6U,0x0821bccfU,0xe6ef15e8U,
    0xd9bae79bU,0xce4a6f36U,0xd4ea9f09U,0xd629b07cU,0xaf31a4b2U,0x312a3f23U,0x30c6a594U,0xc035a266U,
    0x37744ebcU,0xa6fc82caU,0xb0e090d0U,0x1533a7d8U,0x4af10498U,0xf741ecdaU,0x0e7fcd50U,0x2f1791f6U,
    0x8d764dd6U,0x4d43efb0U,0x54ccaa4dU,0xdfe49604U,0xe39ed1b5U,0x1b4c6a88U,0xb8c12c1fU,0x7f466551U,
    0x049d5eeaU,0x5d018c35U,0x73fa8774U,0x2efb0b41U,0x5ab3671dU,0x5292dbd2U,0x33e91056U,0x136dd647U,
    0x8c9ad761U,0x7a37a10cU,0x8e59f814U,0x89eb133cU,0xeecea927U,0x35b761c9U,0xede11ce5U,0x3c7a47b1U,
    0x599cd2dfU,0x3f55f273U,0x791814ceU,0xbf73c737U,0xea53f7cdU,0x5b5ffdaaU,0x14df3d6fU,0x867844dbU,
    0x81caaff3U,0x3eb968c4U,0x2c382434U,0x5fc2a340U,0x72161dc3U,0x0cbce225U,0x8b283c49U,0x41ff0d95U,
    0x7139a801U,0xde080cb3U,0x9cd8b4e4U,0x906456c1U,0x617bcb84U,0x70d532b6U,0x74486c5cU,0x42d0b857U,
};
static const UINT32 Td2[256]={
    0xa75051f4U,0x65537e41U,0xa4c31a17U,0x5e963a27U,0x6bcb3babU,0x45f11f9dU,0x58abacfaU,0x03934be3U,
    0xfa552030U,0x6df6ad76U,0x769188ccU,0x4c25f502U,0xd7fc4fe5U,0xcbd7c52aU,0x44802635U,0xa38fb562U,
    0x5a49deb1U,0x1b6725baU,0x0e9845eaU,0xc0e15dfeU,0x7502c32fU,0xf012814cU,0x97a38d46U,0xf9c66bd3U,
    0x5fe7038fU,0x9c951592U,0x7aebbf6dU,0x59da9552U,0x832dd4beU,0x21d35874U,0x692949e0U,0xc8448ec9U,
    0x896a75c2U,0x7978f48eU,0x3e6b9958U,0x71dd27b9U,0x4fb6bee1U,0xad17f088U,0xac66c920U,0x3ab47dceU,
    0x4a1863dfU,0x3182e51aU,0x33609751U,0x7f456253U,0x77e0b164U,0xae84bb6bU,0xa01cfe81U,0x2b94f908U,
    0x68587048U,0xfd198f45U,0x6c8794deU,0xf8b7527bU,0xd323ab73U,0x02e2724bU,0x8f57e31fU,0xab2a6655U,
    0x2807b2ebU,0xc2032fb5U,0x7b9a86c5U,0x08a5d337U,0x87f23028U,0xa5b223bfU,0x6aba0203U,0x825ced16U,
    0x1c2b8acfU,0xb492a779U,0xf2f0f307U,0xe2a14e69U,0xf4cd65daU,0xbed50605U,0x621fd134U,0xfe8ac4a6U,
    0x539d342eU,0x55a0a2f3U,0xe132058aU,0xeb75a4f6U,0xec390b83U,0xefaa4060U,0x9f065e71U,0x1051bd6eU,
    0x8af93e21U,0x063d96ddU,0x05aedd3eU,0xbd464de6U,0x8db59154U,0x5d0571c4U,0xd46f0406U,0x15ff6050U,
    0xfb241998U,0xe997d6bdU,0x43cc8940U,0x9e7767d9U,0x42bdb0e8U,0x8b880789U,0x5b38e719U,0xeedb79c8U,
    0x0a47a17cU,0x0fe97c42U,0x1ec9f884U,0x00000000U,0x86830980U,0xed48322bU,0x70ac1e11U,0x724e6c5aU,
    0xfffbfd0eU,0x38560f85U,0xd51e3daeU,0x3927362dU,0xd9640a0fU,0xa621685cU,0x54d19b5bU,0x2e3a2436U,
    0x67b10c0aU,0xe70f9357U,0x96d2b4eeU,0x919e1b9bU,0xc54f80c0U,0x20a261dcU,0x4b695a77U,0x1a161c12U,
    0xba0ae293U,0x2ae5c0a0U,0xe0433c22U,0x171d121bU,0x0d0b0e09U,0xc7adf28bU,0xa8b92db6U,0xa9c8141eU,
    0x198557f1U,0x074caf75U,0xddbbee99U,0x60fda37fU,0x269ff701U,0xf5bc5c72U,0x3bc54466U,0x7e345bfbU,
    0x29768b43U,0xc6dccb23U,0xfc68b6edU,0xf163b8e4U,0xdccad731U,0x85104263U,0x22401397U,0x112084c6U,
    0x247d854aU,0x3df8d2bbU,0x3211aef9U,0xa16dc729U,0x2f4b1d9eU,0x30f3dcb2U,0x52ec0d86U,0xe3d077c1U,
    0x166c2bb3U,0xb999a970U,0x48fa1194U,0x642247e9U,0x8cc4a8fcU,0x3f1aa0f0U,0x2cd8567dU,0x90ef2233U,
    0x4ec78749U,0xd1c1d938U,0xa2fe8ccaU,0x0b3698d4U,0x81cfa6f5U,0xde28a57aU,0x8e26dab7U,0xbfa43fadU,
    0x9de42c3aU,0x920d5078U,0xcc9b6a5fU,0x4662547eU,0x13c2f68dU,0xb8e890d8U,0xf75e2e39U,0xaff582c3U,
    0x80be9f5dU,0x937c69d0U,0x2da96fd5U,0x12b3cf25U,0x993bc8acU,0x7da71018U,0x636ee89cU,0xbb7bdb3bU,
    0x7809cd26U,0x18f46e59U,0xb701ec9aU,0x9aa8834fU,0x6e65e695U,0xe67eaaffU,0xcf0821bcU,0xe8e6ef15U,
    0x9bd9bae7U,0x36ce4a6fU,0x09d4ea9fU,0x7cd629b0U,0xb2af31a4U,0x23312a3fU,0x9430c6a5U,0x66c035a2U,
    0xbc37744eU,0xcaa6fc82U,0xd0b0e090U,0xd81533a7U,0x984af104U,0xdaf741ecU,0x500e7fcdU,0xf62f1791U,
    0xd68d764dU,0xb04d43efU,0x4d54ccaaU,0x04dfe496U,0xb5e39ed1U,0x881b4c6aU,0x1fb8c12cU,0x517f4665U,
    0xea049d5eU,0x355d018cU,0x7473fa87U,0x412efb0bU,0x1d5ab367U,0xd25292dbU,0x5633e910U,0x47136dd6U,
    0x618c9ad7U,0x0c7a37a1U,0x148e59f8U,0x3c89eb13U,0x27eecea9U,0xc935b761U,0xe5ede11cU,0xb13c7a47U,
    0xdf599cd2U,0x733f55f2U,0xce791814U,0x37bf73c7U,0xcdea53f7U,0xaa5b5ffdU,0x6f14df3dU,0xdb867844U,
    0xf381caafU,0xc43eb968U,0x342c3824U,0x405fc2a3U,0xc372161dU,0x250cbce2U,0x498b283cU,0x9541ff0dU,
    0x017139a8U,0xb3de080cU,0xe49cd8b4U,0xc1906456U,0x84617bcbU,0xb670d532U,0x5c74486cU,0x5742d0b8U,
};
static const UINT32 Td3[256]={
    0xf4a75051U,0x4165537eU,0x17a4c31aU,0x275e963aU,0xab6bcb3bU,0x9d45f11fU,0xfa58abacU,0xe303934bU,
    0x30fa5520U,0x766df6adU,0xcc769188U,0x024c25f5U,0xe5d7fc4fU,0x2acbd7c5U,0x35448026U,0x62a38fb5U,
    0xb15a49deU,0xba1b6725U,0xea0e9845U,0xfec0e15dU,0x2f7502c3U,0x4cf01281U,0x4697a38dU,0xd3f9c66bU,
    0x8f5fe703U,0x929c9515U,0x6d7aebbfU,0x5259da95U,0xbe832dd4U,0x7421d358U,0xe0692949U,0xc9c8448eU,
    0xc2896a75U,0x8e7978f4U,0x583e6b99U,0xb971dd27U,0xe14fb6beU,0x88ad17f0U,0x20ac66c9U,0xce3ab47dU,
    0xdf4a1863U,0x1a3182e5U,0x51336097U,0x537f4562U,0x6477e0b1U,0x6bae84bbU,0x81a01cfeU,0x082b94f9U,
    0x48685870U,0x45fd198fU,0xde6c8794U,0x7bf8b752U,0x73d323abU,0x4b02e272U,0x1f8f57e3U,0x55ab2a66U,
    0xeb2807b2U,0xb5c2032fU,0xc57b9a86U,0x3708a5d3U,0x2887f230U,0xbfa5b223U,0x036aba02U,0x16825cedU,
    0xcf1c2b8aU,0x79b492a7U,0x07f2f0f3U,0x69e2a14eU,0xdaf4cd65U,0x05bed506U,0x34621fd1U,0xa6fe8ac4U,
    0x2e539d34U,0xf355a0a2U,0x8ae13205U,0xf6eb75a4U,0x83ec390bU,0x60efaa40U,0x719f065eU,0x6e1051bdU,
    0x218af93eU,0xdd063d96U,0x3e05aeddU,0xe6bd464dU,0x548db591U,0xc45d0571U,0x06d46f04U,0x5015ff60U,
    0x98fb2419U,0xbde997d6U,0x4043cc89U,0xd99e7767U,0xe842bdb0U,0x898b8807U,0x195b38e7U,0xc8eedb79U,
    0x7c0a47a1U,0x420fe97cU,0x841ec9f8U,0x00000000U,0x80868309U,0x2bed4832U,0x1170ac1eU,0x5a724e6cU,
    0x0efffbfdU,0x8538560fU,0xaed51e3dU,0x2d392736U,0x0fd9640aU,0x5ca62168U,0x5b54d19bU,0x362e3a24U,
    0x0a67b10cU,0x57e70f93U,0xee96d2b4U,0x9b919e1bU,0xc0c54f80U,0xdc20a261U,0x774b695aU,0x121a161cU,
    0x93ba0ae2U,0xa02ae5c0U,0x22e0433cU,0x1b171d12U,0x090d0b0eU,0x8bc7adf2U,0xb6a8b92dU,0x1ea9c814U,
    0xf1198557U,0x75074cafU,0x99ddbbeeU,0x7f60fda3U,0x01269ff7U,0x72f5bc5cU,0x663bc544U,0xfb7e345bU,
    0x4329768bU,0x23c6dccbU,0xedfc68b6U,0xe4f163b8U,0x31dccad7U,0x63851042U,0x97224013U,0xc6112084U,
    0x4a247d85U,0xbb3df8d2U,0xf93211aeU,0x29a16dc7U,0x9e2f4b1dU,0xb230f3dcU,0x8652ec0dU,0xc1e3d077U,
    0xb3166c2bU,0x70b999a9U,0x9448fa11U,0xe9642247U,0xfc8cc4a8U,0xf03f1aa0U,0x7d2cd856U,0x3390ef22U,
    0x494ec787U,0x38d1c1d9U,0xcaa2fe8cU,0xd40b3698U,0xf581cfa6U,0x7ade28a5U,0xb78e26daU,0xadbfa43fU,
    0x3a9de42cU,0x78920d50U,0x5fcc9b6aU,0x7e466254U,0x8d13c2f6U,0xd8b8e890U,0x39f75e2eU,0xc3aff582U,
    0x5d80be9fU,0xd0937c69U,0xd52da96fU,0x2512b3cfU,0xac993bc8U,0x187da710U,0x9c636ee8U,0x3bbb7bdbU,
    0x267809cdU,0x5918f46eU,0x9ab701ecU,0x4f9aa883U,0x956e65e6U,0xffe67eaaU,0xbccf0821U,0x15e8e6efU,
    0xe79bd9baU,0x6f36ce4aU,0x9f09d4eaU,0xb07cd629U,0xa4b2af31U,0x3f23312aU,0xa59430c6U,0xa266c035U,
    0x4ebc3774U,0x82caa6fcU,0x90d0b0e0U,0xa7d81533U,0x04984af1U,0xecdaf741U,0xcd500e7fU,0x91f62f17U,
    0x4dd68d76U,0xefb04d43U,0xaa4d54ccU,0x9604dfe4U,0xd1b5e39eU,0x6a881b4cU,0x2c1fb8c1U,0x65517f46U,
    0x5eea049dU,0x8c355d01U,0x877473faU,0x0b412efbU,0x671d5ab3U,0xdbd25292U,0x105633e9U,0xd647136dU,
    0xd7618c9aU,0xa10c7a37U,0xf8148e59U,0x133c89ebU,0xa927eeceU,0x61c935b7U,0x1ce5ede1U,0x47b13c7aU,
    0xd2df599cU,0xf2733f55U,0x14ce7918U,0xc737bf73U,0xf7cdea53U,0xfdaa5b5fU,0x3d6f14dfU,0x44db8678U,
    0xaff381caU,0x68c43eb9U,0x24342c38U,0xa3405fc2U,0x1dc37216U,0xe2250cbcU,0x3c498b28U,0x0d9541ffU,
    0xa8017139U,0x0cb3de08U,0xb4e49cd8U,0x56c19064U,0xcb84617bU,0x32b670d5U,0x6c5c7448U,0xb85742d0U,
};
static const UINT8 Td4[256]={
    0x52U,0x09U,0x6aU,0xd5U,0x30U,0x36U,0xa5U,0x38U,0xbfU,0x40U,0xa3U,0x9eU,0x81U,0xf3U,0xd7U,0xfbU,
    0x7cU,0xe3U,0x39U,0x82U,0x9bU,0x2fU,0xffU,0x87U,0x34U,0x8eU,0x43U,0x44U,0xc4U,0xdeU,0xe9U,0xcbU,
    0x54U,0x7bU,0x94U,0x32U,0xa6U,0xc2U,0x23U,0x3dU,0xeeU,0x4cU,0x95U,0x0bU,0x42U,0xfaU,0xc3U,0x4eU,
    0x08U,0x2eU,0xa1U,0x66U,0x28U,0xd9U,0x24U,0xb2U,0x76U,0x5bU,0xa2U,0x49U,0x6dU,0x8bU,0xd1U,0x25U,
    0x72U,0xf8U,0xf6U,0x64U,0x86U,0x68U,0x98U,0x16U,0xd4U,0xa4U,0x5cU,0xccU,0x5dU,0x65U,0xb6U,0x92U,
    0x6cU,0x70U,0x48U,0x50U,0xfdU,0xedU,0xb9U,0xdaU,0x5eU,0x15U,0x46U,0x57U,0xa7U,0x8dU,0x9dU,0x84U,
    0x90U,0xd8U,0xabU,0x00U,0x8cU,0xbcU,0xd3U,0x0aU,0xf7U,0xe4U,0x58U,0x05U,0xb8U,0xb3U,0x45U,0x06U,
    0xd0U,0x2cU,0x1eU,0x8fU,0xcaU,0x3fU,0x0fU,0x02U,0xc1U,0xafU,0xbdU,0x03U,0x01U,0x13U,0x8aU,0x6bU,
    0x3aU,0x91U,0x11U,0x41U,0x4fU,0x67U,0xdcU,0xeaU,0x97U,0xf2U,0xcfU,0xceU,0xf0U,0xb4U,0xe6U,0x73U,
    0x96U,0xacU,0x74U,0x22U,0xe7U,0xadU,0x35U,0x85U,0xe2U,0xf9U,0x37U,0xe8U,0x1cU,0x75U,0xdfU,0x6eU,
    0x47U,0xf1U,0x1aU,0x71U,0x1dU,0x29U,0xc5U,0x89U,0x6fU,0xb7U,0x62U,0x0eU,0xaaU,0x18U,0xbeU,0x1bU,
    0xfcU,0x56U,0x3eU,0x4bU,0xc6U,0xd2U,0x79U,0x20U,0x9aU,0xdbU,0xc0U,0xfeU,0x78U,0xcdU,0x5aU,0xf4U,
    0x1fU,0xddU,0xa8U,0x33U,0x88U,0x07U,0xc7U,0x31U,0xb1U,0x12U,0x10U,0x59U,0x27U,0x80U,0xecU,0x5fU,
    0x60U,0x51U,0x7fU,0xa9U,0x19U,0xb5U,0x4aU,0x0dU,0x2dU,0xe5U,0x7aU,0x9fU,0x93U,0xc9U,0x9cU,0xefU,
    0xa0U,0xe0U,0x3bU,0x4dU,0xaeU,0x2aU,0xf5U,0xb0U,0xc8U,0xebU,0xbbU,0x3cU,0x83U,0x53U,0x99U,0x61U,
    0x17U,0x2bU,0x04U,0x7eU,0xbaU,0x77U,0xd6U,0x26U,0xe1U,0x69U,0x14U,0x63U,0x55U,0x21U,0x0cU,0x7dU,
};
static const UINT32 rcon[]={
    0x01000000,0x02000000,0x04000000,0x08000000,0x10000000,0x20000000,0x40000000,0x80000000,0x1B000000
};
UINT32 decrd[4*15];
UINT8 aes_iv[16];

void aes_init(unsigned char *pass)
{
    UINT32 i=0,j=0,t,*rk;
    CopyMem(aes_iv,pass+(pass[0]&0xF),16);
    rk=decrd;
    rk[0]=GETU32(pass   );rk[1]=GETU32(pass+ 4);rk[2]=GETU32(pass+ 8);rk[3]=GETU32(pass+12);
    rk[4]=GETU32(pass+16);rk[5]=GETU32(pass+20);rk[6]=GETU32(pass+24);rk[7]=GETU32(pass+28);
    while (1) {
        t=rk[7];rk[8]=rk[0]^(Te2[(t>>16)&0xff]&0xff000000)^(Te3[(t>>8)&0xff]&0x00ff0000)^
            (Te0[(t)&0xff]&0x0000ff00)^(Te1[(t>>24)]&0x000000ff)^rcon[i];
        rk[9]=rk[1]^rk[8];rk[10]=rk[2]^rk[9];rk[11]=rk[3]^rk[10];
        if(++i==7) break;
        t=rk[11];rk[12]=rk[4]^(Te2[(t>>24)]&0xff000000)^(Te3[(t>>16)&0xff]&0x00ff0000)^
            (Te0[(t>>8)&0xff]&0x0000ff00)^(Te1[(t)&0xff]&0x000000ff);
        rk[13]=rk[5]^rk[12];rk[14]=rk[6]^rk[13];rk[15]=rk[7]^rk[14];rk+=8;
    }
    rk=decrd;
    for(i=0,j=4*14;i<j;i+=4,j-=4) {
        t=rk[i];rk[i]=rk[j];rk[j]=t;t=rk[i+1];rk[i+1]=rk[j+1];rk[j+1]=t;
        t=rk[i+2];rk[i+2]=rk[j+2];rk[j+2]=t;t=rk[i+3];rk[i+3]=rk[j+3];rk[j+3]=t;
    }
    for(i=1;i<14;i++) {
        rk+=4;
        rk[0]=Td0[Te1[(rk[0]>>24)]&0xff]^Td1[Te1[(rk[0]>>16)&0xff]&0xff]^
            Td2[Te1[(rk[0]>>8)&0xff]&0xff]^Td3[Te1[(rk[0])&0xff]&0xff];
        rk[1]=Td0[Te1[(rk[1]>>24)]&0xff]^Td1[Te1[(rk[1]>>16)&0xff]&0xff]^
            Td2[Te1[(rk[1]>>8)&0xff]&0xff]^Td3[Te1[(rk[1])&0xff]&0xff];
        rk[2]=Td0[Te1[(rk[2]>>24)]&0xff]^Td1[Te1[(rk[2]>>16)&0xff]&0xff]^
            Td2[Te1[(rk[2]>>8)&0xff]&0xff]^Td3[Te1[(rk[2])&0xff]&0xff];
        rk[3]=Td0[Te1[(rk[3]>>24)]&0xff]^Td1[Te1[(rk[3]>>16)&0xff]&0xff]^
            Td2[Te1[(rk[3]>>8)&0xff]&0xff]^Td3[Te1[(rk[3])&0xff]&0xff];
    }
}

void aes_dec(unsigned char *data, UINT32 l)
{
    UINT32 rd[4*15],*rk=rd,n,s0,s1,s2,s3,t0,t1,t2,t3;
    unsigned char ivec[16],c[sizeof(ivec)],d,*out=data;
    CopyMem(&rd,&decrd,sizeof(rd));
    CopyMem(ivec,aes_iv,sizeof(ivec));
    while(l>=16) {
        rk=rd;s0=GETU32(out)^rk[0];s1=GETU32(out+4)^rk[1];s2=GETU32(out+8)^rk[2];s3=GETU32(out+12)^rk[3];
        for(n=14>>1;;){
            t0=Td0[(s0>>24)]^Td1[(s3>>16)&0xff]^Td2[(s2>>8)&0xff]^Td3[(s1)&0xff]^rk[4];
            t1=Td0[(s1>>24)]^Td1[(s0>>16)&0xff]^Td2[(s3>>8)&0xff]^Td3[(s2)&0xff]^rk[5];
            t2=Td0[(s2>>24)]^Td1[(s1>>16)&0xff]^Td2[(s0>>8)&0xff]^Td3[(s3)&0xff]^rk[6];
            t3=Td0[(s3>>24)]^Td1[(s2>>16)&0xff]^Td2[(s1>>8)&0xff]^Td3[(s0)&0xff]^rk[7];
            rk+=8;
            if(--n==0) break;
            s0=Td0[(t0>>24)]^Td1[(t3>>16)&0xff]^Td2[(t2>>8)&0xff]^Td3[(t1)&0xff]^rk[0];
            s1=Td0[(t1>>24)]^Td1[(t0>>16)&0xff]^Td2[(t3>>8)&0xff]^Td3[(t2)&0xff]^rk[1];
            s2=Td0[(t2>>24)]^Td1[(t1>>16)&0xff]^Td2[(t0>>8)&0xff]^Td3[(t3)&0xff]^rk[2];
            s3=Td0[(t3>>24)]^Td1[(t2>>16)&0xff]^Td2[(t1>>8)&0xff]^Td3[(t0)&0xff]^rk[3];
        }
        s0=((uint32_t)Td4[(t0>>24)]<<24)^((uint32_t)Td4[(t3>>16)&0xff]<<16)^
            ((uint32_t)Td4[(t2>>8)&0xff]<<8)^((uint32_t)Td4[(t1)&0xff])^rk[0];
        PUTU32(c,s0);
        s1=((uint32_t)Td4[(t1>>24)]<<24)^((uint32_t)Td4[(t0>>16)&0xff]<<16)^
            ((uint32_t)Td4[(t3>>8)&0xff]<<8)^((uint32_t)Td4[(t2)&0xff])^rk[1];
        PUTU32(c+4,s1);
        s2=((uint32_t)Td4[(t2>>24)]<<24)^((uint32_t)Td4[(t1>>16)&0xff]<<16)^
            ((uint32_t)Td4[(t0>>8)&0xff]<<8)^((uint32_t)Td4[(t3)&0xff])^rk[2];
        PUTU32(c+8,s2);
        s3=((uint32_t)Td4[(t3>>24)]<<24)^((uint32_t)Td4[(t2>>16)&0xff]<<16)^
            ((uint32_t)Td4[(t1>>8)&0xff]<<8)^((uint32_t)Td4[(t0)&0xff])^rk[3];
        PUTU32(c+12,s3);
        for(n=0;n<16;n++) {d=out[n];out[n]=c[n]^ivec[n];ivec[n]=d;}
        l-=16;
        out+=16;
    }
}

/**
 * Read a line from ConIn
 */
int ReadLine(unsigned char *buf, int l)
{
    int i=0;
    EFI_INPUT_KEY key;
    while(1) {
        if(!uefi_call_wrapper(CI->ReadKeyStroke, 2, CI, &key)) {
            if(key.UnicodeChar==CHAR_LINEFEED || key.UnicodeChar==CHAR_CARRIAGE_RETURN) {
                break;
            } else
            if(key.UnicodeChar==CHAR_BACKSPACE || key.ScanCode==SCAN_DELETE) {
                if(i) i--;
                buf[i]=0;
                continue;
            } else
            if(key.ScanCode==SCAN_ESC) {
                buf[0]=0;
                return 0;
            } else
            if(key.UnicodeChar && i<l-1) {
                // convert unicode to utf-8
                if(key.UnicodeChar<0x80)
                    buf[i++]=key.UnicodeChar;
                else if(key.UnicodeChar<0x800) {
                    buf[i++]=((key.UnicodeChar>>6)&0x1F)|0xC0;
                    buf[i++]=(key.UnicodeChar&0x3F)|0x80;
                } else {
                    buf[i++]=((key.UnicodeChar>>12)&0xF)|0xE0;
                    buf[i++]=((key.UnicodeChar>>6)&0x3F)|0x80;
                    buf[i++]=(key.UnicodeChar&0x3F)|0x80;
                }
                buf[i]=0;
            }
        }
    }
    return i;
}
#endif

/**
 * function to convert ascii to number
 */
int atoi(unsigned char*c)
{
    int r=0;
    while(*c>='0'&&*c<='9') {
        r*=10; r+=*c-'0';
        c++;
    }
    return r;
}

/**
 * convert ascii to unicode characters
 */
CHAR16 *
a2u (char *str)
{
    static CHAR16 mem[PAGESIZE];
    int i;

    for (i = 0; str[i]; ++i)
        mem[i] = (CHAR16) str[i];
    mem[i] = 0;
    return mem;
}

/**
 * report status with message to standard output
 */
EFI_STATUS
report(EFI_STATUS Status,CHAR16 *Msg)
{
    Print(L"BOOTBOOT-PANIC: %s (EFI %r)\n",Msg,Status);
    return Status;
}

/**
 * convert ascii octal number to binary number
 */
int oct2bin(unsigned char *str,int size)
{
    int s=0;
    unsigned char *c=str;
    while(size-->0){
        s*=8;
        s+=*c-'0';
        c++;
    }
    return s;
}

/**
 * convert ascii hex number to binary number
 */
int hex2bin(unsigned char *str, int size)
{
    int v=0;
    while(size-->0){
        v <<= 4;
        if(*str>='0' && *str<='9')
            v += (int)((unsigned char)(*str)-'0');
        else if(*str >= 'A' && *str <= 'F')
            v += (int)((unsigned char)(*str)-'A'+10);
        str++;
    }
    return v;
}

// get filesystem drivers for initrd
#include "fs.h"

/**
 * Parse FS0:\BOOTBOOT\CONFIG or /sys/config
 */
EFI_STATUS
ParseEnvironment(unsigned char *cfg, int len, INTN argc, CHAR16 **argv)
{
    unsigned char *ptr=cfg-1;
    int i;
    // failsafe
    if(len>PAGESIZE-1) {
        len=PAGESIZE-1;
    }
    // append temporary variables provided on EFI command line
    // if a key exists multiple times, the last is used
    cfg[len]=0;
    if(argc>2){
        ptr=cfg+len;
        for(i=3;i<argc && ptr+StrLen(argv[i])<cfg+PAGESIZE;i++) {
            CopyMem(ptr,argv[i],StrLen(argv[i]));
            ptr += StrLen(argv[i]);
            *ptr = '\n';
            ptr++;
        }
        *ptr = 0;
        ptr=cfg-1;
    }
    DBG(L" * Environment @%lx %d bytes\n",cfg,len);
    while(ptr<cfg+len) {
        ptr++;
        // failsafe
        if(ptr[0]==0)
            break;
        // skip white spaces
        if(ptr[0]==' '||ptr[0]=='\t'||ptr[0]=='\r'||ptr[0]=='\n')
            continue;
        // skip comments
        if((ptr[0]=='/'&&ptr[1]=='/')||ptr[0]=='#') {
            while(ptr<cfg+len && ptr[0]!='\r' && ptr[0]!='\n' && ptr[0]!=0){
                ptr++;
            }
            ptr--;
            continue;
        }
        if(ptr[0]=='/'&&ptr[1]=='*') {
            ptr+=2;
            while(ptr[0]!=0 && ptr[-1]!='*' && ptr[0]!='/')
                ptr++;
        }
        // parse screen dimensions
        if(!CompareMem(ptr,(const CHAR8 *)"screen=",7)){
            ptr+=7;
            reqwidth=atoi(ptr);
            while(ptr<cfg+len && *ptr!=0 && *(ptr-1)!='x') ptr++;
            reqheight=atoi(ptr);
        }
        // get kernel's filename
        if(!CompareMem(ptr,(const CHAR8 *)"kernel=",7)){
            ptr+=7;
            kernelname=(char*)ptr;
            while(ptr<cfg+len && ptr[0]!='\r' && ptr[0]!='\n' &&
                ptr[0]!=' ' && ptr[0]!='\t' && ptr[0]!=0)
                    ptr++;
            kne=ptr;
            *ptr=0;
            ptr++;
        }
    }
    return EFI_SUCCESS;
}

/**
 * Get a linear frame buffer
 */
EFI_STATUS
GetLFB()
{
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *info;
    UINTN i, imax, SizeOfInfo, nativeMode, selectedMode=9999, sw=0, sh=0, valid;

    //GOP
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);
    if(EFI_ERROR(status))
        return status;

    // minimum resolution
    if(reqwidth < 640)  reqwidth = 640;
    if(reqheight < 480) reqheight = 480;

    // get current video mode
    status = uefi_call_wrapper(gop->QueryMode, 4, gop, gop->Mode==NULL?0:gop->Mode->Mode, &SizeOfInfo, &info);
    if (status == EFI_NOT_STARTED)
        status = uefi_call_wrapper(gop->SetMode, 2, gop, 0);
    if(EFI_ERROR(status))
        return status;
    nativeMode = gop->Mode->Mode;
    imax=gop->Mode->MaxMode;
    for (i = 0; i < imax; i++) {
        status = uefi_call_wrapper(gop->QueryMode, 4, gop, i, &SizeOfInfo, &info);
        // failsafe
        if (EFI_ERROR(status))
            continue;
        valid=0;
        // get the mode for the closest resolution
        if((info->PixelFormat == PixelRedGreenBlueReserved8BitPerColor ||
            info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor
// there's a bug in TianoCore, it reports bad masks in PixelInformation, so we don't use PixelBitMask
//          || (info->PixelFormat == PixelBitMask)
           )){
            if(info->HorizontalResolution >= (unsigned int)reqwidth &&
               info->VerticalResolution >= (unsigned int)reqheight &&
               (selectedMode==9999||(info->HorizontalResolution<sw && info->VerticalResolution < sh))) {
                    selectedMode = i;
                    sw = info->HorizontalResolution;
                    sh = info->VerticalResolution;
            }
            valid=1;
        }
        // make gcc happy
        if(valid){}
#if GOP_DEBUG
        DBG(L"    %c%2d %4d x %4d, %d%c ", i==selectedMode?'+':(i==nativeMode?'-':' '),
            i, info->HorizontalResolution, info->VerticalResolution, info->PixelFormat,valid?' ':'?');
        DBG(L"r:%x g:%x b:%x\n",
                info->PixelFormat==PixelRedGreenBlueReserved8BitPerColor?0xff:(
                info->PixelFormat==PixelBlueGreenRedReserved8BitPerColor?0xff0000:(
                info->PixelFormat==PixelBitMask?info->PixelInformation.RedMask:0)),
                info->PixelFormat==PixelRedGreenBlueReserved8BitPerColor ||
                info->PixelFormat==PixelBlueGreenRedReserved8BitPerColor?0xff00:(
                info->PixelFormat==PixelBitMask?info->PixelInformation.GreenMask:0),
                info->PixelFormat==PixelRedGreenBlueReserved8BitPerColor?0xff0000:(
                info->PixelFormat==PixelBlueGreenRedReserved8BitPerColor?0xff:(
                info->PixelFormat==PixelBitMask?info->PixelInformation.BlueMask:0)));
#endif
    }
    // if we have found a new, better mode
    if(selectedMode != 9999 && selectedMode != nativeMode) {
        status = uefi_call_wrapper(gop->SetMode, 2, gop, selectedMode);
        if(!EFI_ERROR(status))
            nativeMode = selectedMode;
    }
    // get framebuffer properties
    bootboot->fb_ptr=(void*)gop->Mode->FrameBufferBase;
    bootboot->fb_size=gop->Mode->FrameBufferSize;
    bootboot->fb_scanline=4*gop->Mode->Info->PixelsPerScanLine;
    bootboot->fb_width=gop->Mode->Info->HorizontalResolution;
    bootboot->fb_height=gop->Mode->Info->VerticalResolution;
    bootboot->fb_type=
        gop->Mode->Info->PixelFormat==PixelBlueGreenRedReserved8BitPerColor ||
        (gop->Mode->Info->PixelFormat==PixelBitMask && gop->Mode->Info->PixelInformation.BlueMask==0)? FB_ARGB : (
            gop->Mode->Info->PixelFormat==PixelRedGreenBlueReserved8BitPerColor ||
            (gop->Mode->Info->PixelFormat==PixelBitMask && gop->Mode->Info->PixelInformation.RedMask==0)? FB_ABGR : (
                gop->Mode->Info->PixelInformation.BlueMask==0xFF00? FB_RGBA : FB_BGRA
        ));
    DBG(L" * Screen %d x %d, scanline %d, fb @%lx %d bytes, type %d %s\n",
        bootboot->fb_width, bootboot->fb_height, bootboot->fb_scanline,
        bootboot->fb_ptr, bootboot->fb_size, gop->Mode->Info->PixelFormat,
            bootboot->fb_type==FB_ARGB?L"ARGB":(bootboot->fb_type==FB_ABGR?L"ABGR":(
            bootboot->fb_type==FB_RGBA?L"RGBA":L"BGRA")));
    return EFI_SUCCESS;
}

/**
 * Load a file from FS0 into memory
 */
EFI_STATUS
LoadFile(IN CHAR16 *FileName, OUT UINT8 **FileData, OUT UINTN *FileDataLength)
{
    EFI_STATUS          status;
    EFI_FILE_HANDLE     FileHandle;
    EFI_FILE_INFO       *FileInfo;
    UINT64              ReadSize;
    UINTN               BufferSize;
    UINT8               *Buffer;

    if ((RootDir == NULL) || (FileName == NULL)) {
        return report(EFI_NOT_FOUND,L"Empty Root or FileName\n");
    }

    status = uefi_call_wrapper(RootDir->Open, 5, RootDir, &FileHandle, FileName,
        EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    if (EFI_ERROR(status)) {
        return status;
//        Print(L"%s not found\n",FileName);
//        return report(status,L"Open error");
    }
    FileInfo = LibFileInfo(FileHandle);
    if (FileInfo == NULL) {
        uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
        Print(L"%s not found\n",FileName);
        return report(EFI_NOT_FOUND,L"FileInfo error");
    }
    ReadSize = FileInfo->FileSize;
    if (ReadSize > 16*1024*1024)
        ReadSize = 16*1024*1024;
    FreePool(FileInfo);

    BufferSize = (UINTN)((ReadSize+PAGESIZE-1)/PAGESIZE);
    status = uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, BufferSize, (EFI_PHYSICAL_ADDRESS*)&Buffer);
    if (Buffer == NULL) {
        uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
        return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
    }
    status = uefi_call_wrapper(FileHandle->Read, 3, FileHandle, &ReadSize, Buffer);
    uefi_call_wrapper(FileHandle->Close, 1, FileHandle);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(BS->FreePages, 2, (EFI_PHYSICAL_ADDRESS)(Buffer), BufferSize);
        Print(L"%s not found\n",FileName);
        return report(status,L"Read error");
    }

    *FileData = Buffer;
    *FileDataLength = ReadSize;
    return EFI_SUCCESS;
}

/**
 * Locate and load the kernel in initrd
 */
EFI_STATUS
LoadCore()
{
    int i=0,bss=0;
    UINT8 *ptr;
    core.ptr=ptr=NULL;
    while(core.ptr==NULL && fsdrivers[i]!=NULL) {
        core=(*fsdrivers[i++])((unsigned char*)initrd.ptr,kernelname);
    }
    // if every driver failed, try brute force, scan for the first elf or pe executable
    if(core.ptr==NULL) {
        DBG(L" * Autodetecting kernel%s\n","");
        i=initrd.size;
        core.ptr=initrd.ptr;
        while(i-->0) {
            Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(core.ptr);
            pe_hdr *pehdr=(pe_hdr*)(core.ptr + ((mz_hdr*)(core.ptr))->peaddr);
            if((!CompareMem(ehdr->e_ident,ELFMAG,SELFMAG)||!CompareMem(ehdr->e_ident,"OS/Z",4))&&
                ehdr->e_ident[EI_CLASS]==ELFCLASS64&&
                ehdr->e_ident[EI_DATA]==ELFDATA2LSB&&
                ehdr->e_machine==EM_X86_64&&
                ehdr->e_phnum>0){
                    break;
                }
            if(((mz_hdr*)(core.ptr))->magic==MZ_MAGIC && ((mz_hdr*)(core.ptr))->peaddr<65536 && pehdr->magic == PE_MAGIC &&
                pehdr->machine == IMAGE_FILE_MACHINE_AMD64 && pehdr->file_type == PE_OPT_MAGIC_PE32PLUS) {
                    break;
                }
            core.ptr++;
        }
        core.ptr=NULL;
    }

    if(core.ptr!=NULL) {
        Elf64_Ehdr *ehdr=(Elf64_Ehdr *)(core.ptr);
        pe_hdr *pehdr=(pe_hdr*)(core.ptr + ((mz_hdr*)(core.ptr))->peaddr);
        if((!CompareMem(ehdr->e_ident,ELFMAG,SELFMAG)||!CompareMem(ehdr->e_ident,"OS/Z",4))&&
            ehdr->e_ident[EI_CLASS]==ELFCLASS64&&ehdr->e_ident[EI_DATA]==ELFDATA2LSB&&
            ehdr->e_machine==EM_X86_64&&ehdr->e_phnum>0){
            // Parse ELF64
            DBG(L" * Parsing ELF64 @%lx\n",core.ptr);
            Elf64_Phdr *phdr=(Elf64_Phdr *)((UINT8 *)ehdr+ehdr->e_phoff);
            for(i=0;i<ehdr->e_phnum;i++){
                if(phdr->p_type==PT_LOAD && phdr->p_vaddr>>48==0xffff) {
                    // hack to keep symtab and strtab for shared libraries
                    core.size = phdr->p_filesz + (ehdr->e_type==3?0x4000:0);
                    ptr = (UINT8*)ehdr + phdr->p_offset;
                    bss = phdr->p_memsz - core.size;
                    entrypoint = ehdr->e_entry;
                    break;
                }
                phdr=(Elf64_Phdr *)((UINT8 *)phdr+ehdr->e_phentsize);
            }
        } else if(((mz_hdr*)(core.ptr))->magic==MZ_MAGIC && ((mz_hdr*)(core.ptr))->peaddr<65536 && pehdr->magic == PE_MAGIC &&
            pehdr->machine == IMAGE_FILE_MACHINE_AMD64 && pehdr->file_type == PE_OPT_MAGIC_PE32PLUS &&
            (INT64)pehdr->code_base>>48==0xffff) {
                //Parse PE32+
                DBG(L" * Parsing PE32+ @%lx\n",core.ptr);
                core.size = (pehdr->entry_point-pehdr->code_base) + pehdr->text_size + pehdr->data_size;
                ptr = core.ptr;
                bss = pehdr->bss_size;
                entrypoint = (INT64)pehdr->entry_point;
        }
        if(ptr==NULL || core.size<2 || entrypoint==0)
            return report(EFI_LOAD_ERROR,L"Kernel is not a valid executable");
        // create core segment
        uefi_call_wrapper(BS->AllocatePages, 4, 0, 2,
            (core.size + bss + PAGESIZE-1)/PAGESIZE, (EFI_PHYSICAL_ADDRESS*)&core.ptr);
        if (core.ptr == NULL)
            return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
        CopyMem((void*)core.ptr,ptr,core.size);
        if(bss>0)
            ZeroMem((void*)core.ptr + core.size, bss);
        core.size += bss;
        DBG(L" * Entry point @%lx, text @%lx %d bytes\n",entrypoint, core.ptr, core.size);
        core.size = ((core.size+PAGESIZE-1)/PAGESIZE)*PAGESIZE;
        return EFI_SUCCESS;

    }
    return report(EFI_LOAD_ERROR,L"Kernel not found in initrd");
}

/**
 * Initialize logical cores
 * Because Local APIC ID is not contiguous, core id != core num
 */
VOID EFIAPI bootboot_startcore(IN VOID* buf)
{
    // we have a scalar number, not a pointer, so cast it
    UINTN core_num = (UINTN)buf;

    // spinlock until BSP finishes
    do { __asm__ __volatile__ ("pause"); } while(!bsp_done);

    // enable SSE
    __asm__ __volatile__ (
        "movq %%cr0, %%rax;"
        "andb $0xfb, %%al;"
        "orl $0xC0000001, %%eax;"
        "movq %%rax, %%cr0;"
        "movq %%cr4, %%rax;"
        "orw $3 << 8, %%ax;"
        "mov %%rax, %%cr4"
        : );

    // set up paging
    __asm__ __volatile__ (
        "mov %0, %%rax;"
        "mov %%rax, %%cr3"
        : : "b"(paging) : "memory" );

    // set stack and call _start() in sys/core
    __asm__ __volatile__ (
        // get a valid stack for the core we're running on
        "xorq %%rsp, %%rsp;"
        "subq %1, %%rsp;"  // sp = core_num * -1024
        // pass control over
        "pushq %0;"
        "retq"
        : : "a"(entrypoint), "r"(core_num*1024) : "memory" );
}

/**
 * Main EFI application entry point
 */
EFI_STATUS
efi_main (EFI_HANDLE image, EFI_SYSTEM_TABLE *systab)
{
    EFI_LOADED_IMAGE *loaded_image = NULL;
    EFI_GUID lipGuid = LOADED_IMAGE_PROTOCOL;
    EFI_GUID RomTableGuid = EFI_PCI_OPTION_ROM_TABLE_GUID;
    EFI_PCI_OPTION_ROM_TABLE *RomTable;
    EFI_GUID bioGuid = BLOCK_IO_PROTOCOL;
    EFI_BLOCK_IO *bio;
    EFI_HANDLE *handles = NULL;
    EFI_STATUS status=EFI_SUCCESS;
    EFI_MEMORY_DESCRIPTOR *memory_map = NULL, *mement;
    EFI_PARTITION_TABLE_HEADER *gptHdr;
    EFI_PARTITION_ENTRY *gptEnt;
    EFI_INPUT_KEY key;
    EFI_EVENT Event;
    EFI_GUID mpspGuid = EFI_MP_SERVICES_PROTOCOL_GUID;
    EFI_MP_SERVICES_PROTOCOL *mp;
    UINT8 pibuffer[100];
    EFI_PROCESSOR_INFORMATION *pibuf=(EFI_PROCESSOR_INFORMATION*)pibuffer;
    UINTN bsp_num=0, i, j=0, handle_size=0,memory_map_size=0, map_key=0, desc_size=0;
    UINT32 desc_version=0;
    UINT64 lba_s=0,lba_e=0,sysptr;
    MMapEnt *mmapent, *last=NULL;
    file_t ret={NULL,0};
    CHAR16 **argv, *initrdfile, *configfile, *help=
        L"SYNOPSIS\n  BOOTBOOT.EFI [ -h | -? | /h | /? ] [ INITRDFILE [ ENVIRONMENTFILE [...] ] ]\n\nDESCRIPTION\n  Bootstraps an operating system via the BOOTBOOT Protocol.\n  If arguments not given, defaults to\n    FS0:\\BOOTBOOT\\INITRD   as ramdisk image and\n    FS0:\\BOOTBOOT\\CONFIG   for boot environment.\n  Additional \"key=value\" command line arguments will be appended to the\n  environment. If INITRD not found, it will use the first bootable partition\n  in GPT. If CONFIG not found, it will look for /sys/config inside the\n  INITRD (or partition).\n\n  As this is a loader, it is not supposed to return control to the shell.\n\n";
    INTN argc;

    // Initialize UEFI Library
    InitializeLib(image, systab);
    BS = systab->BootServices;
    CI = systab->ConIn;

    // Parse command line arguments
    // BOOTBOOT.EFI [-?|-h|/?|/h] [initrd [config [key=value...]]
    argc = GetShellArgcArgv(image, &argv);
    if(argc>1) {
        if((argv[1][0]=='-'||argv[1][0]=='/')&&(argv[1][1]=='?'||argv[1][1]=='h')){
            Print(L"BOOTBOOT LOADER (build %s)\n\n%s",a2u(__DATE__),help);
            return EFI_SUCCESS;
        }
        initrdfile=argv[1];
    } else {
        initrdfile=L"\\BOOTBOOT\\INITRD";
    }
    if(argc>2) {
        configfile=argv[2];
    } else {
        configfile=L"\\BOOTBOOT\\CONFIG";
    }

    Print(L"Booting OS...\n");

    // get memory for bootboot structure
    uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 1, (EFI_PHYSICAL_ADDRESS*)&bootboot);
    if (bootboot == NULL)
        return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
    ZeroMem((void*)bootboot,PAGESIZE);
    CopyMem(bootboot->magic,BOOTBOOT_MAGIC,4);
    // unlike BIOS+MultiBoot bootboot, no need to check if we have
    // PAE + MSR + LME, as we're already in long mode.
    __asm__ __volatile__ (
        "mov $1, %%eax;"
        "cpuid;"
        "shrl $24, %%ebx;"
        "mov %%bx,%0"
        : "=b"(bootboot->bspid) : : );

    // locate InitRD in ROM
    DBG(L" * Locate initrd in Option ROMs%s\n",L"");
    RomTable = NULL; initrd.ptr = NULL; initrd.size = 0;
    status=EFI_LOAD_ERROR;
    // first, try RomTable
    LibGetSystemConfigurationTable(&RomTableGuid,(void *)&(RomTable));
    if(RomTable!=NULL) {
        for (i=0;i<RomTable->PciOptionRomCount;i++) {
            ret.ptr=(UINT8*)RomTable->PciOptionRomDescriptors[i].RomAddress;
            if(ret.ptr[0]==0x55 && ret.ptr[1]==0xAA && !CompareMem(ret.ptr+8,(const CHAR8 *)"INITRD",6)) {
                CopyMem(&initrd.size,ret.ptr+16,4);
                initrd.ptr=ret.ptr+32;
                status=EFI_SUCCESS;
                break;
            }
        }
    }
    //if not found, scan memory
    if(EFI_ERROR(status) || initrd.ptr==NULL){
        status = uefi_call_wrapper(BS->GetMemoryMap, 5,
            &memory_map_size, memory_map, NULL, &desc_size, NULL);
        if (status!=EFI_BUFFER_TOO_SMALL || memory_map_size==0) {
            return report(EFI_OUT_OF_RESOURCES,L"GetMemoryMap getSize");
        }
        memory_map_size+=2*desc_size;
        uefi_call_wrapper(BS->AllocatePages, 4, 0, 2,
            (memory_map_size+PAGESIZE-1)/PAGESIZE,
            (EFI_PHYSICAL_ADDRESS*)&memory_map);
        if (memory_map == NULL) {
            return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
        }
        status = uefi_call_wrapper(BS->GetMemoryMap, 5,
            &memory_map_size, memory_map, &map_key, &desc_size, &desc_version);
        status=EFI_LOAD_ERROR;
        for(mement=memory_map;
            mement<memory_map+memory_map_size;
            mement=NextMemoryDescriptor(mement,desc_size)) {
                if(mement==NULL || (mement->PhysicalStart==0 && mement->NumberOfPages==0))
                    break;
                // skip free and ACPI memory
                if(mement->Type==7||mement->Type==9||mement->Type==10)
                    continue;
                // according to spec, EFI Option ROMs must start on 512 bytes boundary, not 2048
                for(ret.ptr=(UINT8*)mement->PhysicalStart;
                    ret.ptr<(UINT8*)mement->PhysicalStart+mement->NumberOfPages*PAGESIZE;
                    ret.ptr+=512) {
                    if(ret.ptr[0]==0x55 && ret.ptr[1]==0xAA && !CompareMem(ret.ptr+8,(const CHAR8 *)"INITRD",6)) {
                        CopyMem(&initrd.size,ret.ptr+16,4);
                        initrd.ptr=ret.ptr+32;
                        status=EFI_SUCCESS;
                        goto foundinrom;
                    }
                }
        }
foundinrom:
        uefi_call_wrapper(BS->FreePages, 2, (EFI_PHYSICAL_ADDRESS)memory_map, (memory_map_size+PAGESIZE-1)/PAGESIZE);
    }
    // fall back to INITRD on filesystem
    if(EFI_ERROR(status) || initrd.ptr==NULL){
        // if the user presses any key now, we fallback to backup initrd
        for(i=0;i<500;i++) {
            if(!uefi_call_wrapper(BS->CheckEvent, 1, CI->WaitForKey)) {
                uefi_call_wrapper(CI->ReadKeyStroke, 2, CI, &key);
                Print(L" * Backup initrd\n");
                initrdfile=L"\\BOOTBOOT\\INITRD.BAK";
                break;
            }
            // delay 1ms
            uefi_call_wrapper(BS->Stall, 1, 1000);
        }
        DBG(L" * Locate initrd in %s\n",initrdfile);
        // Initialize FS with the DeviceHandler from loaded image protocol
        status = uefi_call_wrapper(BS->HandleProtocol,
                    3,
                    image,
                    &lipGuid,
                    (void **) &loaded_image);
        if (!EFI_ERROR(status) && loaded_image!=NULL) {
            status=EFI_LOAD_ERROR;
            RootDir = LibOpenRoot(loaded_image->DeviceHandle);
            // load ramdisk
            status=LoadFile(initrdfile,&initrd.ptr, &initrd.size);
        }
    }
    // if not found, try architecture specific initrd file
    if(EFI_ERROR(status) || initrd.ptr==NULL){
        initrdfile=L"\\BOOTBOOT\\X86_64";
        DBG(L" * Locate initrd in %s\n",initrdfile);
        status=LoadFile(initrdfile,&initrd.ptr, &initrd.size);
    }
    // if even that failed, look for a partition
    if(status!=EFI_SUCCESS || initrd.size==0){
        DBG(L" * Locate initrd in GPT%s\n",L"");
        status = uefi_call_wrapper(BS->LocateHandle, 5, ByProtocol, &bioGuid, NULL, &handle_size, handles);
        if (status!=EFI_BUFFER_TOO_SMALL || handle_size==0) {
            return report(EFI_OUT_OF_RESOURCES,L"LocateHandle getSize");
        }
        uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, (handle_size+PAGESIZE-1)/PAGESIZE, (EFI_PHYSICAL_ADDRESS*)&handles);
        if(handles==NULL)
            return report(EFI_OUT_OF_RESOURCES,L"AllocatePages\n");
        uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 1, (EFI_PHYSICAL_ADDRESS*)&initrd.ptr);
        if (initrd.ptr == NULL)
            return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
        lba_s=lba_e=0;
        status = uefi_call_wrapper(BS->LocateHandle, 5, ByProtocol, &bioGuid, NULL, &handle_size, handles);
        for(i=0;i<handle_size/sizeof(EFI_HANDLE);i++) {
            // we have to do it the hard way. HARDDRIVE_DEVICE_PATH does not return partition type or attribs...
            status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[i], &bioGuid, (void **) &bio);
            if(status!=EFI_SUCCESS || bio==NULL || bio->Media->BlockSize==0)
                continue;
            status=bio->ReadBlocks(bio, bio->Media->MediaId, 1, PAGESIZE, initrd.ptr);
            if(status!=EFI_SUCCESS || CompareMem(initrd.ptr,EFI_PTAB_HEADER_ID,8))
                continue;
            gptHdr = (EFI_PARTITION_TABLE_HEADER*)initrd.ptr;
            if(gptHdr->NumberOfPartitionEntries>127) gptHdr->NumberOfPartitionEntries=127;
            // first, look for a partition with bootable flag
            ret.ptr= (UINT8*)(initrd.ptr + (gptHdr->PartitionEntryLBA-1) * bio->Media->BlockSize);
            for(j=0;j<gptHdr->NumberOfPartitionEntries;j++) {
                gptEnt=(EFI_PARTITION_ENTRY*)ret.ptr;
                if((ret.ptr[0]==0 && ret.ptr[1]==0 && ret.ptr[2]==0 && ret.ptr[3]==0) || gptEnt->StartingLBA==0)
                    break;
                // use first partition with bootable flag as INITRD
                if((gptEnt->Attributes & EFI_PART_USED_BY_OS)) goto partfound;
                ret.ptr+=gptHdr->SizeOfPartitionEntry;
            }
            // if none, look for specific partition types
            ret.ptr= (UINT8*)(initrd.ptr + (gptHdr->PartitionEntryLBA-1) * bio->Media->BlockSize);
            for(j=0;j<gptHdr->NumberOfPartitionEntries;j++) {
                gptEnt=(EFI_PARTITION_ENTRY*)ret.ptr;
                if((ret.ptr[0]==0 && ret.ptr[1]==0 && ret.ptr[2]==0 && ret.ptr[3]==0) || gptEnt->StartingLBA==0)
                    break;
                    // use the first OS/Z root partition for this architecture
                if(!CompareMem(&gptEnt->PartitionTypeGUID.Data1,"OS/Z",4) &&
                    gptEnt->PartitionTypeGUID.Data2==0x8664 &&
                    !CompareMem(&gptEnt->PartitionTypeGUID.Data4[4],"root",4)) {
partfound:              lba_s=gptEnt->StartingLBA; lba_e=gptEnt->EndingLBA;
                        initrd.size = (((lba_e-lba_s)*bio->Media->BlockSize + PAGESIZE-1)/PAGESIZE)*PAGESIZE;
                        status=EFI_SUCCESS;
                        goto partok;
                }
                ret.ptr+=gptHdr->SizeOfPartitionEntry;
            }
        }
        return report(EFI_LOAD_ERROR,L"No boot partition");
partok:
        uefi_call_wrapper(BS->FreePages, 2, (EFI_PHYSICAL_ADDRESS)initrd.ptr, 1);
        if(initrd.size>0 && bio!=NULL) {
            uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, initrd.size/PAGESIZE, (EFI_PHYSICAL_ADDRESS*)&initrd.ptr);
            if (initrd.ptr == NULL)
                return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
            status=bio->ReadBlocks(bio, bio->Media->MediaId, lba_s, initrd.size, initrd.ptr);
        } else
            status=EFI_LOAD_ERROR;
    }
    if(status==EFI_SUCCESS && initrd.size>0){
        //check if initrd is gzipped
        if(initrd.ptr[0]==0x1f && initrd.ptr[1]==0x8b){
            unsigned char *addr,f;
            int len=0, r;
            TINF_DATA d;
            DBG(L" * Gzip compressed initrd @%lx %d bytes\n",initrd.ptr,initrd.size);
            // skip gzip header
            addr=initrd.ptr+2;
            if(*addr++!=8) goto gzerr;
            f=*addr++; addr+=6;
            if(f&4) { r=*addr++; r+=(*addr++ << 8); addr+=r; }
            if(f&8) { while(*addr++ != 0); }
            if(f&16) { while(*addr++ != 0); }
            if(f&2) addr+=2;
            d.source = addr;
            // allocate destination buffer
            CopyMem(&len,initrd.ptr+initrd.size-4,4);
            uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, (len+PAGESIZE-1)/PAGESIZE, (EFI_PHYSICAL_ADDRESS*)&addr);
            if(addr==NULL)
                return report(EFI_OUT_OF_RESOURCES,L"AllocatePages\n");
            // decompress
            d.bitcount = 0;
            d.bfinal = 0;
            d.btype = -1;
            d.dict_size = 0;
            d.dict_ring = NULL;
            d.dict_idx = 0;
            d.curlen = 0;
            d.dest = addr;
            d.destSize = len;
            do { r = uzlib_uncompress(&d); } while (!r);
            if (r != TINF_DONE) {
gzerr:          return report(EFI_COMPROMISED_DATA,L"Unable to uncompress");
            }
            // swap initrd.ptr with the uncompressed buffer
            // if it's not page aligned, we came from ROM, no FreePages
            if(((UINT64)initrd.ptr&(PAGESIZE-1))==0)
                uefi_call_wrapper(BS->FreePages, 2, (EFI_PHYSICAL_ADDRESS)initrd.ptr, (initrd.size+PAGESIZE-1)/PAGESIZE);
            initrd.ptr=addr;
            initrd.size=len;
        }
        DBG(L" * Initrd loaded @%lx %d bytes\n",initrd.ptr,initrd.size);
        kne=env.ptr=NULL;
        // if there's an environment file, load it
        if(loaded_image!=NULL && LoadFile(configfile,&env.ptr,&env.size)!=EFI_SUCCESS) {
            env.ptr=NULL;
        }
        if(env.ptr==NULL) {
            // if there were no environment file on boot partition, find it inside the INITRD
            j=0; ret.ptr=NULL; ret.size=0;
            while(ret.ptr==NULL && fsdrivers[j]!=NULL) {
                ret=(*fsdrivers[j++])((unsigned char*)initrd.ptr,cfgname);
            }
            if(ret.ptr!=NULL) {
                if(ret.size>PAGESIZE-1)
                    ret.size=PAGESIZE-1;
                uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 1, (EFI_PHYSICAL_ADDRESS*)&env.ptr);
                if(env.ptr==NULL)
                    return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
                ZeroMem((void*)env.ptr,PAGESIZE);
                CopyMem((void*)env.ptr,ret.ptr,ret.size);
                env.size=ret.size;
            }
        }
        if(env.ptr!=NULL) {
            ParseEnvironment(env.ptr,env.size, argc, argv);
        } else {
            // provide an empty environment for the OS
            env.size=0;
            uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 1, (EFI_PHYSICAL_ADDRESS*)&env.ptr);
            if (env.ptr == NULL) {
                return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
            }
            ZeroMem((void*)env.ptr,PAGESIZE);
            CopyMem((void*)env.ptr,"// N/A",8);
        }

        // get linear frame buffer
        status = GetLFB();
        if (EFI_ERROR(status) || bootboot->fb_width==0 || bootboot->fb_ptr==0)
                return report(status, L"GOP failed, no framebuffer");

        // collect information on system
        bootboot->protocol=PROTOCOL_STATIC | LOADER_UEFI;
        bootboot->size=128;
        bootboot->numcores=1;
        CopyMem((void *)&(bootboot->initrd_ptr),&initrd.ptr,8);
        bootboot->initrd_size=((initrd.size+PAGESIZE-1)/PAGESIZE)*PAGESIZE;
        CopyMem((void *)&(bootboot->arch.x86_64.efi_ptr),&systab,8);

        // System tables and structures
        DBG(L" * System tables%s\n",L"");
        sysptr = 0; LibGetSystemConfigurationTable(&AcpiTableGuid,(void *)&sysptr); bootboot->arch.x86_64.acpi_ptr = sysptr;
        sysptr = 0; LibGetSystemConfigurationTable(&SMBIOSTableGuid,(void *)&sysptr); bootboot->arch.x86_64.smbi_ptr = sysptr;
        sysptr = 0; LibGetSystemConfigurationTable(&MpsTableGuid,(void *)&sysptr); bootboot->arch.x86_64.mp_ptr = sysptr;

        // FIX ACPI table pointer on TianoCore...
        ret.ptr = (UINT8*)(bootboot->arch.x86_64.acpi_ptr);
        if(CompareMem(ret.ptr,(const CHAR8 *)"RSDT", 4) && CompareMem(ret.ptr,(const CHAR8 *)"XSDT", 4)) {
            // scan for the real rsd ptr, as AcpiTableGuid returns bad address
            for(i=1;i<256;i++) {
                if(!CompareMem(ret.ptr+i, (const CHAR8 *)"RSD PTR ", 8)){
                    ret.ptr+=i;
                    break;
                }
            }
            // get ACPI system table
            ACPI_RSDPTR *rsd = (ACPI_RSDPTR*)ret.ptr;
            if(rsd->xsdt!=0)
                bootboot->arch.x86_64.acpi_ptr = rsd->xsdt;
            else
                bootboot->arch.x86_64.acpi_ptr = (UINT64)((UINT32)rsd->rsdt);
        }

        // Date and time
        EFI_TIME t;
        uefi_call_wrapper(ST->RuntimeServices->GetTime, 2, &t, NULL);
        bootboot->datetime[0]=DecimaltoBCD(t.Year/100);
        bootboot->datetime[1]=DecimaltoBCD(t.Year%100);
        bootboot->datetime[2]=DecimaltoBCD(t.Month);
        bootboot->datetime[3]=DecimaltoBCD(t.Day);
        bootboot->datetime[4]=DecimaltoBCD(t.Hour);
        bootboot->datetime[5]=DecimaltoBCD(t.Minute);
        bootboot->datetime[6]=DecimaltoBCD(t.Second);
        bootboot->datetime[7]=DecimaltoBCD(t.Daylight);
        CopyMem((void *)&bootboot->timezone, &t.TimeZone, 2);
        if(bootboot->timezone<-1440||bootboot->timezone>1440)   // TZ in mins
            bootboot->timezone=0;
        DBG(L" * System time %d-%02d-%02d %02d:%02d:%02d GMT%s%d:%02d %s\n",
            t.Year,t.Month,t.Day,t.Hour,t.Minute,t.Second,
            bootboot->timezone>=0?L"+":L"",bootboot->timezone/60,bootboot->timezone%60,
            t.Daylight?L"summertime":L"");
        // get sys/core and parse
        status=LoadCore();
        if (EFI_ERROR(status))
            return status;
        if(kne!=NULL)
            *kne='\n';

        // Symmetric Multi Processing support
        status = uefi_call_wrapper(BS->LocateProtocol, 3, &mpspGuid, NULL, (void**)&mp);
        if(!EFI_ERROR(status)) {
            // override default values in bootboot struct
            status = uefi_call_wrapper(mp->GetNumberOfProcessors, 3, mp, &i, &j);
            if(!EFI_ERROR(status)) {
                // failsafe: we cannot map more stacks (each core has 1k)
                if(i>PAGESIZE/8/2*4) i=PAGESIZE/8/2*4;
                bootboot->numcores = i;
            }
            DBG(L" * SMP numcores %d\n", bootboot->numcores);
            // start APs
            status = uefi_call_wrapper(BS->CreateEvent, 5, 0, TPL_NOTIFY, NULL, NULL, &Event);
            if(!EFI_ERROR(status)) {
                for(i=0; i<bootboot->numcores; i++) {
                    status = uefi_call_wrapper(mp->GetProcessorInfo, 5, mp, i, pibuf);
                    if(!EFI_ERROR(status)) {
                        if(pibuf->StatusFlag & PROCESSOR_AS_BSP_BIT) {
                            bootboot->bspid = pibuf->ProcessorId;
                            bsp_num = i;
                        } else {
                            uefi_call_wrapper(mp->StartupThisAP, 7, mp, bootboot_startcore, i, Event, 0, (VOID*)i, NULL);
                        }
                    }
                }
            }
        }

        // query size of memory map
        status = uefi_call_wrapper(BS->GetMemoryMap, 5,
            &memory_map_size, memory_map, NULL, &desc_size, NULL);
        if (status!=EFI_BUFFER_TOO_SMALL || memory_map_size==0) {
            return report(EFI_OUT_OF_RESOURCES,L"GetMemoryMap getSize");
        }
        // allocate memory for memory descriptors. We assume that one or two new memory
        // descriptor may created by our next allocate calls and we round up to page size
        memory_map_size+=2*desc_size;
        uefi_call_wrapper(BS->AllocatePages, 4, 0, 2,
            (memory_map_size+PAGESIZE-1)/PAGESIZE,
            (EFI_PHYSICAL_ADDRESS*)&memory_map);
        if (memory_map == NULL) {
            return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
        }

        // create page tables
        uefi_call_wrapper(BS->AllocatePages, 4, 0, 2, 23+(bootboot->numcores+3)/4, (EFI_PHYSICAL_ADDRESS*)&paging);
        if (paging == NULL) {
            return report(EFI_OUT_OF_RESOURCES,L"AllocatePages");
        }
        ZeroMem((void*)paging,23*PAGESIZE);
        DBG(L" * Pagetables PML4 @%lx\n",paging);
        //PML4
        paging[0]=(UINT64)((UINT8 *)paging+4*PAGESIZE)+3;   // pointer to 2M PDPE (16G RAM identity mapped)
        paging[511]=(UINT64)((UINT8 *)paging+PAGESIZE)+3;   // pointer to 4k PDPE (core mapped at -2M)
        //4k PDPE
        paging[512+511]=(UINT64)((UINT8 *)paging+2*PAGESIZE+3);
        //4k PDE
        for(i=0;i<31;i++)
            paging[2*512+480+i]=(UINT64)(((UINT8 *)(bootboot->fb_ptr)+(i<<21))+0x83);   //map framebuffer
        paging[2*512+511]=(UINT64)((UINT8 *)paging+3*PAGESIZE+3);
        //4k PT
        paging[3*512+0]=(UINT64)(bootboot)+1;
        paging[3*512+1]=(UINT64)(env.ptr)+1;
        for(i=0;i<(core.size/PAGESIZE);i++)
            paging[3*512+2+i]=(UINT64)((UINT8 *)core.ptr+i*PAGESIZE+3);
        for(i=0; i<(UINTN)((bootboot->numcores+3)/4); i++)
            paging[3*512+511-i]=(UINT64)((UINT8 *)paging+(23+i)*PAGESIZE+3);  // core stacks
        //identity mapping
        //2M PDPE
        for(i=0;i<16;i++)
            paging[4*512+i]=(UINT64)((UINT8 *)paging+(7+i)*PAGESIZE+3);
        //first 2M mapped per page
        paging[7*512]=(UINT64)((UINT8 *)paging+5*PAGESIZE+3);
        for(i=0;i<512;i++)
            paging[5*512+i]=(UINT64)(i*PAGESIZE+3);
        //2M PDE
        for(i=1;i<512*16;i++)
            paging[7*512+i]=(UINT64)((i<<21)+0x83);

        // Get memory map
        int cnt=3;
get_memory_map:
        DBG(L" * Memory Map @%lx %d bytes #%d\n",memory_map, memory_map_size, 4-cnt);
        mmapent=(MMapEnt *)&(bootboot->mmap);
        status = uefi_call_wrapper(BS->GetMemoryMap, 5,
            &memory_map_size, memory_map, &map_key, &desc_size, &desc_version);
        if (EFI_ERROR(status)) {
            return report(status,L"GetMemoryMap");
        }
        last=NULL;
        for(mement=memory_map;
            mement<memory_map+memory_map_size;
            mement=NextMemoryDescriptor(mement,desc_size)) {
            // failsafe
            if(mement==NULL || bootboot->size>=PAGESIZE-128 ||
                (mement->PhysicalStart==0 && mement->NumberOfPages==0))
                break;
            // failsafe, don't report our own structures as free
            if( mement->NumberOfPages==0 ||
                ((mement->PhysicalStart <= (UINT64)bootboot &&
                    mement->PhysicalStart+(mement->NumberOfPages*PAGESIZE) > (UINT64)bootboot) ||
                 (mement->PhysicalStart <= (UINT64)env.ptr &&
                    mement->PhysicalStart+(mement->NumberOfPages*PAGESIZE) > (UINT64)env.ptr) ||
                 (mement->PhysicalStart <= (UINT64)initrd.ptr &&
                    mement->PhysicalStart+(mement->NumberOfPages*PAGESIZE) > (UINT64)initrd.ptr) ||
                 (mement->PhysicalStart <= (UINT64)core.ptr &&
                    mement->PhysicalStart+(mement->NumberOfPages*PAGESIZE) > (UINT64)core.ptr) ||
                 (mement->PhysicalStart <= (UINT64)paging &&
                    mement->PhysicalStart+(mement->NumberOfPages*PAGESIZE) > (UINT64)paging)
                )) {
                    continue;
            }
            mmapent->ptr=mement->PhysicalStart;
            mmapent->size=(mement->NumberOfPages*PAGESIZE)+
                ((mement->Type>0&&mement->Type<5)||mement->Type==7?MMAP_FREE:
                (mement->Type==9 || mement->Type==10 || (bootboot->arch.x86_64.acpi_ptr >= mmapent->ptr &&
                    bootboot->arch.x86_64.acpi_ptr < mmapent->ptr+mement->NumberOfPages*PAGESIZE)?MMAP_ACPI:
                (mement->Type==11||mement->Type==12?MMAP_MMIO:
                MMAP_USED)));
            // merge continous areas of the same type
            if(last!=NULL &&
                MMapEnt_Type(last) == MMapEnt_Type(mmapent) &&
                MMapEnt_Ptr(last)+MMapEnt_Size(last) == MMapEnt_Ptr(mmapent)) {
                    last->size+=MMapEnt_Size(mmapent);
                    mmapent->ptr=mmapent->size=0;
            } else {
                last=mmapent;
                bootboot->size+=16;
                mmapent++;
            }
        }
        // --- NO PRINT AFTER THIS POINT ---

        //inform firmware that we're about to leave it's realm
        status = uefi_call_wrapper(BS->ExitBootServices, 2, image, map_key);
        if(EFI_ERROR(status)){
            cnt--;
            if(cnt>0) goto get_memory_map;
            return report(status,L"ExitBootServices");
        }

        // release AP spinlock
        bsp_done = 1;
        bootboot_startcore((VOID*)bsp_num);
    }
    return report(status,L"Initrd not found");
}

