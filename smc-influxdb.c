/*
 * smc-influxdb Tool
 * Copyright (C) 2006 devnull
 * Copyright (C) 2022 Matt Parkinson
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <IOKit/IOKitLib.h>

#include "smc-influxdb.h"

#include <unistd.h>
#include <time.h>

static io_connect_t conn;



UInt32 _strtoul(char* str, int size, int base)
{
    UInt32 total = 0;
    int i;

    for (i = 0; i < size; i++) {
        if (base == 16)
            total += str[i] << (size - 1 - i) * 8;
        else
            total += (unsigned char)(str[i] << (size - 1 - i) * 8);
    }
    return total;
}



void _ultostr(char* str, UInt32 val)
{
    str[0] = '\0';
    sprintf(str, "%c%c%c%c",
        (unsigned int)val >> 24,
        (unsigned int)val >> 16,
        (unsigned int)val >> 8,
        (unsigned int)val);
}



float _strtof( char* str, int size, int e)
{
    float total = 0;
    int i;
    
    for (i = 0; i < size; i++)
    {
        if (i == (size - 1))
            total += (str[i] & 0xff) >> e;
        else
            total += str[i] << (size - 1 - i) * (8 - e);
    }
    
    total += (str[size-1] & 0x03) * 0.25;
    
    return total;
}



kern_return_t SMCOpen(void)
{
    kern_return_t result;
    io_iterator_t iterator;
    io_object_t device;

    CFMutableDictionaryRef matchingDictionary = IOServiceMatching("AppleSMC");
    result = IOServiceGetMatchingServices(kIOMasterPortDefault, matchingDictionary, &iterator);
    if (result != kIOReturnSuccess) {
        printf("Error: IOServiceGetMatchingServices() = %08x\n", result);
        return 1;
    }

    device = IOIteratorNext(iterator);
    IOObjectRelease(iterator);
    if (device == 0) {
        printf("Error: no SMC found\n");
        return 1;
    }

    result = IOServiceOpen(device, mach_task_self(), 0, &conn);
    IOObjectRelease(device);
    if (result != kIOReturnSuccess) {
        printf("Error: IOServiceOpen() = %08x\n", result);
        return 1;
    }

    return kIOReturnSuccess;
}



kern_return_t SMCClose()
{
    return IOServiceClose(conn);
}



kern_return_t SMCCall(int index, SMCKeyData_t* inputStructure, SMCKeyData_t* outputStructure)
{
    size_t structureInputSize;
    size_t structureOutputSize;

    structureInputSize = sizeof(SMCKeyData_t);
    structureOutputSize = sizeof(SMCKeyData_t);

#if MAC_OS_X_VERSION_10_5
    return IOConnectCallStructMethod(conn, index,
        // inputStructure
        inputStructure, structureInputSize,
        // ouputStructure
        outputStructure, &structureOutputSize);
#else
    return IOConnectMethodStructureIStructureO(conn, index,
        structureInputSize, /* structureInputSize */
        &structureOutputSize, /* structureOutputSize */
        inputStructure, /* inputStructure */
        outputStructure); /* ouputStructure */
#endif
}



kern_return_t SMCReadKey(UInt32Char_t key, SMCVal_t* val)
{
    kern_return_t result;
    SMCKeyData_t inputStructure;
    SMCKeyData_t outputStructure;

    memset(&inputStructure, 0, sizeof(SMCKeyData_t));
    memset(&outputStructure, 0, sizeof(SMCKeyData_t));
    memset(val, 0, sizeof(SMCVal_t));

    inputStructure.key = _strtoul(key, 4, 16);
    inputStructure.data8 = SMC_CMD_READ_KEYINFO;

    result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
        return result;

    val->dataSize = outputStructure.keyInfo.dataSize;
    _ultostr(val->dataType, outputStructure.keyInfo.dataType);
    inputStructure.keyInfo.dataSize = val->dataSize;
    inputStructure.data8 = SMC_CMD_READ_BYTES;

    result = SMCCall(KERNEL_INDEX_SMC, &inputStructure, &outputStructure);
    if (result != kIOReturnSuccess)
        return result;

    memcpy(val->bytes, outputStructure.bytes, sizeof(outputStructure.bytes));

    return kIOReturnSuccess;
}


char hostname[265];
char hostTag[265];
long int ens;


