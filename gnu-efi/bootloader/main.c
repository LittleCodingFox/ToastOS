#include <efi.h>
#include <efilib.h>
#include <elf.h>

typedef unsigned long long size_t;
typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

typedef struct {

	void* BaseAddress;
	size_t BufferSize;
	uint32_t Width;
	uint32_t Height;
	uint32_t PixelsPerScanLine;
} Framebuffer;

#define PSF2_MAGIC0 0x72
#define PSF2_MAGIC1 0xb5
#define PSF2_MAGIC2 0x4a
#define PSF2_MAGIC3 0x86

typedef struct
{
  uint32_t magic;
  uint32_t version;
  uint32_t header_size;
  uint32_t flags;
  uint32_t num_glyph;
  uint32_t bytes_per_glyph;
  uint32_t height;
  uint32_t width;
} __attribute__((packed)) psf2_header_t;

typedef struct
{
  psf2_header_t* header;
  void* glyph_buffer;
} psf2_font_t;

typedef struct {
	uint32_t width;
	uint32_t height;
} psf2_size_t;

Framebuffer framebuffer;

Framebuffer* InitializeGOP() {

	EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	EFI_STATUS status;

	status = uefi_call_wrapper(BS->LocateProtocol, 3, &gopGuid, NULL, (void**)&gop);

	if(EFI_ERROR(status)) {

		Print(L"Unable to locate GOP\n\r");

		return NULL;
	}
	else
	{
		Print(L"GOP located\n\r");
	}

	framebuffer.BaseAddress = (void*)gop->Mode->FrameBufferBase;
	framebuffer.BufferSize = gop->Mode->FrameBufferSize;
	framebuffer.Width = gop->Mode->Info->HorizontalResolution;
	framebuffer.Height = gop->Mode->Info->VerticalResolution;
	framebuffer.PixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;

	return &framebuffer;
}

EFI_FILE* LoadFile(EFI_FILE* Directory, CHAR16* Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {

	EFI_FILE* LoadedFile;

	EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
	SystemTable->BootServices->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
	SystemTable->BootServices->HandleProtocol(LoadedImage->DeviceHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&FileSystem);

	if (Directory == NULL) {
		
		FileSystem->OpenVolume(FileSystem, &Directory);
	}

	EFI_STATUS s = Directory->Open(Directory, &LoadedFile, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);

	if (s != EFI_SUCCESS) {

		return NULL;
	}

	return LoadedFile;

}

psf2_font_t* LoadPSF2Font(EFI_FILE* Directory, CHAR16* Path, EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
	EFI_FILE* font = LoadFile(Directory, Path, ImageHandle, SystemTable);

	if (font == NULL) return NULL;

	psf2_header_t* fontHeader;

	SystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(psf2_header_t), (void**)&fontHeader);

	UINTN size = sizeof(psf2_header_t);

	font->Read(font, &size, fontHeader);

	uint8_t *magic = (uint8_t *)&fontHeader->magic;

	if (magic[0] != PSF2_MAGIC0 ||
		magic[1] != PSF2_MAGIC1 ||
		magic[2] != PSF2_MAGIC2 ||
		magic[3] != PSF2_MAGIC3) {

		return NULL;
	}

	UINTN glyphBufferSize = fontHeader->bytes_per_glyph * fontHeader->num_glyph;

	font->SetPosition(font, fontHeader->header_size);

	void* glyphBuffer;
	{
		SystemTable->BootServices->AllocatePool(EfiLoaderData, glyphBufferSize, (void**)&glyphBuffer);
		font->Read(font, &glyphBufferSize, glyphBuffer);
	}

	psf2_font_t* finishedFont;

	SystemTable->BootServices->AllocatePool(EfiLoaderData, sizeof(psf2_font_t), (void**)&finishedFont);
	finishedFont->header = fontHeader;
	finishedFont->glyph_buffer = glyphBuffer;

	return finishedFont;
}

void EFIPrintCallback(const uint16_t *str)
{
	Print(L"CALLBACK\n");
	Print(L"%s\n", str);
}

int memcmp(const void* aptr, const void* bptr, size_t n){

	const unsigned char* a = aptr, *b = bptr;

	for (size_t i = 0; i < n; i++){

		if (a[i] < b[i]) return -1;
		else if (a[i] > b[i]) return 1;
	}

	return 0;
}

typedef struct {

	Framebuffer* framebuffer;
	psf2_font_t* psf2_Font;
	EFI_MEMORY_DESCRIPTOR* mMap;
	UINTN mMapSize;
	UINTN mMapDescSize;
} BootInfo;

EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {

	InitializeLib(ImageHandle, SystemTable);

	EFI_FILE* Kernel = LoadFile(NULL, L"kernel.elf", ImageHandle, SystemTable);

	if (Kernel == NULL){

		Print(L"Could not load kernel \n\r");

	} else {

		Print(L"Kernel Loaded Successfully \n\r");
	}

	Elf64_Ehdr header;
	{
		UINTN FileInfoSize;
		EFI_FILE_INFO* FileInfo;
		Kernel->GetInfo(Kernel, &gEfiFileInfoGuid, &FileInfoSize, NULL);
		SystemTable->BootServices->AllocatePool(EfiLoaderData, FileInfoSize, (void**)&FileInfo);
		Kernel->GetInfo(Kernel, &gEfiFileInfoGuid, &FileInfoSize, (void**)&FileInfo);

		UINTN size = sizeof(header);
		Kernel->Read(Kernel, &size, &header);
	}

	if (
		memcmp(&header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		header.e_ident[EI_CLASS] != ELFCLASS64 ||
		header.e_ident[EI_DATA] != ELFDATA2LSB ||
		header.e_type != ET_EXEC ||
		header.e_machine != EM_X86_64 ||
		header.e_version != EV_CURRENT
	)
	{
		Print(L"kernel format is bad\r\n");
	}
	else
	{
		Print(L"kernel header successfully verified\r\n");
	}

	Elf64_Phdr* phdrs;
	{
		Kernel->SetPosition(Kernel, header.e_phoff);
		UINTN size = header.e_phnum * header.e_phentsize;
		SystemTable->BootServices->AllocatePool(EfiLoaderData, size, (void**)&phdrs);
		Kernel->Read(Kernel, &size, phdrs);
	}

	for (
		Elf64_Phdr* phdr = phdrs;
		(char*)phdr < (char*)phdrs + header.e_phnum * header.e_phentsize;
		phdr = (Elf64_Phdr*)((char*)phdr + header.e_phentsize)
	)
	{
		switch (phdr->p_type){
			case PT_LOAD:
			{
				int pages = (phdr->p_memsz + 0x1000 - 1) / 0x1000;
				Elf64_Addr segment = phdr->p_paddr;
				SystemTable->BootServices->AllocatePages(AllocateAddress, EfiLoaderData, pages, &segment);

				Kernel->SetPosition(Kernel, phdr->p_offset);
				UINTN size = phdr->p_filesz;
				Kernel->Read(Kernel, &size, (void*)segment);
				break;
			}
		}
	}

	Print(L"Kernel Loaded\n\r");

	psf2_font_t* newFont = LoadPSF2Font(NULL, L"font.psf", ImageHandle, SystemTable);

	if (newFont == NULL)
	{
		Print(L"Font is not valid or is not found\n\r");
	}
	else
	{
		Print(L"Font found. char size = %d\n\r", newFont->header->height);
	}

	Framebuffer* newBuffer = InitializeGOP();

	Print(L"Base: 0x%x\n\rSize: 0x%x\n\rWidth: %d\n\rHeight: %d\n\rPixelsPerScanline: %d\n\r", 
	newBuffer->BaseAddress, 
	newBuffer->BufferSize, 
	newBuffer->Width, 
	newBuffer->Height, 
	newBuffer->PixelsPerScanLine);

	EFI_MEMORY_DESCRIPTOR* Map = NULL;
	UINTN MapSize, MapKey;
	UINTN DescriptorSize;
	UINT32 DescriptorVersion;
	{
		SystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
		SystemTable->BootServices->AllocatePool(EfiLoaderData, MapSize, (void**)&Map);
		SystemTable->BootServices->GetMemoryMap(&MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
	}

	void (*KernelStart)(BootInfo*) = ((__attribute__((sysv_abi)) void (*)(BootInfo*) ) header.e_entry);

	BootInfo bootInfo;
	bootInfo.framebuffer = newBuffer;
	bootInfo.psf2_Font = newFont;
	bootInfo.mMap = Map;
	bootInfo.mMapSize = MapSize;
	bootInfo.mMapDescSize = DescriptorSize;

	Print(L"Memory Map located at %x\n", Map);
	Print(L"Font ptr: %x\n", newFont);

	SystemTable->BootServices->ExitBootServices(ImageHandle, MapKey);

	KernelStart(&bootInfo);

	return EFI_SUCCESS; // Exit the UEFI application
}