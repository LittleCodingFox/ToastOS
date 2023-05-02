#include <string.h>
#include "Panic.hpp"
#include "AHCIDriver.hpp"
#include "paging/PageFrameAllocator.hpp"
#include "paging/PageTableManager.hpp"

namespace Drivers
{
    namespace AHCI
    {
        #define ATA_DEV_BUSY            0x80
        #define ATA_DEV_DRQ             0x08
        #define ATA_CMD_READ_DMA_EX     0x25
        #define ATA_CMD_WRITE_DMA_EX    0x35
        #define ATA_CMD_IDENTIFY        0xEC
        #define HBA_PxIS_TFES           (1 << 30)
        #define HBA_PORT_DEV_PRESENT    0x3
        #define HBA_PORT_IPM_ACTIVE     0x1
        #define SATA_SIG_ATAPI          0xEB140101
        #define SATA_SIG_ATA            0x00000101
        #define SATA_SIG_SEMB           0xC33C0101
        #define SATA_SIG_PM             0x96690101

        #define HBA_PxCMD_CR            0x8000
        #define HBA_PxCMD_FRE           0x0010
        #define HBA_PxCMD_ST            0x0001
        #define HBA_PxCMD_FR            0x4000

        #define READ_AHEAD_SECTORS      8192

        const int AHCISectorSize = 512;
        const int PRDTMaxSize = 4194304;
        const int PRDTMaxSectors = PRDTMaxSize / AHCISectorSize;

        inline uint16_t PRDTLength(uint64_t sectorCount)
        {
            uint64_t t = sectorCount * 512;

            return t / PRDTMaxSize + (t % PRDTMaxSize != 0 ? 1 : 0);
        }

        class AHCIHolder
        {
        public:
            AHCIHolder() : portCount(0) {}

            void ProbePorts();

            PCIDevice *device;
            volatile HBAMemory *ABAR;
            AHCIDriver *ports[32];
            uint8_t portCount;
        };

        void InitializeCommandHeader(volatile HBACommandHeader *commandHeader, bool write, uint64_t count)
        {
            volatile HBACommandHeader *higherCommandHeader = (volatile HBACommandHeader *)TranslateToHighHalfMemoryAddress((uint64_t)commandHeader);

            higherCommandHeader->commandFISLength = sizeof(FISREGHost2Device) / sizeof(uint32_t);
            higherCommandHeader->write = write;
            higherCommandHeader->prdtLength = PRDTLength(count);
        }

        void InitializeCommandTable(volatile HBACommandTable *commandTable, volatile HBACommandHeader *commandHeader, 
            uintptr_t dataAddress, size_t &count)
        {
            volatile HBACommandTable *higherCommandTable = (volatile HBACommandTable *)TranslateToHighHalfMemoryAddress((uint64_t)commandTable);
            volatile HBACommandHeader *higherCommandHeader = (volatile HBACommandHeader *)TranslateToHighHalfMemoryAddress((uint64_t)commandHeader);

            memset((void *)higherCommandTable, 0, sizeof(HBACommandTable) + (higherCommandHeader->prdtLength - 1) * sizeof(HBAPRDTEntry));

            if(IsKernelHigherHalf(dataAddress))
            {
                dataAddress = TranslateToKernelPhysicalMemoryAddress(dataAddress);
            }
            else if(IsHigherHalf(dataAddress))
            {
                dataAddress = TranslateToPhysicalMemoryAddress(dataAddress);
            }

            uint64_t i = 0;

            for(i = 0; i < higherCommandHeader->prdtLength - 1; i++)
            {
                higherCommandTable->prdtEntry[i].dataBaseAddress = (uint64_t)dataAddress;
                higherCommandTable->prdtEntry[i].dataBaseAddressUpper = ((uint64_t)dataAddress >> 32);
                higherCommandTable->prdtEntry[i].interruptOnCompletion = 1;
                higherCommandTable->prdtEntry[i].byteCount = PRDTMaxSize - 1;

                dataAddress += PRDTMaxSize;
                count -= PRDTMaxSectors;
            }

            higherCommandTable->prdtEntry[i].dataBaseAddress = (uint64_t)dataAddress;
            higherCommandTable->prdtEntry[i].dataBaseAddressUpper = ((uint64_t)dataAddress >> 32);
            higherCommandTable->prdtEntry[i].interruptOnCompletion = 1;
            higherCommandTable->prdtEntry[i].byteCount = (count << 9) - 1;
        }

