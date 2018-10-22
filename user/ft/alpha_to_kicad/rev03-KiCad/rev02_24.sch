EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr B 17000 11000
encoding utf-8
Sheet 26 27
Title "rev02_24"
Date "15 10 2016"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 4300 4100 0    60   ~ 12
*) FPGA Power Subsystem -- CORE
Text Notes 7150 5310 0    60   ~ 12
*) VCCINT = 0.6V x (1 + 150 / 226) = 0.998V\n*) OCP_ADJ is not used (default over-current threshold)\n*) MARx are not used (output at nominal 100%)\n*) S_IN/S_OUT are not used (single regulator mode)\n*) S_DELAY is not used (single regulator mode)\n*) M/S is not used (parallel operation not needed)\n*) EA_OUT is not used (default control loop)\n*) Minimal load current is 0A, but we still place\nload of 100 Ohms just in case (gives 10 mA)
Text Notes 8520 10200 0    54   ~ 12
FPGA CORE voltage regulators
Text Notes 2870 6680 0    60   ~ 12
R66
Text Notes 1860 6590 0    60   ~ 12
C205
Text Notes 1860 6790 0    60   ~ 12
22~uF
Text Notes 2260 6590 0    60   ~ 12
C206
Text Notes 2260 6790 0    60   ~ 12
22~uF
Text Notes 5660 6630 0    60   ~ 12
R68
Text Notes 5650 7260 0    60   ~ 12
R69
Text Notes 6330 6590 0    60   ~ 12
C208
Text Notes 6330 6680 0    60   ~ 12
27pF
Text Notes 7690 7090 0    60   ~ 12
R70
Text Notes 5260 7280 0    60   ~ 12
R67
Text Notes 6660 7090 0    60   ~ 12
C209
Text Notes 6660 7290 0    60   ~ 12
47uF
Text Notes 7160 7090 0    60   ~ 12
C210
Text Notes 7160 7290 0    60   ~ 12
47uF
$Comp
L power:GND GND_216
U 1 1 58023E36
P 4200 9500
F 0 "GND_216" H 4200 9500 20  0000 C CNN
F 1 "+GND" H 4200 9430 30  0000 C CNN
F 2 "" H 4200 9500 70  0000 C CNN
F 3 "" H 4200 9500 70  0000 C CNN
	1    4200 9500
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_217
U 1 1 58023E35
P 4700 9500
F 0 "GND_217" H 4700 9500 20  0000 C CNN
F 1 "+GND" H 4700 9430 30  0000 C CNN
F 2 "" H 4700 9500 70  0000 C CNN
F 3 "" H 4700 9500 70  0000 C CNN
	1    4700 9500
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_218
U 1 1 58023E34
P 3400 9500
F 0 "GND_218" H 3400 9500 20  0000 C CNN
F 1 "+GND" H 3400 9430 30  0000 C CNN
F 2 "" H 3400 9500 70  0000 C CNN
F 3 "" H 3400 9500 70  0000 C CNN
	1    3400 9500
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_219
U 1 1 58023E33
P 1800 7000
F 0 "GND_219" H 1800 7000 20  0000 C CNN
F 1 "+GND" H 1800 6930 30  0000 C CNN
F 2 "" H 1800 7000 70  0000 C CNN
F 3 "" H 1800 7000 70  0000 C CNN
	1    1800 7000
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_220
U 1 1 58023E32
P 2200 7000
F 0 "GND_220" H 2200 7000 20  0000 C CNN
F 1 "+GND" H 2200 6930 30  0000 C CNN
F 2 "" H 2200 7000 70  0000 C CNN
F 3 "" H 2200 7000 70  0000 C CNN
	1    2200 7000
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_221
U 1 1 58023E31
P 5900 7600
F 0 "GND_221" H 5900 7600 20  0000 C CNN
F 1 "+GND" H 5900 7530 30  0000 C CNN
F 2 "" H 5900 7600 70  0000 C CNN
F 3 "" H 5900 7600 70  0000 C CNN
	1    5900 7600
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_222
U 1 1 58023E30
P 6600 7600
F 0 "GND_222" H 6600 7600 20  0000 C CNN
F 1 "+GND" H 6600 7530 30  0000 C CNN
F 2 "" H 6600 7600 70  0000 C CNN
F 3 "" H 6600 7600 70  0000 C CNN
	1    6600 7600
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_223
U 1 1 58023E2F
P 7100 7600
F 0 "GND_223" H 7100 7600 20  0000 C CNN
F 1 "+GND" H 7100 7530 30  0000 C CNN
F 2 "" H 7100 7600 70  0000 C CNN
F 3 "" H 7100 7600 70  0000 C CNN
	1    7100 7600
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_224
U 1 1 58023E2E
P 7600 7600
F 0 "GND_224" H 7600 7600 20  0000 C CNN
F 1 "+GND" H 7600 7530 30  0000 C CNN
F 2 "" H 7600 7600 70  0000 C CNN
F 3 "" H 7600 7600 70  0000 C CNN
	1    7600 7600
	1    0    0    -1  
