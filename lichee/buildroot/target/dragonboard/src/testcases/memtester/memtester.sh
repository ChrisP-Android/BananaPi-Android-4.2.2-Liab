#!/bin/sh

source send_cmd_pipe.sh
source script_parser.sh

insmod /system/vendor/modules/sunxi-dbgreg.ko
debug_address_node=/sys/class/misc/sunxi-reg/rw/address
debug_node=/sys/class/sunxi_dump/dump

#sdram config register
SDCR="0xf1c01004"

reg_read()
{
    
 echo $1 > $debug_node 
 cat $debug_node 

}
get_dram_size()
{
    sdcr_value=`reg_read $SDCR`
    let "chip_den=(sdcr_value>>3)&0x7"
    if [ $chip_den -eq 0 ]; then
        dram_size=32
    elif [ $chip_den -eq 1 ]; then
        dram_size=64
    elif [ $chip_den -eq 2 ]; then
        dram_size=128
    elif [ $chip_den -eq 3 ]; then
        dram_size=256
    elif [ $chip_den -eq 4 ]; then
        dram_size=512
    else 
        dram_size=1024
    fi
    
    let "io_width=(sdcr_value>>1)&0x3"
    if [ $io_width -eq 1 ]; then
    let "dram_size<<=1"
    fi
    
    let "system_io_width=(sdcr_value>>6)&0x7"
    if [ $system_io_width -eq 3 ]; then
    let "dram_size<<=1"
    fi
    
    let "rank_number=(sdcr_value>>10)&0x3"
    if [ $rank_number -eq 1 ]; then
    let "dram_size<<=1"
    fi 

    echo $dram_size

}
dram_size=`script_fetch "dram" "dram_size"`
test_size=`script_fetch "dram" "test_size"`
actual_size=`get_dram_size`

factor_N=`cat /proc/ccmu | awk -F: '(NF&&$1~/ *Pll5Ctl.FactorN$/) {print $2}' `
factor_K=`cat /proc/ccmu | awk -F: '(NF&&$1~/ *Pll5Ctl.FactorK$/) {print $2}' `
factor_M=`cat /proc/ccmu | awk -F: '(NF&&$1~/ *Pll5Ctl.FactorM$/) {print $2}' `
let "dram_freq=(24*$factor_N*$factor_K)/$factor_M"

echo "dram_freq=$dram_freq"
echo "config dram_size=$dram_size"M""
echo "actual_size=$actual_size"M""
echo "test_size=$test_size"M""

if [ $actual_size -lt $dram_size ]; then
   SEND_CMD_PIPE_FAIL_EX $3 "size "$actual_size"M"" error"
   exit 0
fi
SEND_CMD_PIPE_MSG $3 "size:$actual_size"M" freq:$dram_freq"MHz""
memtester $test_size"M" 1 > /dev/null 
if [ $? -ne 0 ]; then
    echo "memtest fail"
    SEND_CMD_PIPE_FAIL_EX $3 "size:$actual_size"M" freq:$dram_freq"MHz""
else
    echo "memtest success!"
    SEND_CMD_PIPE_OK_EX $3 "size:$actual_size"M" freq:$dram_freq"MHz""
fi