        void InitializeFISRegCommand(volatile FISREGHost2Device *command, bool write, uintptr_t sector, size_t count)
        {
            volatile FISREGHost2Device *higherCommand = (volatile FISREGHost2Device *)TranslateToHighHalfMemoryAddress((uint64_t)command);

            memset((void *)higherCommand, 0, sizeof(FISREGHost2Device));

            higherCommand->fisType = FIS_TYPE_REG_H2D;

            higherCommand->commandControl = 1;

            if(write)
            {
                higherCommand->command = ATA_CMD_WRITE_DMA_EX;
            }
            else
            {
                higherCommand->command = ATA_CMD_READ_DMA_EX;
            }

            higherCommand->lba0 = (uint8_t)((sector & 0x000000000000FF));
            higherCommand->lba1 = (uint8_t)((sector & 0x0000000000FF00) >> 8);
            higherCommand->lba2 = (uint8_t)((sector & 0x00000000FF0000) >> 16);

            higherCommand->deviceRegister = 1 << 6;

            higherCommand->lba3 = (uint8_t)((sector & 0x000000FF000000) >> 24);
            higherCommand->lba4 = (uint8_t)((sector & 0x0000FF00000000) >> 32);
            higherCommand->lba5 = (uint8_t)((sector & 0x00FF0000000000) >> 40);

            higherCommand->countLow = count & 0xFF;
            higherCommand->countHigh = (count >> 8) & 0xFF;
        }

        bool ATABusyWait(volatile HBAPort *port)
        {
            size_t spinTimeout = 0;

            while((port->taskFileData & (ATA_DEV_BUSY | ATA_DEV_DRQ)) && spinTimeout < 1000000)
            {
                spinTimeout++;
            }

            if(spinTimeout == 1000000)
            {
                return false;
            }

            return true;
        }

        int32_t FindCommandSlot(volatile HBAPort *port)
        {
            uint32_t slots = (port->sataControl | port->commandIssue);

            for(int i = 0; i < 32; i++, slots >>= 1)
            {
                if((slots & 1) == 0)
                {
                    return i;
                }
            }

            return -1;
        }

        PortType CheckPortType(volatile HBAPort *port)
        {
            uint32_t sataStatus = port->sataStatus;

            uint8_t interfacePowerManagement = (sataStatus >> 8) & 0x0F;
            uint8_t deviceDetection = sataStatus & 0x0F;

            if(deviceDetection != HBA_PORT_DEV_PRESENT ||
                interfacePowerManagement != HBA_PORT_IPM_ACTIVE)
            {
                return AHCI_PORT_TYPE_NONE;
            }

            switch(port->signature)
            {
                case SATA_SIG_ATAPI:
                    return AHCI_PORT_TYPE_SATAPI;

                case SATA_SIG_ATA:
                    return AHCI_PORT_TYPE_SATA;

                case SATA_SIG_PM:
                    return AHCI_PORT_TYPE_PM;

                case SATA_SIG_SEMB:
                    return AHCI_PORT_TYPE_SEMB;

                default:
                    return AHCI_PORT_TYPE_SATA;
            }
        }

        void AHCIHolder::ProbePorts()
        {
            uint32_t implementedPorts = ABAR->implementedPorts;

            for(uint32_t i = 0; i < 32; i++, implementedPorts >>= 1)
            {
                if(implementedPorts & 1)
                {
                    PortType type = CheckPortType(&ABAR->ports[i]);

                    if(type == AHCI_PORT_TYPE_SATA || type == AHCI_PORT_TYPE_SATAPI)
                    {
                        AHCIDriver *port = new AHCIDriver();

                        ports[portCount] = port;

                        port->device = device;
                        port->portType = type;
                        port->port = (volatile HBAPort *)&ABAR->ports[i];
                        port->portNumber = portCount;

                        portCount++;
                    }
                }
            }
        }

