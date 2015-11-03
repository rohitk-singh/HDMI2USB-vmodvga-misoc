from migen.fhdl.std import *
from migen.bank.description import *

from hdl.hdmi_in.dma import DMA
from hdl.vga.analysis import FrameExtraction


class VGAIn(Module, AutoCSR):
    def __init__(self, pads, lasmim, n_dma_slots=2, fifo_depth=512 ):

        self.submodules.frame = FrameExtraction(lasmim.dw, fifo_depth)
        
        self.submodules.dma = DMA(lasmim, n_dma_slots)
        self.comb += self.frame.frame.connect(self.dma.frame)
        self.ev = self.dma.ev

    autocsr_exclude = {"ev"}


