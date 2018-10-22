EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr B 17000 11000
encoding utf-8
Sheet 22 27
Title "rev02_20"
Date "15 10 2016"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 1600 4100 0    60   ~ 12
*) Lower Right Bank
Text Notes 3820 7410 0    60   ~ 12
DIGITIZED_NOISE signal should go into\neither W19 or Y18 (i.e. into one of the two\npositive (master) sides of the two available\nMRCC differential pairs)
Text Notes 6500 4100 0    60   ~ 12
*) Signals, that are allowed to be swapped, can be be swapped\nwith each other and/or moved to different pins within their bank.
Text Notes 4800 6500 0    60   ~ 12
<-- FPGA_GPIO_* and FPGA_IRQ_N_* signals can be swapped
Text Notes 4980 4810 0    60   ~ 12
<-- Disable pull-ups on all pins during configuration
Text Notes 8500 10220 0    84   ~ 12
FPGA MKM interface
Text Notes 4280 4890 0    60   ~ 12
R65
$Comp
L Cryptech_Alpha:VCCO_3V3 VCCO_3V3_38
U 1 1 58023EEC
P 4200 4300
F 0 "VCCO_3V3_38" H 4200 4300 20  0000 C CNN
F 1 "+VCCO_3V3" H 4200 4230 30  0000 C CNN
F 2 "" H 4200 4300 70  0000 C CNN
F 3 "" H 4200 4300 70  0000 C CNN
	1    4200 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	3600 7400 2400 7400
Text GLabel 3600 7400 2 48 Input ~ 0
DIGITIZED_NOISE
Wire Wire Line
	3600 6500 2400 6500
Text GLabel 3600 6500 2 48 Output ~ 0
FPGA_GPIO_LED_2
Wire Wire Line
	3600 6600 2400 6600
Text GLabel 3600 6600 2 48 Output ~ 0
FPGA_GPIO_LED_3
Wire Wire Line
	3600 6400 2400 6400
Wire Wire Line
	2900 5600 2400 5600
Wire Wire Line
	2600 4500 2400 4500
Wire Wire Line
	2600 4500 2600 4600
Wire Wire Line
	2600 4600 2600 4700
Wire Wire Line
	2600 4700 2600 4800
Wire Wire Line
	2600 4800 2600 4900
Wire Wire Line
	2600 4900 2400 4900
Wire Wire Line
	2600 4800 2400 4800
Wire Wire Line
	2600 4700 2400 4700
Wire Wire Line
	2600 4600 2400 4600
Wire Wire Line
	2900 4500 2600 4500
Wire Wire Line
	4200 4300 4200 4500
Wire Wire Line
	2600 4900 2600 5000
Wire Wire Line
	2600 5000 2400 5000
Wire Wire Line
	3600 5200 3500 5200
Text GLabel 3600 5200 2 48 UnSpc ~ 0
FPGA_CFG_MOSI
Wire Wire Line
	3600 5300 3500 5300
Text GLabel 3600 5300 2 48 Input ~ 0
FPGA_CFG_MISO
Wire Wire Line
	3600 6200 3500 6200
Text GLabel 3600 6200 2 48 Output ~ 0
FPGA_CFG_CS_N
Wire Wire Line
	3100 5300 2400 5300
Text Label 2460 5300 2 48 ~ 0
FPGA_CFG_MISO1
Wire Wire Line
	3100 5200 2400 5200
Text Label 2460 5200 2 48 ~ 0
FPGA_CFG_MOSI1
Wire Wire Line
	3100 6200 2400 6200
Text Label 2460 6200 2 48 ~ 0
FPGA_CFG_CS_N1
Text GLabel 3600 7000 2 48 Input ~ 0
FMC_A19
Text GLabel 3600 7100 2 48 Input ~ 0
FMC_A20
Wire Wire Line
	3600 7000 2400 7000
Wire Wire Line
	3600 7100 2400 7100
Text GLabel 3610 8100 2 48 Input ~ 0
FMC_A21
Wire Wire Line
	3610 8100 2400 8100
Text GLabel 3600 6800 2 48 Input ~ 0
FMC_A22
Text GLabel 3600 6900 2 48 Input ~ 0
FMC_A23
Wire Wire Line
	3600 6900 2400 6900
Wire Wire Line
	3600 6800 2400 6800
Text GLabel 3600 8500 2 48 Input ~ 0
FMC_A24
Wire Wire Line
	3600 8500 2400 8500
