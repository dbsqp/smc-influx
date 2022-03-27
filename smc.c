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

#include <stdint.h>
#include <inttypes.h>

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

            printf("fan,host=%s,fan=Fan%i rpm=%08.2f,percent=%05.1f %ld\n", hostname, i+1, cur, pct, ens);
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
    printf("temperature,host=%s,sensor=%-16svalue=%08.2f %ld\n", hostname, sensor, temperature, ens);
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

    const char *hostnameFullPtr = hostnameFull;
    hostnameFullPtr = strchr(hostnameFullPtr, '.');
    strncpy(hostname,&hostnameFull[0],hostnameFullPtr-hostnameFull);

    // get epoch in ns
    clock_gettime(CLOCK_REALTIME, &spec);
    sec = spec.tv_sec;
     ns = spec.tv_nsec;
    ens =  sec * 1000000000 + ns;

    // get SMC values and print in line protocol
    SMCOpen();

    influxSMCtemp("TG0P","GPU-Proximity");
    influxSMCtemp("TC0P","CPU-Proximity");
    influxSMCtemp("TH0X","SSD-Cooked-Max");
    influxSMCtemp("TW0P","WiFi-Proximity");

    influxSMCfans();

    SMCClose();

    return 0;
}
