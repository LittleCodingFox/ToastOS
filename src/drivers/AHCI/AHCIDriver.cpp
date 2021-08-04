#include <string.h>
#include "AHCIDriver.hpp"
#include "../../low-level/paging/PageFrameAllocator.hpp"
#include "../../low-level/paging/PageTableManager.hpp"
#include "../../low-level/interrupts/Interrupts.hpp"

namespace Drivers
{
    namespace AHCI
    {
        #define ATA_DEV_BUSY            0x80
        #define ATA_DEV_DRQ             0x08
        #define ATA_CMD_READ_DMA_EX     0x25
        #define ATA_CMD_WRITE_DMA_EX    0x35
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

        void InitializeCommandHeader(volatile HBACommandHeader *commandHeader, bool write)
        {
            commandHeader->commandFISLength = sizeof(FISREGHost2Device) / sizeof(uint32_t);
            commandHeader->write = write;
            commandHeader->prdtLength = 1;
        }

        void InitializeCommandTable(volatile HBACommandTable *commandTable, volatile HBACommandHeader *commandHeader, 
            uintptr_t dataAddress, size_t count)
        {
            memset((void *)commandTable, 0, sizeof(HBACommandTable) + (commandHeader->prdtLength - 1) * sizeof(HBAPRDEntry));

            commandTable->prdtEntry[0].dataBaseAddress = (uint64_t)dataAddress;
            commandTable->prdtEntry[0].dataBaseAddressUpper = (uint64_t)(dataAddress >> 32);
            commandTable->prdtEntry[0].interruptOnCompletion = 1;
            commandTable->prdtEntry[0].byteCount = (count * 512) - 1;
        }

        void InitializeFISRegCommand(volatile FISREGHost2Device *command, bool write, uintptr_t sector, size_t count)
        {
            memset((void *)command, 0, sizeof(FISREGHost2Device));

            command->fisType = FIS_TYPE_REG_H2D;

            command->commandControl = 1;

            if(write)
            {
                command->command = ATA_CMD_WRITE_DMA_EX;
            }
            else
            {
                command->command = ATA_CMD_READ_DMA_EX;
            }

            command->lba0 = (uint8_t)((sector & 0x000000000000FF));
            command->lba1 = (uint8_t)((sector & 0x0000000000FF00) >> 8);
            command->lba2 = (uint8_t)((sector & 0x00000000FF0000) >> 16);

            command->deviceRegister = 1 << 6;

            command->lba3 = (uint8_t)((sector & 0x000000FF000000) >> 24);
            command->lba4 = (uint8_t)((sector & 0x0000FF00000000) >> 32);
            command->lba5 = (uint8_t)((sector & 0x00FF0000000000) >> 40);

            command->countLow = count & 0xFF;
            command->countHigh = (count >> 8) & 0xFF;
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
                printf("ATA Device timed out\n");

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

        void AHCIDriver::ProbePorts()
        {
            uint32_t implementedPorts = ABAR->implementedPorts;

            for(uint32_t i = 0; i < 32; i++, implementedPorts >>= 1)
            {
                if(implementedPorts & 1)
                {
                    PortType type = CheckPortType(&ABAR->ports[i]);

                    if(type == AHCI_PORT_TYPE_SATA || type == AHCI_PORT_TYPE_SATAPI)
                    {
                        Port *port = new Port();
                        ports[portCount] = port;

                        port->portType = type;
                        port->port = &ABAR->ports[i];
                        port->portNumber = portCount;

                        portCount++;
                    }
                }
            }
        }

        void Port::Configure()
        {
            void *newBase = globalAllocator.requestPage();

            globalPageTableManager->identityMap(newBase);

            memset(newBase, 0, 0x1000);

            port->commandListBase = (uint64_t)newBase;
            port->commandListBaseUpper = ((uint64_t)newBase >> 32);

            HBAFIS *fis = new HBAFIS();

            globalPageTableManager->identityMap(fis);

            memset(fis, 0, sizeof(HBAFIS));

            fis->dmaFIS.type = FIS_TYPE_DMA_SETUP;
            fis->linkFIS.type = FIS_TYPE_REG_D2H;
            fis->pioFIS.type = FIS_TYPE_PIO_SETUP;

            port->fisBaseAddress = (uint64_t)fis;
            port->fisBaseAddressUpper = ((uint64_t)fis >> 32);

            volatile HBACommandHeader *commandHeader = (HBACommandHeader *)newBase;

            for(uint32_t i = 0; i < 32; i++)
            {
                commandHeader[i].prdtLength = 8;

                void *commandTableAddress = globalAllocator.requestPage();

                globalPageTableManager->identityMap(commandTableAddress);

                commandHeader[i].commandTableBaseAddress = (uint64_t)commandTableAddress;
                commandHeader[i].commandTableBaseAddressUpper = ((uint64_t)commandTableAddress >> 32);
            }
        }

        void Port::StartCommand()
        {
            port->commandStatus &= ~HBA_PxCMD_ST;

            while(port->commandStatus & HBA_PxCMD_CR) {}

            port->commandStatus |= HBA_PxCMD_FRE;
            port->commandStatus |= HBA_PxCMD_ST;
        }

        void Port::StopCommand()
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

        bool Port::Read(uint64_t sector, uint32_t sectorCount, void *buffer)
        {
            port->interruptStatus = (uint32_t)-1;

            int32_t slot = FindCommandSlot(port);

            if(slot == -1)
            {
                return false;
            }

            volatile HBACommandHeader *commandHeader = (HBACommandHeader *)((uint64_t)port->commandListBase);
            commandHeader += slot;

            InitializeCommandHeader(commandHeader, false);

            volatile HBACommandTable *commandTable = (HBACommandTable *)((uint64_t)commandHeader->commandTableBaseAddress);

            InitializeCommandTable(commandTable, commandHeader, (uintptr_t)buffer, sectorCount);

            volatile FISREGHost2Device *commandFIS = (FISREGHost2Device *)(commandTable->commandFIS);

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
                    printf("ATA Device task file error 1\n");
                    return false;
                }
            }

            if(port->interruptStatus & HBA_PxIS_TFES)
            {
                printf("ATA Device task file error 2\n");
                return false;
            }

            StopCommand();

            return true;
        }

        AHCIDriver::AHCIDriver(PCI::Device *device) : portCount(0)
        {
            printf("Initializing AHCI device %s (vendor: %s)\n",
                PCI::deviceName(device->vendorID, device->deviceID),
                PCI::vendorName(device->vendorID));

            this->device = device;

            ABAR = (volatile HBAMemory *)(device->bars[5].address);

            globalPageTableManager->identityMap((void *)ABAR);

            ProbePorts();

            printf("\tFound %u ports\n", portCount);

            for(uint32_t i = 0; i < portCount; i++)
            {
                Port *port = ports[i];

                port->Configure();

                port->buffer = (uint8_t *)globalAllocator.requestPage();
                globalPageTableManager->identityMap(port->buffer);
                memset(port->buffer, 0, 0x1000);
            }
        }
    }
}