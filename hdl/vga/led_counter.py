from migen.fhdl.std import *
from migen.bank.description import *



class LEDCounter(Module):
    def __init__(self, cnt_width=32):
        self.counter = Signal(cnt_width)
        self.leds = Signal(8)

        self.comb += [
            self.leds.eq(self.counter[-8:])
        ]

        self.sync += [
            self.counter.eq(self.counter+1)
        ]

class LEDCounterCSR(Module, AutoCSR):
    def __init__(self):
        self.r_leds = CSRStorage(8, atomic_write=True)
        self.leds = Signal(8)

        self.comb += self.leds.eq(self.r_leds.storage)