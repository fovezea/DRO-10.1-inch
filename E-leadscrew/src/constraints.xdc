## Clock signal
set_property -dict { PACKAGE_PIN H4    IOSTANDARD LVCMOS33 } [get_ports { clk }]; #IO_L13P_T2_MRCC_35 Sch=sysclk
create_clock -add -name sys_clk_pin -period 10.00 -waveform {0 5} [get_ports { clk }];

## Buttons
set_property -dict { PACKAGE_PIN C3    IOSTANDARD LVCMOS33 } [get_ports { rst_n }]; #IO_L1N_T0_AD4N_35 Sch=btn[0]

## UART
set_property -dict { PACKAGE_PIN C12   IOSTANDARD LVCMOS33 } [get_ports { uart_rx }]; #IO_L11N_T1_SRCC_16 Sch=uart_rx
set_property -dict { PACKAGE_PIN A12   IOSTANDARD LVCMOS33 } [get_ports { uart_tx }]; #IO_L11P_T1_SRCC_16 Sch=uart_tx

## LEDs
set_property -dict { PACKAGE_PIN J1    IOSTANDARD LVCMOS33 } [get_ports { led_test }]; #IO_L2P_T0_AD8P_35 Sch=led[0]

## Arduino Headers (GPIO)
# enc_A: Arduino D2
set_property -dict { PACKAGE_PIN A10   IOSTANDARD LVCMOS33 } [get_ports { enc_A }];
# enc_B: Arduino D3
set_property -dict { PACKAGE_PIN B6    IOSTANDARD LVCMOS33 } [get_ports { enc_B }];
# step_out_1: Arduino D4
set_property -dict { PACKAGE_PIN A5    IOSTANDARD LVCMOS33 } [get_ports { step_out_1 }];
# dir_out_1: Arduino D5
set_property -dict { PACKAGE_PIN B5    IOSTANDARD LVCMOS33 } [get_ports { dir_out_1 }];
# step_out_2: Arduino D6
set_property -dict { PACKAGE_PIN A4    IOSTANDARD LVCMOS33 } [get_ports { step_out_2 }];
# dir_out_2: Arduino D7
set_property -dict { PACKAGE_PIN A3    IOSTANDARD LVCMOS33 } [get_ports { dir_out_2 }];

## ----------------------------------------------------------------------------
## Extended I/O Options (Uncomment to use)
## ----------------------------------------------------------------------------

## FPGA GPIO Header (10-pin)
# set_property -dict { PACKAGE_PIN N14   IOSTANDARD LVCMOS33 } [get_ports { io0 }];
# set_property -dict { PACKAGE_PIN M14   IOSTANDARD LVCMOS33 } [get_ports { io1 }];
# set_property -dict { PACKAGE_PIN C4    IOSTANDARD LVCMOS33 } [get_ports { io2 }];
# set_property -dict { PACKAGE_PIN B13   IOSTANDARD LVCMOS33 } [get_ports { io3 }];
# set_property -dict { PACKAGE_PIN N10   IOSTANDARD LVCMOS33 } [get_ports { io4 }];
# set_property -dict { PACKAGE_PIN M10   IOSTANDARD LVCMOS33 } [get_ports { io5 }];
# set_property -dict { PACKAGE_PIN B14   IOSTANDARD LVCMOS33 } [get_ports { io6 }];
# set_property -dict { PACKAGE_PIN D3    IOSTANDARD LVCMOS33 } [get_ports { io7 }];
# set_property -dict { PACKAGE_PIN P5    IOSTANDARD LVCMOS33 } [get_ports { io8 }];
# set_property -dict { PACKAGE_PIN E11   IOSTANDARD LVCMOS33 } [get_ports { io9 }];

## Grove Connectors
# set_property -dict { PACKAGE_PIN P13   IOSTANDARD LVCMOS33 } [get_ports { grove_sda }];
# set_property -dict { PACKAGE_PIN P12   IOSTANDARD LVCMOS33 } [get_ports { grove_scl }];

## User Switches (K1-K4)
# set_property -dict { PACKAGE_PIN M2    IOSTANDARD LVCMOS33 } [get_ports { sw1 }];
# set_property -dict { PACKAGE_PIN L2    IOSTANDARD LVCMOS33 } [get_ports { sw2 }];
# set_property -dict { PACKAGE_PIN L3    IOSTANDARD LVCMOS33 } [get_ports { sw3 }];
# set_property -dict { PACKAGE_PIN K3    IOSTANDARD LVCMOS33 } [get_ports { sw4 }];

## User Buttons (USER2) - USER1 is Reset
# set_property -dict { PACKAGE_PIN M4    IOSTANDARD LVCMOS33 } [get_ports { btn_user2 }];

## Additional LEDs
# set_property -dict { PACKAGE_PIN A13   IOSTANDARD LVCMOS33 } [get_ports { led_red }];