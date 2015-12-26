from migen.fhdl.std import *
from migen.genlib.cdc import MultiReg, PulseSynchronizer
from migen.genlib.fifo import AsyncFIFO, SyncFIFO
from migen.genlib.record import Record
from migen.bank.description import *
from migen.flow.actor import *


class FrameExtraction(Module, AutoCSR):
    def __init__(self, word_width, fifo_depth):
        self.counter = Signal(20)  # Since math.log(1024*768, 2) = 19.58

        word_layout = [("sof", 1), ("pixels", word_width)]
        self.frame = Source(word_layout)
        self.busy = Signal()

        self._overflow = CSR()
        self._start_counter = CSRStorage(1, reset=0)

        self.sync += [
            If((self._start_counter.storage),
               self.counter.eq(self.counter + 1)
            )
        ]

        ###################################################################################
        # start of frame detection

        new_frame = Signal()
        self.comb += new_frame.eq(self.counter == 0)

        # pack pixels into words
        cur_word = Signal(word_width)
        cur_word_valid = Signal()
        encoded_pixel = Signal(16)
        self.comb += encoded_pixel.eq(self.counter[0:16]),
        pack_factor = word_width//16
        assert(pack_factor & (pack_factor - 1) == 0)  # only support powers of 2
        pack_counter = Signal(max=pack_factor)
        
        #self.sync.pix += [
        self.sync.sys += [
            cur_word_valid.eq(0),
            If(new_frame,
                cur_word_valid.eq(pack_counter == (pack_factor - 1)),
                pack_counter.eq(0),
            ).Elif((self.counter < (80*80)) & (self._start_counter.storage),
                [If(pack_counter == (pack_factor-i-1),
                    cur_word[16*i:16*(i+1)].eq(encoded_pixel)) for i in range(pack_factor)],
                cur_word_valid.eq(pack_counter == (pack_factor - 1)),
                pack_counter.eq(pack_counter + 1)
            )
        ]

        # FIFO
        fifo = RenameClockDomains(SyncFIFO(word_layout, fifo_depth),
            {"write": "sys", "read": "sys"})
        self.submodules += fifo
        self.comb += [
            fifo.din.pixels.eq(cur_word),
            fifo.we.eq(cur_word_valid)
        ]
        
        self.sync.sys += \
            If(new_frame,
                fifo.din.sof.eq(1)
            ).Elif(cur_word_valid,
                fifo.din.sof.eq(0)
            )

        self.comb += [
            self.frame.stb.eq(fifo.readable),
            self.frame.payload.eq(fifo.dout),
            fifo.re.eq(self.frame.ack),
            self.busy.eq(0)
        ]

        # overflow detection
        pix_overflow = Signal()
        pix_overflow_reset = Signal()
        
        self.sync.sys += [
            If(fifo.we & ~fifo.writable,
                pix_overflow.eq(1)
            ).Elif(pix_overflow_reset,
                pix_overflow.eq(0)
            )
        ]

        sys_overflow = Signal()
        self.specials += MultiReg(pix_overflow, sys_overflow)
        self.comb += [
            pix_overflow_reset.eq(self._overflow.re),
        ]

        overflow_mask = Signal()
        self.comb += [
            self._overflow.w.eq(sys_overflow & ~overflow_mask),
        ]
        self.sync += \
            If(self._overflow.re,
                overflow_mask.eq(1)
            ).Elif(pix_overflow_reset,
                overflow_mask.eq(0)
            )
