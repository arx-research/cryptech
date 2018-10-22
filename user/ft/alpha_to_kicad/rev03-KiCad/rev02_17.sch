EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr B 17000 11000
encoding utf-8
Sheet 19 27
Title "rev02_17"
Date "15 10 2016"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 6050 4500 0    60   ~ 12
*) Bottom Bank
Text Notes 1000 4100 0    60   ~ 12
*) Lower Left Bank
Text Notes 8480 5140 0    60   ~ 12
FMC_A[...] signals can be swapped
Text Notes 8140 7850 0    60   ~ 12
<-- FMC_CLK signal _MUST_ go\ninto either W11 or V13 (i.e. into\none of the two positive (master)\nsides of the two available\nMRCC differential pairs)
Text Notes 3260 5160 0    60   ~ 12
FMC_D[...] signals can be swapped
Text Notes 2900 8700 0    60   ~ 12
<-- FMC_* control signals\n      can be swapped
Text Notes 8580 10230 0    84   ~ 12
FPGA FMC interface
$Comp
L Cryptech_Alpha:VCCO_3V3 VCCO_3V3_33
U 1 1 58023F2F
P 2000 4400
F 0 "VCCO_3V3_33" H 2000 4400 20  0000 C CNN
F 1 "+VCCO_3V3" H 2000 4330 30  0000 C CNN
F 2 "" H 2000 4400 70  0000 C CNN
F 3 "" H 2000 4400 70  0000 C CNN
	1    2000 4400
	1    0    0    -1  
$EndComp
$Comp
L Cryptech_Alpha:VCCO_3V3 VCCO_3V3_34
U 1 1 58023F2E
P 7000 4700
F 0 "VCCO_3V3_34" H 7000 4700 20  0000 C CNN
F 1 "+VCCO_3V3" H 7000 4630 30  0000 C CNN
F 2 "" H 7000 4700 70  0000 C CNN
F 3 "" H 7000 4700 70  0000 C CNN
	1    7000 4700
	1    0    0    -1  
$EndComp
Wire Wire Line
	2000 5100 1800 5100
Wire Wire Line
	2000 5000 2000 5100
Wire Wire Line
	2000 4900 2000 5000
Wire Wire Line
	2000 4800 2000 4900
Wire Wire Line
	2000 4700 2000 4800
Wire Wire Line
	2000 4600 2000 4700
Wire Wire Line
	2000 4600 1800 4600
Wire Wire Line
	2000 4700 1800 4700
Wire Wire Line
	2000 4800 1800 4800
Wire Wire Line
	2000 4900 1800 4900
Wire Wire Line
	2000 5000 1800 5000
Wire Wire Line
	2000 4400 2000 4600
Wire Wire Line
	7000 5000 6750 5000
Wire Wire Line
	7000 4700 7000 5000
Wire Wire Line
	7000 5100 6750 5100
Wire Wire Line
	7000 5000 7000 5100
Wire Wire Line
	7000 5200 6750 5200
Wire Wire Line
	7000 5100 7000 5200
Wire Wire Line
	7000 5300 6750 5300
Wire Wire Line
	7000 5200 7000 5300
Wire Wire Line
	7000 5400 6750 5400
Wire Wire Line
	7000 5300 7000 5400
Wire Wire Line
	7890 7800 6750 7800
Text GLabel 7890 7800 2 48 Output ~ 0
FMC_CLK
Wire Wire Line
	2950 5200 1800 5200
Wire Wire Line
	2950 5300 1800 5300
Wire Wire Line
	2950 5400 1800 5400
Text GLabel 2950 5400 2 48 BiDi ~ 0
FMC_D2
Wire Wire Line
	2950 5500 1800 5500
Text GLabel 2950 5500 2 48 BiDi ~ 0
FMC_D3
Wire Wire Line
	2950 5600 1800 5600
Text GLabel 7900 6800 2 48 BiDi ~ 0
FMC_D4
Text GLabel 7900 7300 2 48 BiDi ~ 0
FMC_D5
Text GLabel 7900 7600 2 48 BiDi ~ 0
FMC_D6
Wire Wire Line
	2950 5900 1800 5900
Text GLabel 7900 7700 2 48 BiDi ~ 0
FMC_D7
Wire Wire Line
	2950 6100 1800 6100
Wire Wire Line
	2950 6200 1800 6200
