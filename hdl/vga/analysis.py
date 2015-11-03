from migen.fhdl.std import *
from migen.genlib.record import Record
from migen.bank.description import *
from migen.flow.actor import *



class FrameExtraction(Module, AutoCSR):
    def __init__(self, word_width, fifo_depth):
        self.counter = Signal(word_width)

        word_layout = [("sof", 1), ("pixels", word_width)]
        self.frame = Source(word_layout)
        self.busy = Signal()
        data = Record(word_layout)

        self._overflow = CSR()

        self.sync += [
            If((self.counter == 0),
               data.sof.eq(1)
            ).Else(
                data.sof.eq(0)
            )
        ]

        self.sync += [
            If((self.counter < 768432),
               self.counter.eq(self.counter + 1)
            ).Else(
                self.counter.eq(0)
            )
        ]

        self.comb += [
            self.busy.eq(0),
            data.pixels.eq(self.counter),
            self.frame.stb.eq(1),
            self.frame.payload.eq(data)
        ]