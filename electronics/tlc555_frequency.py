import numpy as np


# inputs
Ct = 470e-12
Ra = 300
Rb = 1600
r_on = 25 # From Figure 2 (at 25'C, Vdd = 3.3V)
t_plh = 160e-9 # From Figure 2 (Vdd = 3.3V)
t_phl = 330e-9 # From Figure 2 (Vdd = 3.3V)

# output (using (7))
t_H = 0.693 * (Ra + Rb) * Ct
t_L = 0.693 * Rb * Ct
t_tot = t_H + t_L
freq = 1 / t_tot
print("Period: {}s".format(t_tot))
print("Frequency: {} Hz".format(freq))

# output (using (8))
t_cH = Ct * (Ra + Rb) * np.log(3 - np.exp(-t_plh / (Ct * (Rb + r_on)))) + t_phl
t_cL = Ct * (Ra + r_on) * np.log(3 - np.exp(-t_phl / (Ct * (Ra + Rb)))) + t_plh
t_tot = t_cH + t_cL
freq = 1 / t_tot
print("Period using corrected formulae: {}s".format(t_tot))
print("Frequency using corrected formulae: {} Hz".format(freq))