        void FISIdentify::ReadModel(char *buffer) const
        {
            memcpy(buffer, model, sizeof(model));

            buffer[40] = 0;

            for(uint32_t i = 0; i < 40; i+=2)
            {
                char c = buffer[i + 1];
                buffer[i + 1] = buffer[i];
                buffer[i] = c;
            }

            char before = '\0';

            for(uint32_t i = 0; i < 40; i++)
            {
                if(buffer[i] == ' ')
                {
                    if(before == ' ')
                    {
                        buffer[i] = '\0';

                        break;
                    }
                    else
                    {
                        before = buffer[i];
                    }
                }
            }
        }

        AHCIDriver::AHCIDriver()
        {
            memset(&identify, 0, sizeof(identify));
            memset(model, 0, sizeof(model));
        }

        bool AHCIDriver::Configure()
        {
            void *newBase = globalAllocator.RequestPage();

            globalPageTableManager->MapMemory((void *)TranslateToHighHalfMemoryAddress((uint64_t)newBase),
                newBase, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

            memset((void *)TranslateToHighHalfMemoryAddress((uint64_t)newBase), 0, 0x1000);

            port->commandListBase = (uint64_t)newBase;

            HBAFIS *fis = new HBAFIS();

            memset(fis, 0, sizeof(HBAFIS));

            fis->dmaFIS.type = FIS_TYPE_DMA_SETUP;
            fis->linkFIS.type = FIS_TYPE_REG_D2H;
            fis->pioFIS.type = FIS_TYPE_PIO_SETUP;

            port->fisBaseAddress = (uint64_t)TranslateToPhysicalMemoryAddress((uint64_t)fis);

            volatile HBACommandHeader *commandHeader = (HBACommandHeader *)TranslateToHighHalfMemoryAddress((uint64_t)newBase);

            for(uint32_t i = 0; i < 32; i++)
            {
                commandHeader[i].prdtLength = 8;

                uint64_t tableSize = sizeof(HBACommandTable) + READ_AHEAD_SECTORS * sizeof(HBAPRDTEntry);
                tableSize = tableSize / 0x1000 + (tableSize % 0x1000 ? 1 : 0);

                void *commandTableAddress = globalAllocator.RequestPages(tableSize);

                auto higher = (void *)TranslateToHighHalfMemoryAddress((uint64_t)commandTableAddress);

                for(uint64_t j = 0; j < tableSize; j++)
                {
                    globalPageTableManager->MapMemory((uint8_t *)higher + j * 0x1000, (uint8_t *)commandTableAddress + j * 0x1000,
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
                }
                
                memset(higher, 0, tableSize * 0x1000);

                commandHeader[i].commandTableBaseAddress = (uint64_t)commandTableAddress;
            }

            if(!Identify())
            {
                memset(&identify, 0, sizeof(identify));

                return false;
            }

            return true;
        }

        bool AHCIDriver::Identify()
        {
            port->interruptStatus = (uint32_t)-1;

            int32_t slot = FindCommandSlot(port);

            if(slot == -1)
            {
                return false;
            }

            volatile HBACommandHeader *commandHeader = (HBACommandHeader *)((uint64_t)port->commandListBase);
            commandHeader += slot;

            InitializeCommandHeader(commandHeader, false, 1);

            volatile HBACommandTable *commandTable = (HBACommandTable *)(uint64_t)((uint64_t)((volatile HBACommandHeader *)
                TranslateToHighHalfMemoryAddress((uint64_t)commandHeader))->commandTableBaseAddress);

            size_t i = 0;

            InitializeCommandTable(commandTable, commandHeader, (uintptr_t)&identify, i);

            volatile FISREGHost2Device *commandFIS = (FISREGHost2Device *)((uint64_t)TranslateToPhysicalMemoryAddress((uint64_t)((volatile HBACommandTable *)
                TranslateToHighHalfMemoryAddress((uint64_t)commandTable))->commandFIS));

            InitializeFISRegCommand(commandFIS, false, 0, 0);

            ((volatile FISREGHost2Device *)TranslateToHighHalfMemoryAddress((uint64_t)commandFIS))->command = ATA_CMD_IDENTIFY;

            if(!ATABusyWait(port))
            {
                return false;
            }

            StartCommand();

            port->commandIssue = 1 << slot;

            for(;;)
            {
                if((port->commandIssue & (1 << slot)) == 0)
                {
                    break;
                }

                if(port->interruptStatus & HBA_PxIS_TFES)
                {
                    return false;
                }
            }

            if(port->interruptStatus & HBA_PxIS_TFES)
            {
                return false;
            }

            StopCommand();

            identify.ReadModel(model);

            char *size = (char *)"bytes";
            uint64_t diskSize = identify.maxLBA48 * AHCISectorSize;

            while(diskSize >= 1024)
            {
                diskSize /= 1024;

                if(strcmp(size, "bytes") == 0)
                {
                    size = (char *)"KB";
                } 
                else if(strcmp(size, "KB") == 0)
                {
                    size = (char *)"MB";
                }
                else if(strcmp(size, "MB") == 0)
                {
                    size = (char *)"GB";
                }
                else if(strcmp(size, "GB") == 0)
                {
                    size = (char *)"TB";

                    break;
                } 
            }

            DEBUG_OUT("[pci] ahci: Identified model %s with size %lu%s", model, diskSize, size);

            return true;
        }

        void AHCIDriver::StartCommand()
        {
            port->commandStatus &= ~HBA_PxCMD_ST;

            for(;;)
            {
                if((port->commandStatus & HBA_PxCMD_CR))
                {
                    continue;
                }

                break;
            }

            port->commandStatus |= HBA_PxCMD_FRE;
            port->commandStatus |= HBA_PxCMD_ST;
        }

        void AHCIDriver::StopCommand()
        {
            port->commandStatus &= ~HBA_PxCMD_ST;

            for(;;)
            {
                if((port->commandStatus & HBA_PxCMD_CR))
                {
                    continue;
                }

                break;
            }

            port->commandStatus &= ~HBA_PxCMD_FRE;
        }

        bool AHCIDriver::ReadInternal(void *buffer, uint64_t sector, uint64_t sectorCount)
        {
            if(sector >= identify.maxLBA48)
            {
                return false;
            }

            if(sector + sectorCount > identify.maxLBA48)
            {
                sectorCount = identify.maxLBA48 - sector;
            }

            port->interruptStatus = (uint32_t)-1;

            int32_t slot = FindCommandSlot(port);

            if(slot == -1)
            {
                return false;
            }

            volatile HBACommandHeader *commandHeader = (HBACommandHeader *)((uint64_t)port->commandListBase);
            commandHeader += slot;

            InitializeCommandHeader(commandHeader, false, sectorCount);

            volatile HBACommandTable *commandTable = (HBACommandTable *)(uint64_t)((uint64_t)((volatile HBACommandHeader *)
                TranslateToHighHalfMemoryAddress((uint64_t)commandHeader))->commandTableBaseAddress);

            InitializeCommandTable(commandTable, commandHeader, (uintptr_t)buffer, sectorCount);

            volatile FISREGHost2Device *commandFIS = (FISREGHost2Device *)((uint64_t)TranslateToPhysicalMemoryAddress((uint64_t)((volatile HBACommandTable *)
                TranslateToHighHalfMemoryAddress((uint64_t)commandTable))->commandFIS));

            InitializeFISRegCommand(commandFIS, false, sector, sectorCount);

            if(!ATABusyWait(port))
            {
                return false;
            }

            StartCommand();

            port->commandIssue = 1 << slot;

            for(;;)
            {
                if((port->commandIssue & (1 << slot)) == 0)
                {
                    break;
                }

                if(port->interruptStatus & HBA_PxIS_TFES)
                {
                    return false;
                }
            }

            if(port->interruptStatus & HBA_PxIS_TFES)
            {
                return false;
            }

            StopCommand();

            return true;
        }

        bool AHCIDriver::Read(void *buffer, uint64_t sector, uint64_t sectorCount)
        {
            for(uint64_t i = 0; i < sectorCount; i++)
            {
                auto cacheBuffer = diskCache.Get(sector + i);

                if(cacheBuffer != NULL)
                {
                    memcpy((uint8_t *)buffer + i * AHCISectorSize, cacheBuffer, AHCISectorSize);
                }
                else
                {
                    uint64_t readAheadSectors = READ_AHEAD_SECTORS;

                    if(sector + readAheadSectors >= identify.maxLBA48)
                    {
                        readAheadSectors = identify.maxLBA48 - sector;
                    }

                    uint8_t *readBuffer = new uint8_t[readAheadSectors * AHCISectorSize];

                    if(!ReadInternal(readBuffer, sector + i, readAheadSectors))
                    {
                        delete [] readBuffer;

                        return false;
                    }

                    auto owner = new DiskCache::OwnerInfo();
                    owner->buffer = readBuffer;

                    for(uint64_t j = 0; j < readAheadSectors; j++)
                    {
                        diskCache.Set(sector + i + j, readBuffer + j * AHCISectorSize, owner);
                    }

                    cacheBuffer = diskCache.Get(sector + i);

                    if(cacheBuffer == NULL)
                    {
                        return false;
                    }

                    memcpy((uint8_t *)buffer + i * AHCISectorSize, cacheBuffer, AHCISectorSize);
                }
            }

            return true;
        }

        bool AHCIDriver::WriteInternal(const void *buffer, uint64_t sector, uint64_t sectorCount)
        {
            port->interruptStatus = (uint32_t)-1;

            int32_t slot = FindCommandSlot(port);

            if(slot == -1)
            {
                return false;
            }

            volatile HBACommandHeader *commandHeader = (volatile HBACommandHeader *)(uint64_t)(port->commandListBase);
            commandHeader += slot;

            InitializeCommandHeader(commandHeader, true, sectorCount);

            volatile HBACommandTable *commandTable = (HBACommandTable *)(uint64_t)((uint64_t)((volatile HBACommandHeader *)
                TranslateToHighHalfMemoryAddress((uint64_t)commandHeader))->commandTableBaseAddress);

            InitializeCommandTable(commandTable, commandHeader, (uintptr_t)buffer, sectorCount);

            volatile FISREGHost2Device *commandFIS = (FISREGHost2Device *)((uint64_t)TranslateToPhysicalMemoryAddress((uint64_t)((volatile HBACommandTable *)
                TranslateToHighHalfMemoryAddress((uint64_t)commandTable))->commandFIS));

            InitializeFISRegCommand(commandFIS, true, sector, sectorCount);

            if(!ATABusyWait(port))
            {
                return false;
            }

            StartCommand();

            port->commandIssue = 1 << slot;

            for(;;)
            {
                if((port->commandIssue & (1 << slot)) == 0)
                {
                    break;
                }

                if(port->interruptStatus & HBA_PxIS_TFES)
                {
                    return false;
                }
            }

            if(port->interruptStatus & HBA_PxIS_TFES)
            {
                return false;
            }

            StopCommand();

            return true;
        }

        bool AHCIDriver::Write(const void *buffer, uint64_t sector, uint64_t sectorCount)
        {
            if(sector + sectorCount >= identify.maxLBA48)
            {
                return false;
            }

            for(uint64_t i = 0; i < sectorCount; i++)
            {
                auto item = diskCache.Get(sector + i);

                if(item != NULL)
                {
                    memcpy(item, (uint8_t *)buffer + i * AHCISectorSize, AHCISectorSize);
                }
                else
                {
                    auto owner = new DiskCache::OwnerInfo();
                    uint8_t *cacheBuffer = new uint8_t[AHCISectorSize];

                    memcpy(cacheBuffer, (uint8_t *)buffer + i * AHCISectorSize, AHCISectorSize);

                    owner->buffer = cacheBuffer;

                    diskCache.Set(sector + i, cacheBuffer, owner);
                }
            }

            if(!WriteInternal(buffer, sector, sectorCount))
            {
                return false;
            }

            return true;
        }

        void PrepareDevice(PCIDevice *device)
        {
            DEBUG_OUT("%s", "[pci] Initializing AHCI device");

            auto bar = device->FetchBar(5);

            if(!bar)
            {
                DEBUG_OUT("%s", "[pci] Failed to initialize AHCI driver: Missing bar 5");

                return;
            }

            AHCIHolder holder;

            holder.device = device;
            holder.ABAR = (volatile HBAMemory *)(TranslateToHighHalfMemoryAddress(bar->address));

            holder.ProbePorts();

            for(uint32_t i = 0; i < holder.portCount; i++)
            {
                AHCIDriver *port = holder.ports[i];

                if(port->Configure())
                {
                    globalDeviceManager.AddDevice(port);
                }
            }
        }
    }
}