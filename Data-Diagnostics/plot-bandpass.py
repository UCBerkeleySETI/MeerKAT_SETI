import pylab as plt
from blimpy import Waterfall
import numpy as np
import sys

file_path = sys.argv[1]
base_path = file_path.split('fil')[0]
ics = Waterfall(file_path)

fig = plt.figure(figsize=[10,10])
plt.subplots_adjust(left=None, bottom=None, right=None, top=None, wspace=0.2, hspace=0.3)

ax = plt.subplot2grid((2,1),(0,0))
ics.plot_waterfall(cb=False)
ax2 = plt.subplot2grid((2,1),(1,0))
ics.plot_spectrum(logged=True)

plt.savefig(base_path+'png')
