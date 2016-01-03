from migen.fhdl.std import *
from migen.bank.description import *
from migen.sim.generic import run_simulation
from migen.fhdl import verilog

h_active = 1024
h_sync_start = 1048
h_sync_end = 1184
hMax = 1344

v_active = 768
v_sync_start = 771
v_sync_end = 777
vMax = 806

class VGAGenerator(Module):
    def __init__(self, cnt_width=24):
        self.counterX = Signal(16)
        self.counterY = Signal(16)
        
        self.r = Signal(8)
        self.g = Signal(8)
        self.b = Signal(8)
        self.de = Signal()
        self.vsync = Signal()
        self.hsync = Signal()
        self.valid = Signal()
        
        hActive = Signal()
        vActive = Signal()
        
        data = Signal(24)
        
        self.comb += [
            self.de.eq(vActive & hActive),
            self.r.eq(data[16:]),
            self.g.eq(data[8:16]),
            self.b.eq(data[0:8]),
            self.valid.eq(1),
        ]

        self.sync += [
            self.counterX.eq(self.counterX+1),
            
            If(self.counterY == 0, 
                vActive.eq(1),
            ).Elif(self.counterY == v_active,
                vActive.eq(0),
            ).Elif(self.counterY == v_sync_start,
                self.vsync.eq(1),
            ).Elif(self.counterY == v_sync_end,
                self.vsync.eq(0),
            ).Elif(self.counterY == vMax - 1,
                self.counterY.eq(0),
                data.eq(0),
            ),
            
            If(self.counterX == 0,
                hActive.eq(1),
            ).Elif(self.counterX == h_active,
                hActive.eq(0),
            ).Elif(self.counterX == h_sync_start,
                self.hsync.eq(1),
            ).Elif(self.counterX == h_sync_end,
                self.hsync.eq(0),
            ).Elif(self.counterX == hMax - 1,
                self.counterX.eq(0),
                self.counterY.eq(self.counterY+1),
            ),
            
            If(self.de == 1,
                data.eq(data+1),
            ),
                
        ]
    
    
    def do_simulation(self, selfp):
         print ("cycle: {cycle} , hsync: {hsync}, vsync: {vsync}, de: {de}, r: {r}, g: {g}, b: {b}".format(
                                                                                                        cycle=selfp.simulator.cycle_counter,
                                                                                                        hsync = selfp.hsync,
                                                                                                        vsync = selfp.vsync,
                                                                                                        de = selfp.de,
                                                                                                        r = selfp.r,
                                                                                                        g = selfp.g,
                                                                                                        b = selfp.b,
                                                                                                        ))
if __name__ == "__main__":
    my_vgagen = VGAGenerator()
    print(verilog.convert(my_vgagen, ios={my_vgagen.hsync, my_vgagen.vsync, my_vgagen.r, my_vgagen.g, my_vgagen.b, my_vgagen.de}))
    #run_simulation(my_vgagen, ncycles=1000, vcd_name="vgagen.vcd")