$EndComp
$Comp
L Cryptech_Alpha:VCC_5V0 VCC_5V0_3
U 1 1 58023E2D
P 1600 6200
F 0 "VCC_5V0_3" H 1600 6200 20  0000 C CNN
F 1 "+VCC_5V0" H 1600 6130 30  0000 C CNN
F 2 "" H 1600 6200 70  0000 C CNN
F 3 "" H 1600 6200 70  0000 C CNN
	1    1600 6200
	1    0    0    -1  
$EndComp
Wire Wire Line
	3400 9400 3400 9500
Wire Wire Line
	1800 6800 1800 7000
Wire Wire Line
	2200 6800 2200 7000
Wire Wire Line
	5900 7500 5900 7600
Wire Wire Line
	6600 7300 6600 7600
Wire Wire Line
	7100 7300 7100 7600
Wire Wire Line
	7600 7400 7600 7600
Wire Wire Line
	8300 6400 8100 6400
Text Label 8300 6400 0 48 ~ 0
FPGA_VCCINT_1V0
Wire Wire Line
	1600 6200 1600 6400
Wire Wire Line
	3400 6400 3400 6600
Wire Wire Line
	3600 6400 3400 6400
Wire Wire Line
	3600 6600 3400 6600
Wire Wire Line
	3400 6400 2800 6400
Wire Wire Line
	2800 6400 2200 6400
Wire Wire Line
	2200 6400 1800 6400
Wire Wire Line
	1800 6400 1800 6500
Wire Wire Line
	2200 6400 2200 6500
Wire Wire Line
	2800 6400 2800 6500
Wire Wire Line
	1800 6400 1600 6400
Connection ~ 3400 6400
Connection ~ 2200 6400
Connection ~ 2800 6400
Connection ~ 1800 6400
Wire Wire Line
	5600 9100 5300 9100
Text GLabel 5600 9100 2 48 Output ~ 0
POK_VCCINT
Wire Wire Line
	3600 9100 3400 9100
Wire Wire Line
	3600 7000 2800 7000
Wire Wire Line
	2800 6900 2800 7000
Wire Wire Line
	3600 6800 3200 6800
Wire Wire Line
	3200 5900 3200 6800
Wire Wire Line
	3200 5900 2700 5900
Text GLabel 2700 5900 0 48 Input ~ 0
PWR_ENA_VCCINT
Wire Wire Line
	5500 7600 5300 7600
Wire Wire Line
	5500 7500 5500 7600
Wire Wire Line
	5500 6400 5500 7100
Wire Wire Line
	5500 6400 5300 6400
Wire Wire Line
	5900 6400 5500 6400
Wire Wire Line
	5900 6400 5900 6500
Wire Wire Line
	6200 6400 5900 6400
Wire Wire Line
	6200 6400 6200 6500
Wire Wire Line
	6600 6400 6600 7000
Wire Wire Line
	6600 6400 6200 6400
Wire Wire Line
	7100 6400 7100 7000
Wire Wire Line
	7100 6400 6600 6400
Wire Wire Line
	7600 6400 7600 7000
Wire Wire Line
	7600 6400 7100 6400
Wire Wire Line
	7700 6400 7600 6400
Connection ~ 5500 6400
Connection ~ 5900 6400
Connection ~ 6200 6400
Connection ~ 6600 6400
Connection ~ 7100 6400
Connection ~ 7600 6400
Wire Wire Line
	5900 7000 5300 7000
Wire Wire Line
	5900 6900 5900 7000
Wire Wire Line
	5900 7000 5900 7100
Wire Wire Line
	6200 7000 5900 7000
Wire Wire Line
	6200 6800 6200 7000
Connection ~ 5900 7000
Text Notes 3900 5400 0    72   ~ 12
U16
Wire Wire Line
	3600 5600 3600 5500
Wire Wire Line
	3600 5700 3600 5600
Wire Wire Line
	3600 5800 3600 5700
Wire Wire Line
	3600 5900 3600 5800
Wire Wire Line
	3600 6000 3600 5900
Wire Wire Line
	3600 6100 3600 6000
Wire Wire Line
	3600 6200 3600 6100
Wire Wire Line
	3600 6300 3600 6200
Wire Wire Line
	3600 6400 3600 6300
Wire Wire Line
	5300 5700 5300 5600
Wire Wire Line
	5300 5800 5300 5700
Wire Wire Line
	5300 5900 5300 5800
Wire Wire Line
	5300 6000 5300 5900
Wire Wire Line
	5300 6100 5300 6000
Wire Wire Line
	5300 6200 5300 6100
Wire Wire Line
	5300 6300 5300 6200
Wire Wire Line
	5300 6400 5300 6300
Text Notes 3110 9170 0    60   ~ 12
C116
Text Notes 2800 9370 0    60   ~ 12
0.047~uF
Connection ~ 3600 6400
Connection ~ 3600 6300
Connection ~ 3600 6200
Connection ~ 3600 6100
Connection ~ 3600 6000
Connection ~ 3600 5900
Connection ~ 3600 5800
Connection ~ 3600 5700
Connection ~ 3600 5600
Connection ~ 5300 6400
Connection ~ 5300 6300
Connection ~ 5300 6200
Connection ~ 5300 6100
Connection ~ 5300 6000
Connection ~ 5300 5900
Connection ~ 5300 5800
Connection ~ 5300 5700
$Comp
L Cryptech_Alpha:R-EU_R0402 R66
U 1 1 58023E2C
P 2800 6700
F 0 "R66" V 2710 6655 60  0000 R TNN
F 1 "4.7k" V 2730 6630 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 2730 6630 60  0001 C CNN
F 3 "" H 2730 6630 60  0000 C CNN
	1    2800 6700
	0    -1   -1   0
$EndComp
$Comp
L Cryptech_Alpha:C-EUC1210 C209
U 1 1 58023E2B
P 6600 7100
F 0 "C209" H 6680 6910 60  0000 L BNN
	1    6600 7100
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:C_1210" H 6680 6910 60  0001 C CNN
$EndComp
$Comp
L Cryptech_Alpha:C-EUC1210 C210
U 1 1 58023E2A
P 7100 7100
F 0 "C210" H 7180 6910 60  0000 L BNN
	1    7100 7100
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:C_1210" H 7180 6910 60  0001 C CNN
$EndComp
$Comp
L Cryptech_Alpha:EN5364QI U16
U 1 1 58023E29
P 4400 7700
F 0 "U16" H 3950 10020 60  0000 L BNN
F 1 "EN5364QI" H 4510 10030 60  0000 L BNN
F 2 "Cryptech_Alpha_Footprints:QFN68" H 4510 10030 60  0001 C CNN
F 3 "" H 4510 10030 60  0000 C CNN
	1    4400 7700
	1    0    0    -1
$EndComp
$Comp
L Cryptech_Alpha:C-EUC0402 C207
U 1 1 58023E28
P 3400 9200
F 0 "C207" H 2800 9150 60  0000 L BNN
	1    3400 9200
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:C_0402" H 2800 9150 60  0001 C CNN
$EndComp
$Comp
L Cryptech_Alpha:C-EUC1210 C205
U 1 1 58023E27
P 1800 6600
F 0 "C205" H 1880 6410 60  0000 L BNN
	1    1800 6600
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:C_1210" H 1880 6410 60  0001 C CNN
$EndComp
$Comp
L Cryptech_Alpha:C-EUC1210 C206
U 1 1 58023E26
P 2200 6600
F 0 "C206" H 2280 6410 60  0000 L BNN
	1    2200 6600
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:C_1210" H 2280 6410 60  0001 C CNN
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0402 R68
U 1 1 58023E25
P 5900 6700
F 0 "R68" V 5810 6655 60  0000 R TNN
F 1 "150k" V 5870 6950 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 5870 6950 60  0001 C CNN
F 3 "" H 5870 6950 60  0000 C CNN
	1    5900 6700
	0    -1   -1   0
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0402 R69
U 1 1 58023E24
P 5900 7300
F 0 "R69" V 5810 7255 60  0000 R TNN
F 1 "226k" V 5850 7550 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 5850 7550 60  0001 C CNN
F 3 "" H 5850 7550 60  0000 C CNN
	1    5900 7300
	0    -1   -1   0
$EndComp
$Comp
L Cryptech_Alpha:C-EUC0402 C208
U 1 1 58023E23
P 6200 6600
F 0 "C208" H 6280 6410 60  0000 L BNN
	1    6200 6600
	1    0    0    -1
F 2 "Cryptech_Alpha_Footprints:C_0402" H 6280 6410 60  0001 C CNN
$EndComp
$Comp
L Cryptech_Alpha:BLM31PG330SN1_1206 FB7
U 1 1 58023E22
P 7900 6500
F 0 "FB7" H 7750 6700 60  0000 L BNN
F 1 "BLM31PG330SN1" H 7750 6400 60  0000 L BNN
F 2 "Cryptech_Alpha_Footprints:L_1206" H 7750 6400 60  0001 C CNN
F 3 "" H 7750 6400 60  0000 C CNN
	1    7900 6500
	1    0    0    -1
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0402 R70
U 1 1 58023E21
P 7600 7200
F 0 "R70" V 7510 7155 60  0000 R TNN
F 1 "100" V 7610 7100 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 7610 7100 60  0001 C CNN
F 3 "" H 7610 7100 60  0000 C CNN
	1    7600 7200
	0    -1   -1   0
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0402 R67
U 1 1 58023E20
P 5500 7300
F 0 "R67" V 5410 7255 60  0000 R TNN
F 1 "0" V 5440 7540 60  0000 R TNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 5440 7540 60  0001 C CNN
F 3 "" H 5440 7540 60  0000 C CNN
	1    5500 7300
	0    -1   -1   0
$EndComp
NoConn ~ 5300 8100
NoConn ~ 5300 8300
NoConn ~ 3600 7400
NoConn ~ 3600 7600
NoConn ~ 3600 7800
NoConn ~ 3600 8000
NoConn ~ 3600 8200
NoConn ~ 3600 8700
NoConn ~ 5300 8700
Wire Wire Line
      8300 6400 8300 6200
$Comp
L Cryptech_Alpha:FPGA_VCCINT_1V0 #PWR?
U 1 1 5AF3F25C
P 8300 6200
F 0 "#PWR?" H 8300 6050 50  0001 C CNN
F 1 "FPGA_VCCINT_1V0" H 8315 6373 50  0000 C CNN
F 2 "" H 8300 6200 60  0000 C CNN
F 3 "" H 8300 6200 60  0000 C CNN
      1    8300 6200
      1    0    0    -1
$EndComp
$Comp
L power:PWR_FLAG #FLG?
U 1 1 5AFA77EC
P 8150 6400
F 0 "#FLG?" H 8150 6475 50  0001 C CNN
F 1 "PWR_FLAG" H 8150 6574 50  0000 C CNN
F 2 "" H 8150 6400 50  0001 C CNN
F 3 "~" H 8150 6400 50  0001 C CNN
       1    8150 6400
       1    0    0    -1  
$EndComp
Connection ~ 8150 6400
$EndSCHEMATC
