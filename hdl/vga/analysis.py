from migen.fhdl.std import *
from migen.genlib.record import Record
from migen.bank.description import *
from migen.flow.actor import *


class FrameExtraction(Module, AutoCSR):
    def __init__(self, word_width, fifo_depth):
        self.counter = Signal(20)  # Since math.log(1024*768, 2) = 19.58

        word_layout = [("sof", 1), ("pixels", word_width)]
        self.frame = Source(word_layout)
        self.busy = Signal()
        data = Record(word_layout)

        self._overflow = CSR()
        self._start_counter = CSRStorage(1, reset=0)

        self.sync += [
            If((self.counter == 0),
               data.sof.eq(1)
            ).Else(
                data.sof.eq(0)
            )
        ]

        self.sync += [
            If((self._start_counter.storage),
               self.counter.eq(self.counter + 1)
            )
        ]
#       self._overflow.w.eq(self.counter > (1024*768))
        self.sync += [
            self._overflow.w.eq(0)
        ]
        
#       self.frame.stb.eq(1), inside self.comb statement

        self.comb += [
            self.busy.eq(0),
            data.pixels.eq(self.counter),
            self.frame.stb.eq((self.counter < (1024*768)) & (self._start_counter.storage)),
            self.frame.payload.eq(data)
        ]
