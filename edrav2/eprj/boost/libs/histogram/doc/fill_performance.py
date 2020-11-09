import os
import numpy as np
import sys
import re
from collections import defaultdict, OrderedDict
from matplotlib.patches import Rectangle
from matplotlib.lines import Line2D
from matplotlib.text import Text
from matplotlib.font_manager import FontProperties
import matplotlib.pyplot as plt
import matplotlib as mpl
mpl.rcParams.update(mpl.rcParamsDefault)

data = defaultdict(lambda:[])
for line in open(sys.argv[1]):
    if line.startswith("baseline"):
        continue
    descr, time = [x for x in line.split() if x]
    dim, label, dist = descr.split("-")
    data[int(dim[0])].append((label, dist, float(time)))

plt.figure()
if os.path.exists("/proc/cpuinfo"):
    cpuinfo = open("/proc/cpuinfo").read()
    m = re.search("model name\s*:\s*(.+)\n", cpuinfo)
    if m:
        plt.title(m.group(1))
i = 0
for dim in sorted(data):
    v = data[dim]
    labels = OrderedDict()
    for label, dist, time in v:
        if label in labels:
            labels[label][dist] = time
        else:
            labels[label] = {dist: time}
    j = 0
    for label, d in labels.items():
        t1 = d["uniform"]
        t2 = d["normal"]
        i -= 1
        z = float(j) / len(labels)
        col = ((1.0-z) * np.array((1.0, 0.0, 0.0))
               + z * np.array((1.0, 1.0, 0.0)))
        if label == "root":
            col = "k"
        if "numpy" in label:
            col = "0.6"
        if "gsl" in label:
            col = "0.3"
        r1 = Rectangle((0, i), t1, 0.5, facecolor=col)
        r2 = Rectangle((0, i+0.5), t2, 0.5, facecolor=col)
        plt.gca().add_artist(r1)
        plt.gca().add_artist(r2)
        tx = Text(-0.1, i+0.5, "%s" % label,
                  va="center", ha="right", clip_on=False)
        plt.gca().add_artist(tx)
        j += 1
    i -= 1
    font0 = FontProperties()
    font0.set_weight("bold")
    tx = Text(-0.1, i+0.6, "%iD" % dim,
              fontproperties=font0, va="center", ha="right", clip_on=False)
    plt.gca().add_artist(tx)
plt.ylim(0, i)
plt.xlim(0, 30)

plt.tick_params("y", left=False, labelleft=False)
plt.xlabel("fill time per random number in nanoseconds (smaller is better)")

plt.savefig("fill_performance.svg")
plt.show()