Wire Wire Line
	2950 6300 1800 6300
Wire Wire Line
	2950 6400 1800 6400
Wire Wire Line
	2950 6500 1800 6500
Text GLabel 2950 6500 2 48 BiDi ~ 0
FMC_D13
Wire Wire Line
	2950 6600 1800 6600
Text GLabel 2950 6600 2 48 BiDi ~ 0
FMC_D14
Wire Wire Line
	2950 6700 1800 6700
Text GLabel 2950 6800 2 48 BiDi ~ 0
FMC_D15
Wire Wire Line
	2950 6800 1800 6800
Text GLabel 2950 6700 2 48 BiDi ~ 0
FMC_D16
Wire Wire Line
	2950 6900 1800 6900
Text GLabel 2950 6900 2 48 BiDi ~ 0
FMC_D17
Wire Wire Line
	2950 7000 1800 7000
Text GLabel 2950 7000 2 48 BiDi ~ 0
FMC_D18
Wire Wire Line
	2950 7100 1800 7100
Text GLabel 2950 7100 2 48 BiDi ~ 0
FMC_D19
Wire Wire Line
	2950 7200 1800 7200
Text GLabel 2950 7200 2 48 BiDi ~ 0
FMC_D20
Wire Wire Line
	2950 7300 1800 7300
Text GLabel 2950 7300 2 48 BiDi ~ 0
FMC_D21
Wire Wire Line
	2950 7400 1800 7400
Text GLabel 2950 7400 2 48 BiDi ~ 0
FMC_D22
Wire Wire Line
	2950 7500 1800 7500
Text GLabel 2950 7500 2 48 BiDi ~ 0
FMC_D23
Wire Wire Line
	2950 7700 1800 7700
Text GLabel 2950 7700 2 48 BiDi ~ 0
FMC_D25
Text GLabel 2950 8400 2 48 Input ~ 0
FMC_NE1
Wire Wire Line
	2960 8700 1800 8700
Text GLabel 2960 8700 2 48 Output ~ 0
FMC_NWAIT
Wire Wire Line
	2960 8800 1800 8800
Text GLabel 2960 8800 2 48 Input ~ 0
FMC_NWE
Wire Wire Line
	7900 5500 6750 5500
Text GLabel 7900 5500 2 48 Input ~ 0
FMC_A0
Wire Wire Line
	7900 5600 6750 5600
Text GLabel 7900 5800 2 48 Input ~ 0
FMC_A1
Wire Wire Line
	7900 5700 6750 5700
Text GLabel 7900 5700 2 48 Input ~ 0
FMC_A2
Wire Wire Line
	7900 5800 6750 5800
Text GLabel 7900 5600 2 48 Input ~ 0
FMC_A3
Wire Wire Line
	7900 5900 6750 5900
Text GLabel 7900 5900 2 48 Input ~ 0
FMC_A4
Wire Wire Line
	7900 6000 6750 6000
Text GLabel 7900 6000 2 48 Input ~ 0
FMC_A5
Wire Wire Line
	7900 6100 6750 6100
Text GLabel 7900 6100 2 48 Input ~ 0
FMC_A6
Wire Wire Line
	7900 6200 6750 6200
Text GLabel 7900 6200 2 48 Input ~ 0
FMC_A7
Wire Wire Line
	7900 6300 6750 6300
Text GLabel 7900 6300 2 48 Input ~ 0
FMC_A8
Wire Wire Line
	7900 6400 6750 6400
Text GLabel 7900 6400 2 48 Input ~ 0
FMC_A9
Wire Wire Line
	7900 6500 6750 6500
Text GLabel 7900 6500 2 48 Input ~ 0
FMC_A10
Wire Wire Line
	7900 6600 6750 6600
Text GLabel 7900 6700 2 48 Input ~ 0
FMC_A11
Wire Wire Line
	7900 6700 6750 6700
Text GLabel 7900 7100 2 48 Input ~ 0
FMC_A12
Wire Wire Line
	7900 6800 6750 6800
Text GLabel 2950 5600 2 48 Input ~ 0
FMC_A13
Wire Wire Line
	7900 6900 6750 6900
Text GLabel 7900 6900 2 48 Input ~ 0
FMC_A14
Wire Wire Line
	7900 7000 6750 7000
