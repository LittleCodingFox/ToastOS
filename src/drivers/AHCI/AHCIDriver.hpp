#pragma once
#include <stdint.h>
#include "../low-level/pci/PCI.hpp"
#include "../low-level/devicemanager/GenericIODevice.hpp"

namespace Drivers
{
    namespace AHCI
    {
        enum PortType
        {
            AHCI_PORT_TYPE_NONE,
            AHCI_PORT_TYPE_SATA,
            AHCI_PORT_TYPE_SEMB,
            AHCI_PORT_TYPE_PM,
            AHCI_PORT_TYPE_SATAPI
        };

        enum FISType
        {
            FIS_TYPE_REG_H2D = 0x27,
            FIS_TYPE_REG_D2H = 0x34,
            FIS_TYPE_DMA_ACT = 0x39,
            FIS_TYPE_DMA_SETUP = 0x41,
            FIS_TYPE_DATA = 0x46,
            FIS_TYPE_BIST = 0x58,
            FIS_TYPE_PIO_SETUP = 0x5F,
            FIS_TYPE_DEV_BITS = 0xA1
        };

        struct __attribute__((packed)) HBAPort
        {
            uint32_t commandListBase;
            uint32_t commandListBaseUpper;
            uint32_t fisBaseAddress;
            uint32_t fisBaseAddressUpper;
            uint32_t interruptStatus;
            uint32_t interruptEnable;
            uint32_t commandStatus;
            uint32_t reserved0;
            uint32_t taskFileData;
            uint32_t signature;
            uint32_t sataStatus;
            uint32_t sataControl;
            uint32_t sataError;
            uint32_t sataActive;
            uint32_t commandIssue;
            uint32_t sataNotification;
            uint32_t fisSwitchControl;
            uint32_t reserved1[11];
            uint32_t vendor[4];
        };

        struct __attribute__((packed)) HBAMemory
        {
            uint32_t hostCapability;
            uint32_t globalHostControl;
            uint32_t interruptStatus;
            uint32_t implementedPorts;
            uint32_t version;
            uint32_t cccControl;
            uint32_t cccPorts;
            uint32_t enclosureManagementLocation;
            uint32_t enclosureManagementControl;
            uint32_t extendedHostCapabilities;
            uint32_t biosHandoffControlStatus;
            uint8_t reserved0[116];
            uint8_t vendor[96];
            HBAPort ports[1];
        };

        struct __attribute__((packed)) HBACommandHeader
        {
            uint8_t commandFISLength : 5;
            uint8_t atapi : 1;
            uint8_t write : 1;
            uint8_t prefetchable : 1;

            uint8_t reset : 1;
            uint8_t bist : 1;
            uint8_t clearBusy : 1;
            uint8_t reserved0 : 1;
            uint8_t portMultiplier : 3;

            uint16_t prdtLength;
            uint32_t prdbCount;
            uint32_t commandTableBaseAddress;
            uint32_t commandTableBaseAddressUpper;
            uint32_t reserved1[4];
        };

        struct __attribute__((packed)) HBAPRDEntry
        {
            uint32_t dataBaseAddress;
            uint32_t dataBaseAddressUpper;
            uint32_t reserved0;

            uint32_t byteCount : 22;
            uint32_t reserved1 : 9;

            uint32_t interruptOnCompletion : 1;
        };

        struct __attribute__((packed)) HBACommandTable
        {
            uint8_t commandFIS[64];
            uint8_t atapiCommand[16];
            uint8_t reserved[48];

            HBAPRDEntry prdtEntry[];
        };

        struct __attribute__((packed)) FISREGHost2Device
        {
            uint8_t fisType;

            uint8_t portMultiplier : 4;
            uint8_t reserved0 : 3;
            uint8_t commandControl : 1;

            uint8_t command;
            uint8_t featureLow;

            uint8_t lba0;
            uint8_t lba1;
            uint8_t lba2;
            uint8_t deviceRegister;

            uint8_t lba3;
            uint8_t lba4;
            uint8_t lba5;
            uint8_t featureHigh;