Text GLabel 3600 8000 2 48 Input ~ 0
FMC_A25
Wire Wire Line
	3600 8000 2400 8000
Text GLabel 3600 7600 2 48 BiDi ~ 0
FMC_D8
Wire Wire Line
	3600 7600 2400 7600
Text GLabel 3600 6700 2 48 BiDi ~ 0
FMC_D9
Wire Wire Line
	3600 6700 2400 6700
Text GLabel 3600 7500 2 48 BiDi ~ 0
FMC_D10
Wire Wire Line
	3600 7500 2400 7500
Text GLabel 3600 7200 2 48 BiDi ~ 0
FMC_D12
Wire Wire Line
	3600 7200 2400 7200
Text GLabel 3600 7300 2 48 BiDi ~ 0
FMC_D28
Wire Wire Line
	3600 7300 2400 7300
Text GLabel 3600 7800 2 48 BiDi ~ 0
FMC_D29
Wire Wire Line
	3600 7800 2400 7800
Text GLabel 3100 5500 2 48 BiDi ~ 0
FMC_D30
Wire Wire Line
	3100 5500 2400 5500
Text GLabel 3100 5400 2 48 BiDi ~ 0
FMC_D31
Wire Wire Line
	3100 5400 2400 5400
Text GLabel 3600 8300 2 48 UnSpc ~ 0
FMC_NL
Wire Wire Line
	3600 8300 2400 8300
Text GLabel 3600 10000 2 48 BiDi ~ 0
FMC_D11
Wire Wire Line
	3600 10000 2400 10000
Wire Wire Line
	4200 5600 2900 5600
Wire Wire Line
	4200 5000 4200 5600
Wire Wire Line
	4200 4500 2900 4500
Wire Wire Line
	4200 4600 4200 4500
$Comp
L Cryptech_Alpha:R-EU_R0402 R65
U 1 1 58023EEB
P 4200 4800
F 0 "R65" V 4110 4755 60  0000 R TNN
F 1 "1k" V 4030 4730 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 4030 4730 60  0001 C CNN
F 3 "" H 4030 4730 60  0000 C CNN
	1    4200 4800
	0    -1   -1   0
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0603 R83
U 1 1 58023EEA
P 3300 5200
F 0 "R83" H 3410 5120 60  0000 R TNN
F 1 "0" H 3220 5120 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 3220 5120 60  0001 C CNN
F 3 "" H 3220 5120 60  0000 C CNN
	1    3300 5200
	-1   0    0    1
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0603 R84
U 1 1 58023EE9
P 3300 5300
F 0 "R84" H 3400 5480 60  0000 R TNN
F 1 "0" H 3210 5470 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 3210 5470 60  0001 C CNN
F 3 "" H 3210 5470 60  0000 C CNN
	1    3300 5300
	-1   0    0    1
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0603 R85
U 1 1 58023EE8
P 3300 6200
F 0 "R85" H 3180 6200 60  0000 R TNN
F 1 "0" H 3170 6300 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 3170 6300 60  0001 C CNN
F 3 "" H 3170 6300 60  0000 C CNN
	1    3300 6200
	-1   0    0    1
$EndComp
$Comp
L Cryptech_Alpha:XC7A200TFBG484_3 U13_12
U 1 1 58023EE7
P 2200 7200
F 0 "U13_12" H 1790 4190 60  0000 L BNN
	1    2200 7200
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:BGA484C100P22X22_2300X2300X254" H 1790 4190 60  0001 C CNN
$EndComp
NoConn ~ 2400 5700
NoConn ~ 2400 5900
NoConn ~ 2400 5800
NoConn ~ 2400 6000
NoConn ~ 2400 6100
NoConn ~ 2400 6300
NoConn ~ 2400 7700
NoConn ~ 2400 7900
NoConn ~ 2400 8200
NoConn ~ 2400 8400
NoConn ~ 2400 8600
NoConn ~ 2400 8700
NoConn ~ 2400 8800
NoConn ~ 2400 8900
NoConn ~ 2400 9000
NoConn ~ 2400 9100
NoConn ~ 2400 9200
NoConn ~ 2400 9300
NoConn ~ 2400 9400
NoConn ~ 2400 9500
NoConn ~ 2400 9600
NoConn ~ 2400 9700
NoConn ~ 2400 9800
NoConn ~ 2400 9900
NoConn ~ 2400 5100
NoConn ~ 3600 6400
$EndSCHEMATC
