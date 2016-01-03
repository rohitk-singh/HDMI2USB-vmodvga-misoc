from migen.fhdl.std import *
from migen.bank.description import *

from hdl.hdmi_in.dma import DMA
from hdl.vga.analysis import FrameExtraction
from hdl.vga.vga_gen import VGAGenerator


class VGAIn(Module, AutoCSR):
    def __init__(self, pads, lasmim, n_dma_slots=2, fifo_depth=512 ):

        self.submodules.vgagen = VGAGenerator()
        self.submodules.frame = FrameExtraction(lasmim.dw, fifo_depth)
        self.comb += [
            self.frame.valid_i.eq(self.vgagen.valid),
            self.frame.de.eq(self.vgagen.de),
            self.frame.vsync.eq(self.vgagen.vsync),
            self.frame.r.eq(self.vgagen.r),
            self.frame.g.eq(self.vgagen.g),
            self.frame.b.eq(self.vgagen.b)
        ]
        
        self.submodules.dma = DMA(lasmim, n_dma_slots)
        self.comb += self.frame.frame.connect(self.dma.frame)
        self.ev = self.dma.ev
        print("LASMI Bus Address Width : {}".format(lasmim.aw))
        print("LASMI Bus Data Width    : {}".format(lasmim.dw))

    autocsr_exclude = {"ev"}