            uint8_t countLow;
            uint8_t countHigh;
            uint8_t isoCommandCompletion;
            uint8_t control;

            uint8_t reserved1[4];
        };

        struct __attribute__((packed)) FISREGDevice2Host
        {
            uint8_t type;

            uint8_t portMultiplier : 4;
            uint8_t reserved0 : 2;
            uint8_t interrupt : 1;
            uint8_t reserved1 : 1;

            uint8_t status;
            uint8_t error;

            uint8_t lba0;
            uint8_t lba1;
            uint8_t lba2;
            uint8_t deviceRegister;

            uint8_t lba3;
            uint8_t lba4;
            uint8_t lba5;
            uint8_t reserved2;

            uint8_t countLow;
            uint8_t countHigh;
            uint8_t reserved3[5];
        };

        struct __attribute__((packed)) FISREGData
        {
            uint8_t type;
            uint8_t portMultiplier : 4;
            uint8_t reserved0 : 4;
            uint8_t reserved1[2];
            uint32_t data[1];
        };

        struct __attribute__((packed)) FISPIOSetup
        {
            uint8_t type;
            uint8_t portMultiplier : 4;
            uint8_t reserved0 : 1;
            uint8_t dataTransferDirection : 1;
            uint8_t interrupt : 1;
            uint8_t reserved1 : 1;

            uint8_t status;
            uint8_t error;

            uint8_t lba0;
            uint8_t lba1;
            uint8_t deviceRegister;

            uint8_t lba3;
            uint8_t lba4;
            uint8_t lba5;
            uint8_t reserved2;

            uint8_t countLow;
            uint8_t countHigh;
            uint8_t reserved3;
            uint8_t newStatus;
            uint16_t transferCount;
            uint8_t reserved4[2];
        };

        struct __attribute__((packed)) FISDMASetup
        {
            uint8_t type;
            uint8_t portMultiplier : 4;
            uint8_t reserved0 : 1;
            uint8_t dataTransferDirection : 1;
            uint8_t interrupt : 1;
            uint8_t autoActivate : 1;
            uint8_t reserved1[2];
            uint64_t dmaBufferIdentifier;
            uint32_t reserved2;
            uint32_t dmaBufferOffset;
            uint32_t transferCount;
            uint32_t reserved3;
        };

        struct __attribute__((packed)) HBAFIS
        {
            FISDMASetup dmaFIS;
            uint8_t padding0[4];
            FISPIOSetup pioFIS;
            uint8_t padding1[12];
            FISREGDevice2Host linkFIS;
            uint8_t padding2[4];
            uint8_t undefined0;
            uint8_t undefined1;
            uint8_t padding3[64];
            uint8_t reserved[96];
        };

        struct __attribute__((packed)) FISIdentify
        {
            uint16_t reserved0[27];
            uint16_t model[20];
            uint16_t reserved1[36];
            uint16_t capabilities;
            uint16_t reserved2[16];
            uint64_t maxLBA48;
            uint16_t reserved3[2];
            uint16_t sectorSizeInfo;
            uint16_t reserved4[9];
            uint16_t logicalSectorSize;
            uint16_t reserved5[139];

            void ReadModel(char *buffer) const;
        };

        static_assert(sizeof(FISIdentify) == 512, "struct has invalid size");

        class AHCIDriver : public Devices::GenericIODevice
        {
        public:
            volatile HBAPort *port;
            PCI::Device *device;
            PortType portType;
            uint8_t portNumber;
            FISIdentify identify;
            char model[41];

            AHCIDriver();
            bool Configure();
            void StartCommand();
            void StopCommand();
            bool Read(void *data, uint64_t sector, uint64_t count) override;
            bool Write(const void *data, uint64_t sector, uint64_t count) override;
            bool Identify();

            inline const char *name() const override 
            {
                return model;
            }

            inline Devices::DeviceType type() const override
            {
                return Devices::DEVICE_TYPE_DISK;
            }
        };

        void HandleMassStorageDevice(PCI::Device *device);
    };
};
