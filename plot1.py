import matplotlib
import matplotlib.pyplot as plt
import numpy as np

x = [0,1,2,3,4,5,6]
SaaS0 = [0.472914,0.520205,0.572226,0.572226,0.572226,0.572226,0.572226]
SaaS1 = [0.628168,0.628168,0.628168,0.628168,0.628168,0.628168,0.628168]
SaaS2 = [0.767358,0.767358,0.767358,0.767358,0.767358,0.767358,0.767358]
SaaS3 = [0.72747, 0.72747,0.72747,0.72747,0.72747,0.72747,0.72747]
SaaS4 = [0.389491, 0.42844, 0.471284, 0.471284,0.471284,0.471284,0.471284]
SaaS5 = [0.279758, 0.307734, 0.338508, 0.372358, 0.409594, 0.450554,0.450554]
SaaS6 = [0.760674, 0.760674,0.760674,0.760674,0.760674,0.760674,0.760674]
SaaS7 = [0.37663, 0.414293, 0.455722, 0.455722,0.455722,0.455722,0.455722]
SaaS8 = [0.230824, 0.253907, 0.279297, 0.307227, 0.33795, 0.371745, 0.408919]
SaaS9 = [0.657205, 0.657205, 0.657205,0.657205,0.657205,0.657205,0.657205]


plt.plot(x, SaaS0, x, SaaS1, x, SaaS2, x, SaaS3, x, SaaS4, x, SaaS5, x, SaaS6, x, SaaS7, x, SaaS8, x, SaaS9 )

plt.show()
