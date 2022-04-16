# SMC-Influxdb

Fetch SMC data from macOS Intel hardware and output in InfluxDB line protocol. Designed to be automatically called by a Telegraf exec input plugin.

Code was mofified for my hardware and influxdb structure. Compiled C based approch taken as script required elevated permissions.
```
sudo powermetrics requires --samplers smc -i1 -n1 | awk
```

## Usage 

```./influxdb-smc -h
usage: influxdb-smc [cgfaA]
  -c  CPU temperature
  -g  GPU temperature
  -w  WiFi temperature
  -s  SSD temperature
  -f  fan speeds
  -a  CPU, GPU and fans - same as -cgf
  -A  all temperature and fan metrics
  -h  this info
```

### Compiling

```
make
```

### Running

```
./influxdb-smc
```

### Output example

```./influxdb-smc -nA
fan,host=Laptop,fan=Left          rpm=01826.33,percent=000.00 1648386301516399000
fan,host=Laptop,fan=Right         rpm=01699.72,percent=000.00 1648386301516399000
temperature,host=Laptop,sensor=CPU             value=00044.25 1648386301516399000
temperature,host=Laptop,sensor=CPU-Virtual-1   value=00049.09 1648386301516399000
temperature,host=Laptop,sensor=CPU-Virtual-2   value=00051.41 1648386301516399000
temperature,host=Laptop,sensor=CPU-Core-1      value=00053.00 1648386301516399000
temperature,host=Laptop,sensor=CPU-Core-2      value=00052.00 1648386301516399000
temperature,host=Laptop,sensor=CPU-Core-3      value=00059.00 1648386301516399000
temperature,host=Laptop,sensor=CPU-Core-4      value=00051.00 1648386301516399000
temperature,host=Laptop,sensor=CPU-Core-5      value=00053.00 1648386301516399000
temperature,host=Laptop,sensor=CPU-Core-6      value=00050.00 1648386301516399000
temperature,host=Laptop,sensor=CPU-Core-7      value=00049.00 1648386301516399000
temperature,host=Laptop,sensor=CPU-Core-8      value=00047.00 1648386301516399000
temperature,host=Laptop,sensor=GPU             value=00042.62 1648386301516399000
temperature,host=Laptop,sensor=GPU-VRAM        value=00041.62 1648386301516399000
temperature,host=Laptop,sensor=SSD             value=00038.51 1648386301516399000
temperature,host=Laptop,sensor=SSD-Filtered    value=00038.42 1648386301516399000
temperature,host=Laptop,sensor=SSD-Drive-0-A   value=00036.03 1648386301516399000
temperature,host=Laptop,sensor=SSD-Drive-0-B   value=00037.24 1648386301516399000
temperature,host=Laptop,sensor=SSD-Drive-1-A   value=00000.00 1648386301516399000
temperature,host=Laptop,sensor=SSD-Drive-1-B   value=00038.51 1648386301516399000
temperature,host=Laptop,sensor=Memory          value=00036.15 1648386301516399000
temperature,host=Laptop,sensor=Memory-Bank     value=00043.06 1648386301516399000
temperature,host=Laptop,sensor=Mainboard       value=00044.88 1648386301516399000
temperature,host=Laptop,sensor=WiFi            value=00042.56 1648386301516399000
temperature,host=Laptop,sensor=Battery-1       value=00033.60 1648386301516399000
temperature,host=Laptop,sensor=Battery-2       value=00033.50 1648386301516399000
temperature,host=Laptop,sensor=Ambient         value=00028.07 1648386301516399000
temperature,host=Laptop,sensor=Palm-Rest-1     value=00033.00 1648386301516399000
temperature,host=Laptop,sensor=Palm-Rest-2     value=00031.00 1648386301516399000
temperature,host=Laptop,sensor=Top-Skin        value=00036.63 1648386301516399000
temperature,host=Laptop,sensor=Heatpipe-Left   value=00040.62 1648386301516399000
temperature,host=Laptop,sensor=Heatpipe-Right  value=00043.50 1648386301516399000
temperature,host=Laptop,sensor=PCH-Die         value=00049.00 1648386301516399000
temperature,host=Laptop,sensor=PECI-GPU        value=00047.00 1648386301516399000
temperature,host=Laptop,sensor=PECI-CPU        value=00054.47 1648386301516399000
temperature,host=Laptop,sensor=PECI-MAX        value=00059.00 1648386301516399000
temperature,host=Laptop,sensor=PECI-SA         value=00051.00 1648386301516399000
```

## Telegraf Input Plugin

Input plugin definition from telegraf.conf
```
[[inputs.exec]]
  interval = "60s"
  commands = ["/Users/dbsqp/.bin/influxdb-smc -A"]
  timeout = "5s"
  data_format = "influx"
```


### Source
* https://github.com/lavoiesl/osx-cpu-temp
* https://github.com/hholtmann/smcFanControl/tree/master/smc-command
* https://github.com/dkorunic/iSMC
