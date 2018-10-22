design documentation
====================

This repo contains a number of documents used during development of
various parts the Cryptech design. Note that the directory contains
source files (for example in OmniGraffle, Open Document file formats) as
well as final documents (normally in PFF file format).

* Alpha_board_drawing
  A drawing/diagram that shows the functional components and their
  functional connectivities for the Cryptech Alpha board being
  developed. The drawing is not formal schematics for the Alpha board.


* Novena-entropy-board
  Sub directory with schematics as well as layout with silk screen for
  the Cryptech Noise board designed to be used as external entropy
  source on the Novena.


* alpha_board_config
  Configuration file for setting up the I/Os and functionality of the
  STM32 microcontroller on the Alpha board. The config file is used by
  the tool [STM32CubeMX][http://www.st.com/web/catalog/tools/FM147/CL1794/SC961/SS1743/PF259242?sc=microxplorer] from ST Microelectronics.


* application-aware-signing
  A drawing that shows a proposed mechanism with data and command flows
  for doing application based (application aware) signing.


* fpga_estimates
  A document that tracks the FPGA resources and performance of the
  different Cryptech cores in different FPGA technologies. The document
  also includes estimates for cores not yet designed or completed. The
  purpose of the document is to provide estimates of FPGA devices needed
  to implement different use cases.


* hsm-board
  A high level functional diagram of the Cryptech HSM board that shows
  the major functional blocks. Used during development of the Alpha
  board to see what components are needed to realise the functionality
  of the blocks. The Alpha_board_drawing is the concrete functional
  realisation of this diagram.


* hsm-keys
  A document that includes diagrams that shows proposed mechanisms for
  handling keys including managament, protection and migration of
  wrapped keys.


* novena-memory-map
  This document contains information about the memory map of the
  Cryptech FPGA design with the specific memory maps of each core as
  well as how the cores are addressed and accessed from SW via different
  interfaces.
