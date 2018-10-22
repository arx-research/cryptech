onerror {resume}
quietly WaveActivateNextPane {} 0
add wave -noupdate -radix hexadecimal /tb_coretest/dut/clk
add wave -noupdate -radix hexadecimal /tb_coretest/dut/reset_n
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_syn
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_data
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_ack
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_syn
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_data
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_ack
add wave -noupdate -divider <NULL>
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_reset_n
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_cs
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_we
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_address
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_write_data
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_read_data
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_error
add wave -noupdate -divider <NULL>
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_reset_n_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_cs_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_we_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/send_response_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/response_sent_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/cmd_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_addr_byte0_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_addr_byte1_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_wr_data_byte0_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_wr_data_byte1_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_wr_data_byte2_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_wr_data_byte3_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_read_data_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/core_error_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/sample_core_output
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_rd_ptr_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_rd_ptr_inc
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_wr_ptr_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_wr_ptr_inc
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_ctr_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_ctr_inc
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_ctr_dec
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_full
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer_empty
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_buffer
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_ptr_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_ptr_inc
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_engine_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_engine_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/test_engine_reg
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed0
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed1
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed2
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed3
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed4
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed5
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed6
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed7
add wave -noupdate -radix hexadecimal /tb_coretest/dut/tx_buffer_muxed8
add wave -noupdate -radix hexadecimal /tb_coretest/dut/update_tx_buffer
add wave -noupdate -radix hexadecimal /tb_coretest/dut/response_type
add wave -noupdate -radix hexadecimal /tb_coretest/dut/rx_byte
TreeUpdate [SetDefaultTree]
WaveRestoreCursors {{Cursor 1} {361000 ps} 0}
quietly wave cursor active 1
configure wave -namecolwidth 290
configure wave -valuecolwidth 203
configure wave -justifyvalue left
configure wave -signalnamewidth 0
configure wave -snapdistance 10
configure wave -datasetprefix 0
configure wave -rowmargin 4
configure wave -childrowmargin 2
configure wave -gridoffset 0
configure wave -gridperiod 1
configure wave -griddelta 40
configure wave -timeline 0
configure wave -timelineunits ps
update
WaveRestoreZoom {0 ps} {451500 ps}