float getSMCrpm(char* key)
{
    SMCVal_t val;
    kern_return_t result;
    float fval;

    result = SMCReadKey(key, &val);
    if (result == kIOReturnSuccess) {
        if (val.dataSize > 0) {
            if (strcmp(val.dataType, "flt " ) == 0) {
                memcpy(&fval,val.bytes,sizeof(float));
                return fval;
            } else if ( strcmp(val.dataType, "fpe2" ) == 0 ) {
                fval = _strtof(val.bytes, val.dataSize, 2);
                return fval;
            }
        }
    }
    return -1.f;
}

void influxSMCfans()
{
    kern_return_t result;
    SMCVal_t val;
    UInt32Char_t key;
    int nFans, i;
    char fanID[8];

    result = SMCReadKey("FNum", &val);

    if (result == kIOReturnSuccess) {
        nFans = _strtoul((char*)val.bytes, val.dataSize, 10);

        for (i = 0; i < nFans; i++) {
            sprintf(key, "F%dID", i);
            result = SMCReadKey(key, &val);
            if (result != kIOReturnSuccess) { continue; }

            sprintf(key, "F%dAc", i);
            double cur = getSMCrpm(key);
            if (cur < 0.f) { continue; }

            sprintf(key, "F%dMn", i);
            float min = getSMCrpm(key);
            if (min < 0.f) { continue; }

            sprintf(key, "F%dMx", i);
            float max = getSMCrpm(key);
            if (max < 0.f) { continue; }

            float pct = (cur - min ) / ( max - min );
            pct *= 100.f;
            if ( pct < 0.f) { pct = 0.f; }

            switch (i) {
                case 0:
                    if ( nFans == 1 ) {
                        strcpy(fanID, "Main");
                    } else {
                        strcpy(fanID, "Left"); 
                    }
                    break;
                case 1:
                    strcpy(fanID, "Right");
                    break;
                case '?':
                    strcpy(fanID, "Other");
                    break;
            }
            if ( cur > 0.0 ) {
                printf("fan,%skey=F%iAc,sensor=%-10s rpm=%08.2f,percent=%06.2f %ld\n", hostTag, i, fanID, cur, pct, ens);
            }
        }
    }
}



double getSMCtemp(char* key)
{
    SMCVal_t val;
    kern_return_t result;

    result = SMCReadKey(key, &val);
    if (result == kIOReturnSuccess) {
        if (val.dataSize > 0) {
            if (strcmp(val.dataType, "sp78" ) == 0) {
                int intValue = val.bytes[0] * 256 + (unsigned char)val.bytes[1];
                return intValue / 256.0;
            }
        }
    }
    return 0.0;
}

void influxSMCtemp( char* key, char* sensor )
{
    double temperature = getSMCtemp( key );
    if ( temperature > 0.0 ) {
        printf("temperature,%skey=%s,sensor=%-16svalue=%08.2f %ld\n", hostTag, key, sensor, temperature, ens);
    }
}