Text GLabel 2970 9600 2 48 Input ~ 0
FMC_A15
Wire Wire Line
	7900 7100 6750 7100
Text GLabel 7900 7000 2 48 Input ~ 0
FMC_A16
Text GLabel 2970 9500 2 48 Input ~ 0
FMC_A17
Wire Wire Line
	7900 7300 6750 7300
Text GLabel 2970 9800 2 48 Input ~ 0
FMC_A18
Wire Wire Line
	7900 7500 6750 7500
Wire Wire Line
	7900 7600 6750 7600
Wire Wire Line
	7900 7700 6750 7700
Wire Wire Line
	7900 7900 6750 7900
Wire Wire Line
	7900 8000 6750 8000
Wire Wire Line
	7900 8100 6750 8100
Wire Bus Line
	9070 5000 8470 5000
Text Label 8470 5000 0 60 ~
FMC_A[0..25]
Wire Bus Line
	3860 5000 3260 5000
Text Label 3260 5000 0 60 ~
FMC_D[0..31]
Wire Wire Line
	2970 9600 1800 9600
Wire Wire Line
	2970 9200 1800 9200
Text GLabel 2970 9100 2 48 BiDi ~ 0
FMC_D0
Wire Wire Line
	2970 9100 1800 9100
Text GLabel 2970 9200 2 48 BiDi ~ 0
FMC_D1
Wire Wire Line
	2970 9800 1800 9800
Wire Wire Line
	2970 9500 1800 9500
Text GLabel 7900 7500 2 48 BiDi ~ 0
FMC_D24
Text GLabel 7900 7900 2 48 BiDi ~ 0
FMC_D26
Text GLabel 7900 6600 2 48 BiDi ~ 0
FMC_D27
Text GLabel 7910 8700 2 48 Input ~ 0
FMC_NOE
Wire Wire Line
	7910 8700 6750 8700
Wire Wire Line
	2950 8400 1800 8400
Text GLabel 2950 5900 2 48 Output ~ 0
MKM_FPGA_CS_N
Text GLabel 2950 6400 2 48 Output ~ 0
MKM_FPGA_SCK
Text GLabel 2950 6100 2 48 Output ~ 0
MKM_FPGA_MOSI
Text GLabel 2950 6200 2 48 Input ~ 0
MKM_FPGA_MISO
Text GLabel 2950 6300 2 48 Output ~ 0
FPGA_GPIO_LED_0
Text GLabel 2950 5300 2 48 Output ~ 0
FPGA_GPIO_LED_1
$Comp
L Cryptech_Alpha:XC7A200TFBG484_2 U13_9
U 1 1 58023F2D
P 6550 6900
F 0 "U13_9" H 6140 4690 60  0000 L BNN
	1    6550 6900
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:BGA484C100P22X22_2300X2300X254" H 6140 4690 60  0001 C CNN
$EndComp
$Comp
L Cryptech_Alpha:XC7A200TFBG484_6 U13_10
U 1 1 58023F2C
P 1600 7300
F 0 "U13_10" H 1190 4290 60  0000 L BNN
	1    1600 7300
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:BGA484C100P22X22_2300X2300X254" H 1190 4290 60  0001 C CNN
$EndComp
NoConn ~ 1800 5700
NoConn ~ 1800 5800
NoConn ~ 1800 6000
NoConn ~ 1800 7600
NoConn ~ 1800 7800
NoConn ~ 1800 7900
NoConn ~ 1800 8000
NoConn ~ 1800 8100
NoConn ~ 1800 8200
NoConn ~ 1800 8300
NoConn ~ 1800 8500
NoConn ~ 1800 8600
NoConn ~ 1800 8900
NoConn ~ 1800 9000
NoConn ~ 1800 9400
NoConn ~ 1800 9300
NoConn ~ 1800 9700
NoConn ~ 1800 9900
NoConn ~ 1800 10000
NoConn ~ 1800 10100
NoConn ~ 6750 8200
NoConn ~ 6750 8300
NoConn ~ 6750 8400
NoConn ~ 6750 8500
NoConn ~ 6750 8600
NoConn ~ 6750 8800
NoConn ~ 6750 8900
NoConn ~ 6750 7400
NoConn ~ 6750 7200
NoConn ~ 2950 5200
NoConn ~ 7900 8000
NoConn ~ 7900 8100
$EndSCHEMATC
