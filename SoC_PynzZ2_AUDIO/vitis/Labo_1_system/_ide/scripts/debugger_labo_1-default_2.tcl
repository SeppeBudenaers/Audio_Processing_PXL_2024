# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: C:\02_PXL\Audio_Processing_PXL_2024\SoC_PynzZ2_AUDIO\vitis\Labo_1_system\_ide\scripts\debugger_labo_1-default_2.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source C:\02_PXL\Audio_Processing_PXL_2024\SoC_PynzZ2_AUDIO\vitis\Labo_1_system\_ide\scripts\debugger_labo_1-default_2.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -nocase -filter {name =~"APU*"}
rst -system
after 3000
targets -set -filter {jtag_cable_name =~ "Xilinx TUL 1234-tulA" && level==0 && jtag_device_ctx=="jsn-TUL-1234-tulA-23727093-0"}
fpga -file C:/02_PXL/Audio_Processing_PXL_2024/SoC_PynzZ2_AUDIO/vitis/Labo_1/_ide/bitstream/AudioProcessing2.bit
targets -set -nocase -filter {name =~"APU*"}
loadhw -hw C:/02_PXL/Audio_Processing_PXL_2024/SoC_PynzZ2_AUDIO/vitis/AudioProcessing2/export/AudioProcessing2/hw/AudioProcessing2.xsa -mem-ranges [list {0x40000000 0xbfffffff}] -regs
configparams force-mem-access 1
targets -set -nocase -filter {name =~"APU*"}
source C:/02_PXL/Audio_Processing_PXL_2024/SoC_PynzZ2_AUDIO/vitis/Labo_1/_ide/psinit/ps7_init.tcl
ps7_init
ps7_post_config
configparams force-mem-access 0