int main(int argc, char* argv[])
{
    int status;
    char hostnameFull[265];

    long int ns;
    time_t sec;
    struct timespec spec;

    // get hostname
    status = gethostname( &hostnameFull[0], 256 );
    if ( status == -1 ) { strcpy(hostnameFull, "NULL"); }

    // strip domain from hostname
    const char *hostnameFullPtr = hostnameFull;
    hostnameFullPtr = strchr(hostnameFullPtr, '.');
    strncpy(hostname,&hostnameFull[0],hostnameFullPtr-hostnameFull);
    
    // capatalise first letter of hostname
    if ( hostname[0]>='a' && hostname[0]<='z' ) {
        hostname[0]=hostname[0]-32;
    }

    // get ns epoch
    clock_gettime(CLOCK_REALTIME, &spec);
    sec = spec.tv_sec;
     ns = spec.tv_nsec;
    ens =  sec * 1000000000 + ns;

    // pass options
    int cpu = 0;
    int gpu = 0;
    int wfi = 0;
    int ssd = 0;
    int fan = 0;
    int all = 0;
    int tag = 0;

    int args;
    while ((args = getopt(argc, argv, "aAcfghwsn?")) != -1) {
        switch (args) {
        case 'a':
            cpu = 1;
            gpu = 1;
            fan = 1;
            wfi = 1;
            ssd = 1;
            break;
        case 'A':
            all = 1;
            break;
        case 'c':
            cpu = 1;
            break;
        case 'f':
            fan = 1;
            break;
        case 'g':
            gpu = 1;
            break;
        case 'w':
            wfi = 1;
            break;
        case 's':
            ssd = 1;
            break;
        case 'n':
            tag = 1;
            break;
        case 'h':
        case '?':
            printf("usage: smc-influx [aAcfghwsn]\n");
            printf("  -c  CPU temperature\n");
            printf("  -g  GPU temperature\n");
            printf("  -w  WiFi temperature\n");
            printf("  -s  SSD temperature\n");
            printf("  -f  fan speeds\n");
            printf("  -a  CPU, GPU and fans - same as -cgf\n");
            printf("  -A  all temperature and fan metrics\n");
            printf("  -n  tag with hostname\n");
            printf("  -h  this info\n");
            return -1;
        }
    }

    // tag with hostname -n
    if ( tag ) { sprintf(hostTag, "host=%s,", hostname); }
    
    // default -a
    if ( !cpu && !gpu && !fan && !wfi && !ssd && !all ) { cpu = gpu = fan = wfi = ssd = 1; }

    // get SMC values and print in line protocol
    SMCOpen();

    if ( all ) {        
        influxSMCtemp("TC0P","CPU");
        influxSMCtemp("TCXr","CPU-Package");
        influxSMCtemp("TC0E","CPU-Virtual-1");
        influxSMCtemp("TC0F","CPU-Virtual-2");
        influxSMCtemp("TC1C","CPU-Core-1");
        influxSMCtemp("TC2C","CPU-Core-2");
        influxSMCtemp("TC3C","CPU-Core-3");
        influxSMCtemp("TC4C","CPU-Core-4");
        influxSMCtemp("TC5C","CPU-Core-5");
        influxSMCtemp("TC6C","CPU-Core-6");
        influxSMCtemp("TC7C","CPU-Core-7");
        influxSMCtemp("TC8C","CPU-Core-8");

        influxSMCtemp("TG0P","GPU");
        influxSMCtemp("TG1P","GPU-VRAM");

        influxSMCtemp("TH0X","SSD");
        influxSMCtemp("TH0F","SSD-Filtered");
        influxSMCtemp("TH0a","SSD-Drive-0-A");
        influxSMCtemp("TH0b","SSD-Drive-0-B");
        influxSMCtemp("TH1a","SSD-Drive-1-A");
        influxSMCtemp("TH1b","SSD-Drive-1-B");
        influxSMCtemp("TH1c","SSD-Drive-1-C");

        influxSMCtemp("Ts0S","Memory");
        influxSMCtemp("TM0P","Memory-Bank");
        influxSMCtemp("TM0p","Memory-DIMM");

        influxSMCtemp("Tm0P","Mainboard");

        influxSMCtemp("TW0P","WiFi");

        influxSMCtemp("TB1T","Battery-1");
        influxSMCtemp("TB2T","Battery-2");

        influxSMCtemp("TA0V","Ambient");
        influxSMCtemp("Ts0P","Palm-Rest-1");
        influxSMCtemp("Ts1P","Palm-Rest-2");
        influxSMCtemp("Ts1S","Top-Skin");
        influxSMCtemp("TA0P","Airflow-1");
        influxSMCtemp("TA1P","Airflow-2");
        influxSMCtemp("Th1H","Heatpipe-Left");
        influxSMCtemp("Th2H","Heatpipe-Right");

        influxSMCtemp("TPCD","PCH-Die");
        influxSMCtemp("TCGC","PECI-GPU");
        influxSMCtemp("TCXC","PECI-CPU");
        influxSMCtemp("TCMX","PECI-MAX");
        influxSMCtemp("TCSA","PECI-SA");

        influxSMCtemp("Te0T","TBT-Diode");
        influxSMCtemp("Tp0C","Power-Supply");
        
        influxSMCfans();
    } else {
        if ( cpu ) { influxSMCtemp("TC0P","CPU"); }
        if ( gpu ) { influxSMCtemp("TG0P","GPU"); }
        if ( ssd ) { influxSMCtemp("TH0X","SSD"); }
        if ( wfi ) { influxSMCtemp("TW0P","WiFi"); }
        if ( fan ) { influxSMCfans(); }
    }

    SMCClose();

    return 0;
}
