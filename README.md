# Netlist2Hypergraph
## Netlist To Hypergraph Converter
Copyright (C) 2025 Broyiii<br>
Author : @Broyiii<br>
Version: 2025-02-13 (V1.00)<br>
Update :<br>
  Netlist To Hypergraph Converter (V1.00.250213)<br>

## How to complie
`mkdir build && cd build`<br>
`cmake ..`<br>
`make`<br>
`./nl2hg`<br>

## How to run
You can use `./nl2hg` to get manual<br>
```
Usage:
        ./nl2hg -v <netlist_dir> -o <hgr_file_fir> [optional command]

Optional Command:
        -p <port_info_dir>
```
e.g.
```
./nl2hg -v ./case/sample.v -o ./output/sample.hgr -p sample.port
```
Then you will find 3 files in `./output/` : sample.hgr, sample.hgr.instinfo, sample.hgr.netinfo