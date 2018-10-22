EESchema Schematic File Version 4
EELAYER 26 0
EELAYER END
$Descr B 17000 11000
encoding utf-8
Sheet 9 27
Title "rev02_07"
Date "15 10 2016"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text Notes 8900 5200 0    60   ~ 12
*) HOLD feature not used
Text Notes 6400 3800 0    84   ~ 12
This memory holds cryptographic keys\nwrapped with the master key.
Text Notes 6400 3200 0    126   ~ 12
Keystore memory, 128 Mbit
Text Notes 13640 10230 0    84   ~ 12
Keystore memory
Text Notes 7190 5380 0    60   ~ 12
IC1
Text Notes 7760 5380 0    60   ~ 12
N25Q128A13ES
Text Notes 9760 5990 0    60   ~ 12
C69
Text Notes 9760 6190 0    60   ~ 12
0.1~uF
$Comp
L power:GND GND_44
U 1 1 5802402E
P 6800 6500
F 0 "GND_44" H 6800 6500 20  0000 C CNN
F 1 "+GND" H 6800 6430 30  0000 C CNN
F 2 "" H 6800 6500 70  0000 C CNN
F 3 "" H 6800 6500 70  0000 C CNN
	1    6800 6500
	1    0    0    -1  
$EndComp
$Comp
L power:GND GND_45
U 1 1 5802402D
P 9700 6500
F 0 "GND_45" H 9700 6500 20  0000 C CNN
F 1 "+GND" H 9700 6430 30  0000 C CNN
F 2 "" H 9700 6500 70  0000 C CNN
F 3 "" H 9700 6500 70  0000 C CNN
	1    9700 6500
	1    0    0    -1  
$EndComp
$Comp
L Cryptech_Alpha:VCCO_3V3 VCCO_3V3_14
U 1 1 5802402C
P 6200 4400
F 0 "VCCO_3V3_14" H 6200 4400 20  0000 C CNN
F 1 "+VCCO_3V3" H 6200 4330 30  0000 C CNN
F 2 "" H 6200 4400 70  0000 C CNN
F 3 "" H 6200 4400 70  0000 C CNN
	1    6200 4400
	1    0    0    -1  
$EndComp
Wire Wire Line
	6800 5800 6800 6500
Wire Wire Line
	7000 5800 6800 5800
Wire Wire Line
	9700 6200 9700 6500
Wire Wire Line
	6200 4400 6200 4700
Wire Wire Line
	8700 5500 8450 5500
Wire Wire Line
	8700 4700 8700 5500
Wire Wire Line
	7000 4700 6500 4700
Wire Wire Line
	8700 4700 7000 4700
Wire Wire Line
	6500 4700 6200 4700
Wire Wire Line
	8700 5600 8450 5600
Wire Wire Line
	8700 5500 8700 5600
Wire Wire Line
	9700 5600 9700 5900
Wire Wire Line
	9700 5600 8700 5600
Wire Wire Line
	7000 4700 7000 4900
Wire Wire Line
	6200 4700 6200 4900
Connection ~ 8700 5500
Connection ~ 8700 5600
Connection ~ 7000 4700
Connection ~ 6200 4700
Wire Wire Line
	7000 5500 6500 5500
Wire Wire Line
	7000 5300 7000 5500
Wire Wire Line
	6500 5500 6270 5500
Text GLabel 6270 5500 0 48 Input ~ 0
KSM_PROM_CS_N
Connection ~ 7000 5500
Wire Wire Line
	8600 5800 8450 5800
Text GLabel 8600 5800 2 48 Input ~ 0
KSM_PROM_MOSI
Wire Wire Line
	8600 5700 8450 5700
Text GLabel 8600 5700 2 48 Input ~ 0
KSM_PROM_SCLK
Wire Wire Line
	7000 5600 6270 5600
Text GLabel 6270 5600 0 48 Output ~ 0
KSM_PROM_MISO
Wire Wire Line
	7000 5700 6200 5700
Wire Wire Line
	6200 5300 6200 5700
$Comp
L Cryptech_Alpha:N25Q128A13ES IC1
U 1 1 5802402B
P 7700 5750
F 0 "IC1" H 7190 5490 60  0000 L BNN
F 1 "N25Q128A13ESE*" H 7470 6120 60  0000 L BNN
F 2 "Cryptech_Alpha_Footprints:SO08" H 7470 6120 60  0001 C CNN
F 3 "" H 7470 6120 60  0000 C CNN
	1    7700 5750
	1    0    0    -1
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0402 R18
U 1 1 5802402A
P 7000 5100
F 0 "R18" V 6941 5250 60  0000 L BNN
F 1 "4.7k" V 7040 5240 60  0000 L BNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 7040 5240 60  0001 C CNN
F 3 "" H 7040 5240 60  0000 C CNN
	1    7000 5100
	0    1    1    0
$EndComp
$Comp
L Cryptech_Alpha:C-EUC0402 C69
U 1 1 58024029
P 9700 6000
F 0 "C69" H 9780 5810 60  0000 L BNN
F 1 "0.1uF" H 9990 6010 60  0000 L BNN
F 2 "Cryptech_Alpha_Footprints:C_0402" H 9990 6010 60  0001 C CNN
F 3 "" H 9990 6010 60  0000 C CNN
	1    9700 6000
	1    0    0    -1
$EndComp
$Comp
L Cryptech_Alpha:R-EU_R0402 R17
U 1 1 58024028
P 6200 5100
F 0 "R17" V 6140 4780 60  0000 L BNN
F 1 "4.7k" V 6250 4780 60  0000 L BNN
F 2 "Cryptech_Alpha_Footprints:R_0402" H 6250 4780 60  0001 C CNN
F 3 "" H 6250 4780 60  0000 C CNN
	1    6200 5100
	0    1    1    0
$EndComp
$EndSCHEMATC